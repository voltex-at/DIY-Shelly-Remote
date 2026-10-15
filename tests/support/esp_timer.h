#ifndef TEST_ESP_TIMER_H
#define TEST_ESP_TIMER_H
#include "esp_err.h"
#include <stdint.h>
int64_t esp_timer_get_time(void);
typedef struct test_timer *esp_timer_handle_t;
typedef struct {void (*callback)(void *);const char *name;} esp_timer_create_args_t;
esp_err_t esp_timer_create(const esp_timer_create_args_t *args,esp_timer_handle_t *timer);
esp_err_t esp_timer_start_once(esp_timer_handle_t timer,uint64_t delay);
esp_err_t esp_timer_stop(esp_timer_handle_t timer);
#endif
