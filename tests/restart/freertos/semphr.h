#ifndef DSR_TEST_RESTART_SEMPHR_H
#define DSR_TEST_RESTART_SEMPHR_H
#include "freertos/FreeRTOS.h"
typedef void *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateMutex(void);
int xSemaphoreTake(SemaphoreHandle_t,TickType_t);
int xSemaphoreGive(SemaphoreHandle_t);
#endif
