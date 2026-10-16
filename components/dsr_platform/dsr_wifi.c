#include "dsr_platform.h"
#include "dsr_activity.h"
#include "esp_bit_defs.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IP_READY BIT0
#define CONNECT_FAILED BIT1
#define STA_STOPPED BIT2
#define STA_STARTED BIT3
#define CANCEL BIT4
#define AP_VISIT BIT5
#define SCAN_RECORDS 48
static EventGroupHandle_t events;
static SemaphoreHandle_t control, info_lock;
static esp_timer_handle_t reconnect_timer;
static esp_netif_t *station;
static dsr_settings_t current;
static dsr_wifi_info_t info;
static bool initialized, driver_started, manual, auto_connect, ready;
static bool stop_fence_pending;
static unsigned retry_seconds = 1;

void dsr_wifi_get_info(dsr_wifi_info_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!info_lock) return;
    xSemaphoreTake(info_lock, portMAX_DELAY); *out = info;
    if (events && (xEventGroupGetBits(events) & AP_VISIT)) {
        out->connected = false; out->ip[0] = 0;
        out->status = manual ? DSR_WIFI_CONNECTING : DSR_WIFI_DRIVER_ERROR;
    }
    xSemaphoreGive(info_lock);
}

bool dsr_wifi_connected(void)
{ return events && (xEventGroupGetBits(events) & (IP_READY | AP_VISIT)) == IP_READY; }

static void reconnect(void *arg)
{
    (void)arg;
    if (xSemaphoreTake(control, 0) != pdTRUE) return;
    xSemaphoreTake(info_lock, portMAX_DELAY);
    bool allowed = auto_connect && !manual && driver_started && !info.connected &&
                   !(xEventGroupGetBits(events) & AP_VISIT);
    if (allowed) info.status = DSR_WIFI_CONNECTING;
    xSemaphoreGive(info_lock);
    if (allowed) (void)esp_wifi_connect();
    xSemaphoreGive(control);
}

static dsr_wifi_status_t reason_status(unsigned reason)
{
    if (reason == WIFI_REASON_AUTH_FAIL || reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
        reason == WIFI_REASON_HANDSHAKE_TIMEOUT) return DSR_WIFI_BAD_PASSWORD;
    if (reason == WIFI_REASON_NO_AP_FOUND || reason == WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY ||
        reason == WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD) return DSR_WIFI_NOT_FOUND;
    return DSR_WIFI_DRIVER_ERROR;
}

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        xEventGroupSetBits(events, STA_STARTED); return;
    }
    xSemaphoreTake(info_lock, portMAX_DELAY);
    bool retry = false;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_STOP) {
        info.connected = false; info.ip[0] = 0;
        xEventGroupClearBits(events, IP_READY);
        xEventGroupSetBits(events, STA_STOPPED);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        unsigned reason = data ? ((wifi_event_sta_disconnected_t *)data)->reason : 0;
        info.connected = false; info.ip[0] = 0; info.status = reason_status(reason);
        xEventGroupClearBits(events, IP_READY); xEventGroupSetBits(events, CONNECT_FAILED);
        if (!manual && info.status == DSR_WIFI_BAD_PASSWORD) auto_connect = false;
        retry = auto_connect && !manual;
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP && data &&
               ((ip_event_got_ip_t *)data)->esp_netif == station && (info.status == DSR_WIFI_CONNECTING || info.connected)) {
        ip_event_got_ip_t *got = data;
        info.connected = true; info.status = DSR_WIFI_CONNECTED;
        snprintf(info.ip, sizeof(info.ip), IPSTR, IP2STR(&got->ip_info.ip));
        retry_seconds = 1; xEventGroupSetBits(events, IP_READY);
    }
    xSemaphoreGive(info_lock);
    if (retry) {
        (void)esp_timer_stop(reconnect_timer);
        xSemaphoreTake(info_lock, portMAX_DELAY);
        unsigned delay = retry_seconds;
        if (retry_seconds < 32) retry_seconds *= 2;
        xSemaphoreGive(info_lock);
        (void)esp_timer_start_once(reconnect_timer, (uint64_t)delay * 1000000);
    }
}

