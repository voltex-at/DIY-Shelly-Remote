/* Actual workspace + config JSON implementation. RTOS/storage providers MODEL. */
#define _POSIX_C_SOURCE 200809L
#include "dsr_workspace.h"
#include "dsr_activity_internal.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,saves;static bool locked,occupied,queue_fail,save_fail,running;
static char queued[64];static size_t item_size;static int mutex_token,queue_token;
static void (*worker)(void *);static jmp_buf scheduler;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
SemaphoreHandle_t xSemaphoreCreateMutex(void){return &mutex_token;}
int xSemaphoreTake(SemaphoreHandle_t h,TickType_t timeout){CHECK(h==&mutex_token&&timeout==portMAX_DELAY&&!locked);locked=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t h){CHECK(h==&mutex_token&&locked);locked=false;return pdTRUE;}
QueueHandle_t xQueueCreate(unsigned count,unsigned size){CHECK(count==1&&size<=sizeof(queued));item_size=size;return &queue_token;}
int xQueueSend(QueueHandle_t h,const void *p,TickType_t wait){CHECK(h==&queue_token&&!wait&&!occupied&&!dsr_activity_restart_reserve());if(queue_fail)return pdFALSE;memcpy(queued,p,item_size);occupied=true;return pdTRUE;}
int xQueueReceive(QueueHandle_t h,void *p,TickType_t wait){CHECK(h==&queue_token&&wait==portMAX_DELAY);if(!occupied)longjmp(scheduler,1);CHECK(!dsr_activity_restart_reserve());memcpy(p,queued,item_size);occupied=false;return pdTRUE;}
int xTaskCreate(TaskFunction_t fn,const char *name,uint32_t bytes,void *arg,UBaseType_t priority,TaskHandle_t *out){CHECK(fn&&!strcmp(name,"dsr_config")&&bytes==8192&&!arg&&priority==3&&!out);worker=fn;return pdPASS;}
esp_err_t dsr_storage_mount(void){return ESP_OK;}
char *dsr_config_store_load(const char *path,bool *found,char error[96]){CHECK(!strcmp(path,"/dsr"));(void)error;*found=true;return dsr_config_default();}
bool dsr_config_store_save(const char *path,const char *json,char error[96])
{CHECK(!strcmp(path,"/dsr")&&dsr_config_revision(json)>0);saves++;if(running){CHECK(!dsr_activity_restart_reserve());bool busy;char message[96];dsr_workspace_status(&busy,message);CHECK(busy);}if(save_fail){snprintf(error,96,"fixture save failure");return false;}return true;}
bool dsr_config_store_collect_photos(const char *path,unsigned *removed,char error[96]){CHECK(!strcmp(path,"/dsr"));(void)error;*removed=0;return true;}
void dsr_photos_load(void){}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){CHECK(!bound);return c!=NULL;}
static void admission_open(void){CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static void execute(void){running=true;if(!setjmp(scheduler))worker(NULL);running=false;CHECK(!locked&&!occupied);}
int main(void)
{
 CHECK(dsr_workspace_start()==ESP_OK&&worker&&dsr_workspace_revision()==1);CHECK(dsr_workspace_start()==ESP_ERR_INVALID_STATE);admission_open();
 char *document=dsr_workspace_snapshot();CHECK(document);unsigned before=saves;
 CHECK(dsr_workspace_submit(document,1)&&occupied&&!dsr_activity_restart_reserve());CHECK(!dsr_workspace_submit(document,1));free(document);execute();
 bool busy;char message[96];dsr_workspace_status(&busy,message);CHECK(!busy&&saves==before+1&&dsr_workspace_revision()==2);admission_open();
 document=dsr_workspace_snapshot();queue_fail=true;CHECK(!dsr_workspace_submit(document,2)&&!occupied);queue_fail=false;admission_open();
 CHECK(!dsr_workspace_submit(document,1));admission_open();
 save_fail=true;CHECK(dsr_workspace_submit(document,2));execute();save_fail=false;CHECK(dsr_workspace_revision()==2);dsr_workspace_status(&busy,message);CHECK(!busy&&strstr(message,"fixture"));admission_open();
 before=saves;CHECK(dsr_workspace_submit("{invalid",2));execute();dsr_workspace_status(&busy,message);CHECK(!busy&&saves==before&&dsr_workspace_revision()==2);admission_open();
 CHECK(dsr_activity_restart_reserve());before=saves;CHECK(!dsr_workspace_submit(document,2)&&!occupied&&saves==before);dsr_activity_restart_cancel();free(document);admission_open();
 dsr_connection_t c={0},taken;uint32_t room=0;
 CHECK(dsr_workspace_bind_with_room(&c,42)&&!dsr_activity_restart_reserve());CHECK(!dsr_workspace_bind(&c));
 CHECK(dsr_workspace_take_bind_with_room(&taken,&room)&&room==42&&!dsr_activity_restart_reserve());CHECK(!dsr_workspace_take_bind_with_room(&taken,&room));
 dsr_workspace_bind_result(DSR_IO);admission_open();dsr_workspace_bind_result(DSR_IO);admission_open();
 CHECK(dsr_activity_restart_reserve());CHECK(!dsr_workspace_bind(&c));dsr_activity_restart_cancel();admission_open();
 printf("PASS: %u actual workspace/JSON admission checks; queue/storage MODEL, queued/save/result/bind/failure cleanup\n",checks);return 0;
}

/* Actor absent from this isolated workspace/restart fixture. */
bool dsr_actions_reserved(const dsr_device_t *device){(void)device;return false;}
void dsr_actions_mode_block(void){}
