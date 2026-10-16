#include "dsr_ble_rpc_protocol.h"
#include "dsr_config.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool bounded(const char *s,size_t maximum)
{if(!s)return false;size_t n=0;while(n<maximum&&s[n])n++;return n>0&&n<maximum;}
static bool client_valid(const char *s)
{if(!bounded(s,32))return false;for(;*s;s++)if(!isalnum((unsigned char)*s)&&*s!='-'&&*s!='_')return false;return true;}
static bool supported_version(const char *text,bool *before_two)
{
 if(!bounded(text,32))return false;
 char version[32];strcpy(version,text);char *dash=strchr(version,'-');if(dash)*dash=0;uint32_t v[3];
 if(!dsr_version_parse(version,v)||(v[0]<1)||(v[0]==1&&(v[1]<7||(v[1]==7&&v[2]<5))))return false;
 *before_two=v[0]==1;return true;
}
static bool unique(const cJSON *r)
{
 if(cJSON_IsObject(r))for(const cJSON *a=r->child;a;a=a->next)for(const cJSON *b=a->next;b;b=b->next)if(!strcmp(a->string,b->string))return false;
 for(const cJSON *v=r->child;v;v=v->next)if(!unique(v))return false;
 return true;
}
static void fail(dsr_rpc_stream_t *s,dsr_result_t result)
{s->step=DSR_RPC_FAILED;s->result=result;s->pending=false;}
dsr_result_t dsr_rpc_stream_begin(dsr_rpc_stream_t *s,uint32_t session,uint16_t connection,uint16_t mtu,const uint8_t *request,size_t length,uint8_t *response,bool write_method,uint32_t now)
{
 if(!s||!session||connection==UINT16_MAX||mtu<23||mtu>517||!request||!length||length>DSR_BLE_RPC_TX_MAX||!response)return DSR_INVALID;
 *s=(dsr_rpc_stream_t){.session=session,.connection=connection,.mtu=mtu,.tx=request,.tx_length=length,.rx=response,.started_ms=now,.write_method=write_method,.step=DSR_RPC_TX_CTL,.result=DSR_BUSY};
 s->rx[0]=0;return DSR_OK;
}
bool dsr_rpc_stream_next(dsr_rpc_stream_t *s,uint32_t now,dsr_rpc_action_t *action)
{
 if(!s||!action||s->step>=DSR_RPC_DONE)return false;
 if(now-s->started_ms>=DSR_BLE_RPC_TIMEOUT_MS){fail(s,DSR_OFFLINE);return false;}
 if(s->pending)return false;
 if(s->sequence==UINT32_MAX){fail(s,DSR_IO);return false;}
 *action=(dsr_rpc_action_t){.kind=s->step,.session=s->session,.sequence=++s->sequence,.connection=s->connection};
 if(s->step==DSR_RPC_TX_CTL){uint32_t n=(uint32_t)s->tx_length;for(unsigned i=0;i<4;i++)action->control[i]=(uint8_t)(n>>(24-8*i));action->bytes=action->control;action->length=4;}
 else if(s->step==DSR_RPC_TX_DATA){size_t n=s->tx_length-s->tx_used,chunk=s->mtu-3u;if(n>chunk)n=chunk;action->bytes=s->tx+s->tx_used;action->length=n;if(s->write_method)s->write_possible=true;}
 s->pending=true;s->pending_length=action->length;return true;
}
bool dsr_rpc_stream_complete(dsr_rpc_stream_t *s,uint32_t session,uint32_t sequence,uint16_t connection,dsr_result_t result,const uint8_t *bytes,size_t length,uint32_t now)
{
 if(!s||!s->pending||session!=s->session||sequence!=s->sequence||connection!=s->connection||s->step>=DSR_RPC_DONE)return false;
 if(now-s->started_ms>=DSR_BLE_RPC_TIMEOUT_MS){fail(s,DSR_OFFLINE);return true;}
 if(result!=DSR_OK){fail(s,result);return true;}
 s->pending=false;
 if(s->step==DSR_RPC_TX_CTL||s->step==DSR_RPC_TX_DATA){
  if(length){fail(s,DSR_BAD_RESPONSE);return true;}
  if(s->step==DSR_RPC_TX_CTL)s->step=DSR_RPC_TX_DATA;
  else{s->tx_used+=s->pending_length;if(s->tx_used==s->tx_length)s->step=DSR_RPC_RX_CTL;}
 }else if(s->step==DSR_RPC_RX_CTL){
  if(!bytes||length!=4){fail(s,DSR_BAD_RESPONSE);return true;}
  uint32_t n=0;for(unsigned i=0;i<4;i++)n=n<<8|bytes[i];
  /* Zero means no frame announced yet, not an empty successful response.
   * The manager schedules bounded RX_CTL polling without replaying TX. */
  if(!n)return true;
  if(n>DSR_BLE_RPC_RX_MAX){fail(s,DSR_BAD_RESPONSE);return true;}
  s->rx_length=n;s->step=DSR_RPC_RX_DATA;
 }else{
  if(!bytes||!length||length>s->mtu-1u||length>s->rx_length-s->rx_used){fail(s,DSR_BAD_RESPONSE);return true;}
  memcpy(s->rx+s->rx_used,bytes,length);s->rx_used+=length;s->rx[s->rx_used]=0;
  if(s->rx_used==s->rx_length){s->step=DSR_RPC_DONE;s->result=DSR_OK;}
 }
 return true;
}
void dsr_rpc_stream_cancel(dsr_rpc_stream_t *s)
{if(s&&s->step<DSR_RPC_DONE)fail(s,DSR_BUSY);}
dsr_result_t dsr_rpc_request(dsr_rpc_method_t method,uint32_t id,const char *client,const dsr_settings_t *home,char **json)
{
 static const char *names[]={"Shelly.GetDeviceInfo","Sys.GetConfig","Shelly.ListMethods","Shelly.GetStatus","WiFi.GetStatus","WiFi.SetConfig"};
 if(json)*json=NULL;
 if(!json||method<0||method>=DSR_RPC_METHOD_COUNT||!id||!client_valid(client)||(method==DSR_RPC_WIFI_SET&&!dsr_wifi_settings_valid(home)))return DSR_INVALID;
 cJSON *root=cJSON_CreateObject();bool ok=root&&cJSON_AddNumberToObject(root,"id",id)&&cJSON_AddStringToObject(root,"src",client)&&cJSON_AddStringToObject(root,"method",names[method]);
 cJSON *password=NULL;
 if(ok&&method==DSR_RPC_INFO){cJSON *params=cJSON_AddObjectToObject(root,"params");ok=params&&cJSON_AddBoolToObject(params,"ident",false);}
 if(ok&&method==DSR_RPC_WIFI_SET){
  cJSON *params=cJSON_AddObjectToObject(root,"params"),*config=params?cJSON_AddObjectToObject(params,"config"):NULL,*sta=config?cJSON_AddObjectToObject(config,"sta"):NULL;
  ok=sta&&cJSON_AddBoolToObject(sta,"enable",true)&&cJSON_AddStringToObject(sta,"ssid",home->ssid)&&(password=cJSON_AddStringToObject(sta,"pass",home->password))&&cJSON_AddStringToObject(sta,"ipv4mode","dhcp");
 }
 char *text=ok?cJSON_PrintUnformatted(root):NULL;
 if(password){volatile char *p=password->valuestring;for(size_t i=0;i<strlen(home->password);i++)p[i]=0;}
 cJSON_Delete(root);if(!text)return DSR_IO;
 if(strlen(text)>DSR_BLE_RPC_TX_MAX){volatile char *p=text;size_t n=strlen(text);while(n--)*p++=0;free(text);return DSR_INVALID;}
 *json=text;return DSR_OK;
}
dsr_result_t dsr_rpc_response(const uint8_t *json,size_t length,uint32_t id,const char *client,const char *device,cJSON **out,bool *notification)
{
 if(out)*out=NULL;
 if(notification)*notification=false;
 if(!json||!length||length>DSR_BLE_RPC_RX_MAX||!id||!client_valid(client)||!out||!notification||(device&&!bounded(device,64))||memchr(json,0,length))return DSR_INVALID;
 char *copy=calloc(length+1,1);if(!copy)return DSR_IO;memcpy(copy,json,length);
 cJSON *r=!strstr(copy,"\\u0000")&&dsr_config_json_depth(copy)?cJSON_ParseWithLengthOpts(copy,length+1,NULL,true):NULL;free(copy);
 if(!cJSON_IsObject(r)||!unique(r)){cJSON_Delete(r);return DSR_BAD_RESPONSE;}
 const cJSON *src=cJSON_GetObjectItemCaseSensitive(r,"src"),*dst=cJSON_GetObjectItemCaseSensitive(r,"dst"),*rid=cJSON_GetObjectItemCaseSensitive(r,"id"),*method=cJSON_GetObjectItemCaseSensitive(r,"method"),*result=cJSON_GetObjectItemCaseSensitive(r,"result"),*error=cJSON_GetObjectItemCaseSensitive(r,"error"),*version=cJSON_GetObjectItemCaseSensitive(r,"jsonrpc");
 dsr_result_t status=DSR_BAD_RESPONSE;
 if(!cJSON_IsString(src)||!bounded(src->valuestring,64)||!cJSON_IsString(dst)||strcmp(dst->valuestring,client)||(version&&(!cJSON_IsString(version)||strcmp(version->valuestring,"2.0"))))goto done;
 if(device&&strcmp(src->valuestring,device)){status=DSR_IDENTITY_MISMATCH;goto done;}
 if(!rid&&cJSON_IsString(method)&&cJSON_IsObject(cJSON_GetObjectItemCaseSensitive(r,"params"))&&!result&&!error){*notification=true;status=DSR_BUSY;goto done;}
 if(!cJSON_IsNumber(rid)||!isfinite(rid->valuedouble)||rid->valuedouble!=id||method||(result!=NULL)==(error!=NULL))goto done;
 for(const cJSON *v=r->child;v;v=v->next)if(strcmp(v->string,"id")&&strcmp(v->string,"src")&&strcmp(v->string,"dst")&&strcmp(v->string,"jsonrpc")&&strcmp(v->string,"result")&&strcmp(v->string,"error"))goto done;
 if(error){const cJSON *code=cJSON_GetObjectItemCaseSensitive(error,"code");if(cJSON_IsObject(error)&&cJSON_IsNumber(code)&&isfinite(code->valuedouble)&&floor(code->valuedouble)==code->valuedouble)status=code->valuedouble==401||code->valuedouble==403?DSR_AUTH_REQUIRED:DSR_BAD_RESPONSE;goto done;}
 if(cJSON_IsObject(result)){
  if(!device){const cJSON *identity=cJSON_GetObjectItemCaseSensitive(result,"id");if(!cJSON_IsString(identity)||strcmp(identity->valuestring,src->valuestring)){status=DSR_IDENTITY_MISMATCH;goto done;}}
  *out=cJSON_DetachItemFromObjectCaseSensitive(r,"result");status=DSR_OK;
 }
 done:cJSON_Delete(r);return status;
}
dsr_result_t dsr_rpc_identity(const cJSON *result,dsr_rpc_identity_t *out)
{
 if(!out)return DSR_INVALID;
 dsr_rpc_identity_t identity={0};dsr_result_t parsed=dsr_shelly_parse_identity(result,&identity.device);if(parsed!=DSR_OK)return parsed;
 if(identity.device.generation<2)return DSR_UNSUPPORTED;
 if(identity.device.auth_required)return DSR_AUTH_REQUIRED;
 const cJSON *ver=cJSON_GetObjectItemCaseSensitive(result,"ver"),*provision=cJSON_GetObjectItemCaseSensitive(result,"provision");
 bool before_two;
 if(!cJSON_IsString(ver)||!supported_version(ver->valuestring,&before_two)||!cJSON_IsString(provision)||(strcmp(provision->valuestring,"pending")&&strcmp(provision->valuestring,"confirmed")))return DSR_UNSUPPORTED;
 strcpy(identity.firmware,ver->valuestring);identity.confirmed=!strcmp(provision->valuestring,"confirmed");*out=identity;return DSR_OK;
}
dsr_result_t dsr_rpc_setup_policy(const cJSON *system,const cJSON *methods,const cJSON *status,const dsr_rpc_identity_t *identity)
{
 if(!identity||!cJSON_IsObject(system)||!unique(system)||!cJSON_IsObject(methods)||!unique(methods)||!cJSON_IsObject(status)||!unique(status))return DSR_BAD_RESPONSE;
 const cJSON *enhanced=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(system,"device"),"enhanced_security"),*list=cJSON_GetObjectItemCaseSensitive(methods,"methods");
 bool before_two=false;if(!supported_version(identity->firmware,&before_two)||(!cJSON_IsFalse(enhanced)&&(enhanced||!before_two)))return DSR_UNSUPPORTED;
 const char *required[]={"WiFi.SetConfig","WiFi.GetStatus","Sys.GetConfig","Shelly.GetDeviceInfo","Shelly.GetStatus"};
 if(!cJSON_IsArray(list)||cJSON_GetArraySize(list)>256)return DSR_BAD_RESPONSE;
 for(unsigned i=0;i<sizeof(required)/sizeof(required[0]);i++){bool found=false;for(const cJSON *v=list->child;v;v=v->next){if(!cJSON_IsString(v))return DSR_BAD_RESPONSE;if(!strcmp(v->valuestring,required[i]))found=true;}if(!found)return DSR_UNSUPPORTED;}
 dsr_lan_capabilities_t caps={0};dsr_result_t parsed=dsr_lan_parse_capabilities(status,identity->device.generation,&caps);if(parsed!=DSR_OK)return parsed;
 if(!caps.switches)return DSR_UNSUPPORTED;
 for(const cJSON *v=status->child;v;v=v->next)if(!strncmp(v->string,"devicepower:",12)&&cJSON_GetObjectItemCaseSensitive(v,"battery"))return DSR_UNSUPPORTED;
 return DSR_OK;
}
dsr_result_t dsr_rpc_station(const cJSON *result,const dsr_settings_t *home,char host[16])
{
 if(!host||!dsr_wifi_settings_valid(home)||!cJSON_IsObject(result)||!unique(result))return DSR_INVALID;
 const cJSON *status=cJSON_GetObjectItemCaseSensitive(result,"status"),*ssid=cJSON_GetObjectItemCaseSensitive(result,"ssid"),*ip=cJSON_GetObjectItemCaseSensitive(result,"sta_ip");
 if(!cJSON_IsString(status)||strcmp(status->valuestring,"got ip")||!cJSON_IsString(ssid)||strcmp(ssid->valuestring,home->ssid)||!cJSON_IsString(ip)||!dsr_ipv4_valid(ip->valuestring))return DSR_BAD_RESPONSE;
 strcpy(host,ip->valuestring);return DSR_OK;
}
