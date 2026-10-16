#pragma once
/* API shape only; these host enum values are not ESP reset-code evidence. */
typedef enum { ESP_RST_UNKNOWN, ESP_RST_TASK_WDT } esp_reset_reason_t;
esp_reset_reason_t esp_reset_reason(void);
