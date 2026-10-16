#include "dsr_battery.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <string.h>

esp_err_t dsr_battery_read(dsr_battery_sample_t *out)
{
 if(!out)return ESP_ERR_INVALID_ARG;
 memset(out,0,sizeof(*out));
 adc_unit_t unit;adc_channel_t channel;
 esp_err_t error=adc_oneshot_io_to_channel(8,&unit,&channel);
 if(error!=ESP_OK)return error;
 if(unit!=ADC_UNIT_1)return ESP_ERR_NOT_SUPPORTED; /* Never use Wi-Fi ADC2. */
 adc_oneshot_unit_handle_t adc=NULL;adc_cali_handle_t calibration=NULL;
 const adc_oneshot_unit_init_cfg_t init={.unit_id=unit,.ulp_mode=ADC_ULP_MODE_DISABLE};
 error=adc_oneshot_new_unit(&init,&adc);if(error!=ESP_OK)return error;
 const adc_oneshot_chan_cfg_t config={.atten=ADC_ATTEN_DB_12,.bitwidth=ADC_BITWIDTH_12};
 error=adc_oneshot_config_channel(adc,channel,&config);if(error!=ESP_OK)goto done;
 const adc_cali_curve_fitting_config_t cal={.unit_id=unit,.chan=channel,.atten=ADC_ATTEN_DB_12,.bitwidth=ADC_BITWIDTH_12};
 out->calibration_error=adc_cali_create_scheme_curve_fitting(&cal,&calibration);
 /* Discard first conversion, then median-filter nine real conversions.
  * Failed samples are not zero or a recycled previous voltage. */
 int raw,values[9];error=adc_oneshot_read(adc,channel,&raw);if(error!=ESP_OK)goto done;
 for(unsigned i=0;i<9;i++){
  error=adc_oneshot_read(adc,channel,&raw);if(error!=ESP_OK)goto done;
  if(raw<0||raw>4095){error=ESP_ERR_INVALID_SIZE;goto done;}values[i]=raw;
  for(unsigned j=i;j&&values[j]<values[j-1];j--){int v=values[j];values[j]=values[j-1];values[j-1]=v;}
 }
 out->has_raw=true;out->raw=(uint16_t)values[4];out->spread=(uint16_t)(values[8]-values[0]);
 /* Ground/saturation does not establish an absent cell or a valid voltage. */
 if(values[4]==0||values[4]==4095){error=ESP_ERR_INVALID_SIZE;goto done;}
 if(out->calibration_error!=ESP_OK)goto done; /* Raw-only, never guessed mV. */
 int millivolts;error=adc_cali_raw_to_voltage(calibration,values[4],&millivolts);
 if(error!=ESP_OK)goto done;
 if(millivolts<=0||millivolts>3300){error=ESP_ERR_INVALID_SIZE;goto done;}
 out->adc_mv=(uint16_t)millivolts;out->bat_mv=(uint16_t)(millivolts*2);out->has_voltage=true;
done:
 if(calibration){esp_err_t cleanup=adc_cali_delete_scheme_curve_fitting(calibration);if(error==ESP_OK)error=cleanup;}
 esp_err_t cleanup=adc_oneshot_del_unit(adc);if(error==ESP_OK)error=cleanup;
 if(error!=ESP_OK)out->has_voltage=false;
 return error;
}
