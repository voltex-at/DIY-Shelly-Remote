#pragma once
#include <stdint.h>
typedef uint32_t StackType_t;
typedef uint32_t TickType_t;
typedef unsigned UBaseType_t;
#define tskIDLE_PRIORITY 0
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
typedef struct { unsigned placeholder[32]; } StaticTask_t;
typedef void *TaskHandle_t;
#define pdMS_TO_TICKS(ms) (ms)