static esp_err_t start_station(const dsr_settings_t *settings, bool connect, const uint8_t *bssid)
{
    wifi_config_t config = {0};
    if (settings) {
        memcpy(config.sta.ssid, settings->ssid, strlen(settings->ssid));
        memcpy(config.sta.password, settings->password, strlen(settings->password));
        wifi_auth_mode_t thresholds[] = {WIFI_AUTH_OPEN, WIFI_AUTH_WPA_PSK, WIFI_AUTH_WPA2_PSK,
                                         WIFI_AUTH_WPA3_PSK, WIFI_AUTH_WPA2_PSK};
        config.sta.threshold.authmode = thresholds[settings->wifi_security];
        config.sta.pmf_cfg.capable = true;
        config.sta.pmf_cfg.required = settings->wifi_security == DSR_WIFI_WPA3;
        config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
        if (bssid) { config.sta.bssid_set = true; memcpy(config.sta.bssid, bssid, 6); }
    }
    esp_err_t error = esp_wifi_set_config(WIFI_IF_STA, &config);
    if (error != ESP_OK) return error;
    xEventGroupClearBits(events, STA_STARTED | CONNECT_FAILED | IP_READY);
    xSemaphoreTake(info_lock, portMAX_DELAY);
    info.status = connect ? DSR_WIFI_CONNECTING : DSR_WIFI_IDLE;
    info.connected = false; info.ip[0] = 0;
    snprintf(info.ssid, sizeof(info.ssid), "%s", settings ? settings->ssid : "");
    xSemaphoreGive(info_lock);
    error = esp_wifi_start();
    if (error != ESP_OK) return error;
    driver_started = true;
    if (!(xEventGroupWaitBits(events, STA_STARTED, pdFALSE, pdTRUE, pdMS_TO_TICKS(2000)) & STA_STARTED)) return ESP_FAIL;
    return connect ? esp_wifi_connect() : ESP_OK;
}

static esp_err_t stop_station(void)
{
    (void)esp_timer_stop(reconnect_timer);
    xSemaphoreTake(info_lock, portMAX_DELAY);
    info.status = DSR_WIFI_IDLE; info.connected = false; info.ip[0] = 0;
    xSemaphoreGive(info_lock);
    xEventGroupClearBits(events, IP_READY | CONNECT_FAILED);
    if (driver_started) {
        xEventGroupClearBits(events, STA_STOPPED);
        esp_err_t error = esp_wifi_stop();
        if (error != ESP_OK) return error;
        driver_started = false; stop_fence_pending = true;
    }
    if (!stop_fence_pending) return ESP_OK;
    /* Drain old disconnect/DHCP events before new credentials. An old GOT_IP
     * cannot validate a candidate network after this event-loop fence. */
    if (!(xEventGroupWaitBits(events, STA_STOPPED, pdFALSE, pdTRUE, pdMS_TO_TICKS(2000)) & STA_STOPPED)) return ESP_FAIL;
    stop_fence_pending = false; return ESP_OK;
}

esp_err_t dsr_wifi_start(const dsr_settings_t *settings)
{
    if (initialized) return ESP_ERR_INVALID_STATE;
    if (settings && settings->ssid[0] && !dsr_wifi_settings_valid(settings)) return ESP_ERR_INVALID_ARG;
    initialized = true;
    control = xSemaphoreCreateMutex(); info_lock = xSemaphoreCreateMutex(); events = xEventGroupCreate();
    if (!control || !info_lock || !events) return ESP_ERR_NO_MEM;
    esp_err_t error = esp_netif_init();
    if (error != ESP_OK) return error;
    error = esp_event_loop_create_default();
    if (error != ESP_OK && error != ESP_ERR_INVALID_STATE) return error;
    station = esp_netif_create_default_wifi_sta();
    if (!station) return ESP_ERR_NO_MEM;
    esp_timer_create_args_t timer = {.callback = reconnect, .name = "dsr_reconnect"};
    error = esp_timer_create(&timer, &reconnect_timer);
    if (error != ESP_OK) return error;
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if ((error = esp_wifi_init(&init)) != ESP_OK ||
        (error = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, NULL)) != ESP_OK ||
        (error = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, NULL)) != ESP_OK ||
        (error = esp_wifi_set_storage(WIFI_STORAGE_RAM)) != ESP_OK ||
        (error = esp_wifi_set_mode(WIFI_MODE_STA)) != ESP_OK) return error;
    if (settings) current = *settings;
    auto_connect = settings && settings->ssid[0];
    error = start_station(auto_connect ? settings : NULL, auto_connect, NULL);
    ready = error == ESP_OK; return error;
}

static dsr_wifi_security_t security(wifi_auth_mode_t auth)
{
    switch (auth) {
    case WIFI_AUTH_OPEN: return DSR_WIFI_OPEN;
    case WIFI_AUTH_WPA_PSK: return DSR_WIFI_WPA;
    case WIFI_AUTH_WPA2_PSK: case WIFI_AUTH_WPA_WPA2_PSK: return DSR_WIFI_WPA2;
    case WIFI_AUTH_WPA3_PSK: return DSR_WIFI_WPA3;
    case WIFI_AUTH_WPA2_WPA3_PSK: return DSR_WIFI_WPA2_WPA3;
    default: return DSR_WIFI_UNSUPPORTED;
    }
}

