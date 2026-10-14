#include "dsr_platform.h"
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
        dsr_event_t event = {.id = job.id, .device = job.device};
        if (!dsr_wifi_connected()) { event.result = DSR_OFFLINE; event.device.online = false; }
        else if (job.kind == DSR_JOB_PROBE) event.result = dsr_http_probe(job.device.host, job.device.channel, &event.device);
        else if (job.kind == DSR_JOB_REFRESH) event.result = dsr_http_refresh(&event.device);
        else event.result = dsr_http_set_output(&event.device, job.on);
        /* There can be only one in-flight job, so this queue cannot be full. */
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
    if (xTaskCreate(network_task, "dsr_network", 8192, NULL, 4, NULL) != pdPASS) goto fail;
    return ESP_OK;
fail:
    if (jobs) vQueueDelete(jobs);
    if (events) vQueueDelete(events);
    if (available) vSemaphoreDelete(available);
    jobs = events = NULL;
    available = NULL;
    return ESP_ERR_NO_MEM;
}

dsr_result_t dsr_worker_submit(const dsr_job_t *job)
{
    if (!jobs || !job || (job->kind != DSR_JOB_PROBE && job->kind != DSR_JOB_REFRESH && job->kind != DSR_JOB_SET_OUTPUT) ||
        !memchr(job->device.host, '\0', sizeof(job->device.host)) ||
        !dsr_ipv4_valid(job->device.host) || job->device.channel > DSR_MAX_CHANNEL) return DSR_INVALID;
    if (xSemaphoreTake(available, 0) != pdTRUE) return DSR_BUSY;
    if (xQueueSend(jobs, job, 0) != pdTRUE) { xSemaphoreGive(available); return DSR_BUSY; }
    return DSR_OK;
}

bool dsr_worker_receive(dsr_event_t *event)
{
    if (!events || !event || xQueueReceive(events, event, 0) != pdTRUE) return false;
    xSemaphoreGive(available);
    return true;
}
