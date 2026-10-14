#include "dsr_board.h"
const char *dsr_board_hardware_id(void) { return "unconfigured"; }
esp_err_t dsr_board_init(void)
{
    /* Deliberately no LCD, touch, LED, power or GPIO operations. */
    return ESP_ERR_NOT_SUPPORTED;
}
