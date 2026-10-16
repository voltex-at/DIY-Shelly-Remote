#include "dsr_platform.h"
#include "dsr_ap_private.h"
#include "dsr_activity.h"
#include "dsr_history.h"
#ifdef DSR_BOARD_ES3C35P
#include "dsr_native_ota.h"
#endif
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <string.h>

static QueueHandle_t jobs, events;
static SemaphoreHandle_t available;
static SemaphoreHandle_t stop_mutex;
static struct {uint8_t phase;dsr_connection_t connection;dsr_stop_result_t result;} urgent_stop;
_Static_assert(sizeof(urgent_stop)<=640,"one bounded urgent STOP lane");
static void erase(void *p,size_t n){volatile unsigned char *v=p;while(n--)*v++=0;}
/* The sole outer slot registers admission independently of UI/job IDs. A STOP
 * supersedes an already accepted movement even if it runs before queue take. */
static struct {dsr_device_t device;uint32_t layout_id;bool active,superseded;} registered_cover;
_Static_assert(sizeof(registered_cover)<=160,"secret-free Cover admission guard");
static bool same_endpoint(const dsr_device_t *a,uint32_t layout,const dsr_connection_t *b)
{return layout==b->layout_id&&a->generation==b->device.generation&&a->endpoint_kind==b->device.endpoint_kind&&a->channel==b->device.channel&&!strcmp(a->device_id,b->device.device_id)&&!strcmp(a->host,b->device.host);}
static bool cover_superseded(const dsr_connection_t *c)
{
 xSemaphoreTake(stop_mutex,portMAX_DELAY);
 bool blocked=!registered_cover.active||registered_cover.superseded||!same_endpoint(&registered_cover.device,registered_cover.layout_id,c);
 xSemaphoreGive(stop_mutex);return blocked;
}
dsr_result_t dsr_worker_stop_request(const dsr_connection_t *c,uint32_t operation_id)
{
 if(!stop_mutex||!c||!operation_id||!dsr_connection_valid(c,true)||c->device.endpoint_kind!=DSR_ENDPOINT_COVER)return DSR_INVALID;
 xSemaphoreTake(stop_mutex,portMAX_DELAY);dsr_result_t result=DSR_BUSY;
 if(!urgent_stop.phase&&dsr_activity_begin()){urgent_stop.connection=*c;urgent_stop.result=(dsr_stop_result_t){.operation_id=operation_id,.layout_id=c->layout_id};urgent_stop.phase=1;if(registered_cover.active&&same_endpoint(&registered_cover.device,registered_cover.layout_id,c))registered_cover.superseded=true;result=DSR_OK;}
 xSemaphoreGive(stop_mutex);return result;
}
bool dsr_worker_stop_pending(void)
{if(!stop_mutex)return false;xSemaphoreTake(stop_mutex,portMAX_DELAY);bool pending=urgent_stop.phase!=0;xSemaphoreGive(stop_mutex);return pending;}
bool dsr_worker_stop_receive(dsr_stop_result_t *result,bool outer_pending)
{
 if(!stop_mutex||!result||outer_pending)return false;
 xSemaphoreTake(stop_mutex,portMAX_DELAY);bool ready=urgent_stop.phase==3;
 if(ready){*result=urgent_stop.result;erase(&urgent_stop,sizeof(urgent_stop));}
 xSemaphoreGive(stop_mutex);if(ready)dsr_activity_end();return ready;
}
static bool run_stop(void)
{
 xSemaphoreTake(stop_mutex,portMAX_DELAY);bool pending=urgent_stop.phase==1;if(pending)urgent_stop.phase=2;xSemaphoreGive(stop_mutex);
 if(!pending)return false;
 /* RUNNING prevents replacement or consume. No mutex spans SDK I/O, and
  * the inner transaction cannot recursively drain its own urgent lane. */
 dsr_http_urgent_hook(NULL);
 dsr_result_t result=!dsr_workspace_mapping_matches(&urgent_stop.connection)?DSR_IDENTITY_MISMATCH:dsr_wifi_connected()&&!dsr_wifi_temporary_active()?dsr_http_cover(&urgent_stop.connection,(dsr_cover_intent_t){.command=DSR_COVER_STOP}):DSR_OFFLINE;
 dsr_http_urgent_hook(run_stop);
 xSemaphoreTake(stop_mutex,portMAX_DELAY);
 if(result!=DSR_OK)urgent_stop.connection.device.online=false;
 urgent_stop.result.result=result;urgent_stop.result.device=urgent_stop.connection.device;urgent_stop.result.telemetry=urgent_stop.connection.telemetry;
 erase(&urgent_stop.connection,sizeof(urgent_stop.connection));urgent_stop.phase=3;xSemaphoreGive(stop_mutex);return true;
}
static void network_task(void *arg)
{
    (void)arg;
    dsr_job_t job;
    for (;;) {
        (void)run_stop();
        if (xQueueReceive(jobs, &job, pdMS_TO_TICKS(50)) != pdTRUE) continue;
        dsr_event_t event = {.id = job.id, .device = job.connection.device};
        if(job.kind==DSR_JOB_HISTORY_SERVICE||job.kind==DSR_JOB_HISTORY_CLOCK){
#ifdef DSR_BOARD_ES3C35P
            if(job.kind==DSR_JOB_HISTORY_SERVICE)dsr_history_service();else dsr_history_clock_execute();
            event.result=DSR_OK;
#else
            event.result=DSR_UNSUPPORTED;
#endif
        } else if(job.kind==DSR_JOB_OTA_CHECK||job.kind==DSR_JOB_OTA_INSTALL){
#ifdef DSR_BOARD_ES3C35P
            dsr_native_ota_execute(&job,&event);
#else
            event.result=DSR_UNSUPPORTED;
#endif
        } else if((job.kind>=DSR_JOB_AP_SCAN&&job.kind<=DSR_JOB_AP_VERIFY)||job.kind==DSR_JOB_LAN_STA_VERIFY){
            dsr_ap_execute(&job,&event);
        } else if(job.kind==DSR_JOB_LAYOUT_PUBLISH||job.kind==DSR_JOB_UNBIND){
            if(job.kind==DSR_JOB_UNBIND)event.result=dsr_connection_remove(&job.connection)==ESP_OK?DSR_OK:DSR_IO;
            else if(!dsr_workspace_mapping_matches(&job.connection))event.result=DSR_CONFLICT;
            else event.result=dsr_connection_save(&job.connection)==ESP_OK?DSR_OK:DSR_IO;
            event.layout_id=job.connection.layout_id;event.telemetry=job.connection.telemetry;
        } else if(job.kind==DSR_JOB_DISPLAY_SAVE){
            /* Read the latest full NVS record inside the sole worker. Never
             * replay UI-cached credentials over a newer Wi-Fi connection. */
            dsr_settings_t current;
            esp_err_t error=dsr_settings_load(&current);
            if(error==ESP_OK){
                current.brightness=job.settings.brightness;
                current.idle_seconds=job.settings.idle_seconds;
                error=dsr_settings_save(&current);
            }
            event.result=error==ESP_OK?DSR_OK:DSR_IO;
            event.brightness=job.settings.brightness;
            event.idle_seconds=job.settings.idle_seconds;
            memset(&current,0,sizeof(current));
        } else if(job.kind==DSR_JOB_BATTERY_READ){
#ifdef DSR_BOARD_ES3C35P
            event.result=dsr_battery_read(&event.battery)==ESP_OK?DSR_OK:DSR_IO;
#else
            event.result=DSR_UNSUPPORTED;
#endif
        } else if (job.kind == DSR_JOB_WIFI_SCAN) {
            event.result = dsr_wifi_scan(&event.scan);
        } else if (job.kind == DSR_JOB_WIFI_CONNECT) {
            event.result = dsr_wifi_connect_saved(&job.settings, &event.wifi_detail);
        } else if (job.kind==DSR_JOB_COVER&&cover_superseded(&job.connection)) {
            event.result=DSR_BUSY; /* Ordinary correlated event; STOP owns its result. */
        } else if (!dsr_wifi_connected()) {
            event.result = DSR_OFFLINE; event.device.online = false;
        } else if (job.kind == DSR_JOB_BIND) {
            event.result = dsr_http_probe_telemetry(&job.connection,&event.device,&event.telemetry);
            /* Verification is RAM-only. The full private record is written by
             * LAYOUT_PUBLISH only after exact durable workspace ACK. */
        } else {
            event.result = job.kind == DSR_JOB_PROBE ? dsr_http_refresh(&job.connection) :job.kind==DSR_JOB_COVER?dsr_http_cover(&job.connection,job.cover):dsr_http_set_output(&job.connection, job.on);
            event.device = job.connection.device;
            event.telemetry=job.connection.telemetry;
            /* Transaction outcomes only: never host, identity or credentials.
             * SET result includes mandatory GetStatus readback; it is not a
             * claim that a timed-out write did or did not reach the relay. */
            if(event.device.endpoint_kind==DSR_ENDPOINT_COVER)ESP_LOGI("dsr_network","Cover readback result=%d state=%u position_available=%u position=%u",(int)event.result,(unsigned)event.telemetry.cover.state,(unsigned)((event.telemetry.cover.flags&DSR_COVER_POSITION)!=0),(unsigned)event.telemetry.cover.current_pos);
            else if(job.kind==DSR_JOB_SET_OUTPUT)ESP_LOGI("dsr_network","Switch.Set/readback result=%d requested=%u confirmed=%u output=%u",(int)event.result,(unsigned)job.on,(unsigned)(event.result==DSR_OK&&event.device.online),(unsigned)event.device.output);
            else ESP_LOGI("dsr_network","Switch.GetStatus result=%d online=%u voltage_available=%u current_available=%u",(int)event.result,(unsigned)event.device.online,(unsigned)event.telemetry.has_voltage,(unsigned)event.telemetry.has_current);
        }
        /* No secrets in events or task-local memory between operations. */
        if(job.kind==DSR_JOB_COVER){xSemaphoreTake(stop_mutex,portMAX_DELAY);erase(&registered_cover,sizeof(registered_cover));xSemaphoreGive(stop_mutex);}
        erase(&job, sizeof(job));
        (void)run_stop();
        xQueueSend(events, &event, portMAX_DELAY);
    }
}

