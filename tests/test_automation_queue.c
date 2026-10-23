/* Actual config task/mailboxes with explicit RTOS and Flash providers. */
#define _POSIX_C_SOURCE 200809L
#include "dsr_workspace.h"
#include "dsr_automation_store.h"
#include "dsr_activity.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,config_saves,journal_saves;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__func__,__LINE__,#x);exit(1);}}while(0)
static int lock,q;static bool queued,queue_full,fail_save;static size_t size;
static unsigned char message[64];static jmp_buf done;static void (*task)(void*);
static uint32_t saved_generation;
SemaphoreHandle_t xSemaphoreCreateMutex(void){return &lock;}
int xSemaphoreTake(SemaphoreHandle_t h,TickType_t t){(void)t;CHECK(h==&lock&&!lock);lock=1;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t h){CHECK(h==&lock&&lock);lock=0;return pdTRUE;}
QueueHandle_t xQueueCreate(unsigned n,unsigned s){CHECK(n==1&&s<=sizeof(message));size=s;return &q;}
int xQueueSend(QueueHandle_t h,const void *p,TickType_t t){(void)t;CHECK(h==&q);if(queue_full||queued)return 0;memcpy(message,p,size);queued=true;return pdTRUE;}
int xQueueReceive(QueueHandle_t h,void *p,TickType_t t){(void)t;CHECK(h==&q);if(!queued)longjmp(done,1);memcpy(p,message,size);queued=false;return pdTRUE;}
int xTaskCreate(void(*f)(void*),const char *n,unsigned s,void *a,unsigned pri,TaskHandle_t *out)
{CHECK(!strcmp(n,"dsr_config")&&s==8192&&!a&&pri==3&&!out);task=f;return pdPASS;}
esp_err_t dsr_storage_mount(void){dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);return ESP_OK;}
char *dsr_config_store_load(const char *d,bool *found,char error[96]){
 CHECK(!strcmp(d,"/dsr"));*found=false;error[0]=0;return NULL;
}
bool dsr_config_store_save(const char *d,const char *json,char error[96]){
 CHECK(!strcmp(d,"/dsr")&&json&&!lock);config_saves++;error[0]=0;return true;
}
bool dsr_config_store_collect_photos(const char *d,unsigned *removed,char error[96]){(void)d;*removed=0;error[0]=0;return true;}
void dsr_photos_load(void){}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){(void)bound;return c!=NULL;}
bool dsr_actions_reserved(const dsr_device_t *d){(void)d;return false;}
void dsr_actions_mode_block(void){}
esp_err_t dsr_auto_store_save(const dsr_auto_record_t *r){
 CHECK(r&&!lock);CHECK(!dsr_activity_restart_reserve());journal_saves++;saved_generation=r->generation;return fail_save?ESP_FAIL:ESP_OK;
}
static void execute(void){if(!setjmp(done))task(NULL);}
int main(void){
 dsr_auto_record_t r={.generation=1};uint32_t generation;esp_err_t result;
 CHECK(!dsr_workspace_auto_submit(&r,1));CHECK(dsr_workspace_start()==ESP_OK);uint32_t revision=dsr_workspace_revision();
 unsigned config_before=config_saves;CHECK(!dsr_workspace_auto_submit(&r,2));
 queue_full=true;CHECK(!dsr_workspace_auto_submit(&r,1));queue_full=false;
 CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 CHECK(dsr_workspace_auto_submit(&r,1));CHECK(!dsr_workspace_auto_submit(&r,1));CHECK(!dsr_workspace_auto_take_result(&generation,&result));
 r.generation=2;execute();CHECK(saved_generation==1&&journal_saves==1);CHECK(dsr_workspace_revision()==revision&&config_saves==config_before);
 CHECK(!dsr_workspace_auto_submit(&r,2)); /* old result remains owned until consumed */
 CHECK(dsr_workspace_auto_take_result(&generation,&result)&&generation==1&&result==ESP_OK);
 CHECK(!dsr_workspace_auto_take_result(&generation,&result));CHECK(dsr_workspace_auto_submit(&r,2));
 fail_save=true;execute();CHECK(dsr_workspace_auto_take_result(&generation,&result)&&generation==2&&result==ESP_FAIL);
 CHECK(dsr_workspace_revision()==revision&&config_saves==config_before);
 CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 /* A journal request is never a configuration-save success, and vice versa. */
 char *snapshot=dsr_workspace_snapshot();CHECK(snapshot&&dsr_workspace_submit(snapshot,revision));
 r.generation=3;CHECK(!dsr_workspace_auto_submit(&r,3));execute();CHECK(dsr_workspace_revision()==revision+1);
 CHECK(!dsr_workspace_auto_take_result(&generation,&result));free(snapshot);
 printf("automation queue: %u checks passed\n",checks);return 0;
}
