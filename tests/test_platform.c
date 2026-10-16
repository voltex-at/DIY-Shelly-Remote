/* Runs actual HTTP parsing, binding checks and NVS adapters with real cJSON.
 * The fake HTTP transport does not validate IDF's Digest cryptography or radio. */
#include "dsr_platform.h"
#include "dsr_fleet.h"
#include "../components/dsr_platform/dsr_ap_private.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "dsr_discovery.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "nvs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
static bool ble_rpc_active;
bool dsr_ble_rpc_active(void){return ble_rpc_active;}
static bool lan_mode,ap_protocol,temporary_ap,cancel_after_drain;
static const char *transport_host="192.168.1.10",*ap_payload,*ap_content_type;
static unsigned ap_restores,ap_suspends,ap_connects;
bool dsr_wifi_connected(void){return !temporary_ap;}
bool dsr_wifi_temporary_active(void){return temporary_ap;}
void dsr_wifi_cancel_connect(void){}
dsr_result_t dsr_wifi_scan(dsr_wifi_scan_t *out){memset(out,0,sizeof(*out));return DSR_OK;}
dsr_result_t dsr_wifi_temporary_connect(const dsr_wifi_ap_t *ap,dsr_wifi_status_t *detail){(void)ap;ap_connects++;temporary_ap=true;*detail=DSR_WIFI_CONNECTED;return DSR_OK;}
dsr_result_t dsr_wifi_restore_home(dsr_wifi_status_t *detail){temporary_ap=false;ap_restores++;*detail=DSR_WIFI_CONNECTED;return DSR_OK;}
dsr_result_t dsr_lan_suspend(void){ap_suspends++;if(cancel_after_drain)dsr_ap_cancel();return DSR_OK;}
void dsr_lan_resume(void){}
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
    CHECK(used < count); CHECK(cfg->disable_auto_redirect); CHECK(cfg->max_authorization_retries == (ap_protocol&&steps[used].post?-1:1));
    bool identity = strcmp(steps[used].path, "/shelly") == 0;
    CHECK(cfg->timeout_ms == (lan_mode&&identity?650:3000));
    CHECK(cfg->auth_type == (identity ? HTTP_AUTH_TYPE_NONE : expected_auth));
    if (identity) CHECK(!cfg->username && !cfg->password);
    else if (expected_auth != HTTP_AUTH_TYPE_NONE) CHECK(!strcmp(cfg->username, "admin") && !strcmp(cfg->password, "test-password"));
    char url[160]; snprintf(url, sizeof(url), "http://%s%s",transport_host, steps[used].path);
    CHECK(!strcmp(cfg->url, url));
    client = (struct test_client){.cfg = *cfg}; return &client;
}
esp_err_t esp_http_client_set_method(esp_http_client_handle_t c, esp_http_client_method_t m)
{ c->post = m == HTTP_METHOD_POST; return ESP_OK; }
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c, const char *key, const char *value)
{ (void)c; CHECK(!strcmp(key, "Content-Type") && !strcmp(value, ap_protocol?ap_content_type:"application/json")); return ESP_OK; }
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
    if (c->post) CHECK(!strcmp(c->body, ap_protocol?ap_payload:"{\"id\":0,\"on\":true}"));
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

