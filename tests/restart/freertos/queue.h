#ifndef DSR_TEST_RESTART_QUEUE_H
#define DSR_TEST_RESTART_QUEUE_H
#include "freertos/FreeRTOS.h"
typedef void *QueueHandle_t;
QueueHandle_t xQueueCreate(unsigned,unsigned);
int xQueueSend(QueueHandle_t,const void *,TickType_t);
int xQueueReceive(QueueHandle_t,void *,TickType_t);
#endif
