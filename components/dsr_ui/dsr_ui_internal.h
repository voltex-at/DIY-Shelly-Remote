#ifndef DSR_UI_INTERNAL_H
#define DSR_UI_INTERNAL_H
#include "lvgl.h"
#include "dsr_platform.h"
#include "dsr_ui_style.h"
lv_obj_t *dsr_ui_label(lv_obj_t *parent, const char *text, int x, int y, int width);
lv_obj_t *dsr_ui_button(lv_obj_t *parent, const char *text, int x, int y, int width,
                       lv_event_cb_t callback, void *context);
lv_obj_t *dsr_ui_screen(const char *title, bool navigation);
void dsr_ui_primary(lv_obj_t *obj);
void dsr_ui_textarea(lv_obj_t *obj);
void dsr_ui_keyboard(lv_obj_t *obj);
void dsr_ui_keyboard_focus(lv_obj_t *keyboard,lv_obj_t *field);
void dsr_ui_home(lv_event_t *event);
void dsr_ui_open_settings(void);
void dsr_network_init(void);
void dsr_network_start_view(void);
bool dsr_network_busy(void);
dsr_result_t dsr_network_request_wifi(dsr_job_kind_t kind, const dsr_settings_t *settings);
void dsr_network_wifi_settings(dsr_settings_t *out);
void dsr_wifi_ui_reset(void);
bool dsr_wifi_ui_active(void);
void dsr_wifi_ui_tick(void);
void dsr_wifi_ui_result(const dsr_event_t *event, dsr_job_kind_t kind);
void dsr_network_reset_view(void);
void dsr_network_home(lv_event_t *event);
void dsr_network_wifi(lv_event_t *event);
void dsr_network_device(lv_event_t *event);
void dsr_ui_edit_dashboard(void);
void dsr_ui_select_dashboard(void);
void dsr_ui_workspace_tick(void);
void dsr_ui_workspace_hide(void);
void dsr_ui_workspace_migrate(const dsr_connection_t *connection);
void dsr_ui_workspace_migrate_bound(const dsr_connection_t *connection,uint32_t room,const char *previous_host);
void dsr_ui_lan_open(lv_event_t *event);
void dsr_ui_lan_tick(void);
void dsr_ui_lan_reset(void);
bool dsr_ui_lan_active(void);
void dsr_ui_lan_bind_result(dsr_result_t result);
dsr_result_t dsr_network_bind_verified(const dsr_connection_t *connection,uint32_t room);
#endif
