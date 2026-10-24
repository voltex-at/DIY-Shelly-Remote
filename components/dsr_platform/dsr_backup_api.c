#include "dsr_backup_api.h"
#include "dsr_maintenance_control.h"
#include "dsr_maintenance_private.h"
#include "dsr_maintenance.h"
#include "dsr_maintenance_io.h"
#include "dsr_backup_sources.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum {TRANSFER_MS=30000,JOB_MS=20000};
typedef struct {
 atomic_uint refs;atomic_bool abandoned;SemaphoreHandle_t done;
 dsr_maint_request_t request;uint8_t *upload;size_t size;
 dsr_backup_image_t *image;dsr_result_t result;
} transfer_t;
static void hex(const uint8_t *in,size_t n,char *out)
{static const char digits[]="0123456789abcdef";for(size_t i=0;i<n;i++){out[2*i]=digits[in[i]>>4];out[2*i+1]=digits[in[i]&15];}out[2*n]=0;}
static bool id_read(httpd_req_t *r,uint8_t id[16])
{
 char value[34];if(httpd_req_get_hdr_value_str(r,"X-DSR-Maintenance",value,sizeof(value))!=ESP_OK||strlen(value)!=32)return false;
 for(unsigned i=0;i<16;i++){unsigned n=0;for(unsigned j=0;j<2;j++){char c=value[2*i+j];if(c>='0'&&c<='9')n=n*16+(unsigned)(c-'0');else if(c>='a'&&c<='f')n=n*16+(unsigned)(c-'a'+10);else return false;}id[i]=(uint8_t)n;}return true;
}
static esp_err_t response(httpd_req_t *r,const char *code,const char *body)
{httpd_resp_set_status(r,code);httpd_resp_set_type(r,"application/json");return httpd_resp_sendstr(r,body);}
static esp_err_t failure(httpd_req_t *r,dsr_result_t result)
{return response(r,result==DSR_BUSY||result==DSR_CONFLICT?"409 Conflict":result==DSR_FULL?"507 Insufficient Storage":"422 Unprocessable Content","{\"error\":\"Maintenance unavailable; check remote screen and request new consent\"}");}
static uint32_t clock_ms(void){return (uint32_t)((uint64_t)esp_timer_get_time()/1000);}
static void transfer_free(transfer_t *t)
{
 if(atomic_fetch_sub_explicit(&t->refs,1,memory_order_acq_rel)!=1)return;
 dsr_backup_image_free(t->image);dsr_maint_io_free(t->upload,t->upload?t->size:0);vSemaphoreDelete(t->done);dsr_backup_wipe(t,sizeof(*t));free(t);
}
static bool active(transfer_t *t)
{return !atomic_load(&t->abandoned)&&dsr_panel_backup_session_valid(t->request.session)&&dsr_maintenance_running(&t->request);}
static void transfer_run(void *owned)
{
 transfer_t *t=owned;dsr_result_t result=DSR_CONFLICT;dsr_maint_stage_t *stage=NULL;
 if(!active(t))goto done;
 if(t->request.op==DSR_MAINT_EXPORT){
  dsr_maint_lease_t lease;if(!dsr_maintenance_acquire(&lease)){result=DSR_BUSY;goto done;}
  result=dsr_backup_sources_capture(&lease,&t->image);dsr_maintenance_release(&lease);
  if(result==DSR_OK&&!active(t))result=DSR_CONFLICT;
 }else{
  uint8_t device[32];result=dsr_backup_device_id(device);if(result!=DSR_OK)goto done;
  result=dsr_backup_image_open(t->upload,t->size,device,&t->image);t->upload=NULL;dsr_backup_wipe(device,sizeof(device));if(result!=DSR_OK)goto done;
  if(!active(t)){result=DSR_CONFLICT;goto done;}
  result=dsr_maint_stage_prepare(t->image,&stage);if(result!=DSR_OK)goto done;
  if(active(t))result=dsr_maintenance_candidate(stage,&t->request);else result=DSR_CONFLICT;
  if(result==DSR_OK)stage=NULL;
 }
done:
 if(stage){dsr_result_t cleanup=dsr_maint_stage_cancel(stage);if(cleanup!=DSR_OK)result=DSR_IO;dsr_maint_stage_free(stage);}
 t->result=result;dsr_maintenance_finished(t->request.nonce,result);xSemaphoreGive(t->done);transfer_free(t);
}
static transfer_t *transfer_new(const dsr_maint_request_t *request)
{
 transfer_t *t=calloc(1,sizeof(*t));if(!t)return NULL;t->done=xSemaphoreCreateBinary();if(!t->done){free(t);return NULL;}
 atomic_init(&t->refs,1);atomic_init(&t->abandoned,false);t->request=*request;t->result=DSR_BUSY;return t;
}
static dsr_result_t transfer_wait(transfer_t *t)
{
 atomic_fetch_add(&t->refs,1);
 if(!dsr_workspace_maintenance_submit(transfer_run,t)){transfer_free(t);dsr_maintenance_finished(t->request.nonce,DSR_BUSY);return DSR_BUSY;}
 if(xSemaphoreTake(t->done,pdMS_TO_TICKS(JOB_MS))!=pdTRUE){atomic_store(&t->abandoned,true);(void)dsr_maintenance_cancel(t->request.nonce);return DSR_BUSY;}
 return t->result;
}
static esp_err_t status_response(httpd_req_t *r,const uint8_t session[32])
{
 dsr_maint_status_t s;dsr_maintenance_status(&s);bool owned=dsr_maintenance_session_matches(session);char id[33];hex(s.request_id,16,id);
 if(!owned)return response(r,"200 OK","{\"owned\":false,\"phase\":0}");
 cJSON *o=cJSON_CreateObject();char *body=NULL;
 if(o&&cJSON_AddBoolToObject(o,"owned",true)&&cJSON_AddStringToObject(o,"request_id",id)&&
    cJSON_AddNumberToObject(o,"op",s.op)&&cJSON_AddNumberToObject(o,"phase",s.phase)&&cJSON_AddNumberToObject(o,"result",s.result)&&cJSON_AddNumberToObject(o,"progress",s.progress)&&
    cJSON_AddNumberToObject(o,"photos",s.photos)&&cJSON_AddNumberToObject(o,"connections",s.connections)&&cJSON_AddNumberToObject(o,"rules",s.rules)&&cJSON_AddBoolToObject(o,"usb_required",s.usb_required)&&
    cJSON_AddStringToObject(o,"firmware",s.archive.firmware)&&(s.archive.created?cJSON_AddNumberToObject(o,"created",s.archive.created):cJSON_AddNullToObject(o,"created")))body=cJSON_PrintUnformatted(o);
 esp_err_t result=body?response(r,"200 OK",body):failure(r,DSR_FULL);free(body);cJSON_Delete(o);return result;
}
static esp_err_t request_operation(httpd_req_t *r,const uint8_t session[32],bool reset)
{
 static uint32_t last_request;static bool requested;uint32_t now=clock_ms();
 if(requested&&(uint32_t)(now-last_request)<1000)return response(r,"429 Too Many Requests","{\"error\":\"Wait before requesting consent again\"}");
 dsr_maint_request_t request={.op=DSR_MAINT_RESET};memcpy(request.session,session,32);
 if(reset){if(r->content_len)return response(r,"400 Bad Request","{\"error\":\"No reset data expected\"}");}
 else{
  if(!r->content_len||r->content_len>32)return response(r,"400 Bad Request","{\"error\":\"Expected export or upload operation\"}");
  char body[33]={0};size_t at=0;while(at<r->content_len){int n=httpd_req_recv(r,body+at,r->content_len-at);if(n<=0||n>(int)(r->content_len-at))break;at+=(unsigned)n;}
  const char *end=NULL;cJSON *root=at==r->content_len&&!memchr(body,0,at)&&!memchr(body,'\\',at)?cJSON_ParseWithLengthOpts(body,at,&end,false):NULL,*op=cJSON_GetObjectItemCaseSensitive(root,"op");
  while(end&&end<body+at&&(*end==' '||*end=='\t'||*end=='\r'||*end=='\n'))end++;
  bool valid=end==body+at&&cJSON_IsObject(root)&&cJSON_GetArraySize(root)==1&&cJSON_IsString(op)&&(!strcmp(op->valuestring,"export")||!strcmp(op->valuestring,"upload"));if(valid)request.op=!strcmp(op->valuestring,"export")?DSR_MAINT_EXPORT:DSR_MAINT_IMPORT_UPLOAD;cJSON_Delete(root);
  if(!valid)return response(r,"400 Bad Request","{\"error\":\"Expected export or upload operation\"}");
 }
 if(!dsr_panel_backup_session_valid(session))return response(r,"401 Unauthorized","{\"error\":\"Pair this browser on the remote\"}");
 dsr_result_t result=dsr_maint_io_random(request.nonce,16);if(result==DSR_OK)result=dsr_maintenance_request(&request);if(result!=DSR_OK)return failure(r,result);
 requested=true;last_request=now;char id[33],body[100];hex(request.nonce,16,id);snprintf(body,sizeof(body),"{\"request_id\":\"%s\",\"waiting_for_lcd\":true}",id);return response(r,"202 Accepted",body);
}
static esp_err_t transfer_response(httpd_req_t *r,const uint8_t session[32],bool upload)
{
 dsr_maint_request_t request={.op=upload?DSR_MAINT_IMPORT_UPLOAD:DSR_MAINT_EXPORT};memcpy(request.session,session,32);
 if(!id_read(r,request.nonce))return response(r,"400 Bad Request","{\"error\":\"Expected current consent ID\"}");
 if(upload){char type[40];if(r->content_len<24||r->content_len>DSR_BACKUP_MAX_BYTES||httpd_req_get_hdr_value_str(r,"Content-Type",type,sizeof(type))!=ESP_OK||strcmp(type,"application/octet-stream"))return response(r,"400 Bad Request","{\"error\":\"Invalid backup transfer size or type\"}");}
 else if(r->content_len)return response(r,"400 Bad Request","{\"error\":\"No export data expected\"}");
 transfer_t *t=transfer_new(&request);if(!t)return failure(r,DSR_FULL);
 esp_err_t response_result;dsr_result_t result=DSR_CONFLICT;if(!dsr_maintenance_take(&request))goto done;
 if(upload){
  t->size=r->content_len;t->upload=dsr_maint_io_alloc(t->size);if(!t->upload){result=DSR_FULL;goto finished;}
  size_t at=0;uint32_t began=clock_ms();unsigned timeouts=0;result=DSR_IO;
  while(at<t->size&&(uint32_t)(clock_ms()-began)<TRANSFER_MS&&active(t)){
   size_t n=t->size-at;if(n>4096)n=4096;int got=httpd_req_recv(r,(char*)t->upload+at,n);
   if(got==HTTPD_SOCK_ERR_TIMEOUT&&++timeouts<=2)continue;
   if(got<=0||got>(int)n)break;
   at+=(unsigned)got;
  }
  if(at!=t->size||!active(t))goto finished;
 }
 result=transfer_wait(t);goto done;
finished:
 dsr_maintenance_finished(request.nonce,result);
done:
 if(result!=DSR_OK)response_result=failure(r,result);
 else if(!dsr_panel_backup_session_valid(session))response_result=response(r,"401 Unauthorized","{\"error\":\"Pair this browser on the remote\"}");
 else if(upload){
  dsr_maint_status_t status;dsr_maintenance_status(&status);char id[33],body[100];hex(status.request_id,16,id);
  if(status.op!=DSR_MAINT_IMPORT_COMMIT||status.phase!=DSR_MAINT_CONFIRM)response_result=failure(r,DSR_CONFLICT);
  else{snprintf(body,sizeof(body),"{\"candidate\":true,\"request_id\":\"%s\"}",id);response_result=response(r,"202 Accepted",body);}
 }else{
  size_t size=0;const uint8_t *data=t->image?dsr_backup_image_bytes(t->image,&size):NULL;
  if(!data||size<24||size>DSR_BACKUP_MAX_BYTES)response_result=failure(r,DSR_IO);
  else{
   char length[16];snprintf(length,sizeof(length),"%u",(unsigned)size);httpd_resp_set_type(r,"application/octet-stream");httpd_resp_set_hdr(r,"X-DSR-Backup-Bytes",length);httpd_resp_set_hdr(r,"Content-Disposition","attachment; filename=snapshot.transport");
   size_t at=0;uint32_t began=clock_ms();response_result=ESP_OK;
   while(at<size&&(uint32_t)(clock_ms()-began)<TRANSFER_MS&&dsr_panel_backup_session_valid(session)){size_t n=size-at;if(n>4096)n=4096;response_result=httpd_resp_send_chunk(r,(const char*)data+at,n);if(response_result!=ESP_OK)break;at+=n;}
   if(response_result==ESP_OK&&at==size)response_result=httpd_resp_send_chunk(r,NULL,0);else response_result=ESP_FAIL;
  }
 }
 transfer_free(t);return response_result;
}
/* IDF 5.5.4 dispatches URI handlers on one HTTPS task. Only this bounded,
 * short-lived transport task may wait on the config owner or a slow socket;
 * STOP/status/cancel remain serviceable on the server task. It is not an RPC
 * worker and never accesses persistent sources itself. */
