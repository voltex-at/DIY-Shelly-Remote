#define _POSIX_C_SOURCE 200809L
#include "dsr_workspace.h"
#include "dsr_activity.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,saves,gcs,attempts,fail_at,allocations_at_save;
static bool fail_save,queued;
static int mutex,queue;
static void (*task)(void *);
static jmp_buf done;
static unsigned char message[64];
static size_t message_size;
static char *durable;
static const char legacy[]="{\"schema\":1,\"revision\":7,\"language\":\"pl\",\"theme\":\"graphite\",\"accent\":5020661,\"active_dashboard\":1,\"rooms\":[],\"devices\":[],\"scenes\":[],\"dashboards\":[{\"id\":1,\"name\":\"Home\",\"tiles\":[]}]}";
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static void *allocate(size_t n){return ++attempts==fail_at?NULL:malloc(n);}
SemaphoreHandle_t xSemaphoreCreateMutex(void){return &mutex;}
int xSemaphoreTake(SemaphoreHandle_t s,TickType_t t){(void)t;CHECK(s==&mutex&&!mutex);mutex=1;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t s){CHECK(s==&mutex&&mutex);mutex=0;return pdTRUE;}
QueueHandle_t xQueueCreate(unsigned n,unsigned bytes){CHECK(n==1&&bytes<=sizeof(message));message_size=bytes;return &queue;}
int xQueueSend(QueueHandle_t q,const void *p,TickType_t t){(void)t;CHECK(q==&queue&&!queued);memcpy(message,p,message_size);queued=true;return pdTRUE;}
int xQueueReceive(QueueHandle_t q,void *p,TickType_t t){(void)t;CHECK(q==&queue);if(!queued)longjmp(done,1);memcpy(p,message,message_size);queued=false;return pdTRUE;}
int xTaskCreate(void(*f)(void *),const char *name,unsigned stack,void *arg,unsigned pri,TaskHandle_t *out){CHECK(!strcmp(name,"dsr_config")&&stack==8192&&!arg&&pri==3&&!out);task=f;return pdPASS;}
esp_err_t dsr_storage_mount(void){dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);return ESP_OK;}
char *dsr_config_store_load(const char *dir,bool *found,char error[96]){CHECK(!strcmp(dir,"/dsr"));*found=true;error[0]=0;return strdup(durable?durable:legacy);}
bool dsr_config_store_save(const char *dir,const char *json,char error[96]){CHECK(!strcmp(dir,"/dsr"));saves++;if(fail_save){strcpy(error,"Injected readback failure");return false;}free(durable);durable=strdup(json);allocations_at_save=attempts;return durable!=NULL;}
bool dsr_config_store_collect_photos(const char *dir,unsigned *removed,char error[96]){CHECK(!strcmp(dir,"/dsr"));gcs++;*removed=0;error[0]=0;return true;}
void dsr_photos_load(void){}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){(void)bound;return c!=NULL;}
static void execute(void){if(!setjmp(done))task(NULL);}
int main(int argc,char **argv)
{
 CHECK(argc==2);fail_at=(unsigned)atoi(argv[1]);cJSON_Hooks hooks={allocate,free};cJSON_InitHooks(&hooks);
 esp_err_t started=dsr_workspace_start();unsigned boot_attempts=attempts;fail_at=0;
 CHECK(saves==0&&gcs==0&&!durable); /* No boot write or GC from legacy1. */
 if(started!=ESP_OK){CHECK(!task&&dsr_workspace_revision()==0&&!dsr_workspace_snapshot());CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();printf("workspace boot OOM %u preserved legacy\n",boot_attempts);return 0;}
 CHECK(task&&dsr_workspace_revision()==7);char *snapshot=dsr_workspace_snapshot();CHECK(snapshot);cJSON *r=cJSON_Parse(snapshot);CHECK(r&&cJSON_GetObjectItemCaseSensitive(r,"schema")->valueint==2&&cJSON_IsArray(cJSON_GetObjectItemCaseSensitive(r,"groups")));cJSON_Delete(r);
 CHECK(!dsr_workspace_submit(snapshot,6));CHECK(dsr_workspace_submit(snapshot,7));CHECK(!dsr_activity_restart_reserve());fail_save=true;execute();CHECK(dsr_workspace_revision()==7&&!durable&&saves==1&&gcs==0);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 bool pending;char status[96];dsr_workspace_status(&pending,status);CHECK(!pending&&!strcmp(status,"Injected readback failure"));
 fail_save=false;CHECK(dsr_workspace_submit(snapshot,7));execute();CHECK(attempts==allocations_at_save);CHECK(dsr_workspace_revision()==8&&saves==2&&gcs==0&&durable&&dsr_config_revision(durable)==8);dsr_workspace_status(&pending,status);CHECK(!pending&&!strcmp(status,"Saved revision 8"));CHECK(!dsr_workspace_submit(snapshot,7));CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();free(snapshot);
 printf("PASS: %u actual workspace checks; lazy1->2, zero boot writes/GC, exact first-save ACK/failure/restart; BOOT_ALLOCATIONS=%u\n",checks,boot_attempts);free(durable);return 0;
}

/* No actor in this isolated workspace fixture. */
bool dsr_actions_reserved(const dsr_device_t *device){(void)device;return false;}
void dsr_actions_mode_block(void){}
