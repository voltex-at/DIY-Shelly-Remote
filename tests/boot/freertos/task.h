#pragma once
#include "freertos/FreeRTOS.h"
TaskHandle_t xTaskCreateStatic(void (*entry)(void *), const char *name,
    uint32_t bytes, void *argument, unsigned priority,
    StackType_t *stack, StaticTask_t *tcb);
void vTaskDelay(TickType_t ticks);
UBaseType_t uxTaskPriorityGet(TaskHandle_t task);
void vTaskPrioritySet(TaskHandle_t task,UBaseType_t priority);
