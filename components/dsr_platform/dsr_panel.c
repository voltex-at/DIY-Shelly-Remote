#include "dsr_workspace.h"
#include "dsr_runtime_api.h"
#include "dsr_panel_permission.h"
#include "dsr_ota_upload.h"
#include "dsr_ota_image.h"
#include "dsr_discovery.h"
#include "dsr_ble.h"
#include "dsr_platform.h"
#include "dsr_activity.h"
#include "dsr_restart.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_app_desc.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "esp_ota_ops.h"
#include "mdns.h"
#include "mbedtls/sha256.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern const unsigned char panel_html_start[] asm("_binary_panel_html_start");
extern const unsigned char panel_html_end[] asm("_binary_panel_html_end");
extern const unsigned char panel_js_start[] asm("_binary_panel_js_start");
extern const unsigned char panel_js_end[] asm("_binary_panel_js_end");
extern const unsigned char runtime_js_start[] asm("_binary_runtime_js_start");
extern const unsigned char runtime_js_end[] asm("_binary_runtime_js_end");
extern const unsigned char panel_css_start[] asm("_binary_panel_css_start");
extern const unsigned char panel_css_end[] asm("_binary_panel_css_end");
/* The marker is validated inside uploaded binaries, not from a client claim. */
__attribute__((used)) const char dsr_ota_board_marker[]=DSR_OTA_IMAGE_BOARD_MARKER;
static SemaphoreHandle_t lock;
static char pairing[17];static int64_t pair_until,ota_until;static unsigned attempts;
static unsigned char token_hash[32];static bool has_token,need_preview;
static unsigned char *preview;static bool preview_ready;
static httpd_handle_t server;
static httpd_handle_t redirect_server;
static bool panel_ready;
static uint32_t boot_id;
bool dsr_panel_ready(void){return panel_ready;}

