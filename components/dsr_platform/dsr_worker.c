#include "dsr_platform.h"
#include "dsr_ap_private.h"
#include "dsr_activity.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <string.h>

static QueueHandle_t jobs, events;
static SemaphoreHandle_t available;
static void network_task(void *arg)
{
    (void)arg;
    dsr_job_t job;
    for (;;) {
        if (xQueueReceive(jobs, &job, portMAX_DELAY) != pdTRUE) continue;
        dsr_event_t event = {.id = job.id, .device = job.connection.device};
        if(job.kind>=DSR_JOB_AP_SCAN&&job.kind<=DSR_JOB_AP_VERIFY){
            dsr_ap_execute(&job,&event);
        } else if(job.kind==DSR_JOB_LAYOUT_PUBLISH||job.kind==DSR_JOB_UNBIND){
            event.result=(job.kind==DSR_JOB_UNBIND?dsr_connection_remove(&job.connection):dsr_connection_publish(&job.connection))==ESP_OK?DSR_OK:DSR_IO;event.layout_id=job.connection.layout_id;
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
        } else if (!dsr_wifi_connected()) {
            event.result = DSR_OFFLINE; event.device.online = false;
        } else if (job.kind == DSR_JOB_BIND) {
            event.result = dsr_http_probe_telemetry(&job.connection,&event.device,&event.telemetry);
            if (event.result == DSR_OK) {
                job.connection.device = event.device;
                if (dsr_connection_save(&job.connection) != ESP_OK||dsr_connection_layout_id(&job.connection,&event.layout_id)!=ESP_OK) event.result = DSR_IO;
            }
        } else {
            event.result = job.kind == DSR_JOB_PROBE ? dsr_http_refresh(&job.connection) :
                dsr_http_set_output(&job.connection, job.on);
            event.device = job.connection.device;
            event.telemetry=job.connection.telemetry;
            /* Transaction outcomes only: never host, identity or credentials.
             * SET result includes mandatory GetStatus readback; it is not a
             * claim that a timed-out write did or did not reach the relay. */
            if(job.kind==DSR_JOB_SET_OUTPUT)ESP_LOGI("dsr_network","Switch.Set/readback result=%d requested=%u confirmed=%u output=%u",(int)event.result,(unsigned)job.on,(unsigned)(event.result==DSR_OK&&event.device.online),(unsigned)event.device.output);
            else ESP_LOGI("dsr_network","Switch.GetStatus result=%d online=%u voltage_available=%u current_available=%u",(int)event.result,(unsigned)event.device.online,(unsigned)event.telemetry.has_voltage,(unsigned)event.telemetry.has_current);
        }
        /* No secrets in events or task-local memory between operations. */
        memset(&job, 0, sizeof(job));
        xQueueSend(events, &event, portMAX_DELAY);
    }
}

esp_err_t dsr_worker_start(void)
{
    if (jobs) return ESP_ERR_INVALID_STATE;
    jobs = xQueueCreate(1, sizeof(dsr_job_t));
    events = xQueueCreate(1, sizeof(dsr_event_t));
    available = xSemaphoreCreateBinary();
    if (!jobs || !events || !available) goto fail;
    xSemaphoreGive(available);
    if (xTaskCreate(network_task, "dsr_network", 10240, NULL, 4, NULL) != pdPASS) goto fail;
    return ESP_OK;
fail:
    if (jobs) vQueueDelete(jobs);
    if (events) vQueueDelete(events);
    if (available) vSemaphoreDelete(available);
    jobs = events = NULL; available = NULL;
    return ESP_ERR_NO_MEM;
}

dsr_result_t dsr_worker_submit(const dsr_job_t *job)
{
    if (!jobs || !job || job->kind < DSR_JOB_PROBE || job->kind > DSR_JOB_AP_VERIFY) return DSR_INVALID;
    if(dsr_wifi_temporary_active()&&(job->kind<DSR_JOB_AP_SCAN||job->kind>DSR_JOB_AP_VERIFY))return DSR_BUSY;
    if(job->kind>=DSR_JOB_AP_SCAN){
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
    if (xQueueSend(jobs, job, 0) != pdTRUE) { dsr_activity_end(); xSemaphoreGive(available); return DSR_BUSY; }
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
