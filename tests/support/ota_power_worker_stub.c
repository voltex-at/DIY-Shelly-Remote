#include "dsr_ota_power_internal.h"
/* These actor fixtures do not request ADC/OTA work. Power handoff itself has
 * its separate production test; fail closed if a request is ever introduced. */
esp_err_t dsr_ota_power_init(SemaphoreHandle_t slot){return slot?ESP_OK:ESP_ERR_INVALID_ARG;}
void dsr_ota_power_deinit(void){}
bool dsr_ota_power_poll(void){return false;}
