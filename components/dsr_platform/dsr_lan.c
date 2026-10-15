#include "dsr_discovery.h"
#include "dsr_workspace.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "mdns.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/etharp.h"
#include "lwip/ip4_addr.h"
#include "lwip/priv/tcpip_priv.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {bool scan;dsr_lan_device_t device;dsr_credentials_t credentials;} lan_job_t;
static QueueHandle_t queue;
static SemaphoreHandle_t mutex;
static dsr_lan_snapshot_t state;
static atomic_bool cancelled;
static bool busy;

void dsr_lan_snapshot(dsr_lan_snapshot_t *out)
{
    if(!out)return;
    if(!mutex){memset(out,0,sizeof(*out));out->phase=DSR_LAN_ERROR;out->result=DSR_IO;return;}
    xSemaphoreTake(mutex,portMAX_DELAY);*out=state;xSemaphoreGive(mutex);
}
void dsr_lan_cancel(void){atomic_store(&cancelled,true);}
static void finish(dsr_lan_phase_t phase,dsr_result_t result)
{xSemaphoreTake(mutex,portMAX_DELAY);state.phase=phase;state.result=result;state.serial++;busy=false;xSemaphoreGive(mutex);}
static bool local_network(char ip[16],char mask[16])
{
    esp_netif_t *netif=esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");esp_netif_ip_info_t info;
    if(!netif||!dsr_wifi_connected()||esp_netif_get_ip_info(netif,&info)!=ESP_OK)return false;
    snprintf(ip,16,IPSTR,IP2STR(&info.ip));snprintf(mask,16,IPSTR,IP2STR(&info.netmask));return true;
}
static void mdns_candidates(dsr_lan_plan_t *plan,const char *service,const char *ip,const char *mask)
{
    if(atomic_load(&cancelled))return;
    mdns_result_t *results=NULL;
    if(mdns_query_ptr(service,"_tcp",1200,24,&results)!=ESP_OK){if(results)mdns_query_results_free(results);return;}
    for(const mdns_result_t *r=results;r;r=r->next){
        if(r->port!=80)continue;
        for(const mdns_ip_addr_t *a=r->addr;a;a=a->next)if(a->addr.type==ESP_IPADDR_TYPE_V4){
            char host[16];snprintf(host,sizeof(host),IPSTR,IP2STR(&a->addr.u_addr.ip4));dsr_lan_plan_add(plan,host,ip,mask);
        }
    }
    mdns_query_results_free(results);
}
typedef struct {struct tcpip_api_call_data call;char hosts[16][16];unsigned count;} arp_call_t;
static err_t arp_read(struct tcpip_api_call_data *base)
{
    arp_call_t *call=(arp_call_t *)base;
    for(size_t i=0;i<ARP_TABLE_SIZE&&call->count<16;i++){
        ip4_addr_t *ip;struct netif *netif;struct eth_addr *mac;
        if(etharp_get_entry(i,&ip,&netif,&mac)&&netif&&mac)ip4addr_ntoa_r(ip,call->hosts[call->count++],16);
    }
    return ERR_OK;
}
static void fallback_candidates(dsr_lan_plan_t *plan,const char *ip,const char *mask)
{
    /* Passive ARP neighbours, previous verified locations and configured hosts.
     * No subnet sweep, broadcast command, pairing or credential probing. */
    arp_call_t arp={0};if(tcpip_api_call(arp_read,&arp.call)==ERR_OK)for(unsigned i=0;i<arp.count;i++)dsr_lan_plan_add(plan,arp.hosts[i],ip,mask);
    char *configuration=dsr_workspace_snapshot();
    if(configuration){cJSON *root=cJSON_Parse(configuration);free(configuration);const cJSON *devices=cJSON_GetObjectItemCaseSensitive(root,"devices"),*d;
        cJSON_ArrayForEach(d,devices){const cJSON *host=cJSON_GetObjectItemCaseSensitive(d,"host");if(cJSON_IsString(host))dsr_lan_plan_add(plan,host->valuestring,ip,mask);}cJSON_Delete(root);
    }
}
static void scan_network(void)
{
    char ip[16],mask[16];if(!local_network(ip,mask)){finish(DSR_LAN_NO_WIFI,DSR_OFFLINE);return;}
    dsr_lan_plan_t plan={0},previous={0};
    xSemaphoreTake(mutex,portMAX_DELAY);
    for(unsigned i=0;i<state.count;i++)dsr_lan_plan_add(&previous,state.devices[i].host,ip,mask);
    state.count=state.probed=state.total=state.skipped=0;memset(state.devices,0,sizeof(state.devices));xSemaphoreGive(mutex);
    int64_t deadline=esp_timer_get_time()+18000000;
    mdns_candidates(&plan,"_shelly",ip,mask);mdns_candidates(&plan,"_http",ip,mask);
    for(unsigned i=0;i<previous.count;i++)dsr_lan_plan_add(&plan,previous.hosts[i],ip,mask);
    fallback_candidates(&plan,ip,mask);
    xSemaphoreTake(mutex,portMAX_DELAY);state.total=plan.count;state.serial++;xSemaphoreGive(mutex);
    for(unsigned i=0;i<plan.count&&esp_timer_get_time()<deadline;i++){
        if(atomic_load(&cancelled))break;
        if(!dsr_wifi_connected()){finish(DSR_LAN_NO_WIFI,DSR_OFFLINE);return;}
        dsr_lan_device_t device={0};dsr_result_t result=dsr_http_lan_identity(plan.hosts[i],&device);
        xSemaphoreTake(mutex,portMAX_DELAY);state.probed++;
        if(result==DSR_OK){if(dsr_lan_upsert(&state,&device)!=DSR_OK)state.skipped++;}else state.skipped++;
        state.serial++;xSemaphoreGive(mutex);
    }
    finish(atomic_load(&cancelled)?DSR_LAN_CANCELLED:DSR_LAN_DONE,DSR_OK);
}
static void lan_task(void *argument)
{
    (void)argument;lan_job_t job;
    for(;;){
        if(xQueueReceive(queue,&job,portMAX_DELAY)!=pdTRUE)continue;
        if(job.scan)scan_network();
        else{
            dsr_lan_capabilities_t caps={0};dsr_result_t result=dsr_http_lan_inspect(&job.device,&job.credentials,&caps);
            memset(&job.credentials,0,sizeof(job.credentials));
            if(atomic_load(&cancelled))finish(DSR_LAN_CANCELLED,DSR_OK);
            else{xSemaphoreTake(mutex,portMAX_DELAY);state.capabilities=caps;xSemaphoreGive(mutex);finish(result==DSR_OK?DSR_LAN_INSPECTED:DSR_LAN_ERROR,result);}
        }
        memset(&job,0,sizeof(job));
    }
}
esp_err_t dsr_lan_init(void)
{
    if(mutex)return ESP_ERR_INVALID_STATE;
    esp_err_t mdns=mdns_init();if(mdns!=ESP_OK&&mdns!=ESP_ERR_INVALID_STATE)return mdns;
    mutex=xSemaphoreCreateMutex();queue=xQueueCreate(1,sizeof(lan_job_t));if(!mutex||!queue)return ESP_ERR_NO_MEM;
    return xTaskCreate(lan_task,"dsr_lan",10240,NULL,3,NULL)==pdPASS?ESP_OK:ESP_ERR_NO_MEM;
}
static bool enqueue(const lan_job_t *job)
{
    if(!mutex||!queue)return false;
    xSemaphoreTake(mutex,portMAX_DELAY);
    bool ok=!busy;
    if(ok){atomic_store(&cancelled,false);ok=xQueueSend(queue,job,0)==pdTRUE;
        if(ok){busy=true;state.phase=job->scan?DSR_LAN_SCANNING:DSR_LAN_INSPECTING;state.result=DSR_OK;state.capabilities=(dsr_lan_capabilities_t){0};state.selected=job->device;state.serial++;}}
    xSemaphoreGive(mutex);return ok;
}
bool dsr_lan_scan(void)
{
    if(!dsr_wifi_connected()){
        if(mutex){xSemaphoreTake(mutex,portMAX_DELAY);if(!busy){state.phase=DSR_LAN_NO_WIFI;state.result=DSR_OFFLINE;state.serial++;}xSemaphoreGive(mutex);}return false;
    }
    lan_job_t job={.scan=true};return enqueue(&job);
}
bool dsr_lan_inspect(const char *host,const char *id,const dsr_credentials_t *credentials)
{
    if(!mutex||!host||!id||!credentials||!memchr(credentials->username,0,sizeof(credentials->username))||!memchr(credentials->password,0,sizeof(credentials->password)))return false;
    lan_job_t job={0};bool found=false;xSemaphoreTake(mutex,portMAX_DELAY);
    for(unsigned i=0;i<state.count;i++)if(!strcmp(state.devices[i].host,host)&&!strcmp(state.devices[i].id,id)){job.device=state.devices[i];found=true;break;}
    xSemaphoreGive(mutex);if(!found||!dsr_wifi_connected())return false;
    job.credentials=*credentials;bool ok=enqueue(&job);memset(&job,0,sizeof(job));return ok;
}
