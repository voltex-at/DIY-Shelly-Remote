#include "dsr_ble.h"
#include <string.h>
#include <limits.h>

/* Bluetooth UUID octets as transmitted (little endian). */
static const uint8_t rpc_uuid[]={0x5f,0x44,0x49,0x5f,0x43,0x56,0x53,0x5f,0x43,0x50,0x52,0x5f,0x53,0x4f,0x6d,0x5f};
static const uint8_t blu_uuid[]={0x25,0x12,0xb5,0xcb,0xd4,0x60,0x80,0x0c,0x15,0xc3,0x9b,0xa9,0xac,0x5a,0x8a,0xde};
static unsigned u16(const uint8_t *p){return (unsigned)p[0]|((unsigned)p[1]<<8);}
static void increment(uint32_t *n){if(*n<UINT32_MAX)(*n)++;}
static bool shelly_name(const char *n){return !strncmp(n,"Shelly",6)||!strncmp(n,"shelly",6);}
static void classify(dsr_ble_device_t *d)
{
 if((d->has_model&&dsr_ble_model_name(d)[0])||(d->evidence&DSR_BLE_BLU_SERVICE)||((d->evidence&(DSR_BLE_MFG|DSR_BLE_BTHOME))==(DSR_BLE_MFG|DSR_BLE_BTHOME))||!strncmp(d->name,"SB",2))d->kind=DSR_BLE_SHELLY_BLU;
 /* Gen2+ Wi-Fi Shelly: advertised RPC UUID, manufacturer RPC flag, or name. */
 else if((d->evidence&(DSR_BLE_RPC|DSR_BLE_RPC_FLAG))||((d->evidence&DSR_BLE_NAME)&&shelly_name(d->name)))d->kind=DSR_BLE_SHELLY_RPC;
 else d->kind=DSR_BLE_OTHER;
}
const char *dsr_ble_model_name(const dsr_ble_device_t *d)
{
 if(!d||!d->has_model||!(d->evidence&DSR_BLE_MFG))return "";
 switch(d->model_id){case 1:return "Shelly BLU Button1";case 2:return "Shelly BLU DoorWindow";case 3:return "Shelly BLU H&T";case 5:return "Shelly BLU Motion";case 6:return "Shelly BLU Wall Switch 4";case 7:return "Shelly BLU RC Button 4";case 8:return "Shelly BLU TRV";case 9:return "Shelly BLU Remote";case 11:return "Shelly BLU Weather Station";case 12:return "Shelly BLU H&T Display ZB";default:return "";}
}
bool dsr_ble_parse(const uint8_t *data,size_t length,dsr_ble_device_t *out)
{
 if(!out||!data||!length||length>DSR_BLE_MAX_ADV)return false;
 dsr_ble_device_t d={0};
 for(size_t at=0;at<length;){
  size_t n=data[at++];if(!n)break;if(n>length-at)return false;
  unsigned type=data[at];const uint8_t *p=data+at+1;size_t size=n-1;
  if(type==8||type==9){
   size_t take=size<sizeof(d.name)-1?size:sizeof(d.name)-1;
   for(size_t i=0;i<take;i++)d.name[i]=p[i]>=32&&p[i]<=126?(char)p[i]:'?';
   d.name[take]=0;
   if(!strncmp(d.name,"Shelly",6)||!strncmp(d.name,"shelly",6)||!strncmp(d.name,"SB",2))d.evidence|=DSR_BLE_NAME;
  }else if(type==6||type==7){
   if(size%16)return false;
   for(size_t i=0;i<size;i+=16){if(!memcmp(p+i,rpc_uuid,16))d.evidence|=DSR_BLE_RPC;if(!memcmp(p+i,blu_uuid,16))d.evidence|=DSR_BLE_BLU_SERVICE;}
  }else if(type==0x16){
   if(size<2)return false;
   if(u16(p)==0xfcd2){
    if(size<3)return false;
    d.evidence|=DSR_BLE_BTHOME;d.bthome_version=p[2]>>5;d.encrypted=(p[2]&1)!=0;
    /* No measurements or type IDs are inferred from encrypted/unknown data. */
    if(d.bthome_version!=2||((p[2]&0x1a)!=0)||(d.encrypted&&size<11))return false;
   }
  }else if(type==0xff){
   if(size<2)return false;
   if(u16(p)==0x0ba9){
    d.evidence|=DSR_BLE_MFG;
    for(size_t i=2;i<size;){unsigned block=p[i++];size_t bytes=block==1||block==0x0b?2:block==0x0a?6:0;
     /* Unknown extension: keep manufacturer evidence but don't guess layout. */
     if(!bytes)break;
     if(bytes>size-i)return false;
     if(block==0x0b){d.has_model=true;d.model_id=(uint16_t)u16(p+i);}
     else if(block==0x01){d.has_flags=true;d.flags=(uint16_t)u16(p+i);if(d.flags&DSR_BLE_FLAG_RPC)d.evidence|=DSR_BLE_RPC_FLAG;}
     else {d.has_mac=true;memcpy(d.mac,p+i,6);}
     i+=bytes;
    }
   }
  }
  at+=n;
 }
 classify(&d);*out=d;return true;
}
void dsr_ble_begin(dsr_ble_snapshot_t *s,uint32_t now)
{
 uint32_t generation=s->generation+1,serial=s->serial+1;
 memset(s,0,sizeof(*s));s->generation=generation;s->serial=serial;s->started_ms=now;s->phase=DSR_BLE_STARTING;s->busy=true;
}
bool dsr_ble_ingest(dsr_ble_snapshot_t *s,uint32_t generation,const uint8_t address[6],uint8_t type,int8_t rssi,const uint8_t *data,size_t length,uint32_t now)
{
 if(!s||!address||generation!=s->generation||s->phase!=DSR_BLE_SCANNING)return false;
 increment(&s->packets);dsr_ble_device_t d;
 if(!dsr_ble_parse(data,length,&d)){increment(&s->malformed);return false;}
 unsigned i=0;while(i<s->count&&(s->devices[i].address_type!=type||memcmp(s->devices[i].address,address,6)))i++;
 if(i==s->count){
  if(!d.evidence)return false;
  if(i==DSR_BLE_MAX_DEVICES){increment(&s->dropped);return false;}
  s->count++;memset(&s->devices[i],0,sizeof(s->devices[i]));memcpy(s->devices[i].address,address,6);s->devices[i].address_type=type;
 }
 dsr_ble_device_t *row=&s->devices[i];row->evidence|=d.evidence;
 if(d.name[0])memcpy(row->name,d.name,sizeof(row->name));
 if(d.has_model){row->has_model=true;row->model_id=d.model_id;}
 if(d.has_flags){row->has_flags=true;row->flags=d.flags;}
 if(d.has_mac){row->has_mac=true;memcpy(row->mac,d.mac,6);}
 if(d.evidence&DSR_BLE_BTHOME){row->bthome_version=d.bthome_version;row->encrypted=d.encrypted;}
 row->rssi=rssi;row->seen_ms=now;classify(row);s->serial++;return true;
}
bool dsr_ble_finish(dsr_ble_snapshot_t *s,uint32_t generation,dsr_ble_phase_t phase,uint32_t now)
{
 if(!s||s->generation!=generation||(s->phase!=DSR_BLE_STARTING&&s->phase!=DSR_BLE_SCANNING))return false;
 if(phase!=DSR_BLE_DONE&&phase!=DSR_BLE_CANCELLED&&phase!=DSR_BLE_UNAVAILABLE)return false;
 s->phase=phase;s->busy=false;s->elapsed_ms=now-s->started_ms;if(s->elapsed_ms>DSR_BLE_SCAN_MS)s->elapsed_ms=DSR_BLE_SCAN_MS;s->serial++;return true;
}
