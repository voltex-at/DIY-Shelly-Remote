/* Actual LAN task/enqueue/suspend code, deterministic RTOS scheduling. The
 * transport fixture proves that authenticated reads finish before radio handoff;
 * it does not emulate radio hardware or Digest authentication. */
#include <assert.h>
#include <setjmp.h>
#include "../components/dsr_platform/dsr_lan.c"
static unsigned checks,delays,http_reads;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
struct test_semaphore {bool locked;};
static struct test_semaphore semaphore;
static lan_job_t queued;
static lan_job_t *running_job;
static bool queued_ready,home=true,drain_on_delay,cancel_ap,http_finished;
static int64_t clock_us;
static jmp_buf task_exit;
SemaphoreHandle_t xSemaphoreCreateMutex(void){return &semaphore;}
int xSemaphoreTake(SemaphoreHandle_t s,TickType_t timeout){(void)timeout;CHECK(s==&semaphore&&!semaphore.locked);semaphore.locked=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t s){CHECK(s==&semaphore&&semaphore.locked);if(!busy&&running_job)CHECK(!running_job->credentials.password[0]);semaphore.locked=false;return pdTRUE;}
QueueHandle_t xQueueCreate(unsigned count,unsigned size){CHECK(count==1&&size==sizeof(lan_job_t));return &queued;}
int xQueueSend(QueueHandle_t q,const void *value,TickType_t timeout){CHECK(q==&queued&&timeout==0&&!queued_ready);queued=*(const lan_job_t *)value;queued_ready=true;return pdTRUE;}
int xQueueReceive(QueueHandle_t q,void *value,TickType_t timeout){CHECK(q==&queued&&timeout==portMAX_DELAY);if(!queued_ready){running_job=NULL;longjmp(task_exit,1);}*(lan_job_t *)value=queued;memset(&queued,0,sizeof(queued));queued_ready=false;running_job=value;return pdTRUE;}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned bytes,void *arg,unsigned priority,TaskHandle_t *out){(void)arg;(void)out;CHECK(fn==lan_task&&!strcmp(name,"dsr_lan")&&bytes==10240&&priority==3);return pdPASS;}
static void drain(void){if(!setjmp(task_exit))lan_task(NULL);}
void vTaskDelay(TickType_t ticks){CHECK(ticks==20);delays++;clock_us+=(int64_t)ticks*1000;CHECK(suspended&&!dsr_lan_scan());if(drain_on_delay){drain_on_delay=false;drain();CHECK(http_finished&&!busy);}}
int64_t esp_timer_get_time(void){return clock_us;}
bool dsr_wifi_connected(void){return home;}
bool dsr_ap_cancelled(void){return cancel_ap;}
esp_err_t mdns_init(void){return ESP_OK;}
esp_err_t mdns_query_ptr(const char *s,const char *p,unsigned t,unsigned l,mdns_result_t **out){(void)s;(void)p;(void)t;(void)l;*out=NULL;return ESP_OK;}
void mdns_query_results_free(mdns_result_t *r){(void)r;}
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *key){(void)key;return NULL;}
esp_err_t esp_netif_get_ip_info(esp_netif_t *n,esp_netif_ip_info_t *out){(void)n;(void)out;return ESP_FAIL;}
int etharp_get_entry(size_t i,ip4_addr_t **ip,struct netif **n,struct eth_addr **m){(void)i;(void)ip;(void)n;(void)m;return 0;}
char *ip4addr_ntoa_r(const ip4_addr_t *a,char *t,int length){(void)a;(void)length;t[0]=0;return t;}
err_t tcpip_api_call(err_t (*fn)(struct tcpip_api_call_data *),struct tcpip_api_call_data *arg){return fn(arg);}
char *dsr_workspace_snapshot(void){return NULL;}
dsr_result_t dsr_http_lan_identity(const char *host,dsr_lan_device_t *out){(void)host;(void)out;CHECK(home);return DSR_OFFLINE;}
dsr_result_t dsr_http_lan_inspect(const dsr_lan_device_t *device,const dsr_credentials_t *credentials,dsr_lan_capabilities_t *out)
{CHECK(home&&busy&&suspended&&!strcmp(device->id,"shelly-A")&&!strcmp(credentials->password,"private-existing-A"));http_reads++;*out=(dsr_lan_capabilities_t){.switches=1};http_finished=true;return DSR_OK;}
static void inspect(void)
{dsr_credentials_t auth={.username="admin",.password="private-existing-A"};CHECK(dsr_lan_inspect("192.168.1.10","shelly-A",&auth));CHECK(busy&&queued_ready);}
int main(void)
{
 CHECK(dsr_lan_suspend()==DSR_IO);CHECK(dsr_lan_init()==ESP_OK);state.count=1;state.devices[0]=(dsr_lan_device_t){.host="192.168.1.10",.id="shelly-A"};
 inspect();drain_on_delay=true;CHECK(dsr_lan_suspend()==DSR_OK&&http_reads==1&&delays==1&&http_finished&&!busy);home=false;CHECK(!dsr_lan_scan());dsr_credentials_t auth={0};CHECK(!dsr_lan_inspect("192.168.1.10","shelly-A",&auth));CHECK(!queued_ready);
 home=true;CHECK(!dsr_lan_scan());dsr_lan_resume();CHECK(!suspended);
 inspect();cancel_ap=true;CHECK(dsr_lan_suspend()==DSR_BUSY&&busy&&suspended);CHECK(!dsr_lan_scan());cancel_ap=false;drain();dsr_lan_resume();
 inspect();unsigned before=delays;CHECK(dsr_lan_suspend()==DSR_BUSY&&busy&&delays-before==600);CHECK(!dsr_lan_scan());drain();dsr_lan_resume();
 CHECK(dsr_lan_suspend()==DSR_OK);CHECK(!dsr_lan_scan());dsr_lan_resume();CHECK(dsr_lan_scan());drain();CHECK(!busy);
 printf("PASS: %u actual LAN barrier checks, active authenticated task drains before AP handoff, timeout/cancel fail closed\n",checks);
 return 0;
}
