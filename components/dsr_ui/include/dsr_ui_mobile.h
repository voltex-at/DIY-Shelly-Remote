#ifndef DSR_UI_MOBILE_H
#define DSR_UI_MOBILE_H
#include <stdbool.h>
#include <stdint.h>
#include "dsr_core.h"
#include "dsr_actions.h"

#define DSR_UI_MAX_DEVICES 16
#define DSR_UI_MAX_ROOMS 8
#define DSR_UI_NO_ROOM 255
typedef enum {DSR_MOBILE_HOME,DSR_MOBILE_ROOMS,DSR_MOBILE_DEVICES,DSR_MOBILE_ROOM,DSR_MOBILE_DETAILS,DSR_MOBILE_DEVICE_SETTINGS,DSR_MOBILE_GROUPS,DSR_MOBILE_SCENES,DSR_MOBILE_GROUP_CONTROL} dsr_mobile_page_t;
typedef enum {DSR_DEVICE_LIGHT,DSR_DEVICE_PLUG,DSR_DEVICE_RELAY,DSR_DEVICE_COVER,DSR_DEVICE_CONTACT,DSR_DEVICE_SENSOR,DSR_DEVICE_RGB} dsr_mobile_kind_t;
/* Confirmed Cover status only; zero means no status sample, never stopped. */
typedef enum {DSR_COVER_UNKNOWN,DSR_COVER_OPENING,DSR_COVER_CLOSING,DSR_COVER_STOPPED,DSR_COVER_CALIBRATING} dsr_mobile_cover_state_t;
typedef struct {
    char name[48],model[40],host[16];
    uint8_t channel,physical_generation;
    uint8_t room,icon; /* icon: 0 = by type, 1..DSR_CONFIG_DEVICE_ICONS = chosen (0.7.51, existing padding) */
    dsr_mobile_kind_t kind;
    bool online,output,busy,can_switch,has_power,has_voltage,has_current;
    bool command_pending;
    dsr_result_t error;
    double power_w,voltage_v,current_a;
    uint32_t id;
    bool has_battery,has_temperature,has_humidity,has_position,has_contact,contact_open,can_cover;
    double battery_pct,temperature_c,humidity_pct;
    union {double position_pct;double blu_age_s;}; /* BLU: seconds since the gateway received the sensor, <0 unknown */
    union {bool has_frequency;bool has_illuminance;bool has_trv_target;};bool can_stop_cover;
    union {double frequency_hz;double illuminance_lux;double trv_target_c;}; /* BLU TRV: gateway-reported target */
    dsr_mobile_cover_state_t cover_state;
    bool has_sample_time;
    uint32_t sample_ms;
    uint8_t fields;
    /* Only supplied for the actually bound transport; never inferred from a card. */
    char identity[64];
    char fw[16];
    dsr_generation_t generation;
    bool can_refresh,can_position,has_energy;
    uint8_t cover_errors,endpoint_kind; /* Actual bound transport family, for typed queries. */
    union {bool can_light;bool can_rgb;};
    union {dsr_light_status_t light;dsr_rgb_status_t rgb;dsr_ht_status_t ht;dsr_blu_status_t blu;}; /* Existing alignment padding: model size is unchanged. */
    double energy_wh;
} dsr_mobile_device_t;
typedef struct {char name[48];uint8_t icon;char photo[33];uint32_t id;} dsr_mobile_room_t;
typedef struct {char name[48];uint32_t id;uint8_t actions;} dsr_mobile_scene_t;
typedef struct {char name[48];uint32_t id;uint8_t members,room;uint16_t member_mask;} dsr_mobile_group_t;
_Static_assert(sizeof(dsr_mobile_group_t)==56,"group membership uses existing mobile padding");
/* Actual private endpoint family, 255 when unbound/read-only. */
uint8_t dsr_mobile_control_family(const dsr_mobile_device_t *device);
typedef struct {uint8_t type,index,span,height,fields;} dsr_mobile_tile_t;
typedef struct {
    bool demo,wifi;
    char home_name[32];
    uint8_t device_count,room_count;
    dsr_mobile_device_t devices[DSR_UI_MAX_DEVICES];
    dsr_mobile_room_t rooms[DSR_UI_MAX_ROOMS];
    char dashboard_name[48];
    uint32_t accent;
    bool configured_layout;
    uint8_t tile_count;
    dsr_mobile_tile_t tiles[24];
    uint8_t scene_count;
    dsr_mobile_scene_t scenes[8];
    uint8_t group_count;
    dsr_mobile_group_t groups[8];
    uint32_t dashboard_id;
} dsr_mobile_model_t;
typedef struct {
    void (*set_output)(unsigned index,bool on);
    void (*open_settings)(void);
    void (*add_device)(void);
    void (*edit_dashboard)(void);
    void (*set_cover)(unsigned index,int direction);
    void (*select_dashboard)(void);
    void (*refresh)(uint32_t id);
    void (*edit_device)(uint32_t id);
    void (*stop_cover)(unsigned index);
    void (*set_position)(unsigned index,uint8_t position);
    void (*add_room)(void);
    void (*edit_room)(uint32_t id);
    void (*open_usage)(uint32_t id);
    void (*open_battery)(void);
    void (*open_actions)(uint8_t kind);
    void (*leave_actions)(void);
    void (*edit_action)(uint8_t kind,uint32_t id);
    void (*run_scene)(uint32_t id);
    void (*run_group)(uint32_t id,bool on);
    void (*set_group)(uint32_t id,dsr_group_intent_t intent);
    void (*set_brightness)(unsigned index,uint8_t brightness);
    void (*set_rgb)(unsigned index,uint8_t r,uint8_t g,uint8_t b);
    void (*set_trv)(unsigned index,uint8_t half_c); /* BLU TRV target, half degrees */
    void (*device_settings)(uint32_t id);
} dsr_mobile_actions_t;

void dsr_mobile_bind(const dsr_mobile_actions_t *actions);
void dsr_mobile_update(const dsr_mobile_model_t *model);
void dsr_mobile_show(dsr_mobile_page_t page,unsigned index);
void dsr_mobile_show_device_settings(uint32_t id);
void dsr_mobile_return_device(uint32_t id);
void dsr_mobile_open_group(uint32_t id);
void dsr_mobile_hide(void);
/* Explicit, isolated on-device design preview. Never calls network actions. */
void dsr_mobile_cover_preview(void);
bool dsr_mobile_active(void);
dsr_mobile_page_t dsr_mobile_page(void);
bool dsr_mobile_configure(dsr_mobile_model_t *model,const char *json);
/* LVGL-owner-only borrowed document, already accepted by dsr_config_validate.
 * No mutation, retained pointers or ownership transfer; raw JSON callers use
 * dsr_mobile_configure so validation cannot be skipped at that boundary. */
struct cJSON;
void dsr_mobile_configure_validated(dsr_mobile_model_t *model,const struct cJSON *document);
#endif
