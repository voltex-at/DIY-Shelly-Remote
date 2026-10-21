#include "dsr_battery.h"
#include "dsr_activity.h"
#include "nvs.h"
#include <string.h>

/* One atomic blob, no changes to legacy Wi-Fi/device records or partitions.
 * Canonical little-endian wire bytes, independent of compiler padding. */
enum {RECORD_SIZE=24};
static void put16(uint8_t *b,uint16_t n){b[0]=(uint8_t)n;b[1]=(uint8_t)(n>>8);}
static uint16_t get16(const uint8_t *b){return (uint16_t)(b[0]|(uint16_t)b[1]<<8);}
static uint32_t checksum(const uint8_t *b)
{
 uint32_t crc=UINT32_MAX;
 for(unsigned i=0;i<20;i++){crc^=b[i];for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1)));}
 return ~crc;
}
static void encode(const dsr_battery_profile_t *p,uint8_t *b)
{
 memset(b,0,RECORD_SIZE);b[0]='B';b[1]='P';b[2]=1;b[3]=RECORD_SIZE;b[4]=p->kind;b[5]=p->trend_enabled;
 const uint16_t values[]={p->nominal_mv,p->capacity_mah,p->empty_mv,p->mid_mv,p->full_mv,p->gain_permille};
 for(unsigned i=0;i<6;i++)put16(b+8+2*i,values[i]);
 uint32_t crc=checksum(b);for(unsigned i=0;i<4;i++)b[20+i]=(uint8_t)(crc>>(8*i));
}
esp_err_t dsr_battery_profile_load(dsr_battery_profile_t *out)
{
 if(!out)return ESP_ERR_INVALID_ARG;
 dsr_battery_profile_defaults(out);nvs_handle_t h;
 esp_err_t e=nvs_open("dsr_battery",NVS_READONLY,&h);
 if(e==ESP_ERR_NVS_NOT_FOUND)return ESP_OK;
 if(e!=ESP_OK)return e;
 uint8_t b[RECORD_SIZE];size_t size=sizeof(b);e=nvs_get_blob(h,"profile",b,&size);nvs_close(h);
 if(e==ESP_ERR_NVS_NOT_FOUND)return ESP_OK;
 if(e!=ESP_OK)return e;
 if(size!=sizeof(b)||b[0]!='B'||b[1]!='P'||b[3]!=RECORD_SIZE)return ESP_ERR_INVALID_SIZE;
 if(b[2]!=1)return ESP_ERR_NOT_SUPPORTED;
 uint32_t crc=checksum(b);
 for(unsigned i=0;i<4;i++)if(b[20+i]!=(uint8_t)(crc>>(8*i)))return ESP_ERR_INVALID_SIZE;
 if(b[6]||b[7])return ESP_ERR_INVALID_SIZE;
 dsr_battery_profile_t p={.kind=b[4],.trend_enabled=b[5],.nominal_mv=get16(b+8),.capacity_mah=get16(b+10),
  .empty_mv=get16(b+12),.mid_mv=get16(b+14),.full_mv=get16(b+16),.gain_permille=get16(b+18)};
 if(!dsr_battery_profile_valid(&p))return ESP_ERR_INVALID_ARG;
 *out=p;return ESP_OK;
}
esp_err_t dsr_battery_profile_save(const dsr_battery_profile_t *p)
{
 if(!dsr_battery_profile_valid(p))return ESP_ERR_INVALID_ARG;
 if(!dsr_activity_begin())return ESP_ERR_INVALID_STATE;
 uint8_t b[RECORD_SIZE];encode(p,b);nvs_handle_t h;
 esp_err_t e=nvs_open("dsr_battery",NVS_READWRITE,&h);
 if(e==ESP_OK){e=nvs_set_blob(h,"profile",b,sizeof(b));if(e==ESP_OK)e=nvs_commit(h);nvs_close(h);}
 if(e==ESP_OK){dsr_battery_profile_t verify;e=dsr_battery_profile_load(&verify);if(e==ESP_OK&&!dsr_battery_profile_equal(p,&verify))e=ESP_FAIL;}
 dsr_activity_end();return e;
}
