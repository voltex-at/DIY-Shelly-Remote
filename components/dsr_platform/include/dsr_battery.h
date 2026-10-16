#ifndef DSR_BATTERY_H
#define DSR_BATTERY_H
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
/* BAT-divider voltage, not proof of battery/USB/charge state. No NVS fields. */
typedef struct {
 bool has_raw,has_voltage;
 uint16_t raw,spread,adc_mv,bat_mv;
 esp_err_t calibration_error;
} dsr_battery_sample_t;
/* On-demand worker only; GPIO8, nominal 100k:100k. Releases ADC/calibration. */
esp_err_t dsr_battery_read(dsr_battery_sample_t *out);
#endif
