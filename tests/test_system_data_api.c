#define main control_fixture_unused_main
#include "test_system_data_control.c"
#undef main
#include "dsr_backup_api.h"
#include "dsr_backup_sources.h"
#include "dsr_maintenance_io.h"
#include "freertos/semphr.h"
static bool session_valid=true,chunked,short_body,wait_timeout;static const char *input="",*id_header="";static size_t input_at,bytes_sent;
static unsigned capture_calls,stage_calls,decode_calls;static bool no_store;static char response[512],status_code[40];
struct dsr_backup_image{uint8_t *data;size_t size;};
bool dsr_panel_backup_session_valid(const uint8_t session[32]){return session_valid&&session&&session[0]==2;}
void *dsr_maint_io_alloc(size_t n){return calloc(1,n);}
void dsr_maint_io_free(void *p,size_t n){if(p)memset(p,0,n);free(p);}
void dsr_backup_wipe(void *p,size_t n){memset(p,0,n);}
dsr_result_t dsr_backup_sources_capture(const dsr_maint_lease_t *l,dsr_backup_image_t **out){CHECK(dsr_maintenance_lease_valid(l));capture_calls++;*out=calloc(1,sizeof(**out));CHECK(*out);(*out)->size=9000;(*out)->data=calloc(1,9000);return DSR_OK;}
const uint8_t *dsr_backup_image_bytes(const dsr_backup_image_t *i,size_t *size){*size=i->size;return i->data;}
void dsr_backup_image_free(dsr_backup_image_t *i){if(i){free(i->data);free(i);}}
dsr_result_t dsr_backup_image_open(uint8_t *p,size_t n,const uint8_t device[32],dsr_backup_image_t **out){(void)device;(void)out;CHECK(p&&n>=24);decode_calls++;free(p);return DSR_INVALID;}
dsr_result_t dsr_maint_stage_prepare(const dsr_backup_image_t *i,dsr_maint_stage_t **out){(void)i;(void)out;stage_calls++;return DSR_IO;}
SemaphoreHandle_t xSemaphoreCreateBinary(void){return calloc(1,sizeof(int));}
int xSemaphoreGive(SemaphoreHandle_t s){*(int*)s=1;return pdTRUE;}
void vSemaphoreDelete(SemaphoreHandle_t s){free(s);}
int xSemaphoreTake(SemaphoreHandle_t s,TickType_t t){(void)t;if(wait_timeout)return pdFALSE;if(job)run();int result=*(int*)s;*(int*)s=0;return result;}
int httpd_req_recv(httpd_req_t *r,char *out,size_t n){(void)r;CHECK(n<=4096);if(short_body)return 0;size_t left=strlen(input)-input_at;if(n>left)n=left;memcpy(out,input+input_at,n);input_at+=n;return (int)n;}
size_t httpd_req_get_hdr_value_len(httpd_req_t *r,const char *name){(void)r;return !strcmp(name,"Transfer-Encoding")&&chunked?7:0;}
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *r,const char *name,char *out,size_t cap){(void)r;const char *v=!strcmp(name,"X-DSR-Maintenance")?id_header:!strcmp(name,"Content-Type")?"application/octet-stream":NULL;if(!v||strlen(v)>=cap)return ESP_FAIL;strcpy(out,v);return ESP_OK;}
esp_err_t httpd_resp_set_status(httpd_req_t *r,const char *s){(void)r;snprintf(status_code,sizeof(status_code),"%s",s);return ESP_OK;}
esp_err_t httpd_resp_set_type(httpd_req_t *r,const char *s){(void)r;(void)s;return ESP_OK;}
esp_err_t httpd_resp_set_hdr(httpd_req_t *r,const char *k,const char *v){(void)r;if(!strcmp(k,"Cache-Control"))no_store=!strcmp(v,"no-store");return ESP_OK;}
esp_err_t httpd_resp_sendstr(httpd_req_t *r,const char *s){(void)r;snprintf(response,sizeof(response),"%s",s);return ESP_OK;}
esp_err_t httpd_resp_send_chunk(httpd_req_t *r,const char *p,size_t n){(void)r;(void)p;CHECK(n<=4096);bytes_sent+=n;return ESP_OK;}
static void call_api(const char *uri,int method,size_t n,const uint8_t *session){httpd_req_t r={.uri=uri,.method=method,.content_len=n};no_store=false;response[0]=status_code[0]=0;input_at=0;(void)dsr_backup_api(&r,session);CHECK(no_store);}
static void request_id(char out[33]){cJSON *root=cJSON_Parse(response);const cJSON *id=cJSON_GetObjectItemCaseSensitive(root,"request_id");CHECK(cJSON_IsString(id)&&strlen(id->valuestring)==32);strcpy(out,id->valuestring);cJSON_Delete(root);}
static void grant(const char *op,char out[33]){char body[32];snprintf(body,sizeof(body),"{\"op\":\"%s\"}",op);input=body;now+=1001;call_api("/api/v1/backup/request",HTTP_POST,strlen(input),(uint8_t[32]){2});CHECK(!strncmp(status_code,"202",3));request_id(out);uint8_t id[16];for(unsigned i=0;i<16;i++){unsigned v;CHECK(sscanf(out+2*i,"%2x",&v)==1);id[i]=(uint8_t)v;}CHECK(dsr_maintenance_confirm(id,false)==DSR_OK);input="";id_header=out;}
int main(void)
{
 uint8_t session[32]={2},other[32]={9};char id[33];
 call_api("/api/v1/backup/export",HTTP_POST,0,NULL);CHECK(!strncmp(status_code,"401",3)&&!bytes_sent&&!capture_calls&&!writes);
 call_api("/api/v1/backup/export",HTTP_GET,0,session);CHECK(!strncmp(status_code,"405",3)&&!bytes_sent);
 id_header="01000000000000000000000000000000";call_api("/api/v1/backup/export",HTTP_POST,0,session);CHECK(!strncmp(status_code,"409",3)&&!capture_calls);
 input="{\"op\":\"export\",\"session\":\"attacker\"}";call_api("/api/v1/backup/request",HTTP_POST,strlen(input),session);CHECK(!strncmp(status_code,"400",3));
 input="{\"op\":\"export\"}x";call_api("/api/v1/backup/request",HTTP_POST,strlen(input),session);CHECK(!strncmp(status_code,"400",3));
 input="{\"op\":\"export\\u0000\"}";call_api("/api/v1/backup/request",HTTP_POST,strlen(input),session);CHECK(!strncmp(status_code,"400",3));
 input="{\"op\":\"export\"}";now+=1001;call_api("/api/v1/backup/request",HTTP_POST,strlen(input),session);CHECK(!strncmp(status_code,"202",3));request_id(id);id_header=id;
 call_api("/api/v1/backup/export",HTTP_POST,0,session);CHECK(!capture_calls&&!bytes_sent);
 uint8_t raw_id[16];for(unsigned i=0;i<16;i++){unsigned value;CHECK(sscanf(id+2*i,"%2x",&value)==1);raw_id[i]=(uint8_t)value;}
 CHECK(dsr_maintenance_confirm(raw_id,false)==DSR_OK);call_api("/api/v1/backup/export",HTTP_POST,0,other);CHECK(!capture_calls&&!bytes_sent);
 call_api("/api/v1/backup/export",HTTP_POST,0,session);CHECK(capture_calls==1&&bytes_sent==9000);call_api("/api/v1/backup/export",HTTP_POST,0,session);CHECK(capture_calls==1&&bytes_sent==9000);
 call_api("/api/v1/backup/import",HTTP_POST,2097153,session);CHECK(!decode_calls&&!stage_calls&&!writes);
 chunked=true;call_api("/api/v1/backup/import",HTTP_POST,24,session);CHECK(!decode_calls&&!stage_calls);chunked=false;
 session_valid=false;call_api("/api/v1/reset/request",HTTP_POST,0,session);CHECK(!writes&&!restarts);session_valid=true;
 grant("upload",id);input="abcdefghijklmnopqrstuvwx";short_body=true;call_api("/api/v1/backup/import",HTTP_POST,32,session);short_body=false;CHECK(!decode_calls&&!stage_calls);
 grant("upload",id);input="abcdefghijklmnopqrstuvwx";call_api("/api/v1/backup/import",HTTP_POST,24,session);CHECK(decode_calls==1&&!stage_calls);
 grant("export",id);wait_timeout=true;call_api("/api/v1/backup/export",HTTP_POST,0,session);CHECK(job&&capture_calls==1);wait_timeout=false;run();CHECK(!job&&capture_calls==1);
 CHECK(!power_reads&&!writes&&!restarts);printf("PASS: %u paired backup API checks; real grants/control, HTTP/queue/data providers MODEL\n",checks);return 0;
}
