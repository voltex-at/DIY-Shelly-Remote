#include "dsr_ui_internal.h"
#include "dsr_platform.h"
#include "esp_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dsr_settings_t settings;
static dsr_connection_t saved, pending;
static bool configured, busy, blocked, wifi_previous;
static dsr_result_t last_result = DSR_OK;
static dsr_job_kind_t operation;
static uint32_t serial, outstanding, last_poll, restart_at;
static bool restarting, storage_error;
static lv_obj_t *wifi_text, *name_text, *state_text, *power_text, *on_button, *off_button, *refresh_button;
static lv_obj_t *form, *keyboard, *message, *fields[5];
static enum { VIEW_NONE, VIEW_HOME, VIEW_WIFI, VIEW_DEVICE } view;

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
    wifi_text = name_text = state_text = power_text = on_button = off_button = refresh_button = NULL;
    form = keyboard = message = NULL;
    memset(fields, 0, sizeof(fields)); view = VIEW_NONE;
}

static void render(void)
{
    if (view != VIEW_HOME) return;
    bool wifi = dsr_wifi_connected();
    lv_label_set_text(wifi_text, settings.ssid[0] ? (wifi ? "Wi-Fi: connected" : "Wi-Fi: connecting (2.4 GHz)") : "Wi-Fi: not configured");
    if (storage_error) lv_label_set_text(wifi_text, "Settings error. Reconfigure on screen.");
    lv_label_set_text(name_text, configured ? saved.device.name : "No Shelly configured");
    const char *state = !configured ? "Menu > Add / replace Shelly" :
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

void dsr_network_home(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Home", true); view = VIEW_HOME;
    wifi_text = dsr_ui_label(root, "", 16, 87, 288);
    name_text = dsr_ui_label(root, "", 16, 135, 288);
    lv_obj_set_height(name_text, 40); lv_label_set_long_mode(name_text, LV_LABEL_LONG_MODE_DOTS);
    state_text = dsr_ui_label(root, "", 16, 185, 288);
    power_text = dsr_ui_label(root, "", 16, 249, 288);
    on_button = dsr_ui_button(root, "ON", 16, 285, 136, set_output, (void *)1);
    off_button = dsr_ui_button(root, "OFF", 168, 285, 136, set_output, NULL);
    refresh_button = dsr_ui_button(root, "Refresh", 16, 347, 288, refresh, NULL);
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
    message = dsr_ui_label(root, "Enter details; keyboard checkmark closes it.", 16, 77, 288);
    form = lv_obj_create(root); lv_obj_set_pos(form, 12, 115); lv_obj_set_size(form, 296, 300);
    lv_obj_set_style_pad_all(form, 8, 0); lv_obj_set_style_border_width(form, 0, 0);
    lv_obj_set_style_bg_color(form, lv_color_hex(0x10141c), 0);
    keyboard = lv_keyboard_create(root); lv_obj_set_size(keyboard, 320, 210);
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

static void save_wifi(lv_event_t *event)
{
    (void)event;
    if (busy) { lv_label_set_text(message, result_text(DSR_BUSY)); return; }
    dsr_settings_t candidate = settings;
    if (!copy_field(candidate.ssid, sizeof(candidate.ssid), 0) || !candidate.ssid[0] ||
        !copy_field(candidate.password, sizeof(candidate.password), 1)) {
        lv_label_set_text(message, "SSID: 1-32 bytes; password: max 64 bytes"); return;
    }
    size_t length = strlen(candidate.password);
    if (length > 0 && length < 8) { lv_label_set_text(message, "WPA password needs at least 8 characters"); return; }
    settings = candidate;
    dsr_result_t result = submit(DSR_JOB_SAVE_WIFI, false);
    lv_label_set_text(message, result == DSR_OK ? "Saving Wi-Fi; restarting after success..." : result_text(result));
}

void dsr_network_wifi(lv_event_t *event)
{
    (void)event; form_start("Wi-Fi setup"); view = VIEW_WIFI;
    field(0, "SSID (2.4 GHz)", settings.ssid, 32, false);
    field(1, "Password (empty for open Wi-Fi)", settings.password, 64, true);
    dsr_ui_button(form, "Save & restart", 0, 174, 268, save_wifi, NULL);
    dsr_ui_label(form, "Credentials stay on this remote.\nPrototype: NVS is not encrypted.", 0, 238, 268);
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
    if (restarting && (int32_t)(now - restart_at) >= 0) esp_restart();
    dsr_event_t event;
    if (dsr_worker_receive(&event) && event.id == outstanding) {
        busy = false; last_poll = now;
        if (operation == DSR_JOB_SAVE_WIFI) {
            if (event.result == DSR_OK) { restarting = true; restart_at = now + 700; }
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
        if (message && event.result != DSR_OK) lv_label_set_text(message, result_text(event.result));
        render();
    }
    bool wifi = dsr_wifi_connected();
    if (wifi != wifi_previous) {
        wifi_previous = wifi; saved.device.online = false;
        if (wifi) last_poll = now - 5000;
        render();
    }
    if (configured && wifi && !busy && !blocked && !restarting && now - last_poll >= 5000) {
        last_poll = now; (void)submit(DSR_JOB_PROBE, false);
    }
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
