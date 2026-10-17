#ifndef TEST_SEMAPHORE_H
#define TEST_SEMAPHORE_H
#include "FreeRTOS.h"
typedef struct test_semaphore *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateMutex(void);
int xSemaphoreTake(SemaphoreHandle_t semaphore,TickType_t timeout);
int xSemaphoreGive(SemaphoreHandle_t semaphore);
#endif
