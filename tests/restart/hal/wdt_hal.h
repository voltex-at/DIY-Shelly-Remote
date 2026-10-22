#ifndef DSR_TEST_RESTART_WDT_HAL_H
#define DSR_TEST_RESTART_WDT_HAL_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {unsigned marker;} wdt_hal_context_t;
typedef enum {WDT_RWDT} wdt_inst_t;
typedef enum {WDT_STAGE0,WDT_STAGE1} wdt_stage_t;
typedef enum {WDT_STAGE_ACTION_RESET_SYSTEM,WDT_STAGE_ACTION_RESET_RTC} wdt_stage_action_t;
void wdt_hal_init(wdt_hal_context_t *,wdt_inst_t,uint32_t,bool);
void wdt_hal_write_protect_disable(wdt_hal_context_t *);
void wdt_hal_config_stage(wdt_hal_context_t *,wdt_stage_t,uint32_t,wdt_stage_action_t);
void wdt_hal_set_flashboot_en(wdt_hal_context_t *,bool);
void wdt_hal_enable(wdt_hal_context_t *);
void wdt_hal_feed(wdt_hal_context_t *);
void wdt_hal_write_protect_enable(wdt_hal_context_t *);
#endif
