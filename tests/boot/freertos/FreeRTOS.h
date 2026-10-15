#pragma once
#include <stdint.h>
typedef uint32_t StackType_t;
typedef uint32_t TickType_t;
typedef struct { unsigned placeholder[32]; } StaticTask_t;
typedef void *TaskHandle_t;
#define pdMS_TO_TICKS(ms) (ms)
