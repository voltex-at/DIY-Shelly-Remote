#include "dsr_ui_internal.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static enum { HIDDEN, SCANNING, NETWORKS, PASSWORD, CONNECTING, CONNECTED } page;
static dsr_wifi_scan_t networks;
static dsr_wifi_ap_t selected;
static dsr_settings_t candidate;
static bool scan_requested;
static lv_obj_t *password, *notice, *cancel_button;
static void show_networks(void);
static void show_password(const char *error);

void dsr_wifi_ui_reset(void)
{
    page = HIDDEN; password = notice = cancel_button = NULL; scan_requested = false;
}
bool dsr_wifi_ui_active(void) { return page != HIDDEN; }

static const char *security_name(dsr_wifi_security_t security)
{
    switch (security) {
    case DSR_WIFI_OPEN: return "Open";
    case DSR_WIFI_WPA: return "WPA";
    case DSR_WIFI_WPA2: return "WPA2";
    case DSR_WIFI_WPA3: return "WPA3";
    case DSR_WIFI_WPA2_WPA3: return "WPA2 / WPA3";
    default: return "Unsupported";
    }
}

static void back_to_list(lv_event_t *event) { (void)event; show_networks(); }
static void toggle_password(lv_event_t *event)
{
    if (password) lv_textarea_set_password_mode(password,
        !lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED));
}

static void cancel_connect(lv_event_t *event)
{
    (void)event; dsr_wifi_cancel_connect();
    if (notice) dsr_sc_label_set_text(notice, "Cancelling...\nKeeping previous settings.");
    if (cancel_button) lv_obj_add_state(cancel_button, LV_STATE_DISABLED);
}

