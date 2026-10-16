#include "dsr_ui_internal.h"
#include "dsr_platform.h"
#include "dsr_ui_mobile.h"
#include "dsr_workspace.h"
#include "dsr_board.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dsr_settings_t settings;
static dsr_settings_t wifi_candidate;
static dsr_connection_t saved, pending;
static bool configured, busy, blocked, wifi_previous;
static dsr_result_t last_result = DSR_OK;
static bool command_error;
/* One user intent may wait behind a read. Never replay SET after a failure. */
static bool queued_output,queued_on,sample_valid;
static dsr_device_t queued_device;
static uint32_t sample_ms;
static dsr_job_kind_t operation;
static uint32_t serial, outstanding, last_poll;
static bool storage_error;
static bool external_bind;
static bool lan_binding;
static uint32_t pending_room=UINT32_MAX;
static dsr_mobile_model_t mobile_model;
static lv_obj_t *form, *keyboard, *message, *fields[5];
static enum { VIEW_NONE, VIEW_HOME, VIEW_DEVICE } view;

static const char *result_text(dsr_result_t result)
{
    switch (result) {
    case DSR_OK: return "Ready";
    case DSR_AUTH_REQUIRED: return "Authentication failed. Check login.";
    case DSR_RATE_LIMITED: return "Shelly limits logins. Wait, then Refresh.";
    case DSR_IDENTITY_MISMATCH: return "Different device at this IP. Add again.";
    case DSR_OFFLINE: return "Shelly offline / Wi-Fi disconnected";
    case DSR_UNSUPPORTED: return "Not a supported relay/switch channel";
    case DSR_BAD_RESPONSE: return "Invalid response / state unconfirmed";
    case DSR_BUSY: return "Operation in progress. Please wait.";
    case DSR_IO: return "Storage or memory error";
    default: return "Check IP, channel and field lengths";
    }
}

void dsr_network_reset_view(void)
{
    dsr_ui_battery_hide();dsr_ui_display_hide();
    dsr_wifi_ui_reset();
    dsr_ui_lan_reset();
    dsr_ui_ble_reset();
    form = keyboard = message = NULL;
    memset(fields, 0, sizeof(fields)); view = VIEW_NONE;
}

static void render(void)
{
    if (view != VIEW_HOME) return;
    memset(&mobile_model, 0, sizeof(mobile_model));
    mobile_model.wifi = dsr_wifi_connected();
    snprintf(mobile_model.home_name, sizeof(mobile_model.home_name), "Mój dom");
    char *configuration=dsr_workspace_snapshot();
    bool layout=configuration&&dsr_mobile_configure(&mobile_model,configuration);free(configuration);
    if (configured) {
        unsigned index=0;
        if(layout){for(;index<mobile_model.device_count;index++)if(!strcmp(mobile_model.devices[index].host,saved.device.host)&&mobile_model.devices[index].channel==saved.device.channel)break;}
        else mobile_model.device_count = 1;
        if(index>=mobile_model.device_count){dsr_mobile_update(&mobile_model);return;}
        dsr_mobile_device_t *d = &mobile_model.devices[index];
        if(!layout){
        snprintf(d->name, sizeof(d->name), "%s", saved.device.name);
        snprintf(d->host, sizeof(d->host), "%s", saved.device.host);
        snprintf(d->model, sizeof(d->model), "Shelly • kanał %u", saved.device.channel);
        d->room = DSR_UI_NO_ROOM; d->kind = DSR_DEVICE_RELAY;
        d->channel=saved.device.channel;
        }
        d->online = mobile_model.wifi && saved.device.online;
        d->output = saved.device.output; d->busy = queued_output||(busy&&operation!=DSR_JOB_PROBE);
        d->command_pending=queued_output||(busy&&operation==DSR_JOB_SET_OUTPUT);
        d->has_sample_time=sample_valid;d->sample_ms=sample_ms;
        d->error=last_result;
        d->can_switch = saved.device.has_switch && !blocked;
        d->has_power = saved.device.has_power; d->power_w = saved.device.power_w;
        d->has_voltage=saved.telemetry.has_voltage;d->voltage_v=saved.telemetry.voltage_v;
        d->has_current=saved.telemetry.has_current;d->current_a=saved.telemetry.current_a;
    }
    dsr_mobile_update(&mobile_model);
}

