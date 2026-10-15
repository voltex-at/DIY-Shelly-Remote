/* Runs actual HTTP parsing, binding checks and NVS adapters with real cJSON.
 * The fake HTTP transport does not validate IDF's Digest cryptography or radio. */
#include "dsr_platform.h"
#include "dsr_discovery.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "nvs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
static bool lan_mode;
int64_t esp_timer_get_time(void){return 0;}
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
    bool identity = strcmp(steps[used].path, "/shelly") == 0;
    CHECK(cfg->timeout_ms == (lan_mode&&identity?650:3000));
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
static void test_discovery(void)
{
    const char info2[]="{\"id\":\"shellyplus2pm-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-002P16EU\",\"gen\":2,\"auth_en\":true,\"name\":\"Kitchen\"}";
    const char info1[]="{\"type\":\"SHSW-21\",\"mac\":\"aabbccddeeff\",\"auth\":false}";
    dsr_lan_device_t d={0},one={0};cJSON *json=cJSON_Parse(info2);
    CHECK(dsr_lan_parse_identity(json,"192.168.1.10",&d)==DSR_OK);
    CHECK(d.generation==2&&d.auth_required&&!strcmp(d.mac,"AABBCCDDEEFF")&&!strcmp(d.name,"Kitchen"));cJSON_Delete(json);
    json=cJSON_Parse(info1);CHECK(dsr_lan_parse_identity(json,"192.168.1.11",&one)==DSR_OK);CHECK(one.generation==1&&!one.auth_required&&!strcmp(one.id,"AABBCCDDEEFF"));cJSON_Delete(json);
    const char *bad[]={"[]","{}","{\"gen\":2,\"id\":\"fake\",\"mac\":\"aabbccddeeff\",\"model\":\"model\",\"auth_en\":false}","{\"type\":\"SHSW-1\",\"mac\":\"000000000000\",\"auth\":false}","{\"type\":\"SHSW-1\",\"mac\":\"aabbccddeeff\",\"auth\":\"false\"}","{\"type\":\"SHSW-1\",\"type\":\"SHSW-2\",\"mac\":\"aabbccddeeff\",\"auth\":false}"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){json=cJSON_Parse(bad[i]);CHECK(dsr_lan_parse_identity(json,"192.168.1.10",&one)!=DSR_OK);cJSON_Delete(json);}
    for(int generation=-1;generation<=5;generation++){json=cJSON_Parse(info2);cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(json,"gen"),generation);CHECK((dsr_lan_parse_identity(json,"192.168.1.10",&one)==DSR_OK)==(generation>=2&&generation<=4));cJSON_Delete(json);}
    dsr_lan_snapshot_t scan={0};CHECK(dsr_lan_upsert(&scan,&d)==DSR_OK&&scan.count==1);strcpy(d.host,"192.168.1.99");CHECK(dsr_lan_upsert(&scan,&d)==DSR_OK&&scan.count==1&&!strcmp(scan.devices[0].host,d.host));
    one=d;strcpy(one.mac,"112233445566");CHECK(dsr_lan_upsert(&scan,&one)==DSR_IDENTITY_MISMATCH&&scan.count==1);
    for(unsigned i=1;i<16;i++){one=d;snprintf(one.id,sizeof(one.id),"shelly-test-%u",i);snprintf(one.mac,sizeof(one.mac),"AABBCCDD%04X",i);CHECK(dsr_lan_upsert(&scan,&one)==DSR_OK);}one=d;strcpy(one.id,"shelly-extra");strcpy(one.mac,"001122334455");CHECK(dsr_lan_upsert(&scan,&one)==DSR_FULL);
    dsr_lan_plan_t plan={0};CHECK(dsr_lan_plan_add(&plan,"192.168.1.10","192.168.1.2","255.255.255.0"));CHECK(!dsr_lan_plan_add(&plan,"192.168.1.10","192.168.1.2","255.255.255.0"));
    const char *invalid[]={"192.168.2.10","192.168.1.0","192.168.1.255","192.168.1.2","224.0.0.1","127.0.0.1","192.168.01.3","https://192.168.1.10","1.2.3.4/"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++)CHECK(!dsr_lan_plan_add(&plan,invalid[i],"192.168.1.2","255.255.255.0"));
    CHECK(!dsr_lan_plan_add(&plan,"192.168.1.11","192.168.1.2","255.0.255.0"));
    for(unsigned i=11;i<42;i++){char ip[16];snprintf(ip,sizeof(ip),"192.168.1.%u",i);CHECK(dsr_lan_plan_add(&plan,ip,"192.168.1.2","255.255.255.0"));}CHECK(plan.count==32);CHECK(!dsr_lan_plan_add(&plan,"192.168.1.50","192.168.1.2","255.255.255.0"));
    dsr_lan_capabilities_t caps;
    json=cJSON_Parse("{\"switch:0\":{\"id\":0,\"output\":true},\"switch:3\":{\"id\":3,\"output\":false},\"cover:1\":{\"id\":1,\"state\":\"stopped\"}}");CHECK(dsr_lan_parse_capabilities(json,2,&caps)==DSR_OK&&caps.switches==9&&caps.covers==2);cJSON_Delete(json);
    json=cJSON_Parse("{\"relays\":[{\"ison\":false},{\"ison\":true}],\"rollers\":[{\"state\":\"stop\"}]}");CHECK(dsr_lan_parse_capabilities(json,1,&caps)==DSR_OK&&caps.switches==3&&caps.covers==1);cJSON_Delete(json);
    const char *bad_caps[]={"{\"switch:0\":{\"id\":1,\"output\":true}}","{\"switch:16\":{\"id\":16,\"output\":true}}","{\"switch:0\":{\"id\":0,\"output\":\"true\"}}","{\"switch:0\":{},\"switch:0\":{}}"};
    for(unsigned i=0;i<sizeof(bad_caps)/sizeof(bad_caps[0]);i++){json=cJSON_Parse(bad_caps[i]);CHECK(dsr_lan_parse_capabilities(json,2,&caps)!=DSR_OK);cJSON_Delete(json);}
    lan_mode=true;step_t data[]={{"/shelly",info2,200,false,false},{"/rpc/Shelly.GetStatus","{\"switch:0\":{\"id\":0,\"output\":false}}",200,false,true}};
    fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_lan_identity("192.168.1.10",&d)==DSR_OK&&used==1);
    dsr_connection_t c=connection();fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_lan_inspect(&d,&c.credentials,&caps)==DSR_OK&&caps.switches==1&&used==2);
    data[1].status=401;fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_lan_inspect(&d,&c.credentials,&caps)==DSR_AUTH_REQUIRED);
    data[1].status=429;fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_lan_inspect(&d,&c.credentials,&caps)==DSR_RATE_LIMITED);
    data[0].body=NULL;data[0].status=0;fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_lan_identity("192.168.1.10",&d)==DSR_OFFLINE);
    data[0].body=info1;data[0].status=200;fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_lan_inspect(&d,&c.credentials,&caps)==DSR_IDENTITY_MISMATCH&&used==1);
    lan_mode=false;strcpy(c.device.device_id,"WRONG-IDENTITY");data[0].body=gen2;fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_probe(&c,&c.device)==DSR_IDENTITY_MISMATCH&&used==1);
}
int main(void)
{
    test_probe(); test_control(); test_discovery(); test_storage();
    printf("PASS: %u platform checks (real cJSON, HTTP fixtures, identity, NVS adapters)\n", checks);
}
