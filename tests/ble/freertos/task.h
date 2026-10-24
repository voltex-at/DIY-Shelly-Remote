#pragma once
#include "freertos/FreeRTOS.h"
#define pdPASS 1
typedef void *TaskHandle_t;
TaskHandle_t xTaskGetCurrentTaskHandle(void);
int xTaskCreatePinnedToCore(void (*fn)(void *),const char *name,unsigned stack,void *arg,unsigned priority,void *handle,int core);
void vTaskDelay(TickType_t ticks);
