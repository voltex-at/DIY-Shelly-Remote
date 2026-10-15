#ifndef DSR_UI_MOBILE_H
#define DSR_UI_MOBILE_H
#include <stdbool.h>
#include <stdint.h>
#include "dsr_core.h"

#define DSR_UI_MAX_DEVICES 16
#define DSR_UI_MAX_ROOMS 8
#define DSR_UI_NO_ROOM 255
typedef enum {DSR_MOBILE_HOME,DSR_MOBILE_ROOMS,DSR_MOBILE_DEVICES,DSR_MOBILE_ROOM,DSR_MOBILE_DETAILS} dsr_mobile_page_t;
typedef enum {DSR_DEVICE_LIGHT,DSR_DEVICE_PLUG,DSR_DEVICE_RELAY,DSR_DEVICE_COVER,DSR_DEVICE_CONTACT,DSR_DEVICE_SENSOR} dsr_mobile_kind_t;
/* Confirmed Cover status only; zero means no status sample, never stopped. */
typedef enum {DSR_COVER_UNKNOWN,DSR_COVER_OPENING,DSR_COVER_CLOSING,DSR_COVER_STOPPED,DSR_COVER_CALIBRATING} dsr_mobile_cover_state_t;
typedef struct {
    char name[48],model[40],host[16];
    uint8_t channel;
    uint8_t room;dsr_mobile_kind_t kind;
    bool online,output,busy,can_switch,has_power,has_voltage,has_current;
    bool command_pending;
    dsr_result_t error;
    double power_w,voltage_v,current_a;
    uint32_t id;
    bool has_battery,has_temperature,has_humidity,has_position,has_contact,contact_open,can_cover;
    double battery_pct,temperature_c,humidity_pct,position_pct;
    bool has_frequency,can_stop_cover;
    double frequency_hz;
    dsr_mobile_cover_state_t cover_state;
    bool has_sample_time;
    uint32_t sample_ms;
    uint8_t fields;
} dsr_mobile_device_t;
typedef struct {char name[48];uint8_t icon;char photo[33];uint32_t id;} dsr_mobile_room_t;
typedef struct {char name[48];uint8_t actions;} dsr_mobile_scene_t;
typedef struct {uint8_t type,index,span,height,fields;} dsr_mobile_tile_t;
typedef struct {
    bool demo,wifi;
    char home_name[32];
    uint8_t device_count,room_count;
    dsr_mobile_device_t devices[DSR_UI_MAX_DEVICES];
    dsr_mobile_room_t rooms[DSR_UI_MAX_ROOMS];
    char dashboard_name[48];
    uint32_t accent;
    bool configured_layout,midnight;
    uint8_t tile_count;
    dsr_mobile_tile_t tiles[24];
    uint8_t scene_count;
    dsr_mobile_scene_t scenes[8];
    uint32_t dashboard_id;
} dsr_mobile_model_t;
typedef struct {
    void (*set_output)(unsigned index,bool on);
    void (*open_settings)(void);
    void (*add_device)(void);
    void (*edit_dashboard)(void);
    void (*set_cover)(unsigned index,int direction);
    void (*select_dashboard)(void);
    void (*refresh)(void);
    void (*edit_device)(unsigned index);
    void (*stop_cover)(unsigned index);
} dsr_mobile_actions_t;

void dsr_mobile_bind(const dsr_mobile_actions_t *actions);
void dsr_mobile_update(const dsr_mobile_model_t *model);
void dsr_mobile_show(dsr_mobile_page_t page,unsigned index);
void dsr_mobile_hide(void);
/* Explicit, isolated on-device design preview. Never calls network actions. */
void dsr_mobile_cover_preview(void);
bool dsr_mobile_active(void);
dsr_mobile_page_t dsr_mobile_page(void);
bool dsr_mobile_configure(dsr_mobile_model_t *model,const char *json);
#endif