static unsigned char blobs[3][8192];
static size_t sizes[3]; static uint32_t schemas[3];
static bool fail_commit;
static bool fail_registry_readback,registry_readback_failed;
static unsigned write_opens,nvs_fault;
static bool writing[3];
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *h)
{ *h = !strcmp(name, "dsr_cfg") ? 0 : !strcmp(name,"dsr_device")?1:2;if(mode==NVS_READWRITE){CHECK(!dsr_activity_restart_reserve());write_opens++;if(nvs_fault==1)return ESP_FAIL;writing[*h]=true;}return mode == NVS_READONLY && !sizes[*h] ? ESP_ERR_NVS_NOT_FOUND : ESP_OK; }
esp_err_t nvs_get_u32(nvs_handle_t h, const char *key, uint32_t *out)
{ (void)key; *out = schemas[h]; return ESP_OK; }
esp_err_t nvs_set_u32(nvs_handle_t h, const char *key, uint32_t value)
{ (void)key;CHECK(writing[h]&&!dsr_activity_restart_reserve());if(nvs_fault==3)return ESP_FAIL;schemas[h] = value; return ESP_OK; }
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *size)
{ (void)key; if(h==2&&registry_readback_failed){registry_readback_failed=false;return ESP_FAIL;}if (*size < sizes[h]) return ESP_ERR_INVALID_SIZE; memcpy(out, blobs[h], sizes[h]); *size = sizes[h]; return ESP_OK; }
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *data, size_t size)
{ (void)key; CHECK(writing[h]&&!dsr_activity_restart_reserve()&&size <= sizeof(blobs[h]));if(nvs_fault==2)return ESP_FAIL;memcpy(blobs[h], data, size); sizes[h] = size; return ESP_OK; }
esp_err_t nvs_commit(nvs_handle_t h) { CHECK(writing[h]&&!dsr_activity_restart_reserve());if(fail_commit||nvs_fault==4)return ESP_FAIL;if(h==2&&fail_registry_readback){fail_registry_readback=false;registry_readback_failed=true;}return ESP_OK; }
void nvs_close(nvs_handle_t h) { if(writing[h])CHECK(!dsr_activity_restart_reserve());writing[h]=false; }
static void admission_open(void){CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}

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
    data[1].body="{\"error\":{\"code\":-104}}";data[1].status=200;
    fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_set_output(&c,true)==DSR_BAD_RESPONSE&&used==2&&!c.device.online);
    data[1].body="{}";data[1].status=401;fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_set_output(&c,true)==DSR_AUTH_REQUIRED&&used==2&&!c.device.online);
    data[1].status=200;data[2].body=NULL;data[2].status=0;fixture(data,3,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_set_output(&c,true)==DSR_OFFLINE&&used==3&&!c.device.online);
}
static void test_telemetry(void)
{
 dsr_connection_t c=connection();dsr_device_t device;dsr_telemetry_t telemetry;
 const char gen3[]="{\"gen\":3,\"id\":\"shelly1pmmini3-AABBCCDDEEFF\"}";
 step_t data[]={{"/shelly",gen3,200,false,false},{"/rpc/Switch.GetStatus?id=0","{\"output\":true,\"apower\":4.1,\"voltage\":230.4,\"current\":0.018}",200,false,true}};
 fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&device,&telemetry)==DSR_OK);CHECK(device.has_power&&device.power_w==4.1);CHECK(telemetry.has_voltage&&telemetry.voltage_v==230.4);CHECK(telemetry.has_current&&telemetry.current_a==0.018);
 c.device=device;c.telemetry=telemetry;
 data[1].body="{\"output\":false}";fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_refresh(&c)==DSR_OK);CHECK(!c.device.has_power&&!c.telemetry.has_voltage&&!c.telemetry.has_current);
 data[1].body="{\"output\":false,\"apower\":0,\"voltage\":0,\"current\":0}";fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_refresh(&c)==DSR_OK);CHECK(c.device.has_power&&c.device.power_w==0&&c.telemetry.has_voltage&&c.telemetry.has_current&&c.telemetry.current_a==0);
 const char *bad[]={"{\"output\":true,\"voltage\":\"230.4\"}","{\"output\":true,\"current\":null}","{\"output\":true,\"voltage\":false}","{\"output\":true,\"current\":[]}","{\"output\":true,\"voltage\":-1}","{\"output\":true,\"current\":-0.1}","{\"output\":true,\"current\":1e999}","{\"output\":true,\"voltage\":1e100}","{\"output\":true,\"apower\":\"4.1\"}"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){data[1].body=bad[i];fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_refresh(&c)==DSR_BAD_RESPONSE);CHECK(!c.device.online&&!c.device.has_power&&!c.telemetry.has_current&&!c.telemetry.has_voltage);}
 data[0].status=401;fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_refresh(&c)==DSR_AUTH_REQUIRED);CHECK(!c.telemetry.has_voltage&&!c.telemetry.has_current);
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
    typedef struct {dsr_device_t device;dsr_credentials_t credentials;} legacy_connection_t;
    legacy_connection_t old={.device=c.device,.credentials=c.credentials};memcpy(blobs[1],&old,sizeof(old));sizes[1]=sizeof(old);schemas[1]=1;
    CHECK(dsr_connection_save(&c) == ESP_OK);
    CHECK(dsr_connection_load(&out, &found) == ESP_OK && found && !out.device.online);
    CHECK(!strcmp(out.credentials.password, c.credentials.password));
    CHECK(sizes[1]==sizeof(legacy_connection_t));
    old=(legacy_connection_t){.device=c.device,.credentials=c.credentials};memcpy(blobs[1],&old,sizeof(old));sizes[1]=sizeof(old);schemas[1]=1;
    CHECK(dsr_connection_load(&out,&found)==ESP_OK&&found&&!strcmp(out.device.device_id,"AABBCCDDEEFF"));CHECK(!out.telemetry.has_voltage&&!out.telemetry.has_current);
    c.telemetry=(dsr_telemetry_t){.has_voltage=true,.has_current=true,.voltage_v=230.4,.current_a=0.018};CHECK(dsr_connection_save(&c)==ESP_OK&&sizes[1]==sizeof(old));CHECK(dsr_connection_load(&out,&found)==ESP_OK&&found&&!out.telemetry.has_voltage&&!out.telemetry.has_current);
    sizes[1]--; CHECK(dsr_connection_load(&out, &found) == ESP_ERR_INVALID_SIZE && !found);
    fail_commit = true; CHECK(dsr_connection_save(&c) == ESP_FAIL); fail_commit = false;
    memset(c.credentials.username, 'a', sizeof(c.credentials.username)); CHECK(!dsr_connection_valid(&c, true));
    admission_open();
    c=connection();c.device.generation=DSR_GEN1;strcpy(c.device.device_id,"AABBCCDDEEFF");
    settings=(dsr_settings_t){.brightness=70,.idle_seconds=60};
    for(unsigned fault=0;fault<=4;fault++){
        nvs_fault=fault;unsigned before=write_opens;
        CHECK((dsr_settings_save(&settings)==ESP_OK)==(fault==0));CHECK(write_opens==before+1&&!writing[0]);admission_open();
        before=write_opens;CHECK((dsr_connection_save(&c)==ESP_OK)==(fault==0||fault==3));CHECK(write_opens==before+1&&!writing[2]);admission_open();
    }
    nvs_fault=0;unsigned before=write_opens;
    CHECK(dsr_settings_save(NULL)==ESP_ERR_INVALID_ARG&&dsr_connection_save(NULL)==ESP_ERR_INVALID_ARG&&write_opens==before);admission_open();
    CHECK(dsr_activity_restart_reserve());CHECK(dsr_settings_save(&settings)==ESP_ERR_INVALID_STATE&&dsr_connection_save(&c)==ESP_ERR_INVALID_STATE&&write_opens==before);dsr_activity_restart_cancel();admission_open();
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
    lan_mode=false;step_t gateway[]={{"/shelly","{\"gen\":3,\"id\":\"shellyblugwg3-aabbccddeeff\",\"model\":\"S3GW-1DBT001\"}",200,false,false}};fixture(gateway,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_probe(&c,&c.device)==DSR_UNSUPPORTED&&used==1);
    lan_mode=false;strcpy(c.device.device_id,"WRONG-IDENTITY");data[0].body=gen2;fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_probe(&c,&c.device)==DSR_IDENTITY_MISMATCH&&used==1);
}
static void test_multi_registry(void)
{
 memset(sizes,0,sizeof(sizes));memset(schemas,0,sizeof(schemas));
 dsr_connection_t a=connection();a.device.generation=DSR_GEN1;strcpy(a.device.device_id,"AABBCCDDEEFF");
 typedef struct {dsr_device_t device;dsr_credentials_t credentials;} legacy_t;
 legacy_t legacy={a.device,a.credentials};memcpy(blobs[1],&legacy,sizeof(legacy));sizes[1]=sizeof(legacy);schemas[1]=1;
 dsr_connection_t loaded[DSR_MAX_DEVICES];size_t n=0;
 CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==1&&!sizes[2]&&loaded[0].legacy_fallback);
 dsr_connection_t b=a;strcpy(b.device.device_id,"112233445566");strcpy(b.device.host,"192.168.1.11");strcpy(b.credentials.password,"second-secret");
 CHECK(dsr_connection_save(&b)==ESP_OK);CHECK(!memcmp(blobs[1],&legacy,sizeof(legacy))&&schemas[1]==1);
 CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==2&&!loaded[0].legacy_fallback&&!loaded[1].legacy_fallback&&!strcmp(loaded[0].device.device_id,a.device.device_id)&&!strcmp(loaded[1].credentials.password,"second-secret"));
 a.layout_id=42;CHECK(dsr_connection_publish(&a)==ESP_OK);
 b.layout_id=42;CHECK(dsr_connection_publish(&b)==ESP_ERR_INVALID_ARG);b.layout_id=43;CHECK(dsr_connection_publish(&b)==ESP_OK);
 dsr_connection_t channel=a;channel.device.channel=1;channel.layout_id=0;CHECK(dsr_connection_save(&channel)==ESP_OK);
 strcpy(a.device.host,"192.168.1.99");a.layout_id=0;CHECK(dsr_connection_save(&a)==ESP_OK);
 CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==3&&loaded[0].layout_id==42&&!strcmp(loaded[0].device.host,a.device.host)&&!strcmp(loaded[2].device.host,a.device.host));
 CHECK(!strcmp(loaded[1].device.host,b.device.host));
 unsigned char durable[8192];size_t durable_size=sizes[2];memcpy(durable,blobs[2],durable_size);
 ((uint32_t *)blobs[2])[0]=99;CHECK(dsr_connections_load(loaded,16,&n)!=ESP_OK&&!n);unsigned before=write_opens;CHECK(dsr_connection_save(&a)!=ESP_OK&&write_opens==before);memcpy(blobs[2],durable,durable_size);
 sizes[2]--;CHECK(dsr_connections_load(loaded,16,&n)!=ESP_OK&&!n);sizes[2]=durable_size;
 /* A committed delete can outlive a failed readback. Retry must converge to
  * success after a valid authoritative registry confirms the tuple absent. */
 fail_registry_readback=true;CHECK(dsr_connection_remove(&b)!=ESP_OK);
 CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==2);
 for(size_t i=0;i<n;i++){CHECK(strcmp(loaded[i].device.device_id,b.device.device_id));}
 unsigned before_retry=write_opens;CHECK(dsr_connection_remove(&b)==ESP_OK&&write_opens==before_retry);
 CHECK(dsr_connection_remove(&b)==ESP_OK);CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==2&&!loaded[0].legacy_fallback&&!loaded[1].legacy_fallback&&!strcmp(loaded[0].device.device_id,a.device.device_id));
 CHECK(dsr_connection_remove(&a)==ESP_OK&&dsr_connection_remove(&channel)==ESP_OK);CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==0);CHECK(!memcmp(blobs[1],&legacy,sizeof(legacy))); /* authoritative empty never resurrects legacy */
 for(unsigned i=0;i<16;i++){a=connection();a.device.generation=DSR_GEN2_PLUS;snprintf(a.device.device_id,sizeof(a.device.device_id),"shelly-%u",i);CHECK(dsr_connection_save(&a)==ESP_OK);}
 CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==16);strcpy(a.device.device_id,"shelly-overflow");CHECK(dsr_connection_save(&a)!=ESP_OK);CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==16);
 admission_open();
}
static void complete_fleet(dsr_fleet_t *f,unsigned index,const dsr_job_t *job,dsr_result_t result,uint32_t now)
{
 dsr_event_t event={.id=job->id,.result=result,.device=job->connection.device,.telemetry=job->connection.telemetry};event.device.online=result==DSR_OK;
 CHECK(dsr_fleet_complete(f,index,&event,now));
}
static void test_multi_scheduler(void)
{
 dsr_fleet_t f={0};dsr_connection_t unpub=connection();strcpy(unpub.device.device_id,"pending");unpub.device.generation=DSR_GEN2_PLUS;unpub.device.online=unpub.device.has_switch=true;
 CHECK(dsr_fleet_upsert(&f,&unpub,0)==DSR_OK&&!f.slots[0].layout_enabled);CHECK(dsr_fleet_request(&f,0,true,true,0)!=DSR_OK);dsr_job_t none;unsigned absent;CHECK(!dsr_fleet_next(&f,5000,true,&none,&absent));
 memset(&f,0,sizeof(f));dsr_connection_t a=connection();a.layout_id=1;strcpy(a.device.device_id,"shelly-A");a.device.generation=DSR_GEN2_PLUS;a.device.online=a.device.has_switch=true;a.device.power_w=12;a.device.has_power=true;a.telemetry=(dsr_telemetry_t){.has_voltage=true,.voltage_v=230};
 dsr_connection_t b=a;b.layout_id=2;strcpy(b.device.host,"192.168.1.11");strcpy(b.device.device_id,"shelly-B");b.device.power_w=55;b.telemetry.voltage_v=240;
 CHECK(dsr_fleet_upsert(&f,&a,0)==DSR_OK&&dsr_fleet_upsert(&f,&b,0)==DSR_OK&&f.count==2);
 dsr_job_t job;unsigned index=99;CHECK(dsr_fleet_next(&f,5000,true,&job,&index)&&index==0&&job.kind==DSR_JOB_PROBE);job.id=1;dsr_fleet_started(&f,index,&job);
 CHECK(dsr_fleet_request(&f,1,true,true,5000)==DSR_OK);CHECK(dsr_fleet_request(&f,1,false,true,5000)==DSR_BUSY);CHECK(!dsr_activity_restart_reserve());
 complete_fleet(&f,0,&job,DSR_OFFLINE,8000);CHECK(!f.slots[0].connection.device.online&&f.slots[1].connection.device.online&&f.slots[1].connection.device.power_w==55);
 CHECK(dsr_fleet_next(&f,8000,true,&job,&index)&&index==1&&job.kind==DSR_JOB_SET_OUTPUT&&job.on);job.id=2;CHECK(dsr_activity_begin());dsr_fleet_started(&f,index,&job);CHECK(!dsr_activity_restart_reserve());
 complete_fleet(&f,1,&job,DSR_OFFLINE,9000);dsr_activity_end();admission_open();CHECK(!dsr_fleet_next(&f,9000,false,&job,&index)); /* never replay timed-out SET */
 dsr_fleet_refresh(&f,10000);CHECK(dsr_fleet_next(&f,10000,true,&job,&index)&&job.kind==DSR_JOB_PROBE);job.id=3;dsr_fleet_started(&f,index,&job);
 dsr_event_t stale={.id=2,.result=DSR_OK,.device=b.device};CHECK(!dsr_fleet_complete(&f,index,&stale,10000));
 stale.id=3;strcpy(stale.device.device_id,"foreign");CHECK(dsr_fleet_complete(&f,index,&stale,10000)&&f.slots[index].blocked);CHECK(strcmp(f.slots[index].connection.device.device_id,"foreign"));
 dsr_fleet_wifi(&f,false,11000);dsr_fleet_wifi(&f,true,12000);CHECK(!f.slots[0].connection.device.online&&!f.slots[1].connection.device.online);
 CHECK(dsr_fleet_upsert(&f,&b,13000)==DSR_OK);f.slots[1].command_id=17;CHECK(dsr_fleet_request(&f,1,false,true,13000)==DSR_OK&&f.slots[1].command_id==17); /* LCD preserves HTTP correlation */
 dsr_fleet_wifi(&f,false,13001);admission_open();
 CHECK(dsr_fleet_upsert(&f,&b,14000)==DSR_OK);CHECK(dsr_fleet_request(&f,1,true,true,30000)==DSR_OFFLINE); /* stale cannot SET */
 printf("Multi memory: connection=%zu slot=%zu fleet=%zu bytes; no extra worker/task\n",sizeof(dsr_connection_t),sizeof(dsr_fleet_slot_t),sizeof(dsr_fleet_t));
}
static void test_ap_protocol(void)
{
 ap_protocol=lan_mode=true;transport_host="192.168.33.1";dsr_ap_prepare();
 const char info2[]="{\"gen\":2,\"id\":\"shellyplus1-aabbccddeeff\",\"mac\":\"AABBCCDDEEFF\",\"model\":\"SNSW-001X16EU\",\"auth_en\":false}";
 const char info1[]="{\"type\":\"SHSW-1\",\"mac\":\"AABBCCDDEEFF\",\"auth\":false}";
 dsr_ap_identity_t expected;step_t data[]={{"/shelly",info2,200,false,false},{"/rpc/WiFi.SetConfig","{\"restart_required\":false}",200,true,false}};
 fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_inspect("ShellyPlus1-AABBCCDDEEFF",&expected)==DSR_OK&&used==1);
 dsr_settings_t home={.ssid="My & WiFi+é",.password="p&+?=x%2F",.wifi_security=DSR_WIFI_WPA2,.brightness=70,.idle_seconds=60};
 ap_payload="{\"config\":{\"sta\":{\"ssid\":\"My & WiFi+é\",\"pass\":\"p&+?=x%2F\",\"enable\":true,\"ipv4mode\":\"dhcp\"}}}";ap_content_type="application/json";
 fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_provision(&expected,&home)==DSR_OK&&used==2);
 transport_host="192.168.1.10";data[1]=(step_t){"/rpc/WiFi.GetStatus","{\"status\":\"got ip\",\"ssid\":\"My & WiFi+é\",\"sta_ip\":\"192.168.1.10\"}",200,false,false};
 fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_verify_lan(&expected,transport_host,&home)==DSR_OK&&used==2);
 expected.ssid[0]=0;fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_lan_verify_station(&expected,transport_host,&home)==DSR_OK&&used==2); /* BLE requires no invented AP SSID. */
 strcpy(expected.ssid,"ShellyPlus1-AABBCCDDEEFF");
 data[1].body="{\"status\":\"got ip\",\"ssid\":\"My & WiFi+é\",\"sta_ip\":\"192.168.1.11\"}";fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_verify_lan(&expected,transport_host,&home)==DSR_BAD_RESPONSE);
 data[1].body="{\"status\":\"connected\",\"ssid\":\"My & WiFi+é\",\"sta_ip\":\"192.168.1.10\"}";fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_verify_lan(&expected,transport_host,&home)==DSR_BAD_RESPONSE);
 data[1].body="{\"status\":\"got ip\",\"ssid\":\"Other home\",\"sta_ip\":\"192.168.1.10\"}";fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_verify_lan(&expected,transport_host,&home)==DSR_BAD_RESPONSE);
 transport_host="192.168.33.1";data[1]=(step_t){"/rpc/WiFi.SetConfig","{\"restart_required\":false}",200,true,false};
 data[1].body=NULL;data[1].status=0;fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_provision(&expected,&home)==DSR_OFFLINE&&used==2); /* no credential POST replay */
 data[1].body="{}";data[1].status=302;fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_provision(&expected,&home)==DSR_BAD_RESPONSE&&used==2);
 data[0].body="{\"gen\":2,\"id\":\"shellyplus1-112233445566\",\"mac\":\"112233445566\",\"model\":\"TEST\",\"auth_en\":false}";fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_provision(&expected,&home)==DSR_IDENTITY_MISMATCH&&used==1);
 data[0].body="{\"gen\":2,\"id\":\"shellyplus1-aabbccddeeff\",\"mac\":\"AABBCCDDEEFF\",\"model\":\"TEST\",\"auth_en\":true}";fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_provision(&expected,&home)==DSR_AUTH_REQUIRED&&used==1);
 data[0].body=info2;dsr_ap_cancel();fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_provision(&expected,&home)==DSR_BUSY&&used==1);dsr_ap_prepare();
 data[0].body=info1;fixture(data,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_inspect("shelly1-DDEEFF",&expected)==DSR_OK);
 data[1]=(step_t){"/settings/sta","{\"enabled\":true,\"ssid\":\"My & WiFi+é\",\"ipv4_method\":\"dhcp\"}",200,true,false};
 ap_payload="enabled=1&ipv4_method=dhcp&ssid=My%20%26%20WiFi%2B%C3%A9&key=p%26%2B%3F%3Dx%252F";ap_content_type="application/x-www-form-urlencoded";
 fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_provision(&expected,&home)==DSR_OK&&used==2);
 home.wifi_security=DSR_WIFI_WPA3;CHECK(dsr_http_ap_provision(&expected,&home)==DSR_UNSUPPORTED&&used==2);home.wifi_security=DSR_WIFI_WPA2;
 transport_host="192.168.1.10";data[1]=(step_t){"/status","{\"wifi_sta\":{\"connected\":true,\"ssid\":\"My & WiFi+é\",\"ip\":\"192.168.1.10\"}}",200,false,false};
 fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_verify_lan(&expected,transport_host,&home)==DSR_OK&&used==2);
 data[1].body="{\"wifi_sta\":{\"connected\":true,\"ssid\":\"Wrong network\",\"ip\":\"192.168.1.10\"}}";fixture(data,2,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_ap_verify_lan(&expected,transport_host,&home)==DSR_BAD_RESPONSE);
 CHECK(dsr_ap_candidate("shelly1-DDEEFF")&&dsr_ap_candidate("ShellyPlus1-AABBCCDDEEFF"));CHECK(!dsr_ap_candidate("FreeWifi")&&!dsr_ap_candidate("shelly1-12345Z")&&!dsr_ap_candidate("shelly1-11223344556"));
 expected.generation=1;CHECK(dsr_ap_identity_matches("shelly1-DDEEFF",&expected)&&!dsr_ap_identity_matches("shelly1-112233",&expected));
 /* Actual AP worker orchestration restores home and never saves settings. */
 CHECK(dsr_settings_save(&home)==ESP_OK);unsigned before=write_opens;
 dsr_job_t job={.kind=DSR_JOB_AP_INSPECT,.ap_confirmed=true,.ap={.ssid="shelly1-DDEEFF",.security=DSR_WIFI_OPEN,.bssid={1,2,3,4,5,6}}};dsr_event_t event={0};
 transport_host="192.168.33.1";data[0].body=info1;fixture(data,1,HTTP_AUTH_TYPE_NONE);dsr_ap_execute(&job,&event);CHECK(event.result==DSR_OK&&temporary_ap&&ap_suspends==1&&write_opens==before);
 job.kind=DSR_JOB_AP_PROVISION;job.ap_identity=event.ap_identity;job.settings=home;data[1]=(step_t){"/settings/sta","{\"enabled\":true,\"ssid\":\"My & WiFi+é\"}",200,true,false};fixture(data,2,HTTP_AUTH_TYPE_NONE);dsr_ap_execute(&job,&event);CHECK(event.result==DSR_OK&&event.ap_write_result==DSR_OK&&!temporary_ap&&ap_restores==1&&write_opens==before&&used==2);
 job.kind=DSR_JOB_AP_INSPECT;job.ap=(dsr_wifi_ap_t){.ssid="shelly1-DDEEFF"};unsigned connects=ap_connects;cancel_after_drain=true;event=(dsr_event_t){0};fixture(data,0,HTTP_AUTH_TYPE_NONE);dsr_ap_execute(&job,&event);CHECK(event.result==DSR_BUSY&&!temporary_ap&&ap_connects==connects&&used==0&&write_opens==before);cancel_after_drain=false;dsr_ap_prepare();
 /* BLE reuses the exact identity+SSID/IP readback without an invented AP
  * name or any home settings/radio write; stale cached home SSID fails closed. */
 job=(dsr_job_t){.kind=DSR_JOB_LAN_STA_VERIFY,.settings=home,.ap_identity={.id="shellyplus1-aabbccddeeff",.mac="AABBCCDDEEFF",.generation=2}};strcpy(job.connection.device.host,"192.168.1.10");
 transport_host="192.168.1.10";data[0]=(step_t){"/shelly",info2,200,false,false};data[1]=(step_t){"/rpc/WiFi.GetStatus","{\"status\":\"got ip\",\"ssid\":\"My & WiFi+é\",\"sta_ip\":\"192.168.1.10\"}",200,false,false};
 unsigned restores=ap_restores,suspends=ap_suspends;fixture(data,2,HTTP_AUTH_TYPE_NONE);dsr_ap_execute(&job,&event);CHECK(event.result==DSR_OK&&used==2&&ap_restores==restores&&ap_suspends==suspends&&write_opens==before);
 strcpy(job.settings.ssid,"stale cached home");fixture(data,0,HTTP_AUTH_TYPE_NONE);dsr_ap_execute(&job,&event);CHECK(event.result==DSR_IO&&used==0&&write_opens==before);
 ble_rpc_active=true;job=(dsr_job_t){.kind=DSR_JOB_AP_INSPECT,.ap={.ssid="shelly1-DDEEFF"}};dsr_ap_execute(&job,&event);CHECK(event.result==DSR_BUSY&&ap_suspends==suspends&&ap_connects==connects);ble_rpc_active=false;
 ap_protocol=lan_mode=false;transport_host="192.168.1.10";admission_open();
}
int main(void)
{
    test_probe(); test_control(); test_telemetry(); test_discovery(); test_storage();test_multi_registry();test_multi_scheduler();test_ap_protocol();
    printf("PASS: %u platform checks (real cJSON, HTTP fixtures, identity, NVS adapters)\n", checks);
}
