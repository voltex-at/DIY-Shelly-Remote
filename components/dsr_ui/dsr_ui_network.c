#include "dsr_ui_internal.h"
#include "dsr_platform.h"
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
static lv_obj_t *wifi_text, *name_text, *state_text, *power_text, *on_button, *off_button, *refresh_button;
static lv_obj_t *setup_button;
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
    wifi_text = name_text = state_text = power_text = on_button = off_button = refresh_button = NULL;
    setup_button = NULL;
    form = keyboard = message = NULL;
    memset(fields, 0, sizeof(fields)); view = VIEW_NONE;
}

static void render(void)
{
    if (view != VIEW_HOME) return;
    bool wifi = dsr_wifi_connected();
    dsr_wifi_info_t info; dsr_wifi_get_info(&info);
    if (wifi) lv_label_set_text_fmt(wifi_text, LV_SYMBOL_WIFI "  %s\n%s", info.ssid, info.ip);
    else lv_label_set_text(wifi_text, info.status == DSR_WIFI_BAD_PASSWORD ? "Wi-Fi authentication failed\nTap to choose a network" :
        settings.ssid[0] ? "Wi-Fi disconnected\nTap to choose a network" : "Wi-Fi not configured\nTap to choose a network");
    if (storage_error) lv_label_set_text(wifi_text, "Settings error. Reconfigure on screen.");
    lv_label_set_text(name_text, configured ? saved.device.name : "No Shelly configured");
    const char *state = !configured ? (wifi ? "Wi-Fi is ready. Add your first Shelly." : "Choose your Wi-Fi to get started.") :
        busy ? "Contacting Shelly..." :
        !wifi ? "Offline (last state not confirmed)" :
        last_result != DSR_OK ? result_text(last_result) :
        !saved.device.online ? "Waiting for confirmed state..." :
        (saved.device.output ? "Relay: ON (confirmed)" : "Relay: OFF (confirmed)");
    lv_label_set_text(state_text, state);
    if (configured && wifi && !busy && last_result == DSR_OK && saved.device.online && saved.device.has_power) {
        char power[48]; snprintf(power, sizeof(power), "Power: %.1f W", saved.device.power_w);
        lv_label_set_text(power_text, power);
    }
    else lv_label_set_text(power_text, "");
    if (!configured) {
        if (setup_button) lv_label_set_text(lv_obj_get_child(setup_button, 0), wifi ? "Add your Shelly" : "Choose Wi-Fi network");
        return;
    }
    bool enabled = configured && wifi && !busy && !blocked && saved.device.online;
    if (enabled) { lv_obj_remove_state(on_button, LV_STATE_DISABLED); lv_obj_remove_state(off_button, LV_STATE_DISABLED); }
    else { lv_obj_add_state(on_button, LV_STATE_DISABLED); lv_obj_add_state(off_button, LV_STATE_DISABLED); }
    if (configured && wifi && !busy) lv_obj_remove_state(refresh_button, LV_STATE_DISABLED);
    else lv_obj_add_state(refresh_button, LV_STATE_DISABLED);
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

static void refresh(lv_event_t *event)
{
    (void)event;
    if (!busy) { blocked = false; (void)submit(DSR_JOB_PROBE, false); }
}

static void set_output(lv_event_t *event)
{
    if (!saved.device.online || !dsr_wifi_connected() || blocked) return;
    (void)submit(DSR_JOB_SET_OUTPUT, lv_event_get_user_data(event) != NULL);
}

static void setup(lv_event_t *event)
{
    if (dsr_wifi_connected()) dsr_network_device(event);
    else dsr_network_wifi(event);
}

void dsr_network_home(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Home", true); view = VIEW_HOME;
    lv_obj_t *wifi_card = dsr_ui_button(root, "", 16, 84, 288, dsr_network_wifi, NULL);
    lv_obj_set_height(wifi_card, 86); lv_obj_clean(wifi_card);
    wifi_text = dsr_ui_label(wifi_card, "", 0, 0, 254);
    name_text = dsr_ui_label(root, "", 16, 194, 288);
    lv_obj_set_height(name_text, 40); lv_label_set_long_mode(name_text, LV_LABEL_LONG_MODE_DOTS);
    state_text = dsr_ui_label(root, "", 16, 245, 288);
    power_text = dsr_ui_label(root, "", 16, 294, 288);
    if (configured) {
        on_button = dsr_ui_button(root, "ON", 16, 332, 136, set_output, (void *)1);
        off_button = dsr_ui_button(root, "OFF", 168, 332, 136, set_output, NULL);
        refresh_button = dsr_ui_button(root, "Refresh", 16, 382, 288, refresh, NULL);
        lv_obj_set_height(refresh_button, 32);
        dsr_ui_primary(on_button);
    } else {
        setup_button = dsr_ui_button(root, "", 16, 330, 288, setup, NULL);
        dsr_ui_primary(setup_button);
    }
    render();
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
    storage_error = dsr_settings_load(&settings) != ESP_OK;
    bool found = false;
    if (dsr_connection_load(&saved, &found) != ESP_OK) storage_error = true;
    configured = found;
    last_poll = lv_tick_get() - 5000;
    lv_timer_create(poll, 200, NULL);
}