static void join(lv_event_t *event)
{
    (void)event;
    if (dsr_network_busy()) { dsr_sc_label_set_text(notice, "Please wait for the current operation."); return; }
    const char *text = password ? lv_textarea_get_text(password) : "";
    if (strlen(text) >= sizeof(candidate.password)) {
        dsr_sc_label_set_text(notice, "Password is too long (byte limit)."); return;
    }
    snprintf(candidate.password, sizeof(candidate.password), "%s", text);
    if (!dsr_wifi_settings_valid(&candidate)) {
        dsr_sc_label_set_text(notice, candidate.wifi_security == DSR_WIFI_WPA3 ? "Enter a password (1-63 bytes)." : "Use 8-63 characters or a 64-digit hex key."); return;
    }
    if (dsr_network_request_wifi(DSR_JOB_WIFI_CONNECT, &candidate) != DSR_OK) {
        dsr_sc_label_set_text(notice, "Cannot start. Please try again."); return;
    }
    lv_obj_t *root = dsr_ui_screen("Connecting to Wi-Fi", false); page = CONNECTING;
    lv_obj_t *ssid = dsr_ui_label(root, selected.ssid, 16, 101, 288);
    lv_obj_set_style_text_align(ssid, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *spinner = lv_spinner_create(root);
    lv_obj_set_pos(spinner, 136, 172); lv_obj_set_size(spinner, 48, 48);
    lv_spinner_set_anim_params(spinner, 1000, 200);
    notice = dsr_ui_label(root, "Authenticating and getting an IP...\nThis can take up to 20 seconds.", 20, 250, 280);
    lv_obj_set_style_text_align(notice, LV_TEXT_ALIGN_CENTER, 0);
    cancel_button = dsr_ui_button(root, "Cancel", 16, 350, 288, cancel_connect, NULL);
}

static void show_password(const char *error)
{
    lv_obj_t *root = dsr_ui_screen("Connect to Wi-Fi", false); page = PASSWORD;
    lv_obj_t *ssid = dsr_ui_label(root, selected.ssid, 16, 81, 288);
    lv_label_set_long_mode(ssid, LV_LABEL_LONG_MODE_DOTS); lv_obj_set_height(ssid, 20);
    if (selected.security == DSR_WIFI_OPEN) {
        dsr_ui_label(root, "Open network\nNo password is required.", 16, 140, 288);
        notice = dsr_ui_label(root, error ? error : "Traffic on this Wi-Fi is not encrypted.", 16, 209, 288);
        lv_obj_t *connect = dsr_ui_button(root, "Connect", 16, 284, 288, join, NULL); dsr_ui_primary(connect);
        dsr_ui_button(root, "Back to networks", 16, 348, 288, back_to_list, NULL); return;
    }
    dsr_ui_label(root, "Password", 16, 108, 190);
    lv_obj_t *show = lv_checkbox_create(root); lv_checkbox_set_text(show, dsr_sc_tr("Show"));
    lv_obj_set_pos(show, 217, 104); lv_obj_set_style_text_color(show, dsr_sc_color(DSR_SC_TEXT), 0);
    lv_obj_add_event_cb(show, toggle_password, LV_EVENT_VALUE_CHANGED, NULL);
    password = lv_textarea_create(root); dsr_ui_textarea(password);
    lv_obj_set_pos(password, 16, 130); lv_obj_set_size(password, 288, 44);
    lv_textarea_set_one_line(password, true); lv_textarea_set_max_length(password, 64);
    lv_textarea_set_password_mode(password, true); lv_textarea_set_text(password, candidate.password);
    notice = dsr_sc_text(root,error ? error : "",16,177,288,&dsr_roboto_12,0xffc077);
    lv_obj_set_height(notice, 18);lv_label_set_long_mode(notice,LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_t *back = dsr_ui_button(root, "Back", 16, 196, 136, back_to_list, NULL);
    lv_obj_t *connect = dsr_ui_button(root, "Connect", 168, 196, 136, join, NULL);
    lv_obj_set_height(back, 44); lv_obj_set_height(connect, 44); dsr_ui_primary(connect);
    lv_obj_t *keyboard = lv_keyboard_create(root); dsr_ui_keyboard(keyboard);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER); lv_keyboard_set_textarea(keyboard, password);
    lv_obj_add_event_cb(keyboard, join, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(keyboard, back_to_list, LV_EVENT_CANCEL, NULL);
}

static void choose_network(lv_event_t *event)
{
    unsigned index = (unsigned)(uintptr_t)lv_event_get_user_data(event);
    if (index >= networks.count || networks.items[index].security == DSR_WIFI_UNSUPPORTED || dsr_network_busy()) return;
    selected = networks.items[index]; dsr_network_wifi_settings(&candidate);
    if (strcmp(candidate.ssid, selected.ssid) || candidate.wifi_security != selected.security) memset(candidate.password, 0, sizeof(candidate.password));
    snprintf(candidate.ssid, sizeof(candidate.ssid), "%s", selected.ssid);
    candidate.wifi_security = (uint8_t)selected.security;
    if (selected.security == DSR_WIFI_OPEN) memset(candidate.password, 0, sizeof(candidate.password));
    show_password(NULL);
}

static void show_networks(void)
{
    lv_obj_t *root = dsr_ui_screen("Wi-Fi networks", false); page = NETWORKS;
    dsr_wifi_info_t info; dsr_wifi_get_info(&info);
    lv_obj_t *caption = dsr_ui_label(root, info.connected ? info.ssid : "Select your 2.4 GHz network", 16, 81, 288);
    lv_label_set_long_mode(caption, LV_LABEL_LONG_MODE_DOTS); lv_obj_set_height(caption, 20);
    lv_obj_set_style_text_color(caption, dsr_sc_color(0x9db5d1), 0);
    lv_obj_t *list = lv_obj_create(root); lv_obj_set_pos(list, 12, 115); lv_obj_set_size(list, 296, 287);
    lv_obj_set_style_pad_all(list, 0, 0); lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_bg_color(list, dsr_sc_color(DSR_SC_BG), 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER); dsr_sc_scrollbar(list);
    if (!networks.count) dsr_ui_label(list, "No networks found.\nTap Refresh to scan again.\nOnly 2.4 GHz Wi-Fi is supported.", 8, 40, 272);
    for (unsigned i = 0; i < networks.count; i++) {
        dsr_wifi_ap_t *ap = &networks.items[i];
        lv_obj_t *row = dsr_ui_button(list, "", 2, (int)i * 78, 280, choose_network, (void *)(uintptr_t)i);
        lv_obj_set_height(row, 70); lv_obj_set_style_pad_all(row, 0, 0); lv_obj_clean(row);
        lv_obj_t *ssid = dsr_ui_label(row, ap->ssid, 12, 10, 218);
        lv_label_set_long_mode(ssid, LV_LABEL_LONG_MODE_DOTS); lv_obj_set_height(ssid, 20);
        lv_obj_t *details = dsr_ui_label(row, "", 12, 39, 254);
        dsr_sc_label_set_text_fmt(details, "%s  |  %d dBm", dsr_sc_tr(security_name(ap->security)), ap->rssi);
        lv_obj_set_style_text_color(details, dsr_sc_color(0xa8bbd3), 0);
        dsr_sc_icon(row,DSR_ICON_WIFI,246,11,22,DSR_SC_MUTED);
        if (ap->security == DSR_WIFI_UNSUPPORTED) lv_obj_add_state(row, LV_STATE_DISABLED);
    }
    dsr_ui_button(root, "Refresh", 12, 420, 140, dsr_network_wifi, NULL);
    dsr_ui_button(root, "Home", 168, 420, 140, dsr_ui_home, NULL);
}

void dsr_network_wifi(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Wi-Fi networks", false); page = SCANNING; scan_requested = true;
    dsr_ui_label(root, "Searching for 2.4 GHz networks...", 16, 97, 288);
    lv_obj_t *spinner = lv_spinner_create(root); lv_obj_set_pos(spinner, 136, 189); lv_obj_set_size(spinner, 48, 48);
    lv_spinner_set_anim_params(spinner, 1000, 200);
    notice = dsr_ui_label(root, "Please wait. You can leave this screen.", 16, 282, 288);
    dsr_ui_button(root, "Home", 16, 420, 288, dsr_ui_home, NULL);
}

void dsr_wifi_ui_tick(void)
{
    if (page == SCANNING && scan_requested && !dsr_network_busy()) {
        dsr_result_t result = dsr_network_request_wifi(DSR_JOB_WIFI_SCAN, NULL);
        if (result == DSR_OK) scan_requested = false;
        else if (result != DSR_BUSY) { scan_requested = false; show_networks(); }
    }
}

void dsr_wifi_ui_result(const dsr_event_t *event, dsr_job_kind_t kind)
{
    if (kind == DSR_JOB_WIFI_SCAN) {
        if (event->result == DSR_OK) networks = event->scan;
        if (page != SCANNING) return;
        show_networks();
        if (event->result != DSR_OK) {
            lv_obj_t *root = lv_screen_active();
            dsr_ui_label(root, "Scan failed. Tap Refresh to try again.", 16, 375, 288);
        }
        return;
    }
    if (page != CONNECTING) return;
    if (event->result == DSR_OK) {
        lv_obj_t *root = dsr_ui_screen("Wi-Fi connected", false); page = CONNECTED;
        dsr_wifi_info_t info; dsr_wifi_get_info(&info);
        lv_obj_t *success = dsr_ui_label(root, LV_SYMBOL_OK "  Connected", 16, 115, 288);
        lv_obj_set_style_text_color(success, dsr_sc_color(0x76e2b6), 0);
        dsr_ui_label(root, candidate.ssid, 16, 168, 288);
        lv_obj_t *ip = dsr_ui_label(root, "", 16, 220, 288); dsr_sc_label_set_text_fmt(ip, "IP address: %s", info.ip);
        dsr_ui_label(root, "Network saved on this remote.\nIt will reconnect automatically.", 16, 269, 288);
        lv_obj_t *done = dsr_ui_button(root, "Continue", 16, 350, 288, dsr_ui_home, NULL); dsr_ui_primary(done);
        dsr_ui_button(root, "Other network", 16, 414, 288, dsr_network_wifi, NULL);
        memset(candidate.password, 0, sizeof(candidate.password));
    } else if (event->wifi_detail == DSR_WIFI_CANCELLED) show_networks();
    else {
        const char *error = "Connection failed. Previous settings kept.";
        if (event->wifi_detail == DSR_WIFI_BAD_PASSWORD) error = "Authentication failed. Check the password.";
        else if (event->wifi_detail == DSR_WIFI_NOT_FOUND) error = "Network unavailable or security changed.";
        else if (event->wifi_detail == DSR_WIFI_TIMED_OUT) error = "No IP received. Check the router and retry.";
        else if (event->wifi_detail == DSR_WIFI_STORAGE_ERROR) error = "Could not save. Previous settings kept.";
        show_password(error);
    }
}
