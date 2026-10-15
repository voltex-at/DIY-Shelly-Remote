#ifndef DSR_WORKSPACE_H
#define DSR_WORKSPACE_H
#include "dsr_config.h"
#include "esp_err.h"
/* These APIs never call LVGL. All flash work belongs to dsr_config task. */
esp_err_t dsr_workspace_start(void);
char *dsr_workspace_snapshot(void); /* malloc; caller frees */
uint32_t dsr_workspace_revision(void);
/* Nonblocking. Configuration stays unchanged until durable write succeeds. */
bool dsr_workspace_submit(const char *json,uint32_t revision);
void dsr_workspace_status(bool *busy,char message[96]);
/* Pairing requires a local touch to open a 120-second, rate-limited window.
 * No shared/factory password. Pairing credentials must never be logged. */
void dsr_panel_pair_begin(char code[17]);
void dsr_panel_pair_cancel(void);
void dsr_panel_allow_ota(void);
esp_err_t dsr_panel_start(void);
void dsr_ota_boot_confirm(void);
/* LVGL owns rendering; the server only requests/copies a published frame. */
bool dsr_panel_preview_requested(void);
void dsr_panel_preview_publish(const void *rgb565,size_t size);
void dsr_panel_tick(void); /* called by existing LVGL owner task */
void dsr_photos_load(void);
const void *dsr_photo_pixels(const char *id);
bool dsr_photo_save(const char *id,const void *pixels,size_t size);
#endif
