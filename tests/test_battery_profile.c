/* Genuine profile, integer estimator and NVS adapter; explicit Flash faults. */
#include "dsr_battery.h"
#include "dsr_activity.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "nvs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,opens,sets,commits,closes;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static uint8_t flash[64],staged[64];static size_t length,staged_length;
static bool exists,have_staged;
static unsigned fault;
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h)
{CHECK(!strcmp(name,"dsr_battery")&&(mode==NVS_READONLY||mode==NVS_READWRITE));opens++;if(fault==1)return ESP_FAIL;if(!exists&&mode==NVS_READONLY)return ESP_ERR_NVS_NOT_FOUND;*h=mode+1;return ESP_OK;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size)
{CHECK(h==1&&!strcmp(key,"profile"));if(fault==4)return ESP_FAIL;if(!exists)return ESP_ERR_NVS_NOT_FOUND;if(*size<length)return ESP_ERR_INVALID_SIZE;memcpy(out,flash,length);*size=length;if(fault==5&&length)((uint8_t *)out)[10]^=1;return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size)
{CHECK(h==2&&!strcmp(key,"profile")&&size==24&&!dsr_activity_restart_reserve());sets++;if(fault==2)return ESP_FAIL;memcpy(staged,data,size);staged_length=size;have_staged=true;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h)
{CHECK(h==2&&have_staged&&!dsr_activity_restart_reserve());commits++;if(fault==3)return ESP_FAIL;memcpy(flash,staged,staged_length);length=staged_length;exists=true;have_staged=false;return ESP_OK;}
void nvs_close(nvs_handle_t h){CHECK(h==1||h==2);closes++;have_staged=false;}
static void idle(void){CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static dsr_battery_sample_t reading(uint16_t mv)
{return (dsr_battery_sample_t){.has_raw=true,.has_voltage=true,.adc_mv=(uint16_t)(mv/2),.bat_mv=mv,.spread=4};}
static dsr_battery_sample_t estimated(uint16_t mv)
{dsr_battery_sample_t s=reading(mv);s.has_percent=s.percent_estimated=true;s.percent=50;return s;}
static void trend_tests(void)
{
 dsr_battery_trend_t t;dsr_battery_trend_reset(&t);CHECK(!dsr_battery_trend_likely(&t,0));
 for(unsigned wrap=0;wrap<2;wrap++){
  uint32_t start=wrap?UINT32_MAX-6000:1000;dsr_battery_trend_reset(&t);
  for(unsigned i=0;i<=10;i++){
   dsr_battery_sample_t s=estimated((uint16_t)(3900+2*i));uint32_t at=start+i*1000;
   dsr_battery_trend_update(&t,&s,at,true);CHECK(dsr_battery_trend_likely(&t,at)==(i==10));
  }
  CHECK(dsr_battery_trend_likely(&t,start+12500));CHECK(!dsr_battery_trend_likely(&t,start+12501));
  dsr_battery_sample_t s=estimated(3914);dsr_battery_trend_update(&t,&s,start+11000,true);CHECK(!dsr_battery_trend_likely(&t,start+11000));
 }
 for(unsigned mode=0;mode<5;mode++){
  dsr_battery_trend_reset(&t);
  for(unsigned i=0;i<30;i++){
   uint16_t mv=mode==0?3900:mode==1?(i?3950:3900):mode==2?(uint16_t)(3900+i/2):mode==3?(uint16_t)(3900+(i%3)*2):(uint16_t)(3950-i*2);
   dsr_battery_sample_t s=estimated(mv);dsr_battery_trend_update(&t,&s,i*1000,true);CHECK(!dsr_battery_trend_likely(&t,i*1000));
  }
 }
 for(unsigned invalid=0;invalid<7;invalid++){
  dsr_battery_trend_reset(&t);
  for(unsigned i=0;i<=10;i++){dsr_battery_sample_t s=estimated((uint16_t)(3900+2*i));dsr_battery_trend_update(&t,&s,i*1000,true);}
  CHECK(dsr_battery_trend_likely(&t,10000));dsr_battery_sample_t s=estimated(3922);
  if(invalid==0)s.has_voltage=false;else if(invalid==1)s.has_percent=false;else if(invalid==2)s.percent_estimated=false;else if(invalid==3)s.spread=121;else if(invalid==4)s.has_charge_state=true;
  dsr_battery_trend_update(&t,invalid==5?NULL:&s,11000,invalid!=6);CHECK(!dsr_battery_trend_likely(&t,11000));
 }
 dsr_battery_trend_reset(&t);
 for(unsigned i=0;i<=10;i++){dsr_battery_sample_t s=estimated((uint16_t)(3900+2*i));dsr_battery_trend_update(&t,&s,i*1000,true);}
 dsr_battery_sample_t s=estimated(3922);dsr_battery_trend_update(&t,&s,13000,true);CHECK(!dsr_battery_trend_likely(&t,13000));
 /* One jump + recycled timestamp never builds ten seconds of observations. */
 dsr_battery_trend_reset(&t);s=estimated(3900);dsr_battery_trend_update(&t,&s,0,true);s=estimated(3940);for(unsigned i=0;i<100;i++)dsr_battery_trend_update(&t,&s,1000,true);CHECK(!dsr_battery_trend_likely(&t,1000));
}
int main(void)
{
 trend_tests();
 dsr_battery_profile_t p,loaded;dsr_battery_profile_defaults(&p);CHECK(dsr_battery_profile_valid(&p)&&p.kind==DSR_BATTERY_NONE);
 CHECK(dsr_battery_profile_load(NULL)==ESP_ERR_INVALID_ARG&&dsr_battery_profile_save(NULL)==ESP_ERR_INVALID_ARG);
 CHECK(dsr_battery_profile_load(&loaded)==ESP_OK&&dsr_battery_profile_equal(&p,&loaded)&&!sets&&!commits);idle();
 dsr_battery_sample_t s=reading(3800);dsr_battery_apply_profile(&p,&s);CHECK(!s.has_percent&&!s.percent_estimated&&!s.has_charge_state&&!s.charging);
 p.kind=DSR_BATTERY_LIPO_1S;
 static const uint16_t volts[]={2600,3300,3550,3800,4000,4200,4340};
 static const unsigned expected[]={0,0,25,50,75,100,100};
 for(unsigned i=0;i<sizeof(volts)/sizeof(volts[0]);i++){s=reading(volts[i]);dsr_battery_apply_profile(&p,&s);CHECK(s.has_percent&&s.percent_estimated&&s.percent==expected[i]&&!s.charging&&!s.has_charge_state);}
 unsigned previous=0;
 for(unsigned mv=2500;mv<=4350;mv+=2){s=reading((uint16_t)mv);dsr_battery_apply_profile(&p,&s);CHECK(s.has_percent&&s.percent>=previous&&s.percent<=100);previous=s.percent;}
 s=reading(3800);p.capacity_mah=1;dsr_battery_apply_profile(&p,&s);CHECK(s.percent==50);p.capacity_mah=20000;dsr_battery_apply_profile(&p,&s);CHECK(s.percent==50);p.capacity_mah=1450;
 p.kind=DSR_BATTERY_LIION_1S;CHECK(dsr_battery_profile_valid(&p));
 p.gain_permille=1050;s=reading(3800);dsr_battery_apply_profile(&p,&s);CHECK(s.bat_mv==3990&&s.percent==74);dsr_battery_apply_profile(&p,&s);CHECK(s.bat_mv==3990&&s.percent==74);
 for(unsigned bad=0;bad<7;bad++){
  s=reading(3800);if(bad==0)s.has_voltage=false;else if(bad==1)s.calibration_error=ESP_FAIL;else if(bad==2)s.adc_mv=0;else if(bad==3)s.adc_mv=1000;else if(bad==4)s.adc_mv=2300;else if(bad==5)s.spread=121;else s.spread=4095;
  dsr_battery_apply_profile(&p,&s);CHECK(!s.has_percent&&!s.percent_estimated&&!s.has_charge_state&&!s.charging);
 }
 s=reading(4000);s.has_percent=true;s.percent=12;s.has_charge_state=true;s.charging=true;dsr_battery_apply_profile(&p,&s);CHECK(s.percent==12&&!s.percent_estimated&&s.bat_mv==4000&&s.charging);
 uint16_t gain=7;s=reading(4000);CHECK(dsr_battery_reference_gain(&s,4200,&gain)&&gain==1050);CHECK(dsr_battery_reference_gain(&s,3600,&gain)&&gain==900);CHECK(dsr_battery_reference_gain(&s,4400,&gain)==false);CHECK(!dsr_battery_reference_gain(&s,3500,&gain));s.spread=121;CHECK(!dsr_battery_reference_gain(&s,4000,&gain));s=reading(4000);s.calibration_error=ESP_FAIL;CHECK(!dsr_battery_reference_gain(&s,4000,&gain));
 static const char *bad_numbers[]={"","-1","+1"," 3700","3700 ","3.7","3700mV","65536","999999","1e3","\n1"};
 for(unsigned i=0;i<sizeof(bad_numbers)/sizeof(bad_numbers[0]);i++){uint16_t out=123;CHECK(!dsr_battery_parse_number(bad_numbers[i],&out)&&out==123);}
 uint16_t number;CHECK(dsr_battery_parse_number("65535",&number)&&number==65535);CHECK(dsr_battery_parse_number("00001",&number)&&number==1);CHECK(!dsr_battery_parse_number(NULL,&number)&&!dsr_battery_parse_number("1",NULL));
 dsr_battery_profile_t invalid;
 for(unsigned i=0;i<12;i++){
  invalid=p;if(i==0)invalid.kind=3;else if(i==1)invalid.nominal_mv=3200;else if(i==2)invalid.capacity_mah=0;else if(i==3)invalid.capacity_mah=20001;else if(i==4)invalid.empty_mv=2799;else if(i==5)invalid.empty_mv=3601;else if(i==6)invalid.mid_mv=invalid.empty_mv;else if(i==7)invalid.full_mv=invalid.mid_mv+99;else if(i==8)invalid.full_mv=4350;else if(i==9)invalid.gain_permille=899;else if(i==10)invalid.gain_permille=1101;else invalid.reserved[1]=1;
  unsigned n=sets;CHECK(!dsr_battery_profile_valid(&invalid)&&dsr_battery_profile_save(&invalid)==ESP_ERR_INVALID_ARG&&sets==n);s=reading(3800);dsr_battery_apply_profile(&invalid,&s);CHECK(!s.has_percent);idle();
 }
 CHECK(dsr_battery_profile_save(&p)==ESP_OK);CHECK(dsr_battery_profile_load(&loaded)==ESP_OK&&dsr_battery_profile_equal(&p,&loaded));idle();
 uint8_t original[24];memcpy(original,flash,24);
 dsr_battery_profile_t replacement=p;replacement.capacity_mah=1800;
 for(unsigned f=1;f<=3;f++){
  fault=f;CHECK(dsr_battery_profile_save(&replacement)!=ESP_OK&&!memcmp(flash,original,24));fault=0;CHECK(dsr_battery_profile_load(&loaded)==ESP_OK&&dsr_battery_profile_equal(&p,&loaded));idle();
 }
 for(unsigned f=4;f<=5;f++){fault=f;CHECK(dsr_battery_profile_save(&replacement)!=ESP_OK);fault=0;idle();}
 CHECK(dsr_battery_profile_save(&p)==ESP_OK);
 /* Every single-bit change in the canonical record fails closed, no writes. */
 unsigned n=sets;
 for(unsigned byte=0;byte<24;byte++)for(unsigned bit=0;bit<8;bit++){
  memcpy(flash,original,24);flash[byte]^=(uint8_t)(1u<<bit);CHECK(dsr_battery_profile_load(&loaded)!=ESP_OK&&loaded.kind==DSR_BATTERY_NONE&&sets==n);
 }
 memcpy(flash,original,24);length=23;CHECK(dsr_battery_profile_load(&loaded)!=ESP_OK&&loaded.kind==DSR_BATTERY_NONE);length=24;
 CHECK(dsr_activity_restart_reserve());CHECK(dsr_battery_profile_save(&p)==ESP_ERR_INVALID_STATE&&sets==n);dsr_activity_restart_cancel();
 p.kind=DSR_BATTERY_NONE;CHECK(dsr_battery_profile_save(&p)==ESP_OK);memset(&loaded,0xa5,sizeof(loaded));CHECK(dsr_battery_profile_load(&loaded)==ESP_OK&&dsr_battery_profile_equal(&p,&loaded));s=reading(3800);dsr_battery_apply_profile(&loaded,&s);CHECK(!s.has_percent);idle();
 CHECK(opens>=closes);printf("PASS: %u battery calibration/model/Flash checks, corruption and restart guards\n",checks);return 0;
}
