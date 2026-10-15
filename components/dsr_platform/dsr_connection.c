#include "dsr_platform.h"
#include "nvs.h"
#include <string.h>

bool dsr_connection_valid(const dsr_connection_t *c, bool bound)
{
    if (!c || !memchr(c->device.host, 0, sizeof(c->device.host)) ||
        !memchr(c->device.name, 0, sizeof(c->device.name)) ||
        !memchr(c->device.device_id, 0, sizeof(c->device.device_id)) ||
        !memchr(c->credentials.username, 0, sizeof(c->credentials.username)) ||
        !memchr(c->credentials.password, 0, sizeof(c->credentials.password)) ||
        !dsr_ipv4_valid(c->device.host) || c->device.channel > DSR_MAX_CHANNEL) return false;
    return !bound || (c->device.device_id[0] &&
        (c->device.generation == DSR_GEN1 || c->device.generation == DSR_GEN2_PLUS));
}

esp_err_t dsr_connection_load(dsr_connection_t *out, bool *found)
{
    if (!out || !found) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out)); *found = false;
    nvs_handle_t handle;
    esp_err_t error = nvs_open("dsr_device", NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (error != ESP_OK) return error;
    uint32_t schema = 0;
    error = nvs_get_u32(handle, "schema", &schema);
    if (error == ESP_OK && schema != 1) error = ESP_ERR_NOT_SUPPORTED;
    dsr_connection_t loaded = {0};
    size_t length = sizeof(loaded);
    if (error == ESP_OK) error = nvs_get_blob(handle, "connection", &loaded, &length);
    if (error == ESP_OK && (length != sizeof(loaded) || !dsr_connection_valid(&loaded, true))) error = ESP_ERR_INVALID_SIZE;
    if (error == ESP_OK) {
        loaded.device.online = loaded.device.has_switch = loaded.device.has_power = false;
        *out = loaded; *found = true;
    }
    nvs_close(handle);
    return error;
}

esp_err_t dsr_connection_save(const dsr_connection_t *connection)
{
    if (!dsr_connection_valid(connection, true)) return ESP_ERR_INVALID_ARG;
    nvs_handle_t handle;
    esp_err_t error = nvs_open("dsr_device", NVS_READWRITE, &handle);
    if (error != ESP_OK) return error;
    dsr_connection_t saved = *connection;
    saved.device.online = saved.device.has_switch = saved.device.has_power = false;
    saved.device.output = false; saved.device.power_w = 0;
    error = nvs_set_blob(handle, "connection", &saved, sizeof(saved));
    if (error == ESP_OK) error = nvs_set_u32(handle, "schema", 1);
    if (error == ESP_OK) error = nvs_commit(handle);
    nvs_close(handle);
    return error;
}