static void random_hex(char *out,size_t bytes)
{uint8_t raw[32];esp_fill_random(raw,bytes);for(size_t i=0;i<bytes;i++)sprintf(out+i*2,"%02x",raw[i]);out[bytes*2]=0;memset(raw,0,sizeof(raw));}
static bool equal(const void *a,const void *b,size_t n)
{const uint8_t *x=a,*y=b;unsigned d=0;while(n--)d|=*x++^*y++;return d==0;}
void dsr_panel_pair_begin(char code[17])
{if(!dsr_boot_ready()||!panel_ready||!lock||xSemaphoreTake(lock,0)!=pdTRUE){code[0]=0;return;}random_hex(pairing,8);memcpy(code,pairing,17);pair_until=esp_timer_get_time()+120000000;attempts=0;xSemaphoreGive(lock);}
bool dsr_panel_allow_ota(void)
{return dsr_panel_ota_enable(lock,panel_ready,&ota_until);}
void dsr_panel_pair_cancel(void)
{if(lock){xSemaphoreTake(lock,portMAX_DELAY);memset(pairing,0,sizeof(pairing));pair_until=0;xSemaphoreGive(lock);}}
bool dsr_panel_preview_requested(void)
{if(!lock)return false;if(xSemaphoreTake(lock,0)!=pdTRUE)return false;bool wanted=need_preview;need_preview=false;xSemaphoreGive(lock);return wanted;}
void dsr_panel_preview_publish(const void *data,size_t size)
{if(!lock||size!=320*480*2||!preview||xSemaphoreTake(lock,0)!=pdTRUE)return;memcpy(preview,data,size);preview_ready=true;xSemaphoreGive(lock);}
static esp_err_t json(httpd_req_t *r,const char *status,const char *body)
{httpd_resp_set_status(r,status);httpd_resp_set_type(r,"application/json");httpd_resp_set_hdr(r,"Cache-Control","no-store");return httpd_resp_sendstr(r,body);}
static bool local_request(httpd_req_t *r)
{
 struct sockaddr_storage peer;socklen_t size=sizeof(peer);bool local=false;
 if(getpeername(httpd_req_to_sockfd(r),(struct sockaddr *)&peer,&size))return false;
 uint32_t ip=0;
 if(peer.ss_family==AF_INET)ip=ntohl(((struct sockaddr_in *)&peer)->sin_addr.s_addr);
#ifdef CONFIG_LWIP_IPV6
 else if(peer.ss_family==AF_INET6){
  const uint8_t *address=(const uint8_t *)&((struct sockaddr_in6 *)&peer)->sin6_addr;
  static const uint8_t mapped[12]={0,0,0,0,0,0,0,0,0,0,255,255};
  if(!memcmp(address,mapped,12)){uint32_t raw;memcpy(&raw,address+12,4);ip=ntohl(raw);}
  else{
   local=(address[0]&0xfe)==0xfc||(address[0]==0xfe&&(address[1]&0xc0)==0x80);
   struct sockaddr_storage own;socklen_t own_size=sizeof(own);
   if(!local&&getsockname(httpd_req_to_sockfd(r),(struct sockaddr *)&own,&own_size)==0&&own.ss_family==AF_INET6)
    local=!memcmp(address,&((struct sockaddr_in6 *)&own)->sin6_addr,8);
  }
 }
#endif
 local=local||(ip>>24)==10||(ip>>16)==0xc0a8||(ip>>20)==0xac1||(ip>>16)==0xa9fe;
 if(!local)return false;
 char host[80]={0},origin[100]={0},site[32]={0};
 if(httpd_req_get_hdr_value_str(r,"Host",host,sizeof(host))!=ESP_OK)return false;
 char *port=strchr(host,':');if(port)*port=0;
 dsr_wifi_info_t wifi;dsr_wifi_get_info(&wifi);
 if(strcmp(host,"shelly-remote.local")&&strcmp(host,wifi.ip))return false;
 if(httpd_req_get_hdr_value_str(r,"Sec-Fetch-Site",site,sizeof(site))==ESP_OK&&!strcmp(site,"cross-site"))return false;
 if(r->method!=HTTP_GET){
  if(httpd_req_get_hdr_value_str(r,"Origin",origin,sizeof(origin))==ESP_OK){
   char expected[100];snprintf(expected,sizeof(expected),"https://%s",host);if(strcmp(origin,expected))return false;
  }
  char request[4]={0};if(httpd_req_get_hdr_value_str(r,"X-DSR-Request",request,sizeof(request))!=ESP_OK||strcmp(request,"1"))return false;
 }
 return true;
}
static bool authorized(httpd_req_t *r)
{
 char cookie[160]={0};if(httpd_req_get_hdr_value_str(r,"Cookie",cookie,sizeof(cookie))!=ESP_OK)return false;
 const char *start=strstr(cookie,"dsr_session=");if(!start)return false;start+=12;
 if(strlen(start)<64)return false;
 char token[65];for(unsigned i=0;i<64;i++){if(!isxdigit((unsigned char)start[i]))return false;token[i]=start[i];}token[64]=0;
 if(start[64]&&start[64]!=';')return false;
 uint8_t digest[32];mbedtls_sha256((const unsigned char *)token,64,digest,0);memset(token,0,sizeof(token));
 xSemaphoreTake(lock,portMAX_DELAY);bool ok=has_token&&equal(digest,token_hash,32);xSemaphoreGive(lock);return ok;
}
static char *body(httpd_req_t *r,size_t maximum)
{
 if(!r->content_len||r->content_len>maximum)return NULL;
 char *data=malloc(r->content_len+1);if(!data)return NULL;size_t at=0;unsigned timeouts=0;
 while(at<r->content_len){int n=httpd_req_recv(r,data+at,r->content_len-at);if(n==HTTPD_SOCK_ERR_TIMEOUT&&++timeouts<=2)continue;if(n<=0){free(data);return NULL;}at+=(unsigned)n;}
 data[at]=0;if(memchr(data,0,at)){free(data);return NULL;}return data;
}
static esp_err_t pair(httpd_req_t *r)
{
 char *data=body(r,16);if(!data)return json(r,"400 Bad Request","{\"error\":\"Enter the 16-character code shown on the remote\"}");
 char token[65];uint8_t digest[32];esp_err_t result=ESP_FAIL;
 if(!dsr_activity_begin()){memset(data,0,strlen(data));free(data);return json(r,"409 Conflict","{\"error\":\"Restart pending; pairing not saved\"}");}
 xSemaphoreTake(lock,portMAX_DELAY);
 bool valid=esp_timer_get_time()<pair_until&&attempts++<5&&strlen(data)==16&&equal(pairing,data,16);
 if(valid){
  random_hex(token,32);mbedtls_sha256((unsigned char *)token,64,digest,0);
  nvs_handle_t n;if(nvs_open("dsr_panel",NVS_READWRITE,&n)==ESP_OK){result=nvs_set_blob(n,"token_hash",digest,32);if(result==ESP_OK)result=nvs_commit(n);nvs_close(n);}
  if(result==ESP_OK){memcpy(token_hash,digest,32);has_token=true;pair_until=0;memset(pairing,0,sizeof(pairing));}
 }
 xSemaphoreGive(lock);memset(data,0,strlen(data));free(data);
 dsr_activity_end();
 if(!valid)return json(r,"401 Unauthorized","{\"error\":\"Pairing expired, locked or invalid; reopen pairing on the remote\"}");
 if(result!=ESP_OK)return json(r,"503 Service Unavailable","{\"error\":\"Pairing could not be saved\"}");
 char cookie[160];snprintf(cookie,sizeof(cookie),"dsr_session=%s; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=2592000",token);memset(token,0,sizeof(token));
 httpd_resp_set_hdr(r,"Set-Cookie",cookie);return json(r,"200 OK","{\"paired\":true}");
}
static esp_err_t ota(httpd_req_t *r)
{
 xSemaphoreTake(lock,portMAX_DELAY);bool allowed=esp_timer_get_time()<ota_until;ota_until=0;xSemaphoreGive(lock);
 if(!allowed)return json(r,"403 Forbidden","{\"error\":\"Allow OTA on the remote first; approval expires after 2 minutes\"}");
 const esp_partition_t *slot=esp_ota_get_next_update_partition(NULL);
 if(!slot||r->content_len<512||r->content_len>slot->size)return json(r,"400 Bad Request","{\"error\":\"Upload the application BIN, not a merged USB image\"}");
 char expected[65]={0};if(httpd_req_get_hdr_value_str(r,"X-SHA256",expected,sizeof(expected))!=ESP_OK||strlen(expected)!=64)return json(r,"400 Bad Request","{\"error\":\"SHA256 required\"}");
 uint8_t digest[32];if(!dsr_ota_upload_sha256(expected,64,digest))return json(r,"400 Bad Request","{}");
 if(!dsr_activity_begin())return json(r,"409 Conflict","{\"error\":\"Restart pending; upload not started\"}");
 esp_err_t error=dsr_ota_upload_receive(r,slot,digest);
 if(error!=ESP_OK){dsr_activity_end();return json(r,"400 Bad Request","{\"error\":\"Image, board marker or checksum rejected; boot slot unchanged\"}");}
 error=esp_ota_set_boot_partition(slot);
 dsr_activity_end();
 if(error!=ESP_OK)return json(r,"500 Internal Server Error","{\"error\":\"Boot slot not changed\"}");
 return json(r,"200 OK","{\"installed\":true,\"restart_required\":true}");
}
static esp_err_t restart(httpd_req_t *r)
{
 dsr_restart_result_t result=dsr_restart_request();
 if(result==DSR_RESTART_ACCEPTED)return json(r,"200 OK","{\"restarting\":true}");
 return json(r,result==DSR_RESTART_BUSY?"409 Conflict":"503 Service Unavailable",
  result==DSR_RESTART_BUSY?"{\"error\":\"Operation pending; retry restart after saving finishes\"}":"{\"error\":\"Restart service unavailable\"}");
}
static cJSON *lan_device_json(const dsr_lan_device_t *d)
{
 cJSON *o=cJSON_CreateObject();cJSON_AddStringToObject(o,"host",d->host);cJSON_AddStringToObject(o,"id",d->id);cJSON_AddStringToObject(o,"mac",d->mac);cJSON_AddStringToObject(o,"model",d->model);cJSON_AddStringToObject(o,"name",d->name);cJSON_AddNumberToObject(o,"generation",d->generation);cJSON_AddBoolToObject(o,"auth_required",d->auth_required);return o;
}
static esp_err_t lan_api(httpd_req_t *r)
{
 if(!strcmp(r->uri,"/api/v1/discovery")&&r->method==HTTP_GET){
  dsr_lan_snapshot_t scan;dsr_lan_snapshot(&scan);cJSON *o=cJSON_CreateObject(),*devices=cJSON_AddArrayToObject(o,"devices");
  cJSON_AddNumberToObject(o,"phase",scan.phase);cJSON_AddNumberToObject(o,"result",scan.result);cJSON_AddNumberToObject(o,"serial",scan.serial);cJSON_AddNumberToObject(o,"probed",scan.probed);cJSON_AddNumberToObject(o,"total",scan.total);cJSON_AddNumberToObject(o,"skipped",scan.skipped);
  for(unsigned i=0;i<scan.count;i++)cJSON_AddItemToArray(devices,lan_device_json(&scan.devices[i]));
  cJSON_AddItemToObject(o,"selected",lan_device_json(&scan.selected));cJSON *caps=cJSON_AddObjectToObject(o,"capabilities");cJSON_AddNumberToObject(caps,"switches",scan.capabilities.switches);cJSON_AddNumberToObject(caps,"covers",scan.capabilities.covers);cJSON_AddNumberToObject(caps,"lights",scan.capabilities.lights);cJSON_AddBoolToObject(caps,"sensors",scan.capabilities.sensors);
  char *text=cJSON_PrintUnformatted(o);cJSON_Delete(o);esp_err_t result=json(r,"200 OK",text?text:"{}");free(text);return result;
 }
 if(r->method==HTTP_POST&&!strcmp(r->uri,"/api/v1/discovery/scan")){bool ok=dsr_lan_scan();return json(r,ok?"202 Accepted":"409 Conflict",ok?"{\"scanning\":true}":"{\"error\":\"Brak Wi-Fi lub wyszukiwanie w toku\"}");}
 if(r->method==HTTP_POST&&!strcmp(r->uri,"/api/v1/discovery/cancel")){dsr_lan_cancel();return json(r,"202 Accepted","{\"cancelling\":true}");}
 if(r->method==HTTP_POST&&!strcmp(r->uri,"/api/v1/discovery/inspect")){
  char *data=body(r,512);cJSON *input=data&&dsr_config_json_depth(data)?cJSON_Parse(data):NULL;dsr_credentials_t credentials={0};
  const cJSON *host=cJSON_GetObjectItemCaseSensitive(input,"host"),*id=cJSON_GetObjectItemCaseSensitive(input,"id"),*username=cJSON_GetObjectItemCaseSensitive(input,"username"),*password=cJSON_GetObjectItemCaseSensitive(input,"password");
  bool valid=cJSON_IsString(host)&&strlen(host->valuestring)<16&&cJSON_IsString(id)&&strlen(id->valuestring)<64&&cJSON_IsString(username)&&strlen(username->valuestring)<sizeof(credentials.username)&&cJSON_IsString(password)&&strlen(password->valuestring)<sizeof(credentials.password);
  if(valid){strcpy(credentials.username,username->valuestring);strcpy(credentials.password,password->valuestring);valid=dsr_lan_inspect(host->valuestring,id->valuestring,&credentials);}
  memset(&credentials,0,sizeof(credentials));if(data){memset(data,0,strlen(data));free(data);}cJSON_Delete(input);return json(r,valid?"202 Accepted":"409 Conflict",valid?"{\"checking\":true}":"{\"error\":\"Urządzenie niedostępne lub operacja w toku\"}");
 }
 return json(r,"404 Not Found","{}");
}
static esp_err_t ble_api(httpd_req_t *r)
{
 if(r->method==HTTP_POST&&!strcmp(r->uri,"/api/v1/ble/scan")){bool ok=dsr_ble_scan();return json(r,ok?"202 Accepted":"409 Conflict",ok?"{\"scanning\":true}":"{\"error\":\"BLE niedostępne lub skan trwa\"}");}
 if(r->method==HTTP_POST&&!strcmp(r->uri,"/api/v1/ble/cancel")){dsr_ble_cancel();return json(r,"202 Accepted","{\"cancelling\":true}");}
 if(r->method==HTTP_GET&&!strcmp(r->uri,"/api/v1/ble")){
  dsr_ble_snapshot_t s;dsr_ble_snapshot(&s);cJSON *o=cJSON_CreateObject(),*devices=cJSON_AddArrayToObject(o,"devices");
  cJSON_AddNumberToObject(o,"phase",s.phase);cJSON_AddBoolToObject(o,"busy",s.busy);cJSON_AddNumberToObject(o,"elapsed_ms",s.elapsed_ms);cJSON_AddNumberToObject(o,"packets",s.packets);cJSON_AddNumberToObject(o,"dropped",s.dropped);cJSON_AddNumberToObject(o,"malformed",s.malformed);
  for(unsigned i=0;i<s.count;i++){const dsr_ble_device_t *d=&s.devices[i];cJSON *row=cJSON_CreateObject();cJSON_AddStringToObject(row,"name",d->name);cJSON_AddStringToObject(row,"model",dsr_ble_model_name(d));cJSON_AddNumberToObject(row,"kind",d->kind);cJSON_AddNumberToObject(row,"rssi",d->rssi);cJSON_AddNumberToObject(row,"age_ms",s.snapshot_ms-d->seen_ms);cJSON_AddBoolToObject(row,"bthome",(d->evidence&DSR_BLE_BTHOME)!=0);cJSON_AddBoolToObject(row,"encrypted",d->encrypted);cJSON_AddBoolToObject(row,"paired",false);cJSON_AddItemToArray(devices,row);}
  char *text=cJSON_PrintUnformatted(o);cJSON_Delete(o);esp_err_t result=json(r,"200 OK",text?text:"{}");free(text);return result;
 }
 return json(r,"404 Not Found","{}");
}
static esp_err_t runtime_api(httpd_req_t *r)
{
 if(!strcmp(r->uri,"/api/v1/devices/runtime")&&r->method==HTTP_GET){
  char *snapshot=dsr_runtime_json(boot_id);if(!snapshot)return json(r,"503 Service Unavailable","{\"error\":\"Runtime snapshot unavailable\"}");
  esp_err_t result=json(r,"200 OK",snapshot);free(snapshot);return result;
 }
 if(!strcmp(r->uri,"/api/v1/devices/output")&&r->method==HTTP_POST){
  char *data=body(r,256);dsr_device_t device;bool on=false;bool valid=dsr_output_parse(data,&device,&on);free(data);
  if(!valid)return json(r,"400 Bad Request","{\"error\":\"Expected exact device identity, generation, channel and boolean on\"}");
  uint32_t operation_id=0;dsr_result_t result=dsr_workspace_output_request(&device,on,&operation_id);
  char response[160];if(result==DSR_OK)snprintf(response,sizeof(response),"{\"accepted\":true,\"operation_id\":%lu,\"boot_id\":%lu}",(unsigned long)operation_id,(unsigned long)boot_id);
  else snprintf(response,sizeof(response),"{\"accepted\":false,\"result\":%u,\"error\":\"Command rejected; refresh this endpoint\"}",(unsigned)result);
  return json(r,result==DSR_OK?"202 Accepted":result==DSR_INVALID?"404 Not Found":result==DSR_OFFLINE?"503 Service Unavailable":"409 Conflict",response);
 }
 return json(r,"404 Not Found","{\"error\":\"Unknown runtime endpoint\"}");
}
static esp_err_t api(httpd_req_t *r)
{
 if(!local_request(r))return json(r,"403 Forbidden","{\"error\":\"Local same-origin access only\"}");
 if(!dsr_boot_ready())return json(r,"503 Service Unavailable","{\"error\":\"Startup incomplete or diagnostic mode; collect USB boot log\"}");
 if(!strcmp(r->uri,"/api/v1/pair")&&r->method==HTTP_POST)return pair(r);
 if(!authorized(r))return json(r,"401 Unauthorized","{\"error\":\"Pair this browser on the remote\"}");
 if(!strcmp(r->uri,"/api/v1/devices/runtime")||!strcmp(r->uri,"/api/v1/devices/output"))return runtime_api(r);
 if(!strncmp(r->uri,"/api/v1/ble",11))return ble_api(r);
 if(!strncmp(r->uri,"/api/v1/discovery",17))return lan_api(r);
 if(!strcmp(r->uri,"/api/v1/config")){
  if(r->method==HTTP_GET){char *config=dsr_workspace_snapshot();if(!config)return json(r,"503 Service Unavailable","{}");esp_err_t e=json(r,"200 OK",config);free(config);return e;}
  if(r->method==HTTP_PUT){char *candidate=body(r,DSR_CONFIG_MAX_JSON),*current=dsr_workspace_snapshot();char error[96]={0};
   uint32_t revision=current?dsr_config_revision(current):0;char *next=candidate&&current?dsr_config_replace(current,candidate,revision,error):NULL;free(current);
   if(!next){free(candidate);return json(r,strstr(error,"conflict")?"409 Conflict":"400 Bad Request","{\"error\":\"Invalid configuration or revision conflict; reload\"}");}free(next);
   bool ok=dsr_workspace_submit(candidate,revision);free(candidate);return json(r,ok?"202 Accepted":"409 Conflict",ok?"{\"saving\":true}":"{\"error\":\"Busy or revision conflict\"}");
  }
 }
 if(!strcmp(r->uri,"/api/v1/status")&&r->method==HTTP_GET){
  bool busy;char message[96];dsr_workspace_status(&busy,message);cJSON *o=cJSON_CreateObject();cJSON_AddNumberToObject(o,"revision",dsr_workspace_revision());cJSON_AddBoolToObject(o,"busy",busy);cJSON_AddStringToObject(o,"message",message);
  bool device_busy;char device_message[64];dsr_workspace_bind_status(&device_busy,device_message);cJSON_AddBoolToObject(o,"device_busy",device_busy);cJSON_AddStringToObject(o,"device_message",device_message);
  cJSON_AddNumberToObject(o,"device_result",dsr_workspace_bind_result_code());cJSON_AddStringToObject(o,"firmware",esp_app_get_description()->version);char *s=cJSON_PrintUnformatted(o);cJSON_Delete(o);esp_err_t e=json(r,"200 OK",s?s:"{}");free(s);return e;
 }
 if(!strcmp(r->uri,"/api/v1/devices/bind")&&r->method==HTTP_POST){
  char *data=body(r,512);cJSON *input=data&&dsr_config_json_depth(data)?cJSON_Parse(data):NULL;dsr_connection_t connection={0};
  const cJSON *host=cJSON_GetObjectItemCaseSensitive(input,"host"),*name=cJSON_GetObjectItemCaseSensitive(input,"name"),*username=cJSON_GetObjectItemCaseSensitive(input,"username"),*password=cJSON_GetObjectItemCaseSensitive(input,"password"),*channel=cJSON_GetObjectItemCaseSensitive(input,"channel");
  bool valid=cJSON_IsString(host)&&strlen(host->valuestring)<sizeof(connection.device.host)&&cJSON_IsString(name)&&strlen(name->valuestring)<sizeof(connection.device.name)&&cJSON_IsString(username)&&strlen(username->valuestring)<sizeof(connection.credentials.username)&&cJSON_IsString(password)&&strlen(password->valuestring)<sizeof(connection.credentials.password)&&cJSON_IsNumber(channel)&&channel->valuedouble==channel->valueint&&channel->valueint>=0&&channel->valueint<=15;
  const cJSON *expected=cJSON_GetObjectItemCaseSensitive(input,"expected_id"),*room=cJSON_GetObjectItemCaseSensitive(input,"room");uint32_t room_id=UINT32_MAX;
  if(expected){if(!cJSON_IsString(expected)||!expected->valuestring[0]||strlen(expected->valuestring)>=sizeof(connection.device.device_id))valid=false;else strcpy(connection.device.device_id,expected->valuestring);}
  if(room){if(!cJSON_IsNumber(room)||room->valuedouble!=room->valueint||room->valueint<0||room->valueint>1000000)valid=false;else{room_id=(uint32_t)room->valueint;if(room_id){char *config=dsr_workspace_snapshot();cJSON *root=config?cJSON_Parse(config):NULL;free(config);bool found=false;const cJSON *v;cJSON_ArrayForEach(v,cJSON_GetObjectItemCaseSensitive(root,"rooms")){const cJSON *id=cJSON_GetObjectItemCaseSensitive(v,"id");if(cJSON_IsNumber(id)&&id->valueint==(int)room_id)found=true;}cJSON_Delete(root);valid=valid&&found;}}}
  if(valid){strcpy(connection.device.host,host->valuestring);strcpy(connection.device.name,name->valuestring);strcpy(connection.credentials.username,username->valuestring);strcpy(connection.credentials.password,password->valuestring);connection.device.channel=(uint8_t)channel->valueint;valid=dsr_workspace_bind_with_room(&connection,room_id);}
  memset(&connection,0,sizeof(connection));if(data){memset(data,0,strlen(data));free(data);}cJSON_Delete(input);return json(r,valid?"202 Accepted":"400 Bad Request",valid?"{\"checking\":true}":"{\"error\":\"Invalid details or another device check is running\"}");
 }
 if(!strcmp(r->uri,"/api/v1/preview.raw")&&r->method==HTTP_GET){
  unsigned char *copy=malloc(320*480*2);if(!copy)return json(r,"503 Service Unavailable","{}");
  xSemaphoreTake(lock,portMAX_DELAY);bool pairing_active=esp_timer_get_time()<pair_until;need_preview=!pairing_active;bool ready=preview_ready&&!pairing_active;if(ready)memcpy(copy,preview,320*480*2);xSemaphoreGive(lock);
  esp_err_t e;if(ready){httpd_resp_set_type(r,"application/octet-stream");httpd_resp_set_hdr(r,"Cache-Control","no-store");e=httpd_resp_send(r,(const char *)copy,320*480*2);}else e=json(r,"202 Accepted","{\"waiting\":true}");free(copy);return e;
 }
 if(!strncmp(r->uri,"/api/v1/photos/",15)&&r->method==HTTP_POST){
  const char *id=r->uri+15;if(strlen(id)!=32||r->content_len!=DSR_PHOTO_BYTES)return json(r,"400 Bad Request","{\"error\":\"Expected a 320x180 RGB565 room photo\"}");
  unsigned char *data=malloc(r->content_len);if(!data)return json(r,"503 Service Unavailable","{}");size_t at=0;while(at<r->content_len){int n=httpd_req_recv(r,(char *)data+at,r->content_len-at);if(n<=0)break;at+=(unsigned)n;}
  bool ok=at==r->content_len&&dsr_photo_save(id,data,at);free(data);return json(r,ok?"200 OK":"400 Bad Request",ok?"{\"saved\":true}":"{\"error\":\"Nie zapisano zdjęcia: suma kontrolna, pamięć lub limit 16 zdjęć. Zapisz usunięcie nieużywanych zdjęć i uruchom pilot ponownie, aby odzyskać miejsce.\"}");
 }
 if(!strcmp(r->uri,"/api/v1/ota")&&r->method==HTTP_POST)return ota(r);
 if(!strcmp(r->uri,"/api/v1/restart")&&r->method==HTTP_POST)return restart(r);
 return json(r,"404 Not Found","{\"error\":\"Unknown API route\"}");
}
static esp_err_t asset(httpd_req_t *r)
{
 if(!local_request(r))return json(r,"403 Forbidden","{}");
 const unsigned char *start,*end;
 if(!strcmp(r->uri,"/")){start=panel_html_start;end=panel_html_end;httpd_resp_set_type(r,"text/html; charset=utf-8");}
 else if(!strcmp(r->uri,"/panel.js")){start=panel_js_start;end=panel_js_end;httpd_resp_set_type(r,"text/javascript; charset=utf-8");}
 else if(!strcmp(r->uri,"/runtime.js")){start=runtime_js_start;end=runtime_js_end;httpd_resp_set_type(r,"text/javascript; charset=utf-8");}
 else if(!strcmp(r->uri,"/panel.css")){start=panel_css_start;end=panel_css_end;httpd_resp_set_type(r,"text/css; charset=utf-8");}
 else return json(r,"404 Not Found","{}");
 httpd_resp_set_hdr(r,"Content-Security-Policy","default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' blob: data:; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
 httpd_resp_set_hdr(r,"X-Content-Type-Options","nosniff");httpd_resp_set_hdr(r,"Referrer-Policy","no-referrer");httpd_resp_set_hdr(r,"Cache-Control","no-cache");return httpd_resp_send(r,(const char *)start,end-start-1);
}
static esp_err_t redirect_https(httpd_req_t *r)
{
 if(!local_request(r))return json(r,"403 Forbidden","{}");
 char host[80]={0},location[96];if(httpd_req_get_hdr_value_str(r,"Host",host,sizeof(host))!=ESP_OK)return ESP_FAIL;
 char *port=strchr(host,':');if(port)*port=0;snprintf(location,sizeof(location),"https://%s/",host);
 httpd_resp_set_status(r,"308 Permanent Redirect");httpd_resp_set_hdr(r,"Location",location);return httpd_resp_sendstr(r,"");
}
esp_err_t dsr_panel_start(void)
{
 esp_fill_random(&boot_id,sizeof(boot_id));if(!boot_id)boot_id=1;
 esp_err_t identity_error=dsr_tls_identity();if(identity_error!=ESP_OK)return identity_error;
 lock=xSemaphoreCreateMutex();if(!lock)return ESP_ERR_NO_MEM;preview=malloc(320*480*2);
 nvs_handle_t n;if(nvs_open("dsr_panel",NVS_READONLY,&n)==ESP_OK){size_t size=32;has_token=nvs_get_blob(n,"token_hash",token_hash,&size)==ESP_OK&&size==32;nvs_close(n);}
 httpd_ssl_config_t tls=HTTPD_SSL_CONFIG_DEFAULT();tls.httpd.stack_size=16384;tls.httpd.max_uri_handlers=7;tls.httpd.max_open_sockets=2;tls.httpd.lru_purge_enable=true;tls.httpd.uri_match_fn=httpd_uri_match_wildcard;tls.httpd.recv_wait_timeout=5;tls.httpd.send_wait_timeout=5;
 tls.servercert=dsr_tls_certificate();tls.servercert_len=strlen((const char *)tls.servercert)+1;tls.prvtkey_pem=dsr_tls_key();tls.prvtkey_len=strlen((const char *)tls.prvtkey_pem)+1;
 esp_err_t error=httpd_ssl_start(&server,&tls);if(error!=ESP_OK)return error;
 const httpd_method_t methods[]={HTTP_GET,HTTP_POST,HTTP_PUT,HTTP_DELETE};
 for(unsigned i=0;i<4;i++){httpd_uri_t route={.uri="/api/v1/*",.method=methods[i],.handler=api};error=httpd_register_uri_handler(server,&route);if(error!=ESP_OK)return error;}
 httpd_uri_t root={.uri="/*",.method=HTTP_GET,.handler=asset};error=httpd_register_uri_handler(server,&root);if(error!=ESP_OK)return error;
 panel_ready=true;
 httpd_config_t plain=HTTPD_DEFAULT_CONFIG();plain.max_open_sockets=1;plain.max_uri_handlers=1;plain.uri_match_fn=httpd_uri_match_wildcard;
 if(httpd_start(&redirect_server,&plain)==ESP_OK){httpd_uri_t redirect={.uri="/*",.method=HTTP_GET,.handler=redirect_https};httpd_register_uri_handler(redirect_server,&redirect);}
 if(mdns_init()==ESP_OK){mdns_hostname_set("shelly-remote");mdns_instance_name_set("DIY Shelly Remote");mdns_service_add(NULL,"_https","_tcp",443,NULL,0);mdns_service_add(NULL,"_http","_tcp",80,NULL,0);}
 return ESP_OK;
}
