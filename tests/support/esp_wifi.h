#ifndef TEST_ESP_WIFI_H
#define TEST_ESP_WIFI_H
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
typedef enum {WIFI_AUTH_OPEN,WIFI_AUTH_WEP,WIFI_AUTH_WPA_PSK,WIFI_AUTH_WPA2_PSK,
 WIFI_AUTH_WPA_WPA2_PSK,WIFI_AUTH_WPA2_ENTERPRISE,WIFI_AUTH_WPA3_PSK,WIFI_AUTH_WPA2_WPA3_PSK} wifi_auth_mode_t;
enum {WIFI_REASON_AUTH_FAIL=202,WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT=15,WIFI_REASON_HANDSHAKE_TIMEOUT=204,
 WIFI_REASON_NO_AP_FOUND=201,WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY=210,WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD=211};
typedef struct {uint16_t reason;} wifi_event_sta_disconnected_t;
typedef struct {uint8_t ssid[33];int8_t rssi;uint8_t primary;wifi_auth_mode_t authmode;} wifi_ap_record_t;
typedef struct {bool show_hidden;} wifi_scan_config_t;
typedef struct {int unused;} wifi_init_config_t;
#define WIFI_INIT_CONFIG_DEFAULT() ((wifi_init_config_t){0})
#define WIFI_STORAGE_RAM 0
#define WIFI_MODE_STA 1
#define WIFI_IF_STA 0
#define WPA3_SAE_PWE_BOTH 3
typedef struct {struct {uint8_t ssid[32],password[64];struct {wifi_auth_mode_t authmode;} threshold;
 struct {bool capable,required;} pmf_cfg;int sae_pwe_h2e;} sta;} wifi_config_t;
esp_err_t esp_wifi_init(const wifi_init_config_t *config);
esp_err_t esp_wifi_set_storage(int storage);
esp_err_t esp_wifi_set_mode(int mode);
esp_err_t esp_wifi_set_config(int interface,const wifi_config_t *config);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config,bool block);
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *count,wifi_ap_record_t *records);
esp_err_t esp_wifi_clear_ap_list(void);
#endif