esp_err_t dsr_worker_start(void)
{
    if (jobs) return ESP_ERR_INVALID_STATE;
    jobs = xQueueCreate(1, sizeof(dsr_job_t));
    events = xQueueCreate(1, sizeof(dsr_event_t));
    available = xSemaphoreCreateBinary();
    stop_mutex=xSemaphoreCreateMutex();
    if (!jobs || !events || !available||!stop_mutex) goto fail;
    dsr_http_urgent_hook(run_stop);
    xSemaphoreGive(available);
#ifdef DSR_BOARD_ES3C35P
    (void)dsr_native_ota_init();
#endif
    if (xTaskCreate(network_task, "dsr_network", 10240, NULL, 4, NULL) != pdPASS) goto fail;
    return ESP_OK;
fail:
    if (jobs) vQueueDelete(jobs);
    if (events) vQueueDelete(events);
    if (available) vSemaphoreDelete(available);
    if(stop_mutex)vSemaphoreDelete(stop_mutex);
    stop_mutex=NULL;dsr_http_urgent_hook(NULL);
    jobs = events = NULL; available = NULL;
    return ESP_ERR_NO_MEM;
}

dsr_result_t dsr_worker_submit(const dsr_job_t *job)
{
    if (!jobs || !job || job->kind < DSR_JOB_PROBE || job->kind > DSR_JOB_COVER) return DSR_INVALID;
    if(job->kind==DSR_JOB_COVER&&(!dsr_cover_intent_valid(job->cover)||job->connection.device.endpoint_kind!=DSR_ENDPOINT_COVER||job->cover.command==DSR_COVER_STOP))return DSR_INVALID;
    if(dsr_wifi_temporary_active()&&(job->kind<DSR_JOB_AP_SCAN||job->kind>DSR_JOB_AP_VERIFY))return DSR_BUSY;
    if(job->kind==DSR_JOB_HISTORY_SERVICE||job->kind==DSR_JOB_HISTORY_CLOCK){
#ifndef DSR_BOARD_ES3C35P
        return DSR_UNSUPPORTED;
#endif
    } else if(job->kind==DSR_JOB_OTA_CHECK||job->kind==DSR_JOB_OTA_INSTALL){
#ifdef DSR_BOARD_ES3C35P
        if(!dsr_native_ota_admitted(job->id,job->kind))return DSR_INVALID;
#else
        return DSR_UNSUPPORTED;
#endif
    } else if(job->kind==DSR_JOB_LAN_STA_VERIFY){
        if(!dsr_station_identity_valid(&job->ap_identity)||job->ap_identity.auth_required||!dsr_ipv4_valid(job->connection.device.host)||!dsr_wifi_settings_valid(&job->settings))return DSR_INVALID;
    }else if(job->kind>=DSR_JOB_AP_SCAN&&job->kind<=DSR_JOB_AP_VERIFY){
        if(job->kind==DSR_JOB_AP_INSPECT&&(!job->ap_confirmed||!memchr(job->ap.ssid,0,sizeof(job->ap.ssid))||!dsr_ap_candidate(job->ap.ssid)||job->ap.security!=DSR_WIFI_OPEN))return DSR_INVALID;
        if((job->kind==DSR_JOB_AP_PROVISION||job->kind==DSR_JOB_AP_VERIFY)&&(!memchr(job->ap_identity.ssid,0,sizeof(job->ap_identity.ssid))||!dsr_ap_identity_matches(job->ap_identity.ssid,&job->ap_identity)||!dsr_wifi_settings_valid(&job->settings)))return DSR_INVALID;
        if(job->kind==DSR_JOB_AP_PROVISION&&!job->ap_confirmed)return DSR_INVALID;
        if(job->kind==DSR_JOB_AP_VERIFY&&!dsr_ipv4_valid(job->connection.device.host))return DSR_INVALID;
    } else if(job->kind==DSR_JOB_DISPLAY_SAVE){
        if(job->settings.brightness<10||job->settings.brightness>100||job->settings.idle_seconds>3600)return DSR_INVALID;
    } else if (job->kind == DSR_JOB_WIFI_CONNECT) {
        if (!dsr_wifi_settings_valid(&job->settings)) return DSR_INVALID;
    } else if (job->kind != DSR_JOB_WIFI_SCAN && job->kind!=DSR_JOB_BATTERY_READ && !dsr_connection_valid(&job->connection, job->kind != DSR_JOB_BIND)) return DSR_INVALID;
    if (xSemaphoreTake(available, 0) != pdTRUE) return DSR_BUSY;
    if (!dsr_activity_begin()) { xSemaphoreGive(available); return DSR_BUSY; }
    if (job->kind == DSR_JOB_WIFI_CONNECT||job->kind==DSR_JOB_AP_INSPECT) dsr_wifi_prepare_connect();
    if(job->kind==DSR_JOB_AP_SCAN||job->kind==DSR_JOB_AP_INSPECT)dsr_ap_prepare();
    /* Registration and enqueue share the STOP mutex. No SDK I/O occurs here. */
    if(job->kind==DSR_JOB_COVER){
        xSemaphoreTake(stop_mutex,portMAX_DELAY);
        bool stopped=urgent_stop.phase&&same_endpoint(&urgent_stop.result.device,urgent_stop.result.layout_id,&job->connection);
        if(urgent_stop.phase<3&&urgent_stop.phase)stopped=same_endpoint(&urgent_stop.connection.device,urgent_stop.connection.layout_id,&job->connection);
        if(stopped){xSemaphoreGive(stop_mutex);dsr_activity_end();xSemaphoreGive(available);return DSR_BUSY;}
        registered_cover.device=job->connection.device;registered_cover.layout_id=job->connection.layout_id;registered_cover.active=true;registered_cover.superseded=false;
        bool queued=xQueueSend(jobs,job,0)==pdTRUE;
        if(!queued)erase(&registered_cover,sizeof(registered_cover));
        xSemaphoreGive(stop_mutex);
        if(!queued){dsr_activity_end();xSemaphoreGive(available);return DSR_BUSY;}
    }else if (xQueueSend(jobs, job, 0) != pdTRUE) { dsr_activity_end(); xSemaphoreGive(available); return DSR_BUSY; }
    return DSR_OK;
}

bool dsr_worker_receive(dsr_event_t *event)
{
    if (!events || !event || xQueueReceive(events, event, 0) != pdTRUE) return false;
    xSemaphoreGive(available);
    /* Event handoff must be covered by the consumer's outer activity until
     * all persistent follow-ups are admitted (the production UI poll does so). */
    dsr_activity_end();
    return true;
}
