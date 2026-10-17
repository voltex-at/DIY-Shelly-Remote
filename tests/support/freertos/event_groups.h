#ifndef TEST_EVENT_GROUPS_H
#define TEST_EVENT_GROUPS_H
#include "FreeRTOS.h"
typedef uint32_t EventBits_t;
typedef struct test_event_group *EventGroupHandle_t;
EventGroupHandle_t xEventGroupCreate(void);
EventBits_t xEventGroupSetBits(EventGroupHandle_t group,EventBits_t bits);
EventBits_t xEventGroupClearBits(EventGroupHandle_t group,EventBits_t bits);
EventBits_t xEventGroupGetBits(EventGroupHandle_t group);
EventBits_t xEventGroupWaitBits(EventGroupHandle_t group,EventBits_t bits,int clear,int all,TickType_t timeout);
#endif
