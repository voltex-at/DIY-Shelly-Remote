#include "dsr_ui_task.h"
#include "dsr_touch_mailbox.h"
#include "dsr_touch_filter.h"
#include "dsr_storage.h"
#include "freertos/task.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "dsr_config_health.h"
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); abort(); } } while (0)
static bool task_failure = true;
static unsigned task_calls, delay_calls;
static bool allow_run;
static unsigned held;
static bool storage_test;
static unsigned current_priority=3,priority_changes,storage_delays;
UBaseType_t uxTaskPriorityGet(TaskHandle_t task){CHECK(!task);return current_priority;}
void vTaskPrioritySet(TaskHandle_t task,UBaseType_t priority)
{CHECK(!task&&storage_test&&priority==(priority_changes%2?3u:0u));current_priority=priority;priority_changes++;}
int64_t esp_timer_get_time(void){static int64_t clock;return clock+=1000;}
static bool startup_ready(void) { return allow_run; }
static void (*task_entry)(void *);
static StackType_t *reserved_stack;
static StaticTask_t *reserved_tcb;
static jmp_buf loop_done;

TaskHandle_t xTaskCreateStatic(void (*entry)(void *), const char *name,
    uint32_t bytes, void *argument, unsigned priority,
    StackType_t *stack, StaticTask_t *tcb)
{
    task_calls++;
    CHECK(!strcmp(name,"dsr_ui") && bytes == 8192 && priority == 5 && !argument);
    CHECK(stack && tcb);
    /* The retry must reuse permanent storage, not an expired main stack. */
    if (reserved_stack) CHECK(stack == reserved_stack && tcb == reserved_tcb);
    reserved_stack = stack; reserved_tcb = tcb; task_entry = entry;
    return task_failure ? NULL : (void *)1;
}
uint32_t lv_timer_handler(void)
{ const uint32_t delays[] = {0, 15, UINT32_MAX}; CHECK(delay_calls < 3 && dsr_ui_task_handler_active()); return delays[delay_calls]; }
void vTaskDelay(TickType_t ticks)
{
    if(storage_test){CHECK(ticks==1);storage_delays++;return;}
    CHECK(!dsr_ui_task_handler_active()); /* never sleeps while inside LVGL */
    if (!allow_run) { CHECK(ticks == 20); held++; longjmp(loop_done,1); }
    const unsigned expected[] = {10, 15, 20}; CHECK(ticks == expected[delay_calls]);
    if (++delay_calls == 3) longjmp(loop_done,1);
}

