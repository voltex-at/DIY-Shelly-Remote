#include "dsr_device_settings.h"
#include "dsr_fleet.h"
#include "dsr_blu.h"
#include "freertos/semphr.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,writes,mode;
static bool held,mapping=true;
static cJSON *configuration;
static dsr_http_job_guard_t guard;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
SemaphoreHandle_t xSemaphoreCreateMutex(void){return (void *)1;}
int xSemaphoreTake(SemaphoreHandle_t m,TickType_t t){(void)t;CHECK(m==(void *)1&&!held);held=true;return 1;}
int xSemaphoreGive(SemaphoreHandle_t m){CHECK(m==(void *)1&&held);held=false;return 1;}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){(void)bound;return c&&dsr_ipv4_valid(c->device.host);}
bool dsr_workspace_mapping_matches(const dsr_connection_t *c){return c->layout_id==7&&mapping;}
bool dsr_endpoint_equal(const dsr_device_t *a,const dsr_device_t *b){return a->channel==b->channel&&a->generation==b->generation&&a->endpoint_kind==b->endpoint_kind&&!strcmp(a->device_id,b->device_id);}
bool dsr_wifi_connected(void){return true;}
bool dsr_wifi_temporary_active(void){return false;}
dsr_http_job_guard_t dsr_http_job_guard(dsr_http_job_guard_t g){dsr_http_job_guard_t old=guard;guard=g;return old;}
bool dsr_blu_gateway_key(uint32_t generation,char address[18],char key[33]){(void)generation;(void)address;(void)key;return false;}
dsr_result_t dsr_http_settings_rpc(const dsr_connection_t *c,const char *method,const cJSON *params,cJSON **out,bool mutating,bool *attempted)
{
 CHECK(!held&&c->layout_id==7);*out=NULL;if(attempted)*attempted=false;
 CHECK(guard&&guard()==DSR_OK);
 if(!strcmp(method,"Shelly.ListMethods"))*out=cJSON_Parse("{\"methods\":[\"Switch.GetConfig\",\"Switch.SetConfig\",\"Sys.GetConfig\",\"Sys.SetConfig\"]}");
 else if(!strcmp(method,"Shelly.GetComponents"))*out=cJSON_Parse("{\"offset\":0,\"total\":1,\"components\":[{\"key\":\"switch:0\"}]}");
 else if(!strcmp(method,"Switch.GetConfig")){*out=cJSON_Duplicate(configuration,true);if(mode==1&&writes==0){cJSON_ReplaceItemInObjectCaseSensitive(*out,"auto_off",cJSON_CreateBool(true));}}
 else if(!strcmp(method,"Switch.SetConfig")){
  CHECK(mutating&&cJSON_GetObjectItemCaseSensitive(params,"id")->valueint==0);writes++;if(attempted)*attempted=true;
  const cJSON *patch=cJSON_GetObjectItemCaseSensitive(params,"config");CHECK(cJSON_GetArraySize(patch)==1);if(mode!=3){const cJSON *v=patch->child;CHECK(v&&v->string&&!strcmp(v->string,"auto_off_delay"));cJSON_ReplaceItemInObjectCaseSensitive(configuration,v->string,cJSON_Duplicate(v,true));}
  if(mode==2)return DSR_OFFLINE;
  *out=cJSON_Parse("{\"restart_required\":false}");
 }else return DSR_UNSUPPORTED;
 return *out?DSR_OK:DSR_IO;
}
static unsigned field(const char *ns,const char *path)
{for(unsigned i=0;dsr_setting_field(i);i++){const dsr_setting_field_t *f=dsr_setting_field(i);if(!strcmp(f->component,ns)&&!strcmp(f->path,path))return i;}CHECK(false);return 0;}
static void model_tests(void)
{
 cJSON *cfg=cJSON_Parse("{\"auto_off\":false,\"auto_off_delay\":60,\"initial_state\":\"off\",\"in_mode\":\"detached\"}"),*patch=NULL;CHECK(cfg);
 unsigned f=field("Switch","auto_off_delay");
 const char *bad[]={"","nan","inf","-1","1junk","1e999","86401"," 3"};for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++)CHECK(dsr_setting_patch(cfg,f,bad[i],&patch)==DSR_INVALID&&!patch);
 CHECK(dsr_setting_patch(cfg,f,"120",&patch)==DSR_OK);CHECK(dsr_setting_value(patch,"auto_off_delay")->valueint==120);cJSON_Delete(patch);
 CHECK(dsr_setting_patch(cfg,field("Switch","auto_off"),"1",&patch)==DSR_INVALID);CHECK(dsr_setting_patch(cfg,field("Switch","auto_off"),"true",&patch)==DSR_OK);cJSON_Delete(patch);
 CHECK(dsr_setting_patch(cfg,field("Switch","initial_state"),"delete",&patch)==DSR_INVALID);CHECK(dsr_setting_patch(cfg,field("Cover","maxtime_open"),"50",&patch)==DSR_INVALID);
 cJSON_Delete(cfg);cfg=cJSON_Parse("{\"sta\":{\"ssid\":\"existing\",\"enable\":true,\"ipv4mode\":\"dhcp\"},\"sta1\":{\"ssid\":null}}");
 f=field("WiFi","sta.ssid");CHECK(dsr_setting_patch(cfg,f,"new",&patch)==DSR_UNSUPPORTED&&!patch);
 CHECK(dsr_setting_network_patch(cfg,f,"new","1234567",&patch)==DSR_INVALID);
 CHECK(dsr_setting_network_patch(cfg,f,"new","12345678",&patch)==DSR_OK);CHECK(!strcmp(dsr_setting_value(patch,"sta.pass")->valuestring,"12345678"));CHECK(cJSON_IsTrue(dsr_setting_value(patch,"sta.enable")));dsr_setting_json_delete(patch);
 CHECK(dsr_setting_network_patch(cfg,f,"open","",&patch)==DSR_OK);CHECK(cJSON_IsNull(dsr_setting_value(patch,"sta.pass")));dsr_setting_json_delete(patch);
 CHECK(dsr_setting_network_patch(cfg,field("WiFi","sta1.ssid"),"backup","12345678",&patch)==DSR_OK);CHECK(dsr_setting_value(patch,"sta1.ssid"));dsr_setting_json_delete(patch);cJSON_Delete(cfg);
}
static dsr_connection_t connection(void)
{dsr_connection_t c={.layout_id=7};strcpy(c.device.host,"192.168.1.5");strcpy(c.device.device_id,"shelly-test-aabbccddeeff");c.device.generation=DSR_GEN2_PLUS;return c;}
static dsr_device_settings_snapshot_t execute(unsigned action,unsigned selection,const char *value)
{dsr_connection_t c=connection();static uint32_t id;dsr_job_t job={.id=++id,.kind=DSR_JOB_DEVICE_SETTINGS,.connection=c};CHECK(dsr_device_settings_prepare(job.id,&c,action,selection,value)==DSR_OK);CHECK(dsr_device_settings_admitted(&job));dsr_job_t forged=job;forged.connection.device.channel=1;CHECK(!dsr_device_settings_admitted(&forged));dsr_event_t event={0};dsr_device_settings_execute(&job,&event);dsr_device_settings_snapshot_t s;dsr_device_settings_snapshot(&s);CHECK(!s.busy&&event.result==s.result);return s;}
static void actor_tests(void)
{
 for(mode=0;mode<4;mode++){
  writes=0;configuration=cJSON_Parse("{\"id\":0,\"name\":null,\"auto_off\":false,\"auto_off_delay\":60}");CHECK(configuration);
  dsr_device_settings_snapshot_t s=execute(DSR_SETTINGS_LOAD,0,NULL);CHECK(s.result==DSR_OK&&s.group_count==2&&!s.ready);
  unsigned group=0;while(group<s.group_count&&strcmp(s.groups[group].component,"Switch"))group++;CHECK(group<s.group_count);
  unsigned saved_mode=mode;mode=0;s=execute(DSR_SETTINGS_GROUP,group,NULL);mode=saved_mode;CHECK(s.result==DSR_OK&&s.ready);
  s=execute(DSR_SETTINGS_SAVE,field("Switch","auto_off_delay"),"120");
  if(mode==0)CHECK(s.result==DSR_OK&&!s.uncertain&&writes==1);
  if(mode==1)CHECK(s.result==DSR_CONFLICT&&!writes);
  if(mode==2||mode==3)CHECK(s.result!=DSR_OK&&s.uncertain&&writes==1);
  dsr_device_settings_close();cJSON_Delete(configuration);configuration=NULL;
 }
 mode=0;configuration=cJSON_Parse("{}");execute(DSR_SETTINGS_LOAD,0,NULL);mapping=false;dsr_device_settings_snapshot_t s=execute(DSR_SETTINGS_GROUP,0,NULL);CHECK(s.result==DSR_CONFLICT);mapping=true;dsr_device_settings_close();cJSON_Delete(configuration);
 dsr_device_settings_visibility(true);CHECK(dsr_device_settings_sensitive());dsr_device_settings_visibility(false);CHECK(!dsr_device_settings_sensitive());
}
int main(void){model_tests();actor_tests();printf("PASS: %u actual settings model/actor checks; identity/HTTP/worker fixtures\n",checks);return 0;}
