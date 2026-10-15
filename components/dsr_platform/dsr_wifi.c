#include "dsr_platform.h"
#include "esp_bit_defs.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static EventGroupHandle_t state;
static esp_timer_handle_t reconnect_timer;
static unsigned retry_seconds = 1;
static bool started;
static void reconnect(void *arg) { (void)arg; (void)esp_wifi_connect(); }
static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        (void)esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(state, BIT0);
        (void)esp_timer_stop(reconnect_timer);
        (void)esp_timer_start_once(reconnect_timer, (uint64_t)retry_seconds * 1000000);
        if (retry_seconds < 32) retry_seconds *= 2;
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        retry_seconds = 1;
        (void)esp_timer_stop(reconnect_timer);
        xEventGroupSetBits(state, BIT0);
    }
}

bool dsr_wifi_connected(void) { return state && (xEventGroupGetBits(state) & BIT0); }

esp_err_t dsr_wifi_start(const dsr_settings_t *settings)
{
    if (!settings || !memchr(settings->ssid, '\0', sizeof(settings->ssid)) ||
        !memchr(settings->password, '\0', sizeof(settings->password)) || !settings->ssid[0]) return ESP_ERR_INVALID_ARG;
    if (started) return ESP_ERR_INVALID_STATE;
    /* Lifecycle is start-once. Initialization failures require a controlled
     * restart; do not retry start and duplicate handlers/netif allocations. */
    started = true;
    esp_err_t error = esp_netif_init();
    if (error != ESP_OK) return error;
    error = esp_event_loop_create_default();
    if (error != ESP_OK && error != ESP_ERR_INVALID_STATE) return error;
    state = xEventGroupCreate();
    if (!state || !esp_netif_create_default_wifi_sta()) return ESP_ERR_NO_MEM;
    esp_timer_create_args_t timer = {.callback = reconnect, .name = "dsr_reconnect"};
    error = esp_timer_create(&timer, &reconnect_timer);
    if (error != ESP_OK) return error;
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    error = esp_wifi_init(&init);
    if (error != ESP_OK) return error;
    error = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, NULL);
    if (error != ESP_OK) return error;
    error = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, NULL);
    if (error != ESP_OK) return error;
    wifi_config_t config = {0};
    memcpy(config.sta.ssid, settings->ssid, strlen(settings->ssid));
    memcpy(config.sta.password, settings->password, strlen(settings->password));
    error = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (error == ESP_OK) error = esp_wifi_set_mode(WIFI_MODE_STA);
    if (error == ESP_OK) error = esp_wifi_set_config(WIFI_IF_STA, &config);
    if (error == ESP_OK) error = esp_wifi_start();
    return error;
}
