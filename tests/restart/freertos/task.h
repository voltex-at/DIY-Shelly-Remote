#ifndef DSR_TEST_RESTART_TASK_H
#define DSR_TEST_RESTART_TASK_H
#include "freertos/FreeRTOS.h"
typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);
BaseType_t xTaskCreate(TaskFunction_t,const char *,uint32_t,void *,UBaseType_t,TaskHandle_t *);
BaseType_t xTaskNotifyGive(TaskHandle_t);
uint32_t ulTaskNotifyTake(BaseType_t,TickType_t);
void vTaskDelay(TickType_t);
void vTaskDelete(TaskHandle_t);
#endif
