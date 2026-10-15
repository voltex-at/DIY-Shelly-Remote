#include "dsr_platform.h"
#include "cJSON.h"
#include "dsr_discovery.h"
#include "dsr_config.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESPONSE_LIMIT 8192

typedef struct { char *text; size_t used; bool overflow; int64_t deadline; } response_t;
static esp_err_t on_http_event(esp_http_client_event_t *event)
{
    response_t *response = event->user_data;
    if(esp_timer_get_time()>response->deadline)return ESP_FAIL;
    /* A Digest challenge can have a body. Discard it before the retry. */
    if (event->event_id == HTTP_EVENT_HEADERS_SENT) {
        response->used = 0; response->overflow = false; response->text[0] = '\0';
    }
    if (event->event_id == HTTP_EVENT_ON_DATA && event->data_len > 0) {
        size_t length = (size_t)event->data_len;
        if (length > RESPONSE_LIMIT - response->used) {
            response->overflow = true;
            return ESP_FAIL;
        }
        memcpy(response->text + response->used, event->data, length);
        response->used += length;
        response->text[response->used] = '\0';
    }
    return ESP_OK;
}

static dsr_result_t fetch_json_timeout(const dsr_connection_t *connection, const dsr_request_t *request,
                               bool identity_request, cJSON **out,int timeout_ms)
{
    *out = NULL;
    if (!dsr_connection_valid(connection, false)) return DSR_INVALID;
    char url[160];
    int length = snprintf(url, sizeof(url), "http://%s%s", connection->device.host, request->path);
    if (length < 0 || (size_t)length >= sizeof(url)) return DSR_INVALID;
    response_t response = {.text = calloc(RESPONSE_LIMIT + 1, 1),.deadline=esp_timer_get_time()+(int64_t)timeout_ms*1000};
    if (!response.text) return DSR_IO;
    bool authenticated = connection->credentials.username[0] != '\0' && !identity_request;
    esp_http_client_config_t config = {
        .url = url, .timeout_ms = timeout_ms, .event_handler = on_http_event,
        .user_data = &response, .disable_auto_redirect = true,
        .username = authenticated ? connection->credentials.username : NULL,
        .password = authenticated ? connection->credentials.password : NULL,
        .auth_type = !authenticated ? HTTP_AUTH_TYPE_NONE :
            (connection->device.generation == DSR_GEN1 ? HTTP_AUTH_TYPE_BASIC : HTTP_AUTH_TYPE_DIGEST),
        .max_authorization_retries = 1,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) { free(response.text); return DSR_IO; }
    esp_err_t error = ESP_OK;
    if (request->post) {
        error = esp_http_client_set_method(client, HTTP_METHOD_POST);
        if (error == ESP_OK) error = esp_http_client_set_header(client, "Content-Type", "application/json");
        if (error == ESP_OK) error = esp_http_client_set_post_field(client, request->body, (int)strlen(request->body));
    }
    if (error == ESP_OK) error = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    dsr_result_t result;
    if (response.overflow) result = DSR_BAD_RESPONSE;
    else if (status == 401 || status == 403) result = DSR_AUTH_REQUIRED;
    else if (status == 429) result = DSR_RATE_LIMITED;
    else if (error != ESP_OK) result = DSR_OFFLINE;
    else if (status != 200) result = DSR_BAD_RESPONSE;
    else {
        *out = dsr_config_json_depth(response.text)?cJSON_ParseWithLengthOpts(response.text, response.used + 1, NULL, true):NULL;
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
static dsr_result_t fetch_json(const dsr_connection_t *c,const dsr_request_t *r,bool identity_request,cJSON **out)
{return fetch_json_timeout(c,r,identity_request,out,3000);}

dsr_result_t dsr_http_lan_identity(const char *host,dsr_lan_device_t *out)
{
    if(!host||!out||!dsr_ipv4_valid(host))return DSR_INVALID;
    dsr_connection_t c={0};snprintf(c.device.host,sizeof(c.device.host),"%s",host);
    dsr_request_t request={.path="/shelly"};cJSON *json=NULL;
    dsr_result_t result=fetch_json_timeout(&c,&request,true,&json,650);
    if(result==DSR_OK)result=dsr_lan_parse_identity(json,host,out);
    cJSON_Delete(json);return result;
}
dsr_result_t dsr_http_lan_inspect(const dsr_lan_device_t *expected,const dsr_credentials_t *credentials,dsr_lan_capabilities_t *out)
{
    if(!expected||!credentials||!out)return DSR_INVALID;
    dsr_lan_device_t observed;dsr_result_t result=dsr_http_lan_identity(expected->host,&observed);
    if(result!=DSR_OK)return result;
    if(strcmp(expected->id,observed.id)||strcmp(expected->mac,observed.mac)||expected->generation!=observed.generation)return DSR_IDENTITY_MISMATCH;
    dsr_connection_t c={.credentials=*credentials};snprintf(c.device.host,sizeof(c.device.host),"%s",expected->host);
    c.device.generation=expected->generation==1?DSR_GEN1:DSR_GEN2_PLUS;
    dsr_request_t request={0};snprintf(request.path,sizeof(request.path),"%s",expected->generation==1?"/status":"/rpc/Shelly.GetStatus");cJSON *json=NULL;
    result=fetch_json(&c,&request,false,&json);memset(&c,0,sizeof(c));
    if(result==DSR_OK)result=dsr_lan_parse_capabilities(json,expected->generation,out);
    cJSON_Delete(json);return result;
}

static bool identifier(const char *text, bool mac)
{
    if (!text || !text[0] || strlen(text) >= sizeof(((dsr_device_t *)0)->device_id)) return false;
    if (mac && strlen(text) != 12) return false;
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if (mac ? !isxdigit(*p) : !(isalnum(*p) || *p == '-' || *p == '_')) return false;
    }
    return true;
}

static dsr_result_t identity(const dsr_connection_t *connection, dsr_device_t *out)
{
    dsr_request_t request = {.path = "/shelly"};
    cJSON *root = NULL;
    dsr_result_t result = fetch_json(connection, &request, true, &root);
    if (result != DSR_OK) return result;
    cJSON *gen = cJSON_GetObjectItemCaseSensitive(root, "gen");
    cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    cJSON *mac = cJSON_GetObjectItemCaseSensitive(root, "mac");
    const char *device_id = NULL, *name = NULL;
    dsr_generation_t generation;
    if (cJSON_IsNumber(gen) && gen->valuedouble >= 2 &&
        floor(gen->valuedouble) == gen->valuedouble && cJSON_IsString(id) && identifier(id->valuestring, false)) {
        generation = DSR_GEN2_PLUS; device_id = name = id->valuestring;
    } else if ((!gen || (cJSON_IsNumber(gen) && gen->valuedouble == 1)) &&
               cJSON_IsString(type) && strncmp(type->valuestring, "SH", 2) == 0 &&
               cJSON_IsString(mac) && identifier(mac->valuestring, true)) {
        generation = DSR_GEN1; device_id = mac->valuestring; name = type->valuestring;
    } else { cJSON_Delete(root); return DSR_UNSUPPORTED; }
    *out = connection->device;
    out->generation = generation;
    snprintf(out->device_id, sizeof(out->device_id), "%s", device_id);
    if (generation == DSR_GEN1) {
        for (char *p = out->device_id; *p; p++) *p = (char)toupper((unsigned char)*p);
    }
    if (!out->name[0]) snprintf(out->name, sizeof(out->name), "%s", name);
    out->online = false; out->has_switch = false; out->has_power = false;
    cJSON_Delete(root);
    return DSR_OK;
}

static dsr_result_t status(dsr_connection_t *connection)
{
    dsr_device_t *device = &connection->device;
    device->online = false; device->has_power = false;
    dsr_request_t request;
    dsr_result_t result = dsr_request_build(device->generation, device->channel, DSR_GET_STATUS, false, &request);
    if (result != DSR_OK) return result;
    cJSON *root = NULL;
    result = fetch_json(connection, &request, false, &root);
    if (result != DSR_OK) return result;
    cJSON *output = cJSON_GetObjectItemCaseSensitive(root, device->generation == DSR_GEN1 ? "ison" : "output");
    if (!cJSON_IsBool(output)) { cJSON_Delete(root); return DSR_UNSUPPORTED; }
    device->output = cJSON_IsTrue(output);
    device->has_switch = device->online = true;
    device->power_w = 0;
    cJSON *power = cJSON_GetObjectItemCaseSensitive(root, "apower");
    if (device->generation == DSR_GEN2_PLUS && cJSON_IsNumber(power) && isfinite(power->valuedouble)) {
        device->has_power = true; device->power_w = power->valuedouble;
    }
    cJSON_Delete(root);
    return DSR_OK;
}

dsr_result_t dsr_http_probe(const dsr_connection_t *connection, dsr_device_t *out)
{
    if (!out || !dsr_connection_valid(connection, false)) return DSR_INVALID;
    dsr_connection_t probed = *connection;
    dsr_result_t result = identity(connection, &probed.device);
    if(result==DSR_OK&&connection->device.device_id[0]&&strcmp(connection->device.device_id,probed.device.device_id))result=DSR_IDENTITY_MISMATCH;
    if (result == DSR_OK) result = status(&probed);
    *out = probed.device;
    return result;
}

static dsr_result_t verify(dsr_connection_t *connection)
{
    if (!dsr_connection_valid(connection, true)) return DSR_INVALID;
    connection->device.online = false;
    dsr_device_t observed = {0};
    dsr_result_t result = identity(connection, &observed);
    if (result != DSR_OK) return result;
    if (strcmp(observed.device_id, connection->device.device_id) ||
        observed.generation != connection->device.generation) return DSR_IDENTITY_MISMATCH;
    return DSR_OK;
}

dsr_result_t dsr_http_refresh(dsr_connection_t *connection)
{
    dsr_result_t result = verify(connection);
    return result == DSR_OK ? status(connection) : result;
}

dsr_result_t dsr_http_set_output(dsr_connection_t *connection, bool on)
{
    dsr_result_t result = verify(connection);
    if (result != DSR_OK) return result;
    if (!connection->device.has_switch) return DSR_UNSUPPORTED;
    dsr_request_t request;
    result = dsr_request_build(connection->device.generation, connection->device.channel, DSR_SET_OUTPUT, on, &request);
    if (result != DSR_OK) return result;
    cJSON *root = NULL;
    result = fetch_json(connection, &request, false, &root);
    cJSON_Delete(root);
    if (result != DSR_OK) return result;
    /* Never display an optimistic position; don't replay after a timeout. */
    result = status(connection);
    if (result == DSR_OK && connection->device.output != on) {
        connection->device.online = false;
        return DSR_BAD_RESPONSE;
    }
    return result;
}
