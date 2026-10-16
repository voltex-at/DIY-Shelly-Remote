#pragma once
#include "freertos/FreeRTOS.h"
#define pdPASS 1
typedef void *TaskHandle_t;
int xTaskCreate(void (*fn)(void *),const char *name,unsigned bytes,void *arg,unsigned priority,TaskHandle_t *out);
void vTaskDelete(TaskHandle_t task);
