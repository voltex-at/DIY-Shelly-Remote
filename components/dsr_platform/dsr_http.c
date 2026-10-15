#include "dsr_platform.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESPONSE_LIMIT 8192

typedef struct { char *text; size_t used; bool overflow; } response_t;
static esp_err_t on_http_event(esp_http_client_event_t *event)
{
    response_t *response = event->user_data;
    if (event->event_id == HTTP_EVENT_ON_DATA && event->data_len > 0) {
        size_t length = (size_t)event->data_len;
        if (response->used + length > RESPONSE_LIMIT) {
            response->overflow = true;
            return ESP_FAIL;
        }
        memcpy(response->text + response->used, event->data, length);
        response->used += length;
        response->text[response->used] = '\0';
    }
    return ESP_OK;
}

static dsr_result_t fetch_json(const char *host, const dsr_request_t *request, cJSON **out)
{
    *out = NULL;
    if (!dsr_ipv4_valid(host)) return DSR_INVALID;
    char url[160];
    int length = snprintf(url, sizeof(url), "http://%s%s", host, request->path);
    if (length < 0 || (size_t)length >= sizeof(url)) return DSR_INVALID;
    response_t response = {.text = calloc(RESPONSE_LIMIT + 1, 1)};
    if (!response.text) return DSR_IO;
    esp_http_client_config_t config = {
        .url = url, .timeout_ms = 3000, .event_handler = on_http_event,
        .user_data = &response, .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) { free(response.text); return DSR_IO; }
    if (request->post) {
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, request->body, (int)strlen(request->body));
    }
    esp_err_t error = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    dsr_result_t result;
    if (response.overflow) result = DSR_BAD_RESPONSE;
    else if (status == 401 || status == 403) result = DSR_AUTH_REQUIRED;
    else if (error != ESP_OK) result = DSR_OFFLINE;
    else if (status != 200) result = DSR_BAD_RESPONSE;
    else {
        *out = cJSON_ParseWithLengthOpts(response.text, response.used + 1, NULL, true);
        result = cJSON_IsObject(*out) ? DSR_OK : DSR_BAD_RESPONSE;
        if (result != DSR_OK) { cJSON_Delete(*out); *out = NULL; }
        if (*out && cJSON_GetObjectItemCaseSensitive(*out, "error")) {
            cJSON_Delete(*out); *out = NULL; result = DSR_BAD_RESPONSE;
        }
    }
    esp_http_client_cleanup(client);
    free(response.text);
    return result;
}

dsr_result_t dsr_http_refresh(dsr_device_t *device)
{
    if (!device) return DSR_INVALID;
    dsr_request_t request;
    dsr_result_t result = dsr_request_build(device->generation, device->channel,
                                          DSR_GET_STATUS, false, &request);
    if (result != DSR_OK) return result;
    cJSON *root = NULL;
    result = fetch_json(device->host, &request, &root);
    device->online = false;
    if (result != DSR_OK) return result;
    cJSON *output = cJSON_GetObjectItemCaseSensitive(root,
                          device->generation == DSR_GEN1 ? "ison" : "output");
    if (!cJSON_IsBool(output)) { cJSON_Delete(root); return DSR_UNSUPPORTED; }
    device->output = cJSON_IsTrue(output);
    device->has_switch = true;
    device->online = true;
    device->has_power = false;
    device->power_w = 0;
    cJSON *power = cJSON_GetObjectItemCaseSensitive(root, "apower");
    if (device->generation == DSR_GEN2_PLUS && cJSON_IsNumber(power) && isfinite(power->valuedouble)) {
        device->has_power = true;
        device->power_w = power->valuedouble;
    }
    cJSON_Delete(root);
    return DSR_OK;
}

dsr_result_t dsr_http_probe(const char *host, uint8_t channel, dsr_device_t *out)
{
    if (!out || !dsr_ipv4_valid(host) || channel > DSR_MAX_CHANNEL) return DSR_INVALID;
    memset(out, 0, sizeof(*out));
    dsr_request_t request = { .path = "/shelly" };
    cJSON *root = NULL;
    dsr_result_t result = fetch_json(host, &request, &root);
    if (result != DSR_OK) return result;
    cJSON *gen = cJSON_GetObjectItemCaseSensitive(root, "gen");
    cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    const char *name = NULL;
    if (cJSON_IsNumber(gen) && gen->valuedouble >= 2 && cJSON_IsString(id)) {
        out->generation = DSR_GEN2_PLUS;
        name = id->valuestring;
    } else if (!gen && cJSON_IsString(type) && strncmp(type->valuestring, "SH", 2) == 0) {
        out->generation = DSR_GEN1;
        name = type->valuestring;
    } else { cJSON_Delete(root); return DSR_UNSUPPORTED; }
    snprintf(out->name, sizeof(out->name), "%s", name);
    snprintf(out->host, sizeof(out->host), "%s", host);
    out->channel = channel;
    cJSON_Delete(root);
    return dsr_http_refresh(out);
}

dsr_result_t dsr_http_set_output(dsr_device_t *device, bool on)
{
    if (!device || !device->has_switch) return DSR_UNSUPPORTED;
    dsr_request_t request;
    dsr_result_t result = dsr_request_build(device->generation, device->channel,
                                          DSR_SET_OUTPUT, on, &request);
    if (result != DSR_OK) return result;
    cJSON *root = NULL;
    result = fetch_json(device->host, &request, &root);
    cJSON_Delete(root);
    if (result != DSR_OK) { device->online = false; return result; }
    /* Show a confirmed state, not an optimistic switch position. */
    result = dsr_http_refresh(device);
    if (result == DSR_OK && device->output != on) return DSR_BAD_RESPONSE;
    return result;
}