static esp_partition_t partition = {.size = 0x3e0000};
static bool missing, nonempty;
static esp_err_t mount_error, read_error, second_mount_error;
static unsigned mount_calls, format_calls, read_calls;
static size_t read_bytes;
esp_err_t esp_vfs_spiffs_register(const esp_vfs_spiffs_conf_t *config)
{
    CHECK(!strcmp(config->base_path,"/dsr") && !strcmp(config->partition_label,"dsrdata"));
    CHECK(config->max_files == 8 && config->format_if_mount_failed==(mount_calls==1));
    if(config->format_if_mount_failed){CHECK(read_bytes==partition.size&&current_priority==0);format_calls++;}
    return mount_calls++ == 0 ? mount_error : second_mount_error;
}
const esp_partition_t *esp_partition_find_first(unsigned type,unsigned subtype,const char *label)
{ CHECK(type == 1 && subtype == 0x82 && !strcmp(label,"dsrdata")); return missing ? NULL : &partition; }
esp_err_t esp_partition_read(const esp_partition_t *p,size_t offset,void *bytes,size_t count)
{
    CHECK(p == &partition && offset == read_bytes && count > 0 && count <= 1024 && offset+count <= p->size);
    read_calls++; read_bytes += count; memset(bytes,0xff,count);
    if (nonempty && offset+count == p->size) ((unsigned char *)bytes)[count-1] = 0;
    return read_error;
}
/* No esp_spiffs_format stub: restoring the double-format path fails to link. */
static void reset_storage(void)
{
    CHECK(current_priority==3);storage_test=true;priority_changes=storage_delays=0;
    missing=nonempty=false; mount_error=ESP_FAIL; read_error=second_mount_error=ESP_OK;
    mount_calls=format_calls=read_calls=0; read_bytes=0; partition.size=0x3e0000;
}
static void touch_mailbox_test(void)
{
 dsr_touch_mailbox_t m={0};dsr_touch_sample_t s;
 /* An entire 40 ms tap happens while a render is busy: latest-only loses it. */
 dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=120,.y=240},100);dsr_touch_push(&m,(dsr_touch_sample_t){0},100);
 s=dsr_touch_pop(&m,100);CHECK(s.pressed&&s.x==120&&s.y==240);s=dsr_touch_pop(&m,100);CHECK(!s.pressed&&s.x==120&&s.y==240);CHECK(!dsr_touch_pop(&m,100).pressed);
 dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=100,.y=100},100);s=dsr_touch_pop(&m,100);CHECK(s.pressed);
 for(unsigned i=0;i<100;i++)dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=(uint16_t)(100+i),.y=150},100);
 CHECK(m.count==1);s=dsr_touch_pop(&m,100);CHECK(s.pressed&&s.x==199&&s.y==150&&s.dragged);
 dsr_touch_push(&m,(dsr_touch_sample_t){0},100);CHECK(!dsr_touch_pop(&m,100).pressed);
 for(unsigned i=0;i<9;i++)dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=i%2==0,.x=12,.y=34},100);
 CHECK(!m.count&&m.muted&&!dsr_touch_pop(&m,100).pressed);dsr_touch_push(&m,(dsr_touch_sample_t){0},100);CHECK(!m.muted);
 dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=20,.y=30},100);CHECK(dsr_touch_pop(&m,100).pressed);
 dsr_touch_push(&m,(dsr_touch_sample_t){0},100);s=dsr_touch_pop(&m,700);CHECK(!s.pressed&&s.cancelled&&!m.count);
 dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=20,.y=30},1000);s=dsr_touch_pop(&m,1600);CHECK(!s.pressed&&s.cancelled&&m.muted);
 dsr_touch_push(&m,(dsr_touch_sample_t){0},1700);CHECK(!m.muted&&!dsr_touch_pop(&m,1700).pressed);
 dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=30,.y=40},UINT32_MAX-50);s=dsr_touch_pop(&m,25);CHECK(s.pressed&&!s.cancelled);
 /* A whole swipe between two display polls retains movement before release. */
 m=(dsr_touch_mailbox_t){0};
 dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=100,.y=200},10);
 dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=100,.y=120},20);
 dsr_touch_push(&m,(dsr_touch_sample_t){0},30);
 uint32_t captured;s=dsr_touch_pop_at(&m,40,&captured);CHECK(s.pressed&&s.y==200&&!s.dragged&&captured==10);
 s=dsr_touch_pop_at(&m,40,&captured);CHECK(s.pressed&&s.y==120&&s.dragged&&captured==20);
 s=dsr_touch_pop_at(&m,40,&captured);CHECK(!s.pressed&&s.dragged&&captured==30&&!m.count);
 /* Movement does not grow the queue; out-and-back keeps its drag evidence. */
 m=(dsr_touch_mailbox_t){0};dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=20,.y=30},1);
 for(unsigned i=0;i<1000;i++)dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=(uint16_t)(20+i%50),.y=30},2);
 dsr_touch_push(&m,(dsr_touch_sample_t){.pressed=true,.x=20,.y=30},3);CHECK(m.count==2);
 (void)dsr_touch_pop(&m,4);s=dsr_touch_pop(&m,4);CHECK(s.pressed&&s.x==20&&s.dragged);
}
static void contact_filter_test(void)
{
 dsr_touch_filter_t f={0};dsr_touch_sample_t s,p={.pressed=true,.x=20,.y=30},up={0};
 CHECK(dsr_touch_filter(&f,p,true,10,&s)&&s.pressed); /* no delay on press */
 CHECK(dsr_touch_filter(&f,up,true,20,&s)&&s.pressed);
 CHECK(dsr_touch_filter(&f,p,true,30,&s)&&s.pressed); /* 10 ms dropout */
 CHECK(dsr_touch_filter(&f,up,false,40,&s)&&s.pressed&&!s.cancelled);
 CHECK(dsr_touch_filter(&f,p,true,50,&s)&&s.pressed);
 for(unsigned i=0;i<3;i++)CHECK(dsr_touch_filter(&f,up,true,60+i*10,&s)&&s.pressed);
 CHECK(dsr_touch_filter(&f,up,true,90,&s)&&!s.pressed&&!s.cancelled&&s.x==20);
 CHECK(dsr_touch_filter(&f,p,true,100,&s)&&s.pressed);
 CHECK(dsr_touch_filter(&f,up,false,199,&s)&&s.pressed);
 CHECK(dsr_touch_filter(&f,up,false,200,&s)&&!s.pressed&&s.cancelled);
 CHECK(!dsr_touch_filter(&f,p,true,210,&s)); /* reconnect cannot click */
 CHECK(!dsr_touch_filter(&f,up,true,220,&s));
 CHECK(dsr_touch_filter(&f,up,true,250,&s)&&!s.pressed&&!s.cancelled);
 CHECK(dsr_touch_filter(&f,p,true,260,&s)&&s.pressed);
 CHECK(dsr_touch_filter(&f,(dsr_touch_sample_t){.cancelled=true},true,270,&s)&&s.cancelled);
 CHECK(!dsr_touch_filter(&f,p,true,280,&s));
 f=(dsr_touch_filter_t){0};CHECK(dsr_touch_filter(&f,p,true,UINT32_MAX-20,&s)&&s.pressed);
 CHECK(dsr_touch_filter(&f,up,true,UINT32_MAX-10,&s)&&s.pressed);
 CHECK(dsr_touch_filter(&f,up,true,25,&s)&&!s.pressed);
}
int main(void)
{
    touch_mailbox_test();
    contact_filter_test();
    /* Dynamic task creation is deliberately absent from this link: restoring
     * the 0.7.1 xTaskCreate path breaks the regression target at link time. */
    CHECK(dsr_ui_task_start(NULL) == ESP_ERR_INVALID_ARG && task_calls == 0);
    CHECK(dsr_ui_task_start(startup_ready) == ESP_ERR_NO_MEM);
    task_failure=false; CHECK(dsr_ui_task_start(startup_ready) == ESP_OK);
    CHECK(dsr_ui_task_start(startup_ready) == ESP_ERR_INVALID_STATE && task_calls == 2);
    if (!setjmp(loop_done)) task_entry(NULL);
    CHECK(held == 1 && delay_calls == 0); /* No LVGL callbacks before readiness. */
    allow_run=true;
    if (!setjmp(loop_done)) task_entry(NULL);
    CHECK(delay_calls == 3);
    allow_run=false;
    if (!setjmp(loop_done)) task_entry(NULL);
    CHECK(held == 2 && delay_calls == 3); /* A latched failure closes the gate. */
    dsr_ui_task_quiesce(); CHECK(!dsr_ui_task_handler_active()); /* returns at once when idle */
    CHECK(dsr_ui_task_started_with(startup_ready) && !dsr_ui_task_started_with(NULL));
    reset_storage(); mount_error=ESP_OK;
    CHECK(dsr_storage_mount() == ESP_OK && mount_calls == 1 && !format_calls && !read_calls);
    dsr_config_health_t health;dsr_config_health_snapshot(&health);CHECK(health.storage==DSR_STORAGE_READY&&health.storage_error==ESP_OK);
    reset_storage(); mount_error=ESP_ERR_NO_MEM;
    CHECK(dsr_storage_mount() == ESP_ERR_NO_MEM && !read_calls && !format_calls);
    dsr_config_health_snapshot(&health);CHECK(health.storage==DSR_STORAGE_MOUNT_ERROR&&health.storage_error==ESP_ERR_NO_MEM);
    reset_storage(); mount_error=ESP_ERR_INVALID_STATE;
    CHECK(dsr_storage_mount() == ESP_ERR_INVALID_STATE && !read_calls && !format_calls);
    reset_storage(); missing=true;
    CHECK(dsr_storage_mount() == ESP_FAIL && !format_calls);
    reset_storage(); read_error=ESP_ERR_INVALID_SIZE;
    CHECK(dsr_storage_mount() == ESP_ERR_INVALID_SIZE && !format_calls);
    dsr_config_health_snapshot(&health);CHECK(health.storage==DSR_STORAGE_READ_ERROR&&health.storage_error==ESP_ERR_INVALID_SIZE);
    reset_storage(); nonempty=true;
    CHECK(dsr_storage_mount() == ESP_FAIL && !format_calls && mount_calls == 1);
    dsr_config_health_snapshot(&health);CHECK(health.storage==DSR_STORAGE_NONEMPTY&&health.storage_error==ESP_FAIL);
    reset_storage();
    CHECK(dsr_storage_mount() == ESP_OK && format_calls == 1 && mount_calls == 2);
    CHECK(read_bytes == 0x3e0000&&priority_changes==2&&current_priority==3&&storage_delays==17);
    reset_storage(); second_mount_error=ESP_ERR_NO_MEM;
    CHECK(dsr_storage_mount() == ESP_ERR_NO_MEM && format_calls == 1 && mount_calls == 2&&priority_changes==2&&current_priority==3);
    dsr_config_health_snapshot(&health);CHECK(health.storage==DSR_STORAGE_PROVISION_ERROR&&health.storage_error==ESP_ERR_NO_MEM);
    reset_storage(); second_mount_error=ESP_FAIL;
    CHECK(dsr_storage_mount() == ESP_FAIL && format_calls == 1 && mount_calls == 2&&priority_changes==2&&current_priority==3);
    reset_storage(); partition.size=1027;
    CHECK(dsr_storage_mount() == ESP_OK && read_calls == 2 && read_bytes == 1027);
    printf("PASS: %u boot-memory checks; static 8192-byte UI task, retry/guard/timing, blank-only provisioning, NO_MEM/read/nonempty preservation; host fixtures, not physical boot\n",checks);
    return 0;
}
