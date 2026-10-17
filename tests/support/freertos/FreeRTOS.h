#ifndef TEST_FREERTOS_H
#define TEST_FREERTOS_H
#include <stdint.h>
#define pdTRUE 1
#define pdFALSE 0
#define portMAX_DELAY UINT32_MAX
#define pdMS_TO_TICKS(x) (x)
typedef uint32_t TickType_t;
typedef unsigned UBaseType_t;
#define tskIDLE_PRIORITY 0
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#endif