static void automatic_resume(void)
{
    xSemaphoreTake(info_lock, portMAX_DELAY);
    manual = false;
    bool retry = auto_connect && !info.connected && !(xEventGroupGetBits(events) & AP_VISIT);
    xSemaphoreGive(info_lock);
    if (retry) (void)esp_timer_start_once(reconnect_timer, 1000000);
}

dsr_result_t dsr_wifi_scan(dsr_wifi_scan_t *out)
{
    if (!out || !events || !reconnect_timer) return DSR_INVALID;
    memset(out, 0, sizeof(*out));
    if (xSemaphoreTake(control, 0) != pdTRUE) return DSR_BUSY;
    if (xEventGroupGetBits(events) & AP_VISIT) { xSemaphoreGive(control); return DSR_BUSY; }
    xSemaphoreTake(info_lock, portMAX_DELAY); manual = true; xSemaphoreGive(info_lock);
    (void)esp_timer_stop(reconnect_timer);
    esp_err_t error = ESP_OK;
    if (!dsr_wifi_connected()) {
        error = stop_station();
        if (error == ESP_OK) error = start_station(current.ssid[0] ? &current : NULL, false, NULL);
    }
    wifi_ap_record_t *records = calloc(SCAN_RECORDS, sizeof(*records));
    if (!records) { automatic_resume(); xSemaphoreGive(control); return DSR_IO; }
    wifi_scan_config_t config = {.show_hidden = false};
    if (error == ESP_OK) error = esp_wifi_scan_start(&config, true);
    uint16_t count = SCAN_RECORDS;
    if (error == ESP_OK) error = esp_wifi_scan_get_ap_records(&count, records);
    (void)esp_wifi_clear_ap_list();
    if (error == ESP_OK) for (unsigned i = 0; i < count; i++) {
        dsr_wifi_ap_t ap = {.rssi = records[i].rssi, .channel = records[i].primary,
                            .security = security(records[i].authmode)};
        memcpy(ap.ssid, records[i].ssid, 32); ap.ssid[32] = 0;
        if (!ap.ssid[0]) continue;
        unsigned at = 0;
        while (at < out->count && (strcmp(out->items[at].ssid, ap.ssid) || out->items[at].security != ap.security)) at++;
        if (at < out->count) { if (out->items[at].rssi >= ap.rssi) continue; }
        else if (out->count < DSR_WIFI_MAX_NETWORKS) out->count++;
        else { at = out->count - 1; if (out->items[at].rssi >= ap.rssi) continue; }
        out->items[at] = ap;
        while (at && out->items[at].rssi > out->items[at - 1].rssi) {
            dsr_wifi_ap_t swap = out->items[at - 1]; out->items[at - 1] = out->items[at]; out->items[at--] = swap;
        }
    }
    free(records); automatic_resume(); xSemaphoreGive(control);
    return error == ESP_OK ? DSR_OK : DSR_IO;
}

void dsr_wifi_cancel_connect(void) { if (events) xEventGroupSetBits(events, CANCEL); }
void dsr_wifi_prepare_connect(void) { if (events) xEventGroupClearBits(events, CANCEL); }

dsr_result_t dsr_wifi_connect_saved(const dsr_settings_t *settings, dsr_wifi_status_t *detail)
{
    if (!detail || !dsr_wifi_settings_valid(settings) || !events || !reconnect_timer) return DSR_INVALID;
    if (xSemaphoreTake(control, 0) != pdTRUE) return DSR_BUSY;
    *detail = DSR_WIFI_DRIVER_ERROR;
    xSemaphoreTake(info_lock, portMAX_DELAY); manual = true; xSemaphoreGive(info_lock);
    esp_err_t error = stop_station();
    if (error == ESP_OK) error = start_station(settings, true, NULL);
    dsr_result_t result = DSR_OFFLINE;
    if (error == ESP_OK) {
        EventBits_t bits = xEventGroupWaitBits(events, IP_READY | CONNECT_FAILED | CANCEL, pdFALSE, pdFALSE, pdMS_TO_TICKS(20000));
        if (bits & CANCEL) *detail = DSR_WIFI_CANCELLED;
        else if (bits & IP_READY) {
            if (dsr_settings_save(settings) == ESP_OK) {
                current = *settings;
                xEventGroupClearBits(events, AP_VISIT);
                xSemaphoreTake(info_lock, portMAX_DELAY); auto_connect = true; retry_seconds = 1; xSemaphoreGive(info_lock);
                *detail = DSR_WIFI_CONNECTED; result = DSR_OK;
            } else { *detail = DSR_WIFI_STORAGE_ERROR; result = DSR_IO; }
        } else if (bits & CONNECT_FAILED) {
            dsr_wifi_info_t observed; dsr_wifi_get_info(&observed); *detail = observed.status;
        } else *detail = DSR_WIFI_TIMED_OUT;
    }
    if (result != DSR_OK) {
        error = stop_station();
        xSemaphoreTake(info_lock, portMAX_DELAY); auto_connect = current.ssid[0] != 0; xSemaphoreGive(info_lock);
        if (error == ESP_OK) error = start_station(current.ssid[0] ? &current : NULL, current.ssid[0] != 0, NULL);
        if (error == ESP_OK) xEventGroupClearBits(events, AP_VISIT);
        if (error != ESP_OK) {
            xSemaphoreTake(info_lock, portMAX_DELAY); info.status = DSR_WIFI_DRIVER_ERROR; xSemaphoreGive(info_lock);
        }
    }
    automatic_resume(); xSemaphoreGive(control);
    return result;
}

