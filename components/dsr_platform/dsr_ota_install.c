#include "dsr_ota_install.h"
#include "dsr_ota_stage.h"
#include "dsr_boot.h"
#include "../dsr_ota/dsr_ota_fetch_private.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
 unsigned char prefix[DSR_OTA_IMAGE_PREFIX_SIZE];
 size_t prefix_used;
 uint32_t received;
 esp_ota_handle_t handle;
 bool live;
 const esp_partition_t *slot;
 const dsr_ota_verified_t *consent;
 const dsr_ota_install_control_t *control;
 dsr_ota_install_report_t *report;
 dsr_ota_verified_t verified;
} install_t;
_Static_assert(sizeof(install_t)<=1024,"bounded native installer workspace");
static bool partition(const esp_partition_t *p,unsigned type,unsigned subtype,
 const char *label,uint32_t address,uint32_t size)
{return p&&p->type==type&&p->subtype==subtype&&p->address==address&&p->size==size&&memchr(p->label,0,sizeof(p->label))&&!strcmp(p->label,label);}
static const esp_partition_t *inactive_slot(void)
{
 const esp_partition_t *a=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_0,"ota_0");
 const esp_partition_t *b=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_1,"ota_1");
 const esp_partition_t *data=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"dsrdata");
 if(!partition(a,ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_0,"ota_0",0x20000,0x600000)||
    !partition(b,ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_1,"ota_1",0x620000,0x600000)||
    !partition(data,ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"dsrdata",0xc20000,0x3e0000))return NULL;
 const esp_partition_t *running=esp_ota_get_running_partition(),*next=esp_ota_get_next_update_partition(NULL);
 if(!((running==a&&next==b)||(running==b&&next==a)))return NULL;
 return next;
}
dsr_ota_install_result_t dsr_ota_install_ready(size_t *inactive_size)
{
 if(!inactive_size)return DSR_INSTALL_ARGUMENT;
 *inactive_size=0;
 if(!dsr_boot_ready())return DSR_INSTALL_BOOT;
 if(dsr_ota_stage_status()!=DSR_OTA_STAGE_IDLE)return DSR_INSTALL_BUSY;
 const esp_partition_t *slot=inactive_slot();if(!slot)return DSR_INSTALL_SLOT;
 *inactive_size=slot->size;return DSR_INSTALL_OK;
}
static bool cancelled(void *context)
{install_t *s=context;return s->control&&s->control->cancelled&&s->control->cancelled(s->control->context);}
static void progress(install_t *s,dsr_ota_install_phase_t phase)
{if(s->control&&s->control->progress)s->control->progress(s->control->context,phase,s->received,s->consent->size);}
static bool write_bytes(install_t *s,const void *bytes,size_t size)
{
 if(!size)return true;
 s->report->sdk_error=esp_ota_write(s->handle,bytes,size);
 if(s->report->sdk_error!=ESP_OK)return false;
 s->report->bytes_written+=(uint32_t)size;return true;
}
static bool sink_write(void *context,const void *bytes,size_t length,bool header_ready)
{
 install_t *s=context;
 if(cancelled(s))return false;
 if(!s->live){
  size_t copied=sizeof(s->prefix)-s->prefix_used;if(copied>length)copied=length;
  memcpy(s->prefix+s->prefix_used,bytes,copied);s->prefix_used+=copied;
  if(header_ready){
   if(s->prefix_used!=sizeof(s->prefix)){s->report->sdk_error=ESP_ERR_INVALID_STATE;return false;}
   s->report->sdk_error=esp_ota_begin(s->slot,OTA_WITH_SEQUENTIAL_WRITES,&s->handle); /* erase per sector while writing; size/SHA enforced by the verified fetch */
   if(s->report->sdk_error!=ESP_OK)return false;
   s->live=true;
   if(!write_bytes(s,s->prefix,sizeof(s->prefix))||!write_bytes(s,(const unsigned char *)bytes+copied,length-copied))return false;
  }
 }else if(!write_bytes(s,bytes,length))return false;
 s->received+=(uint32_t)length;progress(s,DSR_INSTALL_DOWNLOADING);return true;
}
dsr_ota_install_result_t dsr_ota_install(const char *manifest_url,
 const dsr_ota_policy_t *policy,const dsr_ota_verified_t *consent,
 const dsr_ota_install_control_t *control,bool power_confirmed,
 dsr_ota_install_report_t *report)
{
 dsr_ota_install_report_t local={0};if(!report)report=&local;
 *report=(dsr_ota_install_report_t){.fetch_result=DSR_FETCH_ARGUMENT,.manifest_result=DSR_MANIFEST_INVALID,.image_result=DSR_IMAGE_STATE};
 if(!manifest_url||!dsr_ota_fetch_policy_valid(policy)||!consent||
    !memchr(consent->version,0,sizeof(consent->version))||!memchr(consent->url,0,sizeof(consent->url))||
    consent->size<512||consent->size>DSR_OTA_IMAGE_MAX_SIZE)return DSR_INSTALL_ARGUMENT;
 if(!power_confirmed)return DSR_INSTALL_POWER;
 if(!dsr_boot_ready())return DSR_INSTALL_BOOT;
 const esp_partition_t *slot=inactive_slot();if(!slot||policy->inactive_slot_size!=slot->size)return DSR_INSTALL_SLOT;
 dsr_ota_stage_t *stage=dsr_ota_stage_acquire();if(!stage)return DSR_INSTALL_BUSY;
 install_t *s=calloc(1,sizeof(*s));
 if(!s){report->sdk_error=ESP_ERR_NO_MEM;dsr_ota_stage_release(stage);return DSR_INSTALL_SDK;}
 s->slot=slot;s->consent=consent;s->control=control;s->report=report;
 dsr_ota_install_result_t result=DSR_INSTALL_CANCELLED;
 if(cancelled(s))goto done;
 progress(s,DSR_INSTALL_MANIFEST);
 dsr_ota_fetch_control_t fetch_control={cancelled,s};dsr_ota_stream_sink_t sink={sink_write,s};bool changed=false;
 dsr_ota_fetch_diagnostic_t diagnostic={0};
 report->fetch_result=dsr_ota_fetch_stream_app(manifest_url,policy,consent,&fetch_control,&sink,
  &s->verified,&report->manifest_result,&report->image_result,&changed,&diagnostic);
 report->http_status=diagnostic.http_status;report->io_result=diagnostic.io_result;
 if(report->fetch_result!=DSR_FETCH_OK){
  result=changed?DSR_INSTALL_CONSENT_CHANGED:report->sdk_error!=ESP_OK?DSR_INSTALL_SDK:
   report->fetch_result==DSR_FETCH_CANCELLED||cancelled(s)?DSR_INSTALL_CANCELLED:DSR_INSTALL_FETCH;goto done;
 }
 if(!s->live||report->bytes_written!=consent->size){report->sdk_error=ESP_ERR_INVALID_STATE;result=DSR_INSTALL_SDK;goto done;}
 progress(s,DSR_INSTALL_VERIFYING);if(cancelled(s))goto done;
 report->sdk_error=esp_ota_end(s->handle);s->live=false; /* end always invalidates the SDK handle. */
 if(report->sdk_error!=ESP_OK){result=DSR_INSTALL_SDK;goto done;}
 progress(s,DSR_INSTALL_SELECTING_BOOT);if(cancelled(s))goto done;
 report->boot_selection_attempted=true;report->sdk_error=esp_ota_set_boot_partition(slot);
 if(report->sdk_error!=ESP_OK){dsr_ota_stage_uncertain(stage);stage=NULL;result=DSR_INSTALL_SDK;goto done;}
 report->boot_selected=true;dsr_ota_stage_commit(stage);stage=NULL;result=DSR_INSTALL_OK;
 progress(s,DSR_INSTALL_STAGED);
done:
 if(s->live){esp_err_t error=esp_ota_abort(s->handle);s->live=false;if(error!=ESP_OK){report->sdk_error=error;result=DSR_INSTALL_SDK;}}
 free(s);dsr_ota_stage_release(stage);return result;
}
