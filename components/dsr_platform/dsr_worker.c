#include "dsr_platform.h"
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
        if (job.kind == DSR_JOB_WIFI_SCAN) {
            event.result = dsr_wifi_scan(&event.scan);
        } else if (job.kind == DSR_JOB_WIFI_CONNECT) {
            event.result = dsr_wifi_connect_saved(&job.settings, &event.wifi_detail);
        } else if (!dsr_wifi_connected()) {
            event.result = DSR_OFFLINE; event.device.online = false;
        } else if (job.kind == DSR_JOB_BIND) {
            event.result = dsr_http_probe_telemetry(&job.connection,&event.device,&event.telemetry);
            if (event.result == DSR_OK) {
                job.connection.device = event.device;
                if (dsr_connection_save(&job.connection) != ESP_OK) event.result = DSR_IO;
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
    if (!jobs || !job || job->kind < DSR_JOB_PROBE || job->kind > DSR_JOB_WIFI_CONNECT) return DSR_INVALID;
    if (job->kind == DSR_JOB_WIFI_CONNECT) {
        if (!dsr_wifi_settings_valid(&job->settings)) return DSR_INVALID;
    } else if (job->kind != DSR_JOB_WIFI_SCAN && !dsr_connection_valid(&job->connection, job->kind != DSR_JOB_BIND)) return DSR_INVALID;
    if (xSemaphoreTake(available, 0) != pdTRUE) return DSR_BUSY;
    if (job->kind == DSR_JOB_WIFI_CONNECT) dsr_wifi_prepare_connect();
    if (xQueueSend(jobs, job, 0) != pdTRUE) { xSemaphoreGive(available); return DSR_BUSY; }
    return DSR_OK;
}

bool dsr_worker_receive(dsr_event_t *event)
{
    if (!events || !event || xQueueReceive(events, event, 0) != pdTRUE) return false;
    xSemaphoreGive(available);
    return true;
}
