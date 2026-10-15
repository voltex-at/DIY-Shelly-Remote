#include "dsr_ui_internal.h"
#include "dsr_platform.h"
#include "dsr_ui_mobile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dsr_settings_t settings;
static dsr_settings_t wifi_candidate;
static dsr_connection_t saved, pending;
static bool configured, busy, blocked, wifi_previous;
static dsr_result_t last_result = DSR_OK;
static dsr_job_kind_t operation;
static uint32_t serial, outstanding, last_poll;
static bool storage_error;
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
    dsr_wifi_ui_reset();
    form = keyboard = message = NULL;
    memset(fields, 0, sizeof(fields)); view = VIEW_NONE;
}

static void render(void)
{
    if (view != VIEW_HOME) return;
    memset(&mobile_model, 0, sizeof(mobile_model));
    mobile_model.wifi = dsr_wifi_connected();
    snprintf(mobile_model.home_name, sizeof(mobile_model.home_name), "Mój dom");
    if (configured) {
        mobile_model.device_count = 1;
        dsr_mobile_device_t *d = &mobile_model.devices[0];
        snprintf(d->name, sizeof(d->name), "%s", saved.device.name);
        snprintf(d->host, sizeof(d->host), "%s", saved.device.host);
        snprintf(d->model, sizeof(d->model), "Shelly • kanał %u", saved.device.channel);
        d->room = DSR_UI_NO_ROOM; d->kind = DSR_DEVICE_RELAY;
        d->online = mobile_model.wifi && saved.device.online;
        d->output = saved.device.output; d->busy = busy;
        d->can_switch = saved.device.has_switch && !blocked;
        d->has_power = saved.device.has_power; d->power_w = saved.device.power_w;
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
    dsr_network_reset_view(); view = VIEW_HOME;
    render(); dsr_mobile_show(DSR_MOBILE_HOME, 0);
}

static void mobile_output(unsigned index, bool on)
{
    if (index || busy || blocked || !saved.device.online || !dsr_wifi_connected()) return;
    (void)submit(DSR_JOB_SET_OUTPUT, on);
}
static void mobile_add(void) { dsr_network_device(NULL); }

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
    lv_obj_set_height(form, 145);
    lv_obj_remove_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(keyboard, field);
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
    lv_obj_set_style_bg_color(form, lv_color_hex(0x10141c), 0);
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
    if (busy) { lv_label_set_text(message, result_text(DSR_BUSY)); return; }
    if (!dsr_wifi_connected()) { lv_label_set_text(message, "Connect Wi-Fi first"); return; }
    memset(&pending, 0, sizeof(pending));
    const char *channel = lv_textarea_get_text(fields[1]);
    char *end = NULL; long value = strtol(channel, &end, 10);
    if (!channel[0] || !end || *end || value < 0 || value > DSR_MAX_CHANNEL ||
        !copy_field(pending.device.host, sizeof(pending.device.host), 0) ||
        !copy_field(pending.device.name, sizeof(pending.device.name), 2) ||
        !copy_field(pending.credentials.username, sizeof(pending.credentials.username), 3) ||
        !copy_field(pending.credentials.password, sizeof(pending.credentials.password), 4)) {
        lv_label_set_text(message, "Invalid details. Channel range: 0-15"); return;
    }
    pending.device.channel = (uint8_t)value;
    dsr_result_t result = submit(DSR_JOB_BIND, false);
    lv_label_set_text(message, result == DSR_OK ? "Checking device before saving..." : result_text(result));
}

void dsr_network_device(lv_event_t *event)
{
    (void)event; form_start("Add / replace Shelly"); view = VIEW_DEVICE;
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

static void poll(lv_timer_t *timer)
{
    (void)timer;
    uint32_t now = lv_tick_get();
    dsr_event_t event;
    if (dsr_worker_receive(&event) && event.id == outstanding) {
        busy = false; last_poll = now;
        if (operation == DSR_JOB_WIFI_SCAN || operation == DSR_JOB_WIFI_CONNECT) {
            if (operation == DSR_JOB_WIFI_CONNECT && event.result == DSR_OK) {
                settings = wifi_candidate; storage_error = false; saved.device.online = false;
            }
            dsr_wifi_ui_result(&event, operation);
        } else if (operation == DSR_JOB_BIND) {
            if (event.result == DSR_OK) {
                saved = pending; saved.device = event.device; configured = true; blocked = false;
                last_result = DSR_OK; dsr_network_home(NULL);
            }
            memset(&pending, 0, sizeof(pending));
        } else {
            saved.device = event.device; last_result = event.result;
            blocked = event.result == DSR_AUTH_REQUIRED || event.result == DSR_IDENTITY_MISMATCH || event.result == DSR_RATE_LIMITED;
        }
        if (message && event.result != DSR_OK && operation != DSR_JOB_WIFI_SCAN && operation != DSR_JOB_WIFI_CONNECT)
            lv_label_set_text(message, result_text(event.result));
        render();
    }
    bool wifi = dsr_wifi_connected();
    if (wifi != wifi_previous) {
        wifi_previous = wifi; saved.device.online = false;
        if (wifi) last_poll = now - 5000;
        render();
    }
    dsr_wifi_ui_tick();
    if (configured && wifi && !busy && !blocked && !dsr_wifi_ui_active() && now - last_poll >= 5000) {
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
    const dsr_mobile_actions_t actions = {.set_output = mobile_output, .open_settings = dsr_ui_open_settings, .add_device = mobile_add};
    dsr_mobile_bind(&actions);
    storage_error = dsr_settings_load(&settings) != ESP_OK;
    bool found = false;
    if (dsr_connection_load(&saved, &found) != ESP_OK) storage_error = true;
    configured = found;
    last_poll = lv_tick_get() - 5000;
    lv_timer_create(poll, 200, NULL);
}
