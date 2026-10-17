#pragma once
#include "freertos/FreeRTOS.h"
typedef void *QueueHandle_t;
QueueHandle_t xQueueCreate(unsigned count,unsigned size);
int xQueueSend(QueueHandle_t queue,const void *value,TickType_t wait);
int xQueueReceive(QueueHandle_t queue,void *value,TickType_t wait);
void vQueueDelete(QueueHandle_t queue);
