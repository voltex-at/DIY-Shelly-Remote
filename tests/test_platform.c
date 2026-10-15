/* Runs actual HTTP parsing, binding checks and NVS adapters with real cJSON.
 * The fake HTTP transport does not validate IDF's Digest cryptography or radio. */
#include "dsr_platform.h"
#include "esp_http_client.h"
#include "nvs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
#define CHECK(expr) do { checks++; if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); assert(expr); } } while (0)
typedef struct { const char *path, *body; int status; bool post, challenge; } step_t;
static step_t steps[8];
static unsigned count, used;
static esp_http_client_auth_type_t expected_auth;
struct test_client { esp_http_client_config_t cfg; bool post; const char *body; };
static struct test_client client;
static void fixture(const step_t *data, unsigned length, esp_http_client_auth_type_t auth)
{ memcpy(steps, data, length * sizeof(*data)); count = length; used = 0; expected_auth = auth; }
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *cfg)
{
    CHECK(used < count); CHECK(cfg->disable_auto_redirect); CHECK(cfg->max_authorization_retries == 1);
    CHECK(cfg->timeout_ms == 3000);
    bool identity = strcmp(steps[used].path, "/shelly") == 0;
    CHECK(cfg->auth_type == (identity ? HTTP_AUTH_TYPE_NONE : expected_auth));
    if (identity) CHECK(!cfg->username && !cfg->password);
    else if (expected_auth != HTTP_AUTH_TYPE_NONE) CHECK(!strcmp(cfg->username, "admin") && !strcmp(cfg->password, "test-password"));
    char url[160]; snprintf(url, sizeof(url), "http://192.168.1.10%s", steps[used].path);
    CHECK(!strcmp(cfg->url, url));
    client = (struct test_client){.cfg = *cfg}; return &client;
}
esp_err_t esp_http_client_set_method(esp_http_client_handle_t c, esp_http_client_method_t m)
{ c->post = m == HTTP_METHOD_POST; return ESP_OK; }
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c, const char *key, const char *value)
{ (void)c; CHECK(!strcmp(key, "Content-Type") && !strcmp(value, "application/json")); return ESP_OK; }
esp_err_t esp_http_client_set_post_field(esp_http_client_handle_t c, const char *body, int length)
{ CHECK(length == (int)strlen(body)); c->body = body; return ESP_OK; }
static esp_err_t emit(esp_http_client_handle_t c, esp_http_client_event_id_t id, const char *data, int length)
{
    esp_http_client_event_t event = {.event_id = id, .user_data = c->cfg.user_data, .data = (void *)data, .data_len = length};
    return c->cfg.event_handler(&event);
}
esp_err_t esp_http_client_perform(esp_http_client_handle_t c)
{
    const step_t *s = &steps[used]; CHECK(c->post == s->post);
    if (c->post) CHECK(!strcmp(c->body, "{\"id\":0,\"on\":true}"));
    emit(c, HTTP_EVENT_HEADERS_SENT, NULL, 0);
    if (s->challenge) {
        emit(c, HTTP_EVENT_ON_DATA, "{\"error\":401}", 13);
        emit(c, HTTP_EVENT_HEADERS_SENT, NULL, 0);
    }
    if (!s->body) return ESP_FAIL;
    size_t length = strlen(s->body), half = length / 2;
    esp_err_t result = emit(c, HTTP_EVENT_ON_DATA, s->body, (int)half);
    if (result == ESP_OK) result = emit(c, HTTP_EVENT_ON_DATA, s->body + half, (int)(length - half));
    return result;
}
int esp_http_client_get_status_code(esp_http_client_handle_t c) { (void)c; return steps[used].status; }
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c) { (void)c; used++; return ESP_OK; }

