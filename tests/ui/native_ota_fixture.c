/* LCD backend MODEL only. Actual actor/crypto/install/SDK lifetime have their
 * separate suites. No Flash or publisher trust claimed by these screenshots. */
#include "dsr_native_ota.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
static dsr_native_ota_snapshot_t view={.installed="0.3.0-wifi-dev"};
static uint32_t available_at;
static unsigned cancellations;
void test_native_ota_set(dsr_native_ota_state_t state,dsr_native_ota_error_t error,dsr_manifest_result_t why)
{view.state=state;view.error=error;view.manifest_result=why;view.boot_selection_attempted=false;if(state==DSR_NATIVE_AVAILABLE){strcpy(view.version,"9.1.2");view.total=1857984;strcpy(view.notes,"<b>plain untrusted</b>\n#ff0000 literal#");available_at=lv_tick_get();}}
void test_native_ota_uncertain(void){view.state=DSR_NATIVE_FAILED;view.error=DSR_NATIVE_ERROR_INSTALL;view.boot_selection_attempted=true;}
void test_native_ota_notes(const char *text){snprintf(view.notes,sizeof(view.notes),"%s",text);}
unsigned test_native_ota_cancels(void){return cancellations;}
dsr_result_t dsr_native_ota_prepare(uint32_t id,dsr_job_kind_t kind,dsr_ota_channel_t channel,bool power)
{if(kind==DSR_JOB_OTA_INSTALL&&(view.state!=DSR_NATIVE_AVAILABLE||!power||view.channel!=channel))return DSR_INVALID;view.state=kind==DSR_JOB_OTA_CHECK?DSR_NATIVE_CHECKING:DSR_NATIVE_INSTALLING;view.operation_id=id;view.channel=channel;view.error=DSR_NATIVE_ERROR_NONE;return DSR_OK;}
void dsr_native_ota_reject(uint32_t id){if(view.operation_id==id){view.state=DSR_NATIVE_FAILED;view.error=DSR_NATIVE_ERROR_BUSY;}}
void dsr_native_ota_snapshot(dsr_native_ota_snapshot_t *out)
{if(view.state==DSR_NATIVE_AVAILABLE&&lv_tick_get()-available_at>=120000){view.state=DSR_NATIVE_FAILED;view.error=DSR_NATIVE_ERROR_EXPIRED;}*out=view;}
void dsr_native_ota_cancel(void){cancellations++;if(view.state==DSR_NATIVE_AVAILABLE)view.state=DSR_NATIVE_CANCELLED;}
