#include "dsr_boot.h"
#include "dsr_board.h"
#include "dsr_platform.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); abort(); } } while (0)
void app_main(void);
static char trace[32];
static unsigned at, ota_reads, ota_marks;
static esp_err_t nvs_error, board_error, worker_error, wifi_error, storage_error;
static esp_err_t ui_error, backlight_error, state_error, confirm_error;
static esp_ota_img_states_t image_state;
static bool missing_partition;
static const esp_partition_t partition = {0};
static void call(char code) { CHECK(at+1 < sizeof(trace)); trace[at++]=code; trace[at]=0; }
const esp_app_desc_t *esp_app_get_description(void)
{ static const esp_app_desc_t description = {.version="TEST_ONLY startup"}; return &description; }
esp_err_t nvs_flash_init(void) { call('N'); return nvs_error; }
esp_err_t dsr_board_init(void) { call('B'); return board_error; }
const char *dsr_board_hardware_id(void) { return "TEST_ONLY board"; }
esp_err_t dsr_settings_load(dsr_settings_t *settings) { call('S'); memset(settings,0,sizeof(*settings)); return ESP_OK; }
esp_err_t dsr_worker_start(void) { call('W'); return worker_error; }
esp_err_t dsr_wifi_start(const dsr_settings_t *settings) { (void)settings; call('F'); return wifi_error; }
esp_err_t dsr_workspace_start(void) { call('C'); return storage_error; }
esp_err_t dsr_panel_start(void) { call('P'); return ESP_OK; }
esp_err_t dsr_lan_init(void) { call('L'); return ESP_OK; }
esp_err_t dsr_ble_init(void) { call('E'); return ESP_OK; }
void dsr_ui_init(void) { call('I'); }
esp_err_t dsr_board_start_ui_task(bool (*ready)(void))
{ call('T'); CHECK(ready == dsr_boot_ready && !ready()); CHECK(!dsr_ota_boot_confirm()); return ui_error; }
esp_err_t dsr_board_set_backlight(uint8_t percent)
{ call('K'); CHECK(percent == 70 && !dsr_boot_ready()); CHECK(!dsr_ota_boot_confirm()); return backlight_error; }
const esp_partition_t *esp_ota_get_running_partition(void)
{ ota_reads++; return missing_partition ? NULL : &partition; }
esp_err_t esp_ota_get_state_partition(const esp_partition_t *p,esp_ota_img_states_t *state)
{ CHECK(p == &partition); *state=image_state; return state_error; }
esp_err_t esp_ota_mark_app_valid_cancel_rollback(void) { ota_marks++; return confirm_error; }
static void reset(void)
{
    at=ota_reads=ota_marks=0; trace[0]=0; missing_partition=false;
    nvs_error=board_error=worker_error=wifi_error=storage_error=ui_error=backlight_error=state_error=confirm_error=ESP_OK;
    image_state=ESP_OTA_IMG_PENDING_VERIFY;
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
    /* Actual app_main returns normally on every injected critical error.
     * No restart/erase API or ESP_ERROR_CHECK substitute is linked here. */
    for (unsigned restart=0; restart<2; restart++) {
        reset(); ui_error=ESP_ERR_NO_MEM; app_main(); diagnostic("NBSWFCPLEIT");
    }
    reset(); ui_error=ESP_ERR_INVALID_STATE; app_main(); diagnostic("NBSWFCPLEIT");
    reset(); backlight_error=ESP_FAIL; app_main(); diagnostic("NBSWFCPLEITK");
    reset(); worker_error=ESP_ERR_NO_MEM; app_main(); diagnostic("NBSW");
    reset(); nvs_error=ESP_ERR_INVALID_STATE; app_main(); diagnostic("N");
    reset(); board_error=ESP_FAIL; app_main(); diagnostic("NB");
    reset(); app_main();
    CHECK(!strcmp(trace,"NBSWFCPLEITK") && dsr_boot_ready());
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
    reset(); wifi_error=ESP_FAIL; app_main(); CHECK(dsr_boot_ready() && !strcmp(trace,"NBSWFCLEITK"));
    reset(); storage_error=ESP_FAIL; app_main(); CHECK(dsr_boot_ready() && !strcmp(trace,"NBSWFCLEITK"));
    printf("PASS: %u startup/recovery checks; actual app_main fault injection, UI/backlight/worker/NVS errors, subsequent boot, OTA pending/valid/error guards; host only\n",checks);
    return 0;
}
