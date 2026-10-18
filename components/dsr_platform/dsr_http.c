#include "dsr_platform.h"
#include "dsr_ap_private.h"
#include "cJSON.h"
#include "dsr_discovery.h"
#include "dsr_config.h"
#include "dsr_device_settings.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESPONSE_LIMIT 8192
static bool (*urgent_hook)(void);
static dsr_http_job_guard_t job_guard;
static dsr_http_job_write_hook_t write_hook;
dsr_http_job_write_hook_t dsr_http_job_write_hook(dsr_http_job_write_hook_t hook)
{dsr_http_job_write_hook_t previous=write_hook;write_hook=hook;return previous;}
dsr_http_job_guard_t dsr_http_job_guard(dsr_http_job_guard_t guard)
{dsr_http_job_guard_t previous=job_guard;job_guard=guard;return previous;}
static dsr_result_t allowed(void){return job_guard?job_guard():DSR_OK;}
void dsr_http_urgent_hook(bool (*hook)(void)){urgent_hook=hook;}
static bool urgent(void){return urgent_hook&&urgent_hook();}
static void wipe(void *memory,size_t size)
{volatile unsigned char *p=memory;while(size--)*p++=0;}

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

static dsr_result_t fetch_json_payload(const dsr_connection_t *connection, const dsr_request_t *request,
                               bool identity_request, cJSON **out,int timeout_ms,const char *payload,const char *content_type,bool control_write)
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
        /* AP provisioning never resends its secret-bearing body; device commands may answer a digest challenge. */
        .max_authorization_retries = payload&&!control_write&&strcmp(request->path,"/rpc")?-1:1,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) { free(response.text); return DSR_IO; }
    esp_err_t error = ESP_OK;
    if (request->post) {
        error = esp_http_client_set_method(client, HTTP_METHOD_POST);
        if (error == ESP_OK) error = esp_http_client_set_header(client, "Content-Type", content_type?content_type:"application/json");
        if (error == ESP_OK) error = esp_http_client_set_post_field(client, payload?payload:request->body, (int)strlen(payload?payload:request->body));
    }
    if (error == ESP_OK) {
        dsr_result_t admission=control_write&&!strcmp(request->path,"/rpc")?allowed():DSR_OK;
        if(admission!=DSR_OK){esp_http_client_cleanup(client);wipe(response.text,RESPONSE_LIMIT+1);free(response.text);return admission;}
        if(control_write&&write_hook)write_hook();
        error = esp_http_client_perform(client);
    }
    int status = esp_http_client_get_status_code(client);
    dsr_result_t result;
    if (response.overflow) result = DSR_BAD_RESPONSE;
    else if (status == 401 || status == 403) result = DSR_AUTH_REQUIRED;
    else if (status == 429) result = DSR_RATE_LIMITED;
    else if (error != ESP_OK) result = DSR_OFFLINE;
    else if (status != 200) result = DSR_BAD_RESPONSE;
    else {
        *out = dsr_config_json_depth(response.text)?cJSON_ParseWithLengthOpts(response.text, response.used + 1, NULL, true):NULL;
        bool null_ack=request->post&&(!strncmp(request->path,"/rpc/Cover.",11)||!strcmp(request->path,"/rpc/Light.Set")||!strcmp(request->path,"/rpc/RGB.Set")||!strcmp(request->path,"/rpc/BluTrv.Call"))&&cJSON_IsNull(*out);
        result = (cJSON_IsObject(*out)||null_ack)&&!strstr(response.text,"\\u0000") ? DSR_OK : DSR_BAD_RESPONSE;
        if (result != DSR_OK) { cJSON_Delete(*out); *out = NULL; }
        if (*out && cJSON_GetObjectItemCaseSensitive(*out, "error")) {
            cJSON_Delete(*out); *out = NULL; result = DSR_BAD_RESPONSE;
        }
    }
    esp_http_client_cleanup(client);
    if(payload)wipe(response.text,RESPONSE_LIMIT+1);
    free(response.text);
    return result;
}
static dsr_result_t fetch_json_timeout(const dsr_connection_t *c,const dsr_request_t *r,bool identity_request,cJSON **out,int timeout_ms)
{return fetch_json_payload(c,r,identity_request,out,timeout_ms,NULL,NULL,false);}
static dsr_result_t fetch_command(const dsr_connection_t *c,const dsr_request_t *r,cJSON **out)
{return fetch_json_payload(c,r,false,out,3000,NULL,NULL,true);}
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
dsr_result_t dsr_http_lan_inspect(const dsr_lan_device_t *expected,const dsr_credentials_t *credentials,dsr_lan_capabilities_t *out,dsr_blu_labels_t *labels)
{
    if(labels)memset(labels,0,sizeof(*labels));
    if(!expected||!credentials||!out)return DSR_INVALID;
    dsr_lan_device_t observed;dsr_result_t result=dsr_http_lan_identity(expected->host,&observed);
    if(result!=DSR_OK)return result;
    if(strcmp(expected->id,observed.id)||strcmp(expected->mac,observed.mac)||expected->generation!=observed.generation)return DSR_IDENTITY_MISMATCH;
    dsr_connection_t c={.credentials=*credentials};snprintf(c.device.host,sizeof(c.device.host),"%s",expected->host);
    c.device.generation=expected->generation==1?DSR_GEN1:DSR_GEN2_PLUS;
    dsr_request_t request={0};snprintf(request.path,sizeof(request.path),"%s",expected->generation==1?"/status":"/rpc/Shelly.GetStatus");cJSON *json=NULL;
    result=fetch_json(&c,&request,false,&json);
    if(result==DSR_OK)result=dsr_lan_parse_capabilities(json,expected->generation,out);
    if(result==DSR_OK&&observed.profile){
        if(observed.profile!=1)out->switches=0;
        if(observed.profile!=2)out->covers=0;
        if(observed.profile!=3)out->lights=0;
        if(observed.profile!=4)out->rgbs=0;
        if(!out->switches&&!out->covers&&!out->lights&&!out->rgbs&&!out->ht&&!out->blu)result=DSR_UNSUPPORTED;
    }
    /* BTHome devices are dynamic components; a gateway's Shelly.GetStatus may
     * omit them. Only Bluetooth-capable devices are asked; a failed page keeps
     * what earlier pages proved and never fails the inspection. */
    if(result==DSR_OK&&expected->generation>=2&&(cJSON_GetObjectItemCaseSensitive(json,"ble")||cJSON_GetObjectItemCaseSensitive(json,"blugw"))){
        unsigned offset=0;
        for(unsigned page=0;page<12;page++){
            dsr_request_t list={0};snprintf(list.path,sizeof(list.path),"/rpc/Shelly.GetComponents?dynamic_only=true&offset=%u",offset);cJSON *body=NULL;
            unsigned next=0,total=0;dsr_result_t listed=fetch_json(&c,&list,false,&body);
            if(listed==DSR_OK)listed=dsr_lan_parse_components(body,offset,&out->blu,labels,&next,&total);
            cJSON_Delete(body);if(listed!=DSR_OK||next>=total)break;offset=next;
        }
    }
    memset(&c,0,sizeof(c));
    if(result==DSR_OK&&out->rgbs&&strcmp(observed.model,DSR_RGB_MODEL))result=DSR_UNSUPPORTED;
    if(result==DSR_OK&&out->ht&&(observed.generation!=3||strcmp(observed.model,DSR_HT_MODEL)))out->ht=false;
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

static bool light_unique(const cJSON *r);
/* Printable ASCII only and bounded: the strings come from the network and are shown on the LCD. */
static void info_text(const cJSON *item,char *out,size_t size)
{
    out[0]=0;
    if(!cJSON_IsString(item)||!item->valuestring[0]||strlen(item->valuestring)>=size)return;
    for(const unsigned char *p=(const unsigned char *)item->valuestring;*p;p++)if(*p<0x20||*p>0x7e)return;
    strcpy(out,item->valuestring);
}
static dsr_result_t identity(const dsr_connection_t *connection, dsr_device_t *out, dsr_device_info_t *info)
{
    dsr_request_t request = {.path = "/shelly"};
    cJSON *root = NULL;
    dsr_result_t result = fetch_json(connection, &request, true, &root);
    if (result != DSR_OK) return result;
    if(!light_unique(root)){cJSON_Delete(root);return DSR_BAD_RESPONSE;}
    /* BLU Gateway Gen3 has no Switch component. Never route its manual bind
     * through a relay endpoint or disclose saved relay credentials to it. */
    const cJSON *model=cJSON_GetObjectItemCaseSensitive(root,"model");
    if(cJSON_IsString(model)&&!strcmp(model->valuestring,"S3GW-1DBT001")&&connection->device.endpoint_kind!=DSR_ENDPOINT_BLU){cJSON_Delete(root);return DSR_UNSUPPORTED;}
    const cJSON *profile=cJSON_GetObjectItemCaseSensitive(root,"profile");
    if(profile&&connection->device.endpoint_kind<=DSR_ENDPOINT_RGB){
        const char *expected=connection->device.endpoint_kind==DSR_ENDPOINT_COVER?"cover":connection->device.endpoint_kind==DSR_ENDPOINT_LIGHT?"light":connection->device.endpoint_kind==DSR_ENDPOINT_RGB?"rgb":"switch";
        if(!cJSON_IsString(profile)||strcmp(profile->valuestring,expected)){cJSON_Delete(root);return DSR_UNSUPPORTED;}
    }
    if(connection->device.endpoint_kind==DSR_ENDPOINT_RGB){
        const cJSON *profile=cJSON_GetObjectItemCaseSensitive(root,"profile");
        const cJSON *rgb_gen=cJSON_GetObjectItemCaseSensitive(root,"gen");
        if(!light_unique(root)){cJSON_Delete(root);return DSR_BAD_RESPONSE;}
        if(!cJSON_IsNumber(rgb_gen)||rgb_gen->valuedouble!=2||connection->device.channel!=0||!cJSON_IsString(model)||strcmp(model->valuestring,DSR_RGB_MODEL)||!cJSON_IsString(profile)||strcmp(profile->valuestring,"rgb")){cJSON_Delete(root);return DSR_UNSUPPORTED;}
    }
    if(cJSON_IsString(model)&&!strcmp(model->valuestring,DSR_HT_MODEL)&&connection->device.endpoint_kind!=DSR_ENDPOINT_HT){cJSON_Delete(root);return DSR_UNSUPPORTED;}
    if(connection->device.endpoint_kind==DSR_ENDPOINT_HT){
        const cJSON *physical_gen=cJSON_GetObjectItemCaseSensitive(root,"gen");
        if(!light_unique(root)){cJSON_Delete(root);return DSR_BAD_RESPONSE;}
        if(connection->device.channel!=0||!cJSON_IsNumber(physical_gen)||physical_gen->valuedouble!=3||!cJSON_IsString(model)||strcmp(model->valuestring,DSR_HT_MODEL)){cJSON_Delete(root);return DSR_UNSUPPORTED;}
    }
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
    if (info && generation == DSR_GEN2_PLUS && out->endpoint_kind!=DSR_ENDPOINT_BLU) {
        info_text(cJSON_GetObjectItemCaseSensitive(root, "model"), info->model, sizeof(info->model));
        info_text(cJSON_GetObjectItemCaseSensitive(root, "ver"), info->fw, sizeof(info->fw));
        info->generation=(uint8_t)gen->valueint;
    }
    cJSON_Delete(root);
    if(out->endpoint_kind==DSR_ENDPOINT_BLU){
        /* Endpoint identity = gateway id + BLU MAC: a cid reassigned by the
         * gateway to another sensor is an identity mismatch, not new data. */
        if(generation!=DSR_GEN2_PLUS)return DSR_UNSUPPORTED;
        unsigned cid=DSR_BLU_FIRST_CID+out->channel;dsr_request_t blu={0};cJSON *config=NULL;char mac[13];
        snprintf(blu.path,sizeof(blu.path),"/rpc/BTHomeDevice.GetConfig?id=%u",cid);
        dsr_result_t r=fetch_json(connection,&blu,false,&config);
        if(r==DSR_OK)r=dsr_blu_parse_address(config,cid,mac);
        if(r==DSR_OK&&strlen(out->device_id)+1+12>=sizeof(out->device_id))r=DSR_BAD_RESPONSE;
        if(r==DSR_OK){strcat(out->device_id,"_");strcat(out->device_id,mac);}
        cJSON_Delete(config);if(r!=DSR_OK)return r;
    }
    return DSR_OK;
}

static bool cover_parse(const cJSON *r,uint8_t channel,dsr_cover_status_t *out)
{
 if(!cJSON_IsObject(r))return false;
 for(const cJSON *a=r->child;a;a=a->next)for(const cJSON *b=a->next;b;b=b->next)if(!strcmp(a->string,b->string))return false;
 const cJSON *id=cJSON_GetObjectItemCaseSensitive(r,"id"),*state=cJSON_GetObjectItemCaseSensitive(r,"state"),*control=cJSON_GetObjectItemCaseSensitive(r,"pos_control");
 if(!cJSON_IsNumber(id)||id->valuedouble!=channel||!cJSON_IsString(state)||!cJSON_IsBool(control))return false;
 const char *states[]={"","open","closed","opening","closing","stopped","calibrating"};dsr_cover_status_t c={0};
 for(unsigned i=1;i<sizeof(states)/sizeof(states[0]);i++)if(!strcmp(state->valuestring,states[i]))c.state=(uint8_t)i;
 if(!c.state)return false;
 c.flags=DSR_COVER_VALID|(cJSON_IsTrue(control)?DSR_COVER_POSITION_CONTROL:0);
 const char *positions[]={"current_pos","target_pos"};
 for(unsigned i=0;i<2;i++){const cJSON *v=cJSON_GetObjectItemCaseSensitive(r,positions[i]);if(!v||cJSON_IsNull(v))continue;
  if(!cJSON_IsNumber(v)||!isfinite(v->valuedouble)||v->valuedouble<0||v->valuedouble>100||floor(v->valuedouble)!=v->valuedouble)return false;
  if(i){c.target_pos=(uint8_t)v->valueint;c.flags|=DSR_COVER_TARGET;}else{c.current_pos=(uint8_t)v->valueint;c.flags|=DSR_COVER_POSITION;}
 }
 const cJSON *errors=cJSON_GetObjectItemCaseSensitive(r,"errors");
 if(errors){if(!cJSON_IsArray(errors)||cJSON_GetArraySize(errors)>16)return false;
  const char *known[]={"overtemp","overpower","overvoltage","undervoltage","overcurrent","obstruction","safety_switch"};
  const cJSON *v;cJSON_ArrayForEach(v,errors){if(!cJSON_IsString(v)||!v->valuestring[0]||strlen(v->valuestring)>64)return false;uint8_t bit=128;
   for(unsigned i=0;i<7;i++)if(!strcmp(v->valuestring,known[i]))bit=(uint8_t)(1u<<i);
   c.errors|=bit;
  }
 }
 *out=c;return true;
}
static bool light_unique(const cJSON *r)
{
 if(cJSON_IsObject(r))for(const cJSON *a=r->child;a;a=a->next){
  if(!a->string)return false;
  for(const cJSON *b=a->next;b;b=b->next)if(!strcmp(a->string,b->string))return false;
 }
 for(const cJSON *v=r?r->child:NULL;v;v=v->next)if(!light_unique(v))return false;
 return true;
}
static bool light_percent(const cJSON *v)
{return cJSON_IsNumber(v)&&isfinite(v->valuedouble)&&v->valuedouble>=0&&v->valuedouble<=100&&floor(v->valuedouble)==v->valuedouble;}
static bool light_parse(const cJSON *r,uint8_t channel,dsr_light_status_t *out)
{
 if(!cJSON_IsObject(r)||!light_unique(r))return false;
 const cJSON *id=cJSON_GetObjectItemCaseSensitive(r,"id"),*output=cJSON_GetObjectItemCaseSensitive(r,"output"),*brightness=cJSON_GetObjectItemCaseSensitive(r,"brightness");
 if(!cJSON_IsNumber(id)||id->valuedouble!=channel||!cJSON_IsBool(output)||!light_percent(brightness))return false;
 dsr_light_status_t s={.brightness=(uint8_t)brightness->valueint,.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS};
 const cJSON *transition=cJSON_GetObjectItemCaseSensitive(r,"transition");
 if(transition){
  const cJSON *target=cJSON_GetObjectItemCaseSensitive(transition,"target"),*level=cJSON_GetObjectItemCaseSensitive(target,"brightness"),*on=cJSON_GetObjectItemCaseSensitive(target,"output");
  if(!cJSON_IsObject(transition)||!cJSON_IsObject(target)||!light_percent(level)||!cJSON_IsBool(on))return false;
  s.target=(uint8_t)level->valueint;s.flags|=DSR_LIGHT_TRANSITION|DSR_LIGHT_TARGET|DSR_LIGHT_TARGET_OUTPUT;
  if(cJSON_IsTrue(on))s.flags|=DSR_LIGHT_TARGET_ON;
 }
 const cJSON *calibration=cJSON_GetObjectItemCaseSensitive(r,"calibration");
 if(calibration){if(!cJSON_IsObject(calibration))return false;s.conditions|=DSR_LIGHT_CALIBRATING;const cJSON *progress=cJSON_GetObjectItemCaseSensitive(calibration,"progess");if(progress&&!light_percent(progress))return false;}
 const char *keys[]={"errors","flags"};
 for(unsigned n=0;n<2;n++){
  const cJSON *array=cJSON_GetObjectItemCaseSensitive(r,keys[n]);if(!array)continue;
  if(!cJSON_IsArray(array)||cJSON_GetArraySize(array)>16)return false;
  const cJSON *v;cJSON_ArrayForEach(v,array){
   if(!cJSON_IsString(v)||!v->valuestring[0]||strlen(v->valuestring)>64)return false;
   if(n){s.conditions|=!strcmp(v->valuestring,"uncalibrated")?DSR_LIGHT_UNCALIBRATED:!strcmp(v->valuestring,"no_load")?DSR_LIGHT_NO_LOAD:DSR_LIGHT_UNKNOWN_CONDITION;}
   else {const char *known[]={"overtemp","overpower","overvoltage","undervoltage","overcurrent","unsupported_load"};uint8_t bit=128;for(unsigned i=0;i<6;i++)if(!strcmp(v->valuestring,known[i]))bit=(uint8_t)(1u<<i);s.errors|=bit;}
  }
 }
 *out=s;return true;
}
static bool rgb_array(const cJSON *a,uint8_t out[3])
{
 if(!cJSON_IsArray(a)||cJSON_GetArraySize(a)!=3)return false;
 unsigned i=0;for(const cJSON *v=a->child;v;v=v->next,i++){
  if(!cJSON_IsNumber(v)||!isfinite(v->valuedouble)||v->valuedouble<0||v->valuedouble>255||floor(v->valuedouble)!=v->valuedouble)return false;
  out[i]=(uint8_t)v->valueint;
 }
 return true;
}
/* Validate the complete target before JSON release. Only the actual current
 * sample is retained; a partial cached target never acknowledges a command. */
static bool rgb_parse(const cJSON *r,uint8_t channel,dsr_rgb_status_t *out,const dsr_rgb_intent_t *intent,bool *matches)
{
 if(!cJSON_IsObject(r)||!light_unique(r)||channel!=0)return false;
 const cJSON *id=cJSON_GetObjectItemCaseSensitive(r,"id"),*output=cJSON_GetObjectItemCaseSensitive(r,"output"),*brightness=cJSON_GetObjectItemCaseSensitive(r,"brightness");
 uint8_t color[3];
 if(!cJSON_IsNumber(id)||id->valuedouble!=channel||!cJSON_IsBool(output)||!light_percent(brightness)||!rgb_array(cJSON_GetObjectItemCaseSensitive(r,"rgb"),color))return false;
 dsr_rgb_status_t sample={.r=color[0],.g=color[1],.b=color[2],.brightness=(uint8_t)brightness->valueint,.flags=DSR_RGB_VALID};
 const cJSON *transition=cJSON_GetObjectItemCaseSensitive(r,"transition");
 if(transition){
  const cJSON *target=cJSON_GetObjectItemCaseSensitive(transition,"target");
  output=cJSON_GetObjectItemCaseSensitive(target,"output");brightness=cJSON_GetObjectItemCaseSensitive(target,"brightness");
  if(!cJSON_IsObject(transition)||!cJSON_IsObject(target)||!cJSON_IsBool(output)||!light_percent(brightness)||!rgb_array(cJSON_GetObjectItemCaseSensitive(target,"rgb"),color))return false;
  sample.flags|=DSR_RGB_TRANSITION;
 }
 const cJSON *errors=cJSON_GetObjectItemCaseSensitive(r,"errors");
 if(errors){
  if(!cJSON_IsArray(errors)||cJSON_GetArraySize(errors)>16)return false;
  for(const cJSON *v=errors->child;v;v=v->next){if(!cJSON_IsString(v)||!v->valuestring[0]||strlen(v->valuestring)>64)return false;sample.flags|=DSR_RGB_ERROR;}
 }
 if(matches&&intent)*matches=!(sample.flags&DSR_RGB_ERROR)&&(intent->command==DSR_RGB_SET_COLOR?!memcmp(color,intent->rgb,3):intent->command==DSR_RGB_SET_BRIGHTNESS?brightness->valueint==intent->brightness:cJSON_IsTrue(output)==(intent->command==DSR_RGB_ON));
 *out=sample;return true;
}
/* BLU TRV classification carried in the caller's telemetry (see DSR_BLU_TRV_CHECKED). */
static uint8_t trv_hint(const dsr_connection_t *c){return c->device.endpoint_kind==DSR_ENDPOINT_BLU?(uint8_t)(c->telemetry.blu.errors&(DSR_BLU_TRV|DSR_BLU_TRV_CHECKED)):0u;}
static dsr_result_t status_expected(dsr_connection_t *connection,const dsr_rgb_intent_t *expected)
{
    dsr_device_t *device = &connection->device;
    device->online = false; device->has_power = false;
    uint8_t hint=trv_hint(connection);
    memset(&connection->telemetry,0,sizeof(connection->telemetry));
    dsr_request_t request;
    dsr_result_t result;
    if(device->endpoint_kind==DSR_ENDPOINT_BLU){
        if(device->generation!=DSR_GEN2_PLUS)return DSR_UNSUPPORTED;
        unsigned cid=DSR_BLU_FIRST_CID+device->channel;cJSON *root=NULL;
        memset(&request,0,sizeof(request));snprintf(request.path,sizeof(request.path),"/rpc/BTHomeDevice.GetStatus?id=%u",cid);
        result=fetch_json(connection,&request,false,&root);
        if(result==DSR_OK){result=dsr_blu_parse_device(root,cid,&connection->telemetry);}
        cJSON_Delete(root);root=NULL;
        dsr_blu_object_t objects[DSR_BLU_MAX_OBJECTS];unsigned count=0;
        if(result==DSR_OK){snprintf(request.path,sizeof(request.path),"/rpc/BTHomeDevice.GetKnownObjects?id=%u",cid);result=fetch_json(connection,&request,false,&root);
            if(result==DSR_OK){result=dsr_blu_parse_known(root,cid,objects,DSR_BLU_MAX_OBJECTS,&count);}
            cJSON_Delete(root);root=NULL;}
        for(unsigned i=0;result==DSR_OK&&i<count;i++){
            if(!dsr_blu_sensor_shown(objects[i].obj_id))continue;
            snprintf(request.path,sizeof(request.path),"/rpc/BTHomeSensor.GetStatus?id=%u",(unsigned)objects[i].sensor);
            result=fetch_json(connection,&request,false,&root);
            if(result==DSR_OK){result=dsr_blu_apply_sensor(root,objects[i].sensor,objects[i].obj_id,&connection->telemetry);}
            cJSON_Delete(root);root=NULL;
        }
        if(result==DSR_OK){
            int trv=(hint&DSR_BLU_TRV_CHECKED)?((hint&DSR_BLU_TRV)?1:0):-1;
            if(trv<0){
                snprintf(request.path,sizeof(request.path),"/rpc/BluTrv.GetConfig?id=%u",cid);dsr_result_t asked=fetch_json(connection,&request,false,&root);
                /* An error answer means "no such BluTrv". Transport failures
                 * stay unknown and are asked again next poll. */
                if(asked==DSR_OK||asked==DSR_BAD_RESPONSE)trv=asked==DSR_OK&&dsr_blu_trv_linked(root,cid,cid);
                cJSON_Delete(root);root=NULL;
            }
            if(trv>=0)connection->telemetry.blu.errors|=DSR_BLU_TRV_CHECKED;
            if(trv==1){
                snprintf(request.path,sizeof(request.path),"/rpc/BluTrv.GetStatus?id=%u",cid);result=fetch_json(connection,&request,false,&root);
                if(result==DSR_OK)result=dsr_blu_parse_trv(root,cid,&connection->telemetry);
                cJSON_Delete(root);root=NULL;
            }
        }
        if(result!=DSR_OK)memset(&connection->telemetry,0,sizeof(connection->telemetry));
        device->has_switch=device->has_power=device->output=false;device->power_w=0;device->online=result==DSR_OK;return result;
    }
    if(device->endpoint_kind==DSR_ENDPOINT_HT){
        if(device->generation!=DSR_GEN2_PLUS||device->channel!=0)return DSR_UNSUPPORTED;
        memset(&request,0,sizeof(request));strcpy(request.path,"/rpc/Shelly.GetStatus");cJSON *root=NULL;
        result=fetch_json(connection,&request,false,&root);if(result==DSR_OK)result=dsr_ht_parse(root,&connection->telemetry);cJSON_Delete(root);
        device->has_switch=device->has_power=device->output=false;device->power_w=0;device->online=result==DSR_OK;return result;
    }
    if(device->endpoint_kind==DSR_ENDPOINT_COVER||device->endpoint_kind==DSR_ENDPOINT_LIGHT||device->endpoint_kind==DSR_ENDPOINT_RGB){
        if(device->generation!=DSR_GEN2_PLUS)return DSR_UNSUPPORTED;
        memset(&request,0,sizeof(request));snprintf(request.path,sizeof(request.path),"/rpc/%s.GetStatus?id=%u",device->endpoint_kind==DSR_ENDPOINT_COVER?"Cover":device->endpoint_kind==DSR_ENDPOINT_RGB?"RGB":"Light",device->channel);result=DSR_OK;
    }else result = dsr_request_build(device->generation, device->channel, DSR_GET_STATUS, false, &request);
    if (result != DSR_OK) return result;
    cJSON *root = NULL;
    result = fetch_json(connection, &request, false, &root);
    if (result != DSR_OK) return result;
    cJSON *output = cJSON_GetObjectItemCaseSensitive(root, device->generation == DSR_GEN1 ? "ison" : "output");
    bool matches=true;
    bool parsed=device->endpoint_kind==DSR_ENDPOINT_COVER?cover_parse(root,device->channel,&connection->telemetry.cover):device->endpoint_kind==DSR_ENDPOINT_LIGHT?light_parse(root,device->channel,&connection->telemetry.light):device->endpoint_kind==DSR_ENDPOINT_RGB?rgb_parse(root,device->channel,&connection->telemetry.rgb,expected,&matches):cJSON_IsBool(output);
    if(!parsed||!matches){cJSON_Delete(root);return !parsed?DSR_UNSUPPORTED:DSR_BAD_RESPONSE;}
    cJSON *power = cJSON_GetObjectItemCaseSensitive(root, "apower");
    cJSON *voltage=cJSON_GetObjectItemCaseSensitive(root,"voltage"),*current=cJSON_GetObjectItemCaseSensitive(root,"current");
    if(device->generation==DSR_GEN2_PLUS){
        /* Dimmer without neutral may omit measurements. Explicit null is
         * absence for Light, never a fabricated zero W/V/A. */
        if(device->endpoint_kind==DSR_ENDPOINT_LIGHT||device->endpoint_kind==DSR_ENDPOINT_RGB){if(cJSON_IsNull(power))power=NULL;if(cJSON_IsNull(voltage))voltage=NULL;if(cJSON_IsNull(current))current=NULL;}
        const cJSON *optional[]={power,voltage,current};
        for(unsigned i=0;i<3;i++)if(optional[i]&&(!cJSON_IsNumber(optional[i])||!isfinite(optional[i]->valuedouble)||fabs(optional[i]->valuedouble)>1e9||(i&&optional[i]->valuedouble<0))){cJSON_Delete(root);return DSR_BAD_RESPONSE;}
        if(voltage){connection->telemetry.has_voltage=true;connection->telemetry.voltage_v=voltage->valuedouble;}
        if(current){connection->telemetry.has_current=true;connection->telemetry.current_a=current->valuedouble;}
        /* Switch.GetStatus aenergy.total is Wh, not by_minute's mWh. Gen1
         * /relay has no such field; its separate /meter uses W-min and is not
         * fabricated or fetched by this relay adapter. Optional null is absent. */
        const cJSON *energy=cJSON_GetObjectItemCaseSensitive(root,"aenergy");
        if(energy&&!cJSON_IsNull(energy)){
            if(!cJSON_IsObject(energy)){cJSON_Delete(root);return DSR_BAD_RESPONSE;}
            const cJSON *total=cJSON_GetObjectItemCaseSensitive(energy,"total");
            if(total&&!cJSON_IsNull(total)){
                if(!cJSON_IsNumber(total)||!isfinite(total->valuedouble)||total->valuedouble<0||total->valuedouble>1e12){cJSON_Delete(root);return DSR_BAD_RESPONSE;}
                connection->telemetry.has_energy=true;connection->telemetry.energy_wh=total->valuedouble;
            }
        }
    }
    device->output = cJSON_IsTrue(output);
    device->has_switch = device->endpoint_kind==DSR_ENDPOINT_SWITCH;device->online = true;
    device->power_w = 0;
    if (device->generation == DSR_GEN2_PLUS && cJSON_IsNumber(power) && isfinite(power->valuedouble)) {
        device->has_power = true; device->power_w = power->valuedouble;
    }
    cJSON_Delete(root);
    return DSR_OK;
}

static dsr_result_t status(dsr_connection_t *c){return status_expected(c,NULL);}

/* Read-only advisory name, after identity and status succeeded. A failed or
 * malformed optional response does not turn valid control telemetry into an
 * error, or erase the last name. Never read another device's protected config. */
static void read_name(dsr_connection_t *c,dsr_device_info_t *info)
{
 if(!info||c->device.generation!=DSR_GEN2_PLUS||c->device.endpoint_kind==DSR_ENDPOINT_BLU)return;
 if(urgent()||allowed()!=DSR_OK)return;
 dsr_request_t request={.path="/rpc/Sys.GetConfig"};cJSON *root=NULL;
 dsr_result_t result=fetch_json_timeout(c,&request,false,&root,650);
 const cJSON *device=cJSON_GetObjectItemCaseSensitive(root,"device"),*name=cJSON_GetObjectItemCaseSensitive(device,"name");
 char next[48];
 if(result==DSR_OK&&light_unique(root)&&cJSON_IsObject(device)&&(cJSON_IsNull(name)||cJSON_IsString(name))){
  if(cJSON_IsNull(name)){next[0]=0;info->name_read=true;}
  else info->name_read=dsr_text_copy(next,sizeof(next),name->valuestring);
  if(info->name_read){const char *fallback=dsr_device_model_name(c->device.device_id);dsr_text_copy(c->device.name,sizeof(c->device.name),next[0]?next:fallback?fallback:c->device.device_id);}
 }
 cJSON_Delete(root);
}

dsr_result_t dsr_http_probe_info(const dsr_connection_t *connection,dsr_device_t *out,dsr_telemetry_t *telemetry,dsr_device_info_t *info)
{
    if(info)memset(info,0,sizeof(*info));
    if (!out || !dsr_connection_valid(connection, false)) return DSR_INVALID;
    dsr_connection_t probed = *connection;
    probed.device.online=probed.device.has_power=false;
    uint8_t hint=trv_hint(connection);
    memset(&probed.telemetry,0,sizeof(probed.telemetry));probed.telemetry.blu.errors=hint;
    dsr_result_t result = identity(connection, &probed.device, info);
    if(result==DSR_OK&&connection->device.device_id[0]&&strcmp(connection->device.device_id,probed.device.device_id)){
        /* BLU bind from the LAN wizard knows only the gateway id; the probe
         * appends the sensor MAC. Any other difference is a mismatch. */
        size_t n=strlen(connection->device.device_id);
        bool gateway_only=connection->device.endpoint_kind==DSR_ENDPOINT_BLU&&!strchr(connection->device.device_id,'_')&&!strncmp(connection->device.device_id,probed.device.device_id,n)&&probed.device.device_id[n]=='_';
        if(!gateway_only)result=DSR_IDENTITY_MISMATCH;
    }
    if (result == DSR_OK){(void)urgent();result = status(&probed);}
    if(result==DSR_OK)read_name(&probed,info);
    else if(info)memset(info,0,sizeof(*info));
    *out = probed.device;
    if(telemetry)*telemetry=probed.telemetry;
    return result;
}
dsr_result_t dsr_http_probe_telemetry(const dsr_connection_t *connection,dsr_device_t *out,dsr_telemetry_t *telemetry)
{return dsr_http_probe_info(connection,out,telemetry,NULL);}
dsr_result_t dsr_http_probe(const dsr_connection_t *connection,dsr_device_t *out)
{return dsr_http_probe_telemetry(connection,out,NULL);}

static dsr_result_t verify_info(dsr_connection_t *connection, dsr_device_info_t *info)
{
    if (!dsr_connection_valid(connection, true)) return DSR_INVALID;
    dsr_result_t permission=allowed();if(permission!=DSR_OK)return permission;
    connection->device.online = false;
    connection->device.has_power=false;memset(&connection->telemetry,0,sizeof(connection->telemetry));
    dsr_device_t observed = {0};
    dsr_result_t result = identity(connection, &observed, info);
    if (result != DSR_OK) return result;
    if (strcmp(observed.device_id, connection->device.device_id) ||
        observed.generation != connection->device.generation) { if (info) memset(info, 0, sizeof(*info)); return DSR_IDENTITY_MISMATCH; }
    return DSR_OK;
}
static dsr_result_t verify(dsr_connection_t *connection){return verify_info(connection, NULL);}

dsr_result_t dsr_http_refresh_info(dsr_connection_t *connection, dsr_device_info_t *info)
{
    if (info) memset(info, 0, sizeof(*info));
    if(!dsr_connection_valid(connection,true))return DSR_INVALID;
    uint8_t hint=trv_hint(connection);
    dsr_result_t result = verify_info(connection, info);connection->telemetry.blu.errors=hint;
    (void)urgent();if(result==DSR_OK)result=allowed();if(result==DSR_OK)result=status(connection);
    if(result==DSR_OK)read_name(connection,info);else if(info)memset(info,0,sizeof(*info));
    return result;
}
dsr_result_t dsr_http_refresh(dsr_connection_t *connection){return dsr_http_refresh_info(connection, NULL);}

dsr_result_t dsr_http_set_output(dsr_connection_t *connection, bool on)
{
    if(connection&&DSR_ENDPOINT_IS_SENSOR(connection->device.endpoint_kind))return DSR_UNSUPPORTED;
    dsr_result_t result = verify(connection);
    if (result != DSR_OK) return result;
    if (connection->device.endpoint_kind!=DSR_ENDPOINT_SWITCH||!connection->device.has_switch) return DSR_UNSUPPORTED;
    if(urgent())return DSR_BUSY;
    result=allowed();if(result!=DSR_OK)return result;
    dsr_request_t request;
    result = dsr_request_build(connection->device.generation, connection->device.channel, DSR_SET_OUTPUT, on, &request);
    if (result != DSR_OK) return result;
    cJSON *root = NULL;
    result = fetch_command(connection, &request, &root);
    cJSON_Delete(root);
    if (result != DSR_OK) return result;
    if(urgent())return DSR_BAD_RESPONSE;
    result=allowed();if(result!=DSR_OK)return result;
    /* Never display an optimistic position; don't replay after a timeout. */
    result = status(connection);
    if (result == DSR_OK && connection->device.output != on) {
        connection->device.online = false;
        return DSR_BAD_RESPONSE;
    }
    return result;
}

dsr_result_t dsr_http_cover(dsr_connection_t *c,dsr_cover_intent_t intent)
{
 if(!c||c->device.endpoint_kind!=DSR_ENDPOINT_COVER||c->device.generation!=DSR_GEN2_PLUS||!dsr_cover_intent_valid(intent))return DSR_INVALID;
 dsr_result_t result=verify(c);if(result!=DSR_OK)return result;
 if(urgent())return DSR_BUSY;
 /* STOP verifies identity/auth at its own POST; it may stop calibration or
  * unknown position. Every other move requires a fresh typed status. */
 if(intent.command!=DSR_COVER_STOP){result=allowed();if(result!=DSR_OK)return result;result=status(c);if(result!=DSR_OK)return result;if(urgent())return DSR_BUSY;if(!dsr_cover_can_move(&c->telemetry.cover,intent))return DSR_UNSUPPORTED;}
 result=allowed();if(result!=DSR_OK)return result;
 dsr_request_t request;result=dsr_cover_request_build(c->device.channel,intent,&request);if(result!=DSR_OK)return result;
 cJSON *ack=NULL;result=fetch_command(c,&request,&ack);
 /* Only the documented null ACK confirms acceptance, never position. */
 if(result==DSR_OK&&!cJSON_IsNull(ack))result=DSR_BAD_RESPONSE;
 cJSON_Delete(ack);c->device.online=false;
 if(urgent())return DSR_BAD_RESPONSE;
 if(result!=DSR_OK)return result;
 result=allowed();if(result!=DSR_OK)return result;
 return status(c);
}

dsr_result_t dsr_http_trv(dsr_connection_t *c,dsr_trv_intent_t intent)
{
 if(!c||c->device.endpoint_kind!=DSR_ENDPOINT_BLU||c->device.generation!=DSR_GEN2_PLUS||!dsr_trv_intent_valid(intent))return DSR_INVALID;
 uint8_t hint=trv_hint(c);
 dsr_result_t result=verify(c);if(result!=DSR_OK)return result; /* gateway id + sensor MAC */
 c->telemetry.blu.errors=hint;
 if(urgent())return DSR_BUSY;
 result=allowed();if(result!=DSR_OK)return result;
 result=status(c);if(result!=DSR_OK)return result;
 if(!(c->telemetry.blu.errors&DSR_BLU_TRV))return DSR_UNSUPPORTED;
 if(urgent())return DSR_BUSY;
 result=allowed();if(result!=DSR_OK)return result;
 unsigned cid=DSR_BLU_FIRST_CID+c->device.channel;char payload[112];
 snprintf(payload,sizeof(payload),"{\"id\":%u,\"method\":\"TRV.SetTarget\",\"params\":{\"id\":0,\"target_C\":%u.%u}}",cid,(unsigned)intent.half_c/2u,(unsigned)(intent.half_c%2u)*5u);
 dsr_request_t request={.post=true};strcpy(request.path,"/rpc/BluTrv.Call");cJSON *ack=NULL;
 /* The gateway relays over BLE: "normally within 10 seconds". */
 result=fetch_json_payload(c,&request,false,&ack,15000,payload,"application/json",true);
 if(result==DSR_OK&&!cJSON_IsNull(ack))result=DSR_BAD_RESPONSE;
 cJSON_Delete(ack);c->device.online=false;
 if(urgent())return DSR_BAD_RESPONSE;
 if(result!=DSR_OK)return result; /* No replay after an uncertain write. */
 result=allowed();if(result!=DSR_OK)return result;
 result=status(c);if(result!=DSR_OK)return result;
 /* The gateway acknowledged the TRV call. Its mirrored target_C can lag the
  * BLE write by a poll; the LCD shows what the gateway reports, never the request. */
 return DSR_OK;
}
dsr_result_t dsr_http_light(dsr_connection_t *c,dsr_light_intent_t intent)
{
 if(!c||c->device.endpoint_kind!=DSR_ENDPOINT_LIGHT||c->device.generation!=DSR_GEN2_PLUS||!dsr_light_intent_valid(intent))return DSR_INVALID;
 dsr_result_t result=verify(c);if(result!=DSR_OK)return result;
 if(urgent())return DSR_BUSY;
 result=allowed();if(result!=DSR_OK)return result;
 result=status(c);if(result!=DSR_OK)return result;
 if(urgent())return DSR_BUSY;
 if(!dsr_light_can_set(&c->telemetry.light,intent))return DSR_UNSUPPORTED;
 result=allowed();if(result!=DSR_OK)return result;
 dsr_request_t request;result=dsr_light_request_build(c->device.channel,intent,&request);if(result!=DSR_OK)return result;
 cJSON *ack=NULL;result=fetch_command(c,&request,&ack);
 if(result==DSR_OK&&!cJSON_IsNull(ack))result=DSR_BAD_RESPONSE;
 cJSON_Delete(ack);c->device.online=false;
 if(urgent())return DSR_BAD_RESPONSE;
 if(result!=DSR_OK)return result; /* No replay after an uncertain write. */
 result=allowed();if(result!=DSR_OK)return result;
 result=status(c);if(result!=DSR_OK)return result;
 const dsr_light_status_t *s=&c->telemetry.light;
 /* During a transition the current output may still match the previous
  * state. The target decides whether this particular intent was confirmed. */
 bool output_matches=(s->flags&DSR_LIGHT_TRANSITION)?
  (s->flags&DSR_LIGHT_TARGET_OUTPUT)&&((s->flags&DSR_LIGHT_TARGET_ON)!=0)==(intent.command==DSR_LIGHT_ON):c->device.output==(intent.command==DSR_LIGHT_ON);
 bool matches=intent.command==DSR_LIGHT_SET_BRIGHTNESS?(s->flags&DSR_LIGHT_TRANSITION?s->target:s->brightness)==intent.brightness:
  output_matches;
 if(!matches||s->errors||s->conditions){c->device.online=false;return DSR_BAD_RESPONSE;}
 return DSR_OK;
}

dsr_result_t dsr_http_rgb(dsr_connection_t *c,dsr_rgb_intent_t intent)
{
 if(!c||c->device.endpoint_kind!=DSR_ENDPOINT_RGB||c->device.generation!=DSR_GEN2_PLUS||c->device.channel!=0||!dsr_rgb_intent_valid(intent))return DSR_INVALID;
 dsr_result_t result=verify(c);if(result!=DSR_OK)return result;
 if(urgent())return DSR_BUSY;
 result=allowed();if(result!=DSR_OK)return result;
 result=status(c);if(result!=DSR_OK)return result;
 if(urgent())return DSR_BUSY;
 if(!dsr_rgb_can_set(&c->telemetry.rgb,intent))return DSR_UNSUPPORTED;
 result=allowed();if(result!=DSR_OK)return result;
 dsr_request_t request;result=dsr_rgb_request_build(c->device.channel,intent,&request);if(result!=DSR_OK)return result;
 cJSON *ack=NULL;result=fetch_command(c,&request,&ack);
 if(result==DSR_OK&&!cJSON_IsNull(ack))result=DSR_BAD_RESPONSE;
 cJSON_Delete(ack);c->device.online=false;
 if(urgent())return DSR_BAD_RESPONSE;
 if(result!=DSR_OK)return result; /* Never replay an uncertain SET. */
 result=allowed();if(result!=DSR_OK)return result;
 return status_expected(c,&intent);
}

static void ap_identity(const dsr_lan_device_t *source,const char *ssid,dsr_ap_identity_t *out)
{
 memset(out,0,sizeof(*out));snprintf(out->id,sizeof(out->id),"%s",source->id);snprintf(out->mac,sizeof(out->mac),"%s",source->mac);snprintf(out->model,sizeof(out->model),"%s",source->model);snprintf(out->ssid,sizeof(out->ssid),"%s",ssid);out->generation=source->generation;out->auth_required=source->auth_required;
}
static bool ap_same(const dsr_ap_identity_t *a,const dsr_ap_identity_t *b)
{return a->generation==b->generation&&!strcmp(a->id,b->id)&&!strcmp(a->mac,b->mac);}
dsr_result_t dsr_http_ap_inspect(const char *ssid,dsr_ap_identity_t *out)
{
 if(!out||!dsr_ap_candidate(ssid))return DSR_INVALID;
 dsr_lan_device_t observed;dsr_result_t result=dsr_http_lan_identity("192.168.33.1",&observed);
 if(result!=DSR_OK)return result;
 dsr_ap_identity_t candidate;ap_identity(&observed,ssid,&candidate);
 if(!dsr_ap_identity_matches(ssid,&candidate))return DSR_IDENTITY_MISMATCH;
 if(candidate.auth_required)return DSR_AUTH_REQUIRED;
 *out=candidate;return DSR_OK;
}
static bool form_append(char *out,size_t capacity,size_t *used,const char *value)
{
 static const char hex[]="0123456789ABCDEF";
 for(const unsigned char *p=(const unsigned char *)value;*p;p++){
  bool plain=(*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='-'||*p=='_'||*p=='.'||*p=='~';
  if(*used+(plain?1:3)>=capacity)return false;
  if(plain)out[(*used)++]=(char)*p;else{out[(*used)++]='%';out[(*used)++]=hex[*p>>4];out[(*used)++]=hex[*p&15];}
 }
 out[*used]=0;return true;
}
dsr_result_t dsr_http_ap_provision(const dsr_ap_identity_t *expected,const dsr_settings_t *home)
{
 if(!expected||!home||!dsr_ap_identity_matches(expected->ssid,expected)||!dsr_wifi_settings_valid(home)||!strcmp(home->ssid,expected->ssid))return DSR_INVALID;
 if(expected->generation==1&&home->wifi_security==DSR_WIFI_WPA3)return DSR_UNSUPPORTED;
 dsr_ap_identity_t observed;dsr_result_t result=dsr_http_ap_inspect(expected->ssid,&observed);
 if(result!=DSR_OK)return result;
 if(!ap_same(expected,&observed))return DSR_IDENTITY_MISMATCH;
 if(dsr_ap_cancelled())return DSR_BUSY;
 char *payload=NULL;cJSON *document=NULL;
 if(expected->generation==1){
  payload=calloc(512,1);if(!payload)return DSR_IO;strcpy(payload,"enabled=1&ipv4_method=dhcp&ssid=");size_t used=strlen(payload);
  if(!form_append(payload,512,&used,home->ssid)){free(payload);return DSR_INVALID;}
  memcpy(payload+used,"&key=",5);used+=5;
  if(!form_append(payload,512,&used,home->password)){wipe(payload,512);free(payload);return DSR_INVALID;}
 }else {
  document=cJSON_CreateObject();cJSON *config=cJSON_AddObjectToObject(document,"config"),*sta=config?cJSON_AddObjectToObject(config,"sta"):NULL;
  bool ok=sta&&cJSON_AddStringToObject(sta,"ssid",home->ssid)&&cJSON_AddStringToObject(sta,"pass",home->password)&&cJSON_AddBoolToObject(sta,"enable",true)&&cJSON_AddStringToObject(sta,"ipv4mode","dhcp");
  payload=ok?cJSON_PrintUnformatted(document):NULL;
  cJSON *password=sta?cJSON_GetObjectItemCaseSensitive(sta,"pass"):NULL;if(cJSON_IsString(password))wipe(password->valuestring,strlen(password->valuestring));cJSON_Delete(document);
  if(!payload)return DSR_IO;
 }
 dsr_connection_t c={0};strcpy(c.device.host,"192.168.33.1");dsr_request_t request={.post=true};snprintf(request.path,sizeof(request.path),"%s",expected->generation==1?"/settings/sta":"/rpc/WiFi.SetConfig");cJSON *response=NULL;
 if(dsr_ap_cancelled())result=DSR_BUSY;
 else result=fetch_json_payload(&c,&request,true,&response,3000,payload,expected->generation==1?"application/x-www-form-urlencoded":"application/json",false);
 if(result==DSR_OK){
  if(expected->generation==1){const cJSON *enabled=cJSON_GetObjectItemCaseSensitive(response,"enabled"),*ssid=cJSON_GetObjectItemCaseSensitive(response,"ssid");if(!cJSON_IsTrue(enabled)||!cJSON_IsString(ssid)||strcmp(ssid->valuestring,home->ssid))result=DSR_BAD_RESPONSE;}
  else {const cJSON *restart=cJSON_GetObjectItemCaseSensitive(response,"restart_required");if(!cJSON_IsBool(restart)||cJSON_IsTrue(restart))result=DSR_UNSUPPORTED;}
 }
 cJSON_Delete(response);wipe(payload,strlen(payload));free(payload);return result;
}
dsr_result_t dsr_http_lan_verify_station(const dsr_ap_identity_t *expected,const char *host,const dsr_settings_t *home)
{
 if(!home||!dsr_station_identity_valid(expected)||!dsr_ipv4_valid(host)||!dsr_wifi_settings_valid(home))return DSR_INVALID;
 dsr_lan_device_t identity;dsr_result_t result=dsr_http_lan_identity(host,&identity);if(result!=DSR_OK)return result;
 dsr_ap_identity_t observed;ap_identity(&identity,expected->ssid,&observed);if(!ap_same(expected,&observed)||observed.auth_required)return DSR_IDENTITY_MISMATCH;
 dsr_connection_t c={0};snprintf(c.device.host,sizeof(c.device.host),"%s",host);dsr_request_t request={0};snprintf(request.path,sizeof(request.path),"%s",expected->generation==1?"/status":"/rpc/WiFi.GetStatus");cJSON *response=NULL;
 result=fetch_json(&c,&request,true,&response);
 if(result==DSR_OK){const cJSON *wifi=expected->generation==1?cJSON_GetObjectItemCaseSensitive(response,"wifi_sta"):response,*ssid=cJSON_GetObjectItemCaseSensitive(wifi,"ssid"),*ip=cJSON_GetObjectItemCaseSensitive(wifi,expected->generation==1?"ip":"sta_ip"),*connected=cJSON_GetObjectItemCaseSensitive(wifi,expected->generation==1?"connected":"status");
  bool ready=expected->generation==1?cJSON_IsTrue(connected):cJSON_IsString(connected)&&!strcmp(connected->valuestring,"got ip");
  if(!ready||!cJSON_IsString(ssid)||strcmp(ssid->valuestring,home->ssid)||!cJSON_IsString(ip)||strcmp(ip->valuestring,host))result=DSR_BAD_RESPONSE;
 }
 cJSON_Delete(response);return result;
}

dsr_result_t dsr_http_ap_verify_lan(const dsr_ap_identity_t *expected,const char *host,const dsr_settings_t *home)
{
 if(!expected||!dsr_ap_identity_matches(expected->ssid,expected))return DSR_INVALID;
 return dsr_http_lan_verify_station(expected,host,home);
}

dsr_result_t dsr_http_settings_identity(const dsr_connection_t *c)
{
 if(!c||c->device.generation!=DSR_GEN2_PLUS||!dsr_ipv4_valid(c->device.host)||!c->device.device_id[0])return DSR_INVALID;
 dsr_lan_device_t found;dsr_result_t r=dsr_http_lan_identity(c->device.host,&found);if(r!=DSR_OK)return r;
 if(found.generation<2||found.generation>4)return DSR_UNSUPPORTED;
 const char *suffix=c->device.endpoint_kind==DSR_ENDPOINT_BLU?strrchr(c->device.device_id,'_'):NULL;
 size_t n=suffix?(size_t)(suffix-c->device.device_id):strlen(c->device.device_id);
 return strlen(found.id)==n&&!memcmp(found.id,c->device.device_id,n)?DSR_OK:DSR_IDENTITY_MISMATCH;
}
dsr_result_t dsr_http_settings_rpc(const dsr_connection_t *c,const char *method,const cJSON *params,cJSON **out,bool mutating,bool *attempted)
{
 if(out)*out=NULL;
 if(attempted)*attempted=false;
 if(!out||!c||!method||strlen(method)>63||!strchr(method,'.'))return DSR_INVALID;
 for(const char *p=method;*p;p++)if(!isalnum((unsigned char)*p)&&*p!='.')return DSR_INVALID;
 dsr_result_t r=dsr_http_settings_identity(c);if(r!=DSR_OK)return r;
 cJSON *frame=cJSON_CreateObject();if(!frame)return DSR_IO;
 bool ok=cJSON_AddNumberToObject(frame,"id",1)&&cJSON_AddStringToObject(frame,"method",method);
 if(params){cJSON *copy=cJSON_Duplicate(params,true);if(!copy||!cJSON_AddItemToObject(frame,"params",copy)){cJSON_Delete(copy);ok=false;}}
 char *payload=ok?cJSON_PrintUnformatted(frame):NULL;dsr_setting_json_delete(frame);if(!payload)return DSR_IO;
 if(strlen(payload)>2048){wipe(payload,strlen(payload));free(payload);return DSR_INVALID;}
 dsr_request_t request={.post=true,.path="/rpc"};cJSON *response=NULL;
 if(mutating&&attempted)*attempted=true; /* Conservative: never replay after entering transport. */
 r=fetch_json_payload(c,&request,false,&response,4000,payload,"application/json",mutating);
 wipe(payload,strlen(payload));free(payload);
 if(r==DSR_OK){
  const cJSON *id=cJSON_GetObjectItemCaseSensitive(response,"id"),*result=cJSON_GetObjectItemCaseSensitive(response,"result");
  if(!light_unique(response)||!cJSON_IsNumber(id)||id->valuedouble!=1||!result||(!cJSON_IsObject(result)&&!cJSON_IsNull(result)&&!cJSON_IsArray(result)))r=DSR_BAD_RESPONSE;
  else {*out=cJSON_Duplicate(result,true);if(!*out)r=DSR_IO;}
 }
 cJSON_Delete(response);return r;
}
