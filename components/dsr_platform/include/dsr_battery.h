#ifndef DSR_BATTERY_H
#define DSR_BATTERY_H
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
typedef enum {DSR_BATTERY_NONE,DSR_BATTERY_LIPO_1S,DSR_BATTERY_LIION_1S} dsr_battery_kind_t;
/* Interpretation only: never programs the fixed on-board charger. The three
 * voltage anchors describe the user's cell under its normal operating load.
 * Capacity is metadata, not a current sensor or a run-time prediction. */
typedef struct {
 uint16_t nominal_mv,capacity_mah,empty_mv,mid_mv,full_mv,gain_permille;
 uint8_t kind,trend_enabled,reserved[2];
} dsr_battery_profile_t;
_Static_assert(sizeof(dsr_battery_profile_t)==16,"bounded battery profile");
void dsr_battery_profile_defaults(dsr_battery_profile_t *out);
bool dsr_battery_profile_valid(const dsr_battery_profile_t *profile);
bool dsr_battery_profile_equal(const dsr_battery_profile_t *a,const dsr_battery_profile_t *b);
/* A strict decimal mV/mAh field parser; no signs, spaces, tails or overflow. */
bool dsr_battery_parse_number(const char *text,uint16_t *out);
/* Small, separate versioned NVS record; missing means disabled. Boot read;
 * subsequent reads/writes belong to the sole worker. Save includes readback. */
esp_err_t dsr_battery_profile_load(dsr_battery_profile_t *out);
esp_err_t dsr_battery_profile_save(const dsr_battery_profile_t *profile);
/* BAT-divider voltage, not proof of battery/USB/charge state. No NVS fields. */
typedef struct {
 bool has_raw,has_voltage;
 uint16_t raw,spread,adc_mv,bat_mv;
 uint8_t percent;
 esp_err_t calibration_error;
 /* Raw GPIO8 never sets these. The worker may add an explicitly marked SOC
  * estimate. Only a verified gauge/charger may set has_charge_state. */
 bool has_percent,has_charge_state,charging,percent_estimated;
} dsr_battery_sample_t;
_Static_assert(sizeof(dsr_battery_sample_t)==20,"reuse volatile measurement padding");
/* Only a valid calibrated ADC + explicitly enabled profile can yield an
 * approximate SOC. Never invents battery presence or charging state. */
void dsr_battery_apply_profile(const dsr_battery_profile_t *profile,dsr_battery_sample_t *sample);
bool dsr_battery_reference_gain(const dsr_battery_sample_t *sample,uint16_t reference_mv,uint16_t *gain);
/* Voltage trend is an inference, never a confirmed charger capability. */
typedef struct {
 uint32_t started,last_at,last_rise;
 uint16_t base_mv,peak_mv;
 uint8_t rises;
 bool tracking,likely;
} dsr_battery_trend_t;
void dsr_battery_trend_reset(dsr_battery_trend_t *trend);
void dsr_battery_trend_update(dsr_battery_trend_t *trend,const dsr_battery_sample_t *sample,uint32_t now,bool enabled);
bool dsr_battery_trend_likely(const dsr_battery_trend_t *trend,uint32_t now);
/* On-demand worker only; GPIO8, nominal 100k:100k. Releases ADC/calibration. */
esp_err_t dsr_battery_read(dsr_battery_sample_t *out);
#endif
