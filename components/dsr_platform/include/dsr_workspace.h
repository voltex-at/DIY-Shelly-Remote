#ifndef DSR_WORKSPACE_H
#define DSR_WORKSPACE_H
#include "dsr_config.h"
#include "esp_err.h"
#include "dsr_platform.h"
#define DSR_PHOTO_WIDTH 320
#define DSR_PHOTO_HEIGHT 180
#define DSR_PHOTO_BYTES (DSR_PHOTO_WIDTH*DSR_PHOTO_HEIGHT*2)
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
bool dsr_panel_ready(void);
void dsr_ota_boot_confirm(void);
/* LVGL owns rendering; the server only requests/copies a published frame. */
bool dsr_panel_preview_requested(void);
void dsr_panel_preview_publish(const void *rgb565,size_t size);
void dsr_panel_tick(void); /* called by existing LVGL owner task */
void dsr_photos_load(void);
const void *dsr_photo_pixels(const char *id);
bool dsr_photo_save(const char *id,const void *pixels,size_t size);
const char *dsr_photo_id(unsigned index);
esp_err_t dsr_tls_identity(void);
const unsigned char *dsr_tls_certificate(void);
const unsigned char *dsr_tls_key(void);
const char *dsr_tls_fingerprint(void);
bool dsr_workspace_bind(const dsr_connection_t *connection);
bool dsr_workspace_bind_with_room(const dsr_connection_t *connection,uint32_t room);
bool dsr_workspace_take_bind(dsr_connection_t *connection);
bool dsr_workspace_take_bind_with_room(dsr_connection_t *connection,uint32_t *room);
void dsr_workspace_bind_result(dsr_result_t result);
void dsr_workspace_bind_status(bool *busy,char message[64]);
dsr_result_t dsr_workspace_bind_result_code(void);
#endif