static dsr_result_t submit(dsr_job_kind_t kind, bool on)
{
    if (busy) return DSR_BUSY;
    dsr_job_t job = {.id = ++serial, .kind = kind, .connection = kind == DSR_JOB_BIND ? pending : saved,
                     .settings = settings, .on = on};
    dsr_result_t result = dsr_worker_submit(&job);
    if (result == DSR_OK) { busy = true; operation = kind; outstanding = job.id; }
    memset(&job, 0, sizeof(job));
    render();
    return result;
}

bool dsr_network_busy(void) { return busy; }
dsr_result_t dsr_network_request_display(uint8_t brightness,uint16_t seconds)
{
 if(busy)return DSR_BUSY;
 dsr_job_t job={.id=++serial,.kind=DSR_JOB_DISPLAY_SAVE};
 job.settings.brightness=brightness;job.settings.idle_seconds=seconds;
 dsr_result_t accepted=dsr_worker_submit(&job);
 if(accepted==DSR_OK){busy=true;operation=DSR_JOB_DISPLAY_SAVE;outstanding=job.id;}
 return accepted;
}
dsr_result_t dsr_network_request_battery(void)
{
 if(busy)return DSR_BUSY;
 const dsr_job_t job={.id=++serial,.kind=DSR_JOB_BATTERY_READ};
 dsr_result_t accepted=dsr_worker_submit(&job);
 if(accepted==DSR_OK){busy=true;operation=DSR_JOB_BATTERY_READ;outstanding=job.id;}
 return accepted;
}
void dsr_network_wifi_settings(dsr_settings_t *out) { *out = settings; }
dsr_result_t dsr_network_request_wifi(dsr_job_kind_t kind, const dsr_settings_t *candidate)
{
    if (busy || (kind != DSR_JOB_WIFI_SCAN && kind != DSR_JOB_WIFI_CONNECT)) return DSR_BUSY;
    if (candidate) wifi_candidate = *candidate;
    dsr_job_t job = {.id = ++serial, .kind = kind, .settings = wifi_candidate};
    dsr_result_t result = dsr_worker_submit(&job);
    if (result == DSR_OK) { busy = true; operation = kind; outstanding = job.id; }
    memset(&job, 0, sizeof(job)); return result;
}

void dsr_network_home(lv_event_t *event)
{
    (void)event;
    dsr_ui_workspace_hide();
    dsr_network_reset_view(); view = VIEW_HOME;
    render(); dsr_mobile_show(DSR_MOBILE_HOME, 0);
}
void dsr_network_rooms(void)
{
    dsr_network_home(NULL); /* Re-read the published shared configuration first. */
    dsr_mobile_show(DSR_MOBILE_ROOMS,0);
}

static void mobile_output(unsigned index, bool on)
{
    if (index>=mobile_model.device_count||strcmp(mobile_model.devices[index].host,saved.device.host)||mobile_model.devices[index].channel!=saved.device.channel||queued_output||(busy&&operation!=DSR_JOB_PROBE)||blocked||!saved.device.online||!dsr_wifi_connected()) return;
    last_result=DSR_OK;command_error=false;
    if(busy){queued_output=true;queued_on=on;queued_device=saved.device;render();return;}
    dsr_result_t result=submit(DSR_JOB_SET_OUTPUT,on);
    if(result!=DSR_OK){last_result=result;command_error=true;render();}
}
static void mobile_add(void) { dsr_network_device(NULL); }
static void mobile_edit_device(unsigned index)
{if(index<mobile_model.device_count&&mobile_model.devices[index].id)dsr_ui_edit_device(mobile_model.devices[index].id);}
static void mobile_refresh(void)
{
 if((busy&&operation!=DSR_JOB_PROBE)||queued_output||!configured)return;
 command_error=false;blocked=false;
 if(!dsr_wifi_connected()){last_result=DSR_OFFLINE;render();return;}
 if(busy){last_result=DSR_OK;render();return;} /* Join the existing read. */
 dsr_result_t result=submit(DSR_JOB_PROBE,false);if(result!=DSR_OK){last_result=result;render();}
}

