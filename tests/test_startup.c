#include "dsr_boot.h"
#include "dsr_board.h"
#include "dsr_ui.h"
#include "dsr_platform.h"
#include "dsr_config_health.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); abort(); } } while (0)
void app_main(void);
static char trace[32];
static unsigned at, ota_reads, ota_marks, reset_reads;
static esp_err_t nvs_error, board_error, worker_error, wifi_error, storage_error;
static esp_err_t ui_error, backlight_error, state_error, confirm_error;
static esp_err_t restart_error;
static esp_ota_img_states_t image_state;
static bool missing_partition;
static uint8_t desired_brightness,restored_brightness;
static const esp_partition_t partition = {0};
static void call(char code) { CHECK(at+1 < sizeof(trace)); trace[at++]=code; trace[at]=0; }
const esp_app_desc_t *esp_app_get_description(void)
{ static const esp_app_desc_t description = {.version="TEST_ONLY startup"}; return &description; }
esp_reset_reason_t esp_reset_reason(void) { CHECK(at==0); reset_reads++; return ESP_RST_TASK_WDT; }
esp_err_t nvs_flash_init(void) { CHECK(reset_reads==1); call('N'); return nvs_error; }
esp_err_t dsr_board_init(void) { call('B'); return board_error; }
const char *dsr_board_hardware_id(void) { return "TEST_ONLY board"; }
esp_err_t dsr_settings_load(dsr_settings_t *settings) { call('S'); memset(settings,0,sizeof(*settings)); settings->brightness=20;settings->idle_seconds=60; return ESP_OK; }
esp_err_t dsr_worker_start(void) { call('W'); return worker_error; }
esp_err_t dsr_wifi_start(const dsr_settings_t *settings) { (void)settings; call('F'); return wifi_error; }
static const dsr_storage_observer_t fake_observer;
static unsigned observer_sets;
const dsr_storage_observer_t *dsr_ui_storage_observer(void) { return &fake_observer; }
/* Registered before storage, every boot; storage tests own the hook calls. */
void dsr_storage_set_observer(const dsr_storage_observer_t *o) { CHECK(o==&fake_observer&&trace[at-1]=='F'); observer_sets++; }
esp_err_t dsr_workspace_start(void)
{
    call('C'); CHECK(observer_sets==1);
    dsr_config_health_storage(storage_error==ESP_OK?DSR_STORAGE_READY:DSR_STORAGE_NONEMPTY,storage_error);
    dsr_config_health_workspace(storage_error==ESP_OK?DSR_WORKSPACE_READY:DSR_WORKSPACE_MOUNTING,storage_error);
    return storage_error;
}
esp_err_t dsr_panel_start(void) { call('P'); return ESP_OK; }
esp_err_t dsr_lan_init(void) { call('L'); return ESP_OK; }
esp_err_t dsr_ble_init(void) { call('E'); return ESP_OK; }
esp_err_t dsr_restart_start(void) { return restart_error; }
void dsr_ui_splash_begin(void) { call('Z'); }
void dsr_ui_splash_end(void) { call('z'); }
void dsr_ui_init(void) { call('I'); desired_brightness=20; /* UI fixture loads the saved brightness; real UI separately tested. */ }
esp_err_t dsr_board_start_ui_task(bool (*ready)(void))
{ call('T'); CHECK(ready == dsr_boot_ui_gate && !ready()); CHECK(!dsr_ota_boot_confirm()); return ui_error; }
esp_err_t dsr_board_restore_backlight(void)
{ call('K'); CHECK(desired_brightness==20&&trace[at-2]=='T');restored_brightness=desired_brightness; CHECK(!dsr_boot_ready()); CHECK(!dsr_ota_boot_confirm()); return backlight_error; }
const esp_partition_t *esp_ota_get_running_partition(void)
{ ota_reads++; return missing_partition ? NULL : &partition; }
esp_err_t esp_ota_get_state_partition(const esp_partition_t *p,esp_ota_img_states_t *state)
{ CHECK(p == &partition); *state=image_state; return state_error; }
esp_err_t esp_ota_mark_app_valid_cancel_rollback(void) { CHECK(!dsr_activity_restart_reserve()); ota_marks++; return confirm_error; }
static void reset(void)
{
    at=ota_reads=ota_marks=reset_reads=observer_sets=0;desired_brightness=70;restored_brightness=0; trace[0]=0; missing_partition=false;
    nvs_error=board_error=worker_error=wifi_error=storage_error=ui_error=backlight_error=state_error=confirm_error=ESP_OK;
    restart_error=ESP_OK;
    image_state=ESP_OTA_IMG_PENDING_VERIFY;
    dsr_config_health_storage(DSR_STORAGE_NOT_STARTED,ESP_OK);
    dsr_config_health_workspace(DSR_WORKSPACE_NOT_STARTED,ESP_OK);
}
static void diagnostic(const char *expected)
{
    CHECK(!strcmp(trace,expected));
    CHECK(dsr_boot_state() == DSR_BOOT_DIAGNOSTIC && !dsr_boot_ready());
    CHECK(!dsr_boot_finish()); /* A later callback cannot re-open the gate. */
    CHECK(!dsr_ota_boot_confirm() && !ota_reads && !ota_marks);
}
int main(void)
{
    CHECK(dsr_boot_state() == DSR_BOOT_STARTING && !dsr_boot_ready());
    CHECK(!dsr_ota_boot_confirm() && !ota_reads);
    /* The first-format screen opens only the UI gate: never boot READY or OTA. */
    CHECK(!dsr_boot_ui_gate()); dsr_boot_provisioning_ui(true);
    CHECK(dsr_boot_ui_gate() && !dsr_boot_ready() && !dsr_ota_boot_confirm() && !ota_reads);
    dsr_boot_provisioning_ui(false); CHECK(!dsr_boot_ui_gate());
    /* Actual app_main returns normally on every injected critical error.
     * No restart/erase API or ESP_ERROR_CHECK substitute is linked here. */
    for (unsigned restart=0; restart<2; restart++) {
        reset(); ui_error=ESP_ERR_NO_MEM; app_main(); diagnostic("NBZSWFCPLEzIT");
    }
    reset(); ui_error=ESP_ERR_INVALID_STATE; app_main(); diagnostic("NBZSWFCPLEzIT");
    reset(); backlight_error=ESP_FAIL; app_main(); diagnostic("NBZSWFCPLEzITK");
    reset(); worker_error=ESP_ERR_NO_MEM; app_main(); diagnostic("NBZSW");
    reset(); nvs_error=ESP_ERR_INVALID_STATE; app_main(); diagnostic("N");
    reset(); board_error=ESP_FAIL; app_main(); diagnostic("NB");
    reset(); app_main();
    CHECK(!strcmp(trace,"NBZSWFCPLEzITK") && dsr_boot_ready() && restored_brightness==20);
    CHECK(dsr_ota_boot_confirm() && ota_marks == 1);
    confirm_error=ESP_FAIL; CHECK(!dsr_ota_boot_confirm() && ota_marks == 2);
    confirm_error=ESP_OK; CHECK(dsr_ota_boot_confirm() && ota_marks == 3);
    image_state=ESP_OTA_IMG_VALID; CHECK(dsr_ota_boot_confirm() && ota_marks == 3);
    image_state=ESP_OTA_IMG_UNDEFINED; CHECK(dsr_ota_boot_confirm() && ota_marks == 3);
    image_state=ESP_OTA_IMG_NEW; CHECK(!dsr_ota_boot_confirm() && ota_marks == 3);
    image_state=ESP_OTA_IMG_INVALID; CHECK(!dsr_ota_boot_confirm() && ota_marks == 3);
    image_state=ESP_OTA_IMG_ABORTED; CHECK(!dsr_ota_boot_confirm() && ota_marks == 3);
    state_error=ESP_ERR_NOT_FOUND; CHECK(dsr_ota_boot_confirm() && ota_marks == 3);
    state_error=ESP_FAIL; CHECK(!dsr_ota_boot_confirm() && ota_marks == 3);
    missing_partition=true; CHECK(!dsr_ota_boot_confirm() && ota_marks == 3);
    dsr_boot_diagnostic("TEST_ONLY failure",ESP_FAIL);
    unsigned previous_reads=ota_reads; CHECK(!dsr_ota_boot_confirm() && ota_reads == previous_reads && ota_marks == 3);
    /* A fresh app_main represents an explicit subsequent boot, not an
     * automatic retry loop. It can recover from a previous process latch. */
    reset(); wifi_error=ESP_FAIL; app_main(); CHECK(dsr_boot_ready() && !strcmp(trace,"NBZSWFCLEzITK"));
    reset(); storage_error=ESP_FAIL; app_main(); CHECK(dsr_boot_ready() && !strcmp(trace,"NBZSWFCLEzITK"));
    /* A working recovery UI is not a healthy OTA boot. Preserve the SDK's
     * pending state so the previous app remains available on the next reset. */
    CHECK(!dsr_ota_boot_confirm()&&ota_marks==0&&dsr_boot_ui_gate());
    dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);
    const dsr_workspace_phase_t incomplete[]={DSR_WORKSPACE_NOT_STARTED,DSR_WORKSPACE_MOUNTING,
        DSR_WORKSPACE_LOADING,DSR_WORKSPACE_PROVISIONING,DSR_WORKSPACE_ALLOCATING,DSR_WORKSPACE_TASK};
    for(size_t i=0;i<sizeof(incomplete)/sizeof(*incomplete);i++){
        dsr_config_health_workspace(incomplete[i],ESP_OK);
        CHECK(!dsr_ota_boot_confirm()&&ota_marks==0);
    }
    dsr_config_health_workspace(DSR_WORKSPACE_READY,ESP_FAIL);
    CHECK(!dsr_ota_boot_confirm()&&ota_marks==0);
    dsr_config_health_workspace(DSR_WORKSPACE_READY,ESP_OK);
    const dsr_storage_phase_t unavailable[]={DSR_STORAGE_NOT_STARTED,DSR_STORAGE_MOUNTING,
        DSR_STORAGE_MOUNT_ERROR,DSR_STORAGE_NONEMPTY,DSR_STORAGE_READ_ERROR,
        DSR_STORAGE_PROVISION_ERROR,DSR_STORAGE_PROVISIONING};
    for(size_t i=0;i<sizeof(unavailable)/sizeof(*unavailable);i++){
        dsr_config_health_storage(unavailable[i],ESP_OK);
        CHECK(!dsr_ota_boot_confirm()&&ota_marks==0);
    }
    dsr_config_health_storage(DSR_STORAGE_READY,ESP_FAIL);
    CHECK(!dsr_ota_boot_confirm()&&ota_marks==0);
    dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);
    CHECK(dsr_ota_boot_confirm()&&ota_marks==1); /* Both stores ready: normal confirmation. */
    reset(); restart_error=ESP_ERR_NO_MEM; app_main(); CHECK(dsr_boot_ready()&&!strcmp(trace,"NBZSWFCPLEzITK"));
    CHECK(dsr_activity_restart_reserve());unsigned marks=ota_marks;CHECK(!dsr_ota_boot_confirm()&&ota_marks==marks);dsr_activity_restart_cancel();
    printf("PASS: %u startup/recovery checks; actual app_main fault injection, UI/backlight/worker/NVS errors, subsequent boot, OTA pending/valid/error guards; host only\n",checks);
    return 0;
}
