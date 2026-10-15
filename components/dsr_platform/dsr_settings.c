#include "dsr_platform.h"
#include "nvs.h"
#include <string.h>

static bool valid(const dsr_settings_t *s)
{
    return s && memchr(s->ssid, '\0', sizeof(s->ssid)) &&
           memchr(s->password, '\0', sizeof(s->password)) &&
           s->wifi_security < DSR_WIFI_UNSUPPORTED &&
           s->brightness <= 100 && s->idle_seconds <= 3600;
}

esp_err_t dsr_settings_load(dsr_settings_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    out->brightness = 70;
    out->idle_seconds = 60;
    nvs_handle_t handle;
    esp_err_t error = nvs_open("dsr_cfg", NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (error != ESP_OK) return error;
    uint32_t schema = 0;
    error = nvs_get_u32(handle, "schema", &schema);
    if (error == ESP_OK && schema != 1 && schema != 2) error = ESP_ERR_NOT_SUPPORTED;
    dsr_settings_t loaded = {0};
    size_t length = sizeof(loaded);
    if (error == ESP_OK) error = nvs_get_blob(handle, "settings", &loaded, &length);
    /* Schema 2 uses former padding; explicitly migrate schema 1. */
    if (error == ESP_OK && schema == 1) loaded.wifi_security = loaded.password[0] ? DSR_WIFI_WPA : DSR_WIFI_OPEN;
    if (error == ESP_OK && (length != sizeof(loaded) || !valid(&loaded))) error = ESP_ERR_INVALID_SIZE;
    if (error == ESP_OK) *out = loaded;
    nvs_close(handle);
    return error;
}

esp_err_t dsr_settings_save(const dsr_settings_t *settings)
{
    if (!valid(settings)) return ESP_ERR_INVALID_ARG;
    nvs_handle_t handle;
    esp_err_t error = nvs_open("dsr_cfg", NVS_READWRITE, &handle);
    if (error != ESP_OK) return error;
    error = nvs_set_blob(handle, "settings", settings, sizeof(*settings));
    if (error == ESP_OK) error = nvs_set_u32(handle, "schema", 2);
    if (error == ESP_OK) error = nvs_commit(handle);
    nvs_close(handle);
    return error;
}