bool dsr_wifi_ap_cancelled(void)
{ return !events || (xEventGroupGetBits(events) & CANCEL) != 0; }

static dsr_wifi_status_t wait_connection(bool cancellable)
{
    EventBits_t mask = IP_READY | CONNECT_FAILED | (cancellable ? CANCEL : 0);
    EventBits_t bits = xEventGroupWaitBits(events, mask, pdFALSE, pdFALSE, pdMS_TO_TICKS(20000));
    if (cancellable && (bits & CANCEL)) return DSR_WIFI_CANCELLED;
    if (bits & IP_READY) return DSR_WIFI_CONNECTED;
    if (bits & CONNECT_FAILED) {
        xSemaphoreTake(info_lock, portMAX_DELAY); dsr_wifi_status_t status = info.status; xSemaphoreGive(info_lock);
        return status;
    }
    return DSR_WIFI_TIMED_OUT;
}

dsr_result_t dsr_wifi_visit_ap(const dsr_wifi_ap_target_t *target, bool confirmed,
    dsr_wifi_ap_action_t action, void *context, dsr_wifi_ap_visit_t *out)
{
    if (!out) return DSR_INVALID;
    memset(out, 0, sizeof(*out)); out->action_result = DSR_INVALID;
    out->ap_status = out->home_status = DSR_WIFI_IDLE;
    bool nonzero = false;
    if (target) for (unsigned i = 0; i < 6; i++) nonzero |= target->bssid[i] != 0;
    if (!target || !confirmed || !action || !nonzero || (target->bssid[0] & 1) ||
        !dsr_wifi_settings_valid(&target->settings) || !ready || !events || !reconnect_timer) return DSR_INVALID;
    if (xSemaphoreTake(control, 0) != pdTRUE) return DSR_BUSY;
    if ((xEventGroupGetBits(events) & AP_VISIT) || !dsr_activity_begin()) {
        xSemaphoreGive(control); return DSR_BUSY;
    }
    /* A prequeued cancellation never disconnects the current LAN. */
    if (dsr_wifi_ap_cancelled()) {
        out->ap_status = DSR_WIFI_CANCELLED;
        dsr_activity_end(); xSemaphoreGive(control); return DSR_OFFLINE;
    }
    xSemaphoreTake(info_lock, portMAX_DELAY); manual = true; xSemaphoreGive(info_lock);
    out->switch_attempted = true;
    xEventGroupSetBits(events, AP_VISIT);
    esp_err_t error = stop_station();
    if (error == ESP_OK && !dsr_wifi_ap_cancelled()) error = start_station(&target->settings, true, target->bssid);
    out->ap_status = dsr_wifi_ap_cancelled() ? DSR_WIFI_CANCELLED :
        error == ESP_OK ? wait_connection(true) : DSR_WIFI_DRIVER_ERROR;
    dsr_result_t result = DSR_OFFLINE;
    if (out->ap_status == DSR_WIFI_CONNECTED && !dsr_wifi_ap_cancelled()) {
        out->action_called = true; out->action_result = action(context);
        result = out->action_result;
        if (dsr_wifi_ap_cancelled()) { out->ap_status = DSR_WIFI_CANCELLED; result = DSR_OFFLINE; }
    }
    /* Cancellation does not cancel recovery. Never start with home credentials
     * until the old event-loop STOP fence is positively observed. */
    error = stop_station();
    if (error == ESP_OK) error = start_station(current.ssid[0] ? &current : NULL, current.ssid[0] != 0, NULL);
    if (error == ESP_OK) {
        out->home_config_restored = true;
        out->home_status = current.ssid[0] ? wait_connection(false) : DSR_WIFI_IDLE;
        out->home_connected = out->home_status == DSR_WIFI_CONNECTED;
        xEventGroupClearBits(events, AP_VISIT);
        if (current.ssid[0] && !out->home_connected) result = DSR_OFFLINE;
    } else {
        out->home_status = DSR_WIFI_DRIVER_ERROR; result = DSR_IO;
        /* Fail closed: no automatic reconnect can reuse the foreign config.
         * Explicit saved-network connect remains available for recovery. */
    }
    automatic_resume(); dsr_activity_end(); xSemaphoreGive(control);
    return result;
}
