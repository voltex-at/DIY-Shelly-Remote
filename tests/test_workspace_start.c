/* Real workspace startup/snapshot gate with explicit Flash/RTOS failures.
 * Each scenario is a fresh process, matching real boot lifetime. */
#include "dsr_workspace.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,mode,writes;
static bool malloc_failure;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size){if(malloc_failure){malloc_failure=false;return NULL;}return __real_malloc(size);}
static int lock,queue;
SemaphoreHandle_t xSemaphoreCreateMutex(void){return mode==4?NULL:&lock;}
int xSemaphoreTake(SemaphoreHandle_t m,TickType_t t){(void)t;CHECK(m==&lock&&!lock);lock=1;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t m){CHECK(m==&lock&&lock);lock=0;return pdTRUE;}
QueueHandle_t xQueueCreate(unsigned n,unsigned size){CHECK(n==1&&size>0);return mode==5?NULL:&queue;}
int xQueueSend(QueueHandle_t q,const void *p,TickType_t t){(void)p;(void)t;CHECK(q==&queue);return pdTRUE;}
int xQueueReceive(QueueHandle_t q,void *p,TickType_t t){(void)q;(void)p;(void)t;abort();}
int xTaskCreate(void (*f)(void *),const char *n,unsigned stack,void *a,unsigned priority,TaskHandle_t *out)
{CHECK(f&&!strcmp(n,"dsr_config")&&stack==8192&&!a&&priority==3&&!out);return mode==6?0:pdPASS;}
esp_err_t dsr_storage_mount(void){dsr_config_health_storage(mode==1?DSR_STORAGE_NONEMPTY:DSR_STORAGE_READY,mode==1?ESP_FAIL:ESP_OK);return mode==1?ESP_FAIL:ESP_OK;}
char *dsr_config_store_load(const char *d,bool *found,char error[96]){CHECK(!strcmp(d,"/dsr"));*found=false;snprintf(error,96,"%s",mode==2?"TEST invalid checkpoints":"");return NULL;}
bool dsr_config_store_save(const char *d,const char *json,char error[96]){(void)error;CHECK(!strcmp(d,"/dsr")&&json);writes++;return mode!=3;}
bool dsr_config_store_collect_photos(const char *d,unsigned *removed,char error[96]){(void)d;*removed=0;error[0]=0;return true;}
void dsr_photos_load(void){}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){(void)c;(void)bound;return true;}
int main(int argc,char **argv)
{
 CHECK(argc==2);mode=(unsigned)strtoul(argv[1],NULL,10);CHECK(mode<=6);char *json=(void *)1;CHECK(dsr_workspace_snapshot_checked(&json)==DSR_SNAPSHOT_UNAVAILABLE&&json==NULL);
 CHECK(dsr_workspace_start()==(mode?mode>=4?ESP_ERR_NO_MEM:ESP_FAIL:ESP_OK));dsr_config_health_t h;dsr_config_health_snapshot(&h);
 if(mode){
  CHECK(h.workspace_error!=ESP_OK&&h.workspace!=DSR_WORKSPACE_READY);CHECK(dsr_workspace_snapshot_checked(&json)==DSR_SNAPSHOT_UNAVAILABLE&&!json);CHECK(!dsr_workspace_snapshot());CHECK(dsr_workspace_revision()==0);CHECK(!dsr_workspace_submit("{}",1));dsr_connection_t c={0};CHECK(!dsr_workspace_bind(&c));bool busy;char message[96];dsr_workspace_status(&busy,message);CHECK(!busy&&strstr(message,"unavailable"));CHECK(writes==(mode>=3?1u:0u));
 }else{
  CHECK(h.workspace==DSR_WORKSPACE_READY&&h.workspace_error==ESP_OK);CHECK(dsr_workspace_snapshot_checked(&json)==DSR_SNAPSHOT_OK&&json);free(json);malloc_failure=true;CHECK(dsr_workspace_snapshot_checked(&json)==DSR_SNAPSHOT_NO_MEMORY&&!json);CHECK(dsr_workspace_revision()==1);CHECK(dsr_workspace_snapshot_checked(&json)==DSR_SNAPSHOT_OK&&json);free(json);
 }
 printf("PASS: %u workspace startup checks; scenario=%u, readiness/snapshot/OOM and no writes on unavailable Flash\n",checks,mode);return 0;
}

bool dsr_actions_reserved(const dsr_device_t *device){(void)device;return false;}
void dsr_actions_mode_block(void){}