static void hide_keyboard(lv_event_t *event)
{
    (void)event;
    if (!keyboard || !form) return;
    lv_keyboard_set_textarea(keyboard, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_height(form, 300);
}

static void focus_field(lv_event_t *event)
{
    lv_obj_t *field = lv_event_get_target(event);
    if (!keyboard || !form) return;
    lv_obj_set_height(form, 121);
    dsr_ui_keyboard_focus(keyboard,field);
    lv_obj_scroll_to_view(field, LV_ANIM_OFF);
}

static lv_obj_t *field(unsigned index, const char *caption, const char *text, unsigned max, bool secret)
{
    int y = (int)index * 82;
    dsr_ui_label(form, caption, 0, y, 268);
    lv_obj_t *obj = lv_textarea_create(form);
    dsr_ui_textarea(obj);
    lv_obj_set_pos(obj, 0, y + 24); lv_obj_set_size(obj, 268, 44);
    lv_textarea_set_one_line(obj, true);
    lv_textarea_set_max_length(obj, max);
    lv_textarea_set_password_mode(obj, secret);
    lv_textarea_set_text(obj, text);
    lv_obj_add_event_cb(obj, focus_field, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj, focus_field, LV_EVENT_CLICKED, NULL);
    fields[index] = obj;
    return obj;
}

static void form_start(const char *title)
{
    lv_obj_t *root = dsr_ui_screen(title, true);
    message = dsr_ui_label(root, "Enter the device address and login.", 16, 77, 288);
    form = lv_obj_create(root); lv_obj_set_pos(form, 12, 115); lv_obj_set_size(form, 296, 300);
    lv_obj_set_style_pad_all(form, 8, 0); lv_obj_set_style_border_width(form, 0, 0);
    lv_obj_set_style_bg_color(form, dsr_sc_color(DSR_SC_BG), 0);
    lv_obj_set_style_text_color(form, lv_color_hex(0xf0f4fb), 0);
    keyboard = lv_keyboard_create(root); lv_obj_set_size(keyboard, 320, 210);
    dsr_ui_keyboard(keyboard);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(keyboard, hide_keyboard, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(keyboard, hide_keyboard, LV_EVENT_CANCEL, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

static bool copy_field(char *dest, size_t capacity, unsigned index)
{
    const char *text = lv_textarea_get_text(fields[index]);
    if (strlen(text) >= capacity) return false; /* UTF-8 byte limit, not character count */
    snprintf(dest, capacity, "%s", text); return true;
}

static void bind_device(lv_event_t *event)
{
    (void)event;
    if (busy) { dsr_sc_label_set_text(message, result_text(DSR_BUSY)); return; }
    if (!dsr_wifi_connected()) { dsr_sc_label_set_text(message, "Connect Wi-Fi first"); return; }
    memset(&pending, 0, sizeof(pending));
    pending_room=UINT32_MAX;lan_binding=false;
    const char *channel = lv_textarea_get_text(fields[1]);
    char *end = NULL; long value = strtol(channel, &end, 10);
    if (!channel[0] || !end || *end || value < 0 || value > DSR_MAX_CHANNEL ||
        !copy_field(pending.device.host, sizeof(pending.device.host), 0) ||
        !copy_field(pending.device.name, sizeof(pending.device.name), 2) ||
        !copy_field(pending.credentials.username, sizeof(pending.credentials.username), 3) ||
        !copy_field(pending.credentials.password, sizeof(pending.credentials.password), 4)) {
        dsr_sc_label_set_text(message, "Invalid details. Channel range: 0-15"); return;
    }
    pending.device.channel = (uint8_t)value;
    dsr_result_t result = submit(DSR_JOB_BIND, false);
    dsr_sc_label_set_text(message, result == DSR_OK ? "Checking device before saving..." : result_text(result));
}

static void manual_device(lv_event_t *event)
{
    (void)event; form_start("Zaawansowane • adres IP"); view = VIEW_DEVICE;
    char channel[4]; snprintf(channel, sizeof(channel), "%u", saved.device.channel);
    lv_obj_t *ip = field(0, "IPv4 address (same LAN)", saved.device.host, 15, false);
    lv_textarea_set_accepted_chars(ip, "0123456789.");
    lv_obj_t *ch = field(1, "Relay/switch channel (first is 0)", channel, 2, false);
    lv_textarea_set_accepted_chars(ch, "0123456789");
    field(2, "Name (optional)", saved.device.name, 47, false);
    field(3, "Login (Gen2+: admin)", configured ? saved.credentials.username : "admin", 32, false);
    field(4, "Shelly password (optional)", saved.credentials.password, 64, true);
    dsr_ui_button(form, "Check & save device", 0, 420, 268, bind_device, NULL);
    dsr_ui_label(form, "Supports one relay/switch.\nA successful check replaces the saved device.\nLocal HTTP; Gen1 Basic / Gen2+ Digest.\nCredentials in unencrypted NVS.", 0, 484, 268);
}

void dsr_network_device(lv_event_t *event)
{
    (void)event;lv_obj_t *root=dsr_ui_screen("Dodaj urządzenie Shelly",true);
    dsr_ui_button(root,"Skanuj sieć Wi-Fi",12,96,296,dsr_ui_lan_open,NULL);
    lv_obj_t *ap=dsr_ui_button(root,"Wi-Fi AP",12,152,296,NULL,NULL);lv_obj_add_state(ap,LV_STATE_DISABLED);
    dsr_ui_button(root,"Bluetooth LE",12,208,296,dsr_ui_ble_open,NULL);
    dsr_ui_button(root,"Dodaj ręcznie (IP)",12,264,296,manual_device,NULL);
    dsr_ui_label(root,"LAN i skan BLE są dostępne.\nWi-Fi AP i parowanie BLE nie są jeszcze wdrożone.",16,329,288);
}
dsr_result_t dsr_network_bind_verified(const dsr_connection_t *connection,uint32_t room)
{
    if(!connection||busy)return DSR_BUSY;
    if(!dsr_wifi_connected())return DSR_OFFLINE;
    pending=*connection;pending_room=room;dsr_result_t result=submit(DSR_JOB_BIND,false);
    if(result==DSR_OK)lan_binding=true;else{memset(&pending,0,sizeof(pending));pending_room=UINT32_MAX;}
    return result;
}

static void poll(lv_timer_t *timer)
{
    (void)timer;
    uint32_t now = lv_tick_get();
    dsr_event_t event;
    if (dsr_worker_receive(&event) && event.id == outstanding) {
        busy = false; if(operation!=DSR_JOB_BATTERY_READ&&operation!=DSR_JOB_DISPLAY_SAVE)last_poll = now;
        if(operation==DSR_JOB_DISPLAY_SAVE){
            if(event.result==DSR_OK){settings.brightness=event.brightness;settings.idle_seconds=event.idle_seconds;}
            dsr_ui_display_result(&event);
        } else if(operation==DSR_JOB_BATTERY_READ)dsr_ui_battery_result(&event);
        else if (operation == DSR_JOB_WIFI_SCAN || operation == DSR_JOB_WIFI_CONNECT) {
            if (operation == DSR_JOB_WIFI_CONNECT && event.result == DSR_OK) {
                settings = wifi_candidate; storage_error = false; saved.device.online = false;
            }
            dsr_wifi_ui_result(&event, operation);
        } else if (operation == DSR_JOB_BIND) {
            if (event.result == DSR_OK) {
                char old_host[16]={0};if(configured&&!strcmp(saved.device.device_id,event.device.device_id))snprintf(old_host,sizeof(old_host),"%s",saved.device.host);
                saved = pending; saved.device = event.device;saved.telemetry=event.telemetry; configured = true; blocked = false;command_error=false;
                dsr_ui_workspace_migrate_bound(&saved,pending_room,old_host);
                sample_valid=event.device.online;sample_ms=now;
                last_result = DSR_OK; if(!external_bind)dsr_network_home(NULL);
            }
            if(lan_binding){if(event.result!=DSR_OK)dsr_ui_lan_bind_result(event.result);lan_binding=false;}
            pending_room=UINT32_MAX;
            memset(&pending, 0, sizeof(pending));
            if(external_bind){dsr_workspace_bind_result(event.result);external_bind=false;}
        } else {
            saved.device = event.device;saved.telemetry=event.telemetry;
            if(event.result==DSR_OK&&event.device.online){sample_valid=true;sample_ms=now;}
            if(operation==DSR_JOB_SET_OUTPUT){last_result=event.result;command_error=event.result!=DSR_OK;}
            else if(!command_error)last_result=event.result;
            blocked = event.result == DSR_AUTH_REQUIRED || event.result == DSR_IDENTITY_MISMATCH || event.result == DSR_RATE_LIMITED;
        }
        if (message && event.result != DSR_OK && operation != DSR_JOB_WIFI_SCAN && operation != DSR_JOB_WIFI_CONNECT && operation!=DSR_JOB_BATTERY_READ && operation!=DSR_JOB_DISPLAY_SAVE)
            dsr_sc_label_set_text(message, result_text(event.result));
        if(queued_output){
            bool on=queued_on;
            dsr_result_t result=event.result;
            if(result==DSR_OK&&!dsr_wifi_connected())result=DSR_OFFLINE;
            if(result==DSR_OK&&(!saved.device.online||!saved.device.has_switch))result=DSR_BAD_RESPONSE;
            if(result==DSR_OK&&(strcmp(saved.device.device_id,queued_device.device_id)||strcmp(saved.device.host,queued_device.host)||saved.device.generation!=queued_device.generation||saved.device.channel!=queued_device.channel))result=DSR_IDENTITY_MISMATCH;
            queued_output=false;memset(&queued_device,0,sizeof(queued_device));
            if(result==DSR_OK)result=submit(DSR_JOB_SET_OUTPUT,on);
            if(result!=DSR_OK){last_result=result;command_error=true;
                if(result==DSR_AUTH_REQUIRED||result==DSR_IDENTITY_MISMATCH||result==DSR_RATE_LIMITED)blocked=true;
                if(result==DSR_IDENTITY_MISMATCH)saved.device.online=false;
            }
        }
        render();
    }
    bool wifi = dsr_wifi_connected();
    if (wifi != wifi_previous) {
        wifi_previous = wifi; saved.device.online = false;
        if(!wifi&&queued_output){queued_output=false;memset(&queued_device,0,sizeof(queued_device));last_result=DSR_OFFLINE;command_error=true;}
        if (wifi) last_poll = now - 5000;
        render();
    }
    dsr_wifi_ui_tick();
    dsr_ui_lan_tick();
    dsr_ui_ble_tick();
    dsr_ui_battery_tick();
    dsr_ui_workspace_tick();dsr_panel_tick();
    if(!busy){dsr_connection_t connection;uint32_t room;if(dsr_workspace_take_bind_with_room(&connection,&room)){
        pending=connection;pending_room=room;memset(&connection,0,sizeof(connection));dsr_result_t result=submit(DSR_JOB_BIND,false);
        if(result==DSR_OK)external_bind=true;else{memset(&pending,0,sizeof(pending));dsr_workspace_bind_result(result);}
    }}
    static uint32_t config_revision;
    uint32_t next_revision=dsr_workspace_revision();
    if(config_revision!=next_revision){config_revision=next_revision;render();}
    if (configured && wifi && !busy && !blocked && !dsr_wifi_ui_active() && !dsr_ui_lan_active() && now - last_poll >= 5000) {
        last_poll = now; (void)submit(DSR_JOB_PROBE, false);
    }
}

void dsr_network_start_view(void)
{
    if (!settings.ssid[0]) dsr_network_wifi(NULL);
    else dsr_network_home(NULL);
}

void dsr_network_init(void)
{
    const dsr_mobile_actions_t actions = {.set_output = mobile_output, .open_settings = dsr_ui_open_settings, .add_device = mobile_add,.edit_dashboard=dsr_ui_edit_dashboard,.select_dashboard=dsr_ui_select_dashboard,.refresh=mobile_refresh,.edit_device=mobile_edit_device,.add_room=dsr_ui_add_room,.edit_room=dsr_ui_edit_room};
    dsr_mobile_bind(&actions);
    storage_error = dsr_settings_load(&settings) != ESP_OK;
    dsr_ui_display_init(settings.brightness,settings.idle_seconds);
    bool found = false;
    if (dsr_connection_load(&saved, &found) != ESP_OK) storage_error = true;
    configured = found;
    if(configured)dsr_ui_workspace_migrate(&saved);
    last_poll = lv_tick_get() - 5000;
    lv_timer_create(poll, 200, NULL);
}