static unsigned char blobs[2][1024];
static size_t sizes[2]; static uint32_t schemas[2];
static bool fail_commit;
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *h)
{ *h = !strcmp(name, "dsr_cfg") ? 0 : 1; return mode == NVS_READONLY && !sizes[*h] ? ESP_ERR_NVS_NOT_FOUND : ESP_OK; }
esp_err_t nvs_get_u32(nvs_handle_t h, const char *key, uint32_t *out)
{ (void)key; *out = schemas[h]; return ESP_OK; }
esp_err_t nvs_set_u32(nvs_handle_t h, const char *key, uint32_t value)
{ (void)key; schemas[h] = value; return ESP_OK; }
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *size)
{ (void)key; if (*size < sizes[h]) return ESP_ERR_INVALID_SIZE; memcpy(out, blobs[h], sizes[h]); *size = sizes[h]; return ESP_OK; }
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *data, size_t size)
{ (void)key; CHECK(size <= sizeof(blobs[h])); memcpy(blobs[h], data, size); sizes[h] = size; return ESP_OK; }
esp_err_t nvs_commit(nvs_handle_t h) { (void)h; return fail_commit ? ESP_FAIL : ESP_OK; }
void nvs_close(nvs_handle_t h) { (void)h; }

static const char gen2[] = "{\"gen\":2,\"id\":\"shellyplus1-AABBCCDDEEFF\"}";
static const char gen1[] = "{\"type\":\"SHSW-1\",\"mac\":\"aabbccddeeff\"}";
static dsr_connection_t connection(void)
{
    dsr_connection_t c = {.device = {.host = "192.168.1.10"},
        .credentials = {.username = "admin", .password = "test-password"}};
    return c;
}
static void test_probe(void)
{
    dsr_connection_t c = connection(); dsr_device_t device;
    step_t data[] = {{"/shelly", gen2, 200, false, false}, {"/rpc/Switch.GetStatus?id=0", "{\"output\":false,\"apower\":12.5}", 200, false, true}};
    fixture(data, 2, HTTP_AUTH_TYPE_DIGEST);
    CHECK(dsr_http_probe(&c, &device) == DSR_OK && used == 2);
    CHECK(device.generation == DSR_GEN2_PLUS && device.online && !device.output);
    CHECK(device.has_power && device.power_w == 12.5 && !strcmp(device.device_id, "shellyplus1-AABBCCDDEEFF"));
    data[0].body = gen1; data[1].path = "/relay/0"; data[1].body = "{\"ison\":true}";
    fixture(data, 2, HTTP_AUTH_TYPE_BASIC);
    CHECK(dsr_http_probe(&c, &device) == DSR_OK);
    CHECK(device.generation == DSR_GEN1 && device.output && !device.has_power && !strcmp(device.device_id, "AABBCCDDEEFF"));
    c.device.channel = 16;
    CHECK(dsr_http_probe(&c, &device) == DSR_INVALID);
    c = connection(); data[0].body = "{\"gen\":2,\"id\":\"bad/id\"}";
    fixture(data, 1, HTTP_AUTH_TYPE_NONE); CHECK(dsr_http_probe(&c, &device) == DSR_UNSUPPORTED);
    data[0].body = "{\"type\":\"SHSW-1\"}";
    fixture(data, 1, HTTP_AUTH_TYPE_NONE); CHECK(dsr_http_probe(&c, &device) == DSR_UNSUPPORTED);
    data[0].body = gen2; data[1].path = "/rpc/Switch.GetStatus?id=0";
    const char *bad[] = {"{\"output\":\"false\"}", "[]", "{broken", "{\"error\":{\"code\":-1}}", "{} trailing"};
    for (unsigned i = 0; i < sizeof(bad) / sizeof(*bad); i++) {
        data[1].body = bad[i]; fixture(data, 2, HTTP_AUTH_TYPE_DIGEST);
        CHECK(dsr_http_probe(&c, &device) != DSR_OK && !device.online);
    }
    int codes[] = {401, 403, 429, 302, 500};
    dsr_result_t errors[] = {DSR_AUTH_REQUIRED, DSR_AUTH_REQUIRED, DSR_RATE_LIMITED, DSR_BAD_RESPONSE, DSR_BAD_RESPONSE};
    for (unsigned i = 0; i < 5; i++) {
        data[1].body = "{}"; data[1].status = codes[i]; fixture(data, 2, HTTP_AUTH_TYPE_DIGEST);
        CHECK(dsr_http_probe(&c, &device) == errors[i]);
    }
    data[1].status = 0; data[1].body = NULL;
    fixture(data, 2, HTTP_AUTH_TYPE_DIGEST); CHECK(dsr_http_probe(&c, &device) == DSR_OFFLINE);
    char huge[8194]; memset(huge, 'a', sizeof(huge) - 1); huge[sizeof(huge) - 1] = 0;
    data[1].status = 200; data[1].body = huge;
    fixture(data, 2, HTTP_AUTH_TYPE_DIGEST); CHECK(dsr_http_probe(&c, &device) == DSR_BAD_RESPONSE);
}
static void test_control(void)
{
    dsr_connection_t c = connection();
    c.device.generation = DSR_GEN2_PLUS; c.device.has_switch = true;
    strcpy(c.device.device_id, "shellyplus1-AABBCCDDEEFF");
    step_t data[] = {{"/shelly", gen2, 200, false, false}, {"/rpc/Switch.Set", "{\"was_on\":false}", 200, true, true},
        {"/rpc/Switch.GetStatus?id=0", "{\"output\":true}", 200, false, true}};
    fixture(data, 3, HTTP_AUTH_TYPE_DIGEST); CHECK(dsr_http_set_output(&c, true) == DSR_OK && c.device.output && used == 3);
    data[2].body = "{\"output\":false}";
    fixture(data, 3, HTTP_AUTH_TYPE_DIGEST); CHECK(dsr_http_set_output(&c, true) == DSR_BAD_RESPONSE && !c.device.online);
    data[0].body = "{\"gen\":2,\"id\":\"shellyplus1-OTHER\"}";
    fixture(data, 1, HTTP_AUTH_TYPE_DIGEST); CHECK(dsr_http_set_output(&c, true) == DSR_IDENTITY_MISMATCH && used == 1);
    fixture(data, 1, HTTP_AUTH_TYPE_DIGEST); CHECK(dsr_http_refresh(&c) == DSR_IDENTITY_MISMATCH && used == 1);
    data[0].body = gen2; data[1].body = NULL; data[1].status = 0;
    fixture(data, 2, HTTP_AUTH_TYPE_DIGEST); CHECK(dsr_http_set_output(&c, true) == DSR_OFFLINE && used == 2);
    CHECK(!c.device.online); /* no automatic second write or optimistic state */
}
static void test_storage(void)
{
    dsr_settings_t settings; CHECK(dsr_settings_load(&settings) == ESP_OK && settings.brightness == 70 && !settings.ssid[0]);
    strcpy(settings.ssid, "test-network"); strcpy(settings.password, "test-password");
    CHECK(dsr_settings_save(&settings) == ESP_OK);
    dsr_settings_t loaded; CHECK(dsr_settings_load(&loaded) == ESP_OK && !strcmp(loaded.ssid, settings.ssid));
    schemas[0] = 99; CHECK(dsr_settings_load(&loaded) == ESP_ERR_NOT_SUPPORTED && !loaded.ssid[0]);
    CHECK(sizes[0] != 0); /* no auto erase on schema mismatch */
    dsr_connection_t c = connection(), out; bool found;
    CHECK(dsr_connection_load(&out, &found) == ESP_OK && !found);
    CHECK(dsr_connection_save(&c) == ESP_ERR_INVALID_ARG);
    c.device.generation = DSR_GEN1; strcpy(c.device.device_id, "AABBCCDDEEFF"); c.device.online = true;
    CHECK(dsr_connection_save(&c) == ESP_OK);
    CHECK(dsr_connection_load(&out, &found) == ESP_OK && found && !out.device.online);
    CHECK(!strcmp(out.credentials.password, c.credentials.password));
    sizes[1]--; CHECK(dsr_connection_load(&out, &found) == ESP_ERR_INVALID_SIZE && !found);
    fail_commit = true; CHECK(dsr_connection_save(&c) == ESP_FAIL); fail_commit = false;
    memset(c.credentials.username, 'a', sizeof(c.credentials.username)); CHECK(!dsr_connection_valid(&c, true));
}
int main(void)
{
    test_probe(); test_control(); test_storage();
    printf("PASS: %u platform checks (real cJSON, HTTP fixtures, identity, NVS adapters)\n", checks);
}