typedef struct {httpd_req_t *request;uint8_t session[32];bool upload;} async_transfer_t;
static atomic_bool transport_busy;
static void transfer_task(void *owned)
{
 async_transfer_t *a=owned;httpd_req_t *r=a->request;
 httpd_handle_t server=r->handle;int fd=httpd_req_to_sockfd(r);
 httpd_resp_set_hdr(r,"Connection","close");
 (void)transfer_response(r,a->session,a->upload);
 (void)httpd_req_async_handler_complete(r);
 (void)httpd_sess_trigger_close(server,fd);
 dsr_backup_wipe(a,sizeof(*a));free(a);atomic_store(&transport_busy,false);vTaskDelete(NULL);
}
static esp_err_t transfer_async(httpd_req_t *r,const uint8_t session[32],bool upload)
{
 bool expected=false;if(!atomic_compare_exchange_strong(&transport_busy,&expected,true))return failure(r,DSR_BUSY);
 async_transfer_t *a=calloc(1,sizeof(*a));
 if(!a){atomic_store(&transport_busy,false);return failure(r,DSR_FULL);}
 memcpy(a->session,session,32);a->upload=upload;
 if(httpd_req_async_handler_begin(r,&a->request)!=ESP_OK){free(a);atomic_store(&transport_busy,false);return failure(r,DSR_BUSY);}
 if(xTaskCreate(transfer_task,"dsr_backup_http",16384,a,3,NULL)==pdPASS)return ESP_OK;
 /* The original body was detached: respond using its owned async copy. */
 httpd_handle_t server=a->request->handle;int fd=httpd_req_to_sockfd(a->request);
 httpd_resp_set_hdr(a->request,"Connection","close");esp_err_t result=failure(a->request,DSR_BUSY);
 (void)httpd_req_async_handler_complete(a->request);(void)httpd_sess_trigger_close(server,fd);
 dsr_backup_wipe(a,sizeof(*a));free(a);atomic_store(&transport_busy,false);return result;
}
esp_err_t dsr_backup_api(httpd_req_t *r,const uint8_t session[32])
{
 httpd_resp_set_hdr(r,"Cache-Control","no-store");httpd_resp_set_hdr(r,"Pragma","no-cache");httpd_resp_set_hdr(r,"X-Content-Type-Options","nosniff");
 if(!session||!dsr_panel_backup_session_valid(session))return response(r,"401 Unauthorized","{\"error\":\"Pair this browser on the remote\"}");
 if(!strcmp(r->uri,"/api/v1/maintenance/status")){if(r->method!=HTTP_GET)return response(r,"405 Method Not Allowed","{}");return status_response(r,session);}
 if(r->method!=HTTP_POST)return response(r,"405 Method Not Allowed","{}");
 if(httpd_req_get_hdr_value_len(r,"Transfer-Encoding"))return response(r,"400 Bad Request","{\"error\":\"Content-Length required\"}");
 if(!strcmp(r->uri,"/api/v1/backup/request"))return request_operation(r,session,false);
 if(!strcmp(r->uri,"/api/v1/reset/request"))return request_operation(r,session,true);
 if(!strcmp(r->uri,"/api/v1/backup/export"))return transfer_async(r,session,false);
 if(!strcmp(r->uri,"/api/v1/backup/import"))return transfer_async(r,session,true);
 if(!strcmp(r->uri,"/api/v1/maintenance/cancel")){
  uint8_t id[16];if(r->content_len||!id_read(r,id)||!dsr_maintenance_session_matches(session)||!dsr_maintenance_cancel(id))return failure(r,DSR_CONFLICT);
  return response(r,"200 OK","{\"cancelled\":true}");
 }
 return response(r,"404 Not Found","{}");
}
