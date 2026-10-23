#include "dsr_ota_power.h"
#include "dsr_battery.h"

dsr_ota_power_result_t dsr_ota_power_read(void)
{
 dsr_battery_sample_t sample={0};
 if(dsr_battery_read(&sample)!=ESP_OK)return DSR_OTA_POWER_UNKNOWN;
 dsr_battery_profile_t profile;
 if(dsr_battery_profile_load(&profile)==ESP_OK)dsr_battery_apply_profile(&profile,&sample);
 if(!sample.has_percent||sample.percent>100)return DSR_OTA_POWER_UNKNOWN;
 return sample.percent<DSR_OTA_MIN_BATTERY_PERCENT?DSR_OTA_POWER_LOW:DSR_OTA_POWER_READY;
}
