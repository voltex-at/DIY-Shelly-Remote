#ifndef DSR_UI_MOBILE_H
#define DSR_UI_MOBILE_H
#include <stdbool.h>
#include <stdint.h>

#define DSR_UI_MAX_DEVICES 16
#define DSR_UI_MAX_ROOMS 8
#define DSR_UI_NO_ROOM 255
typedef enum {DSR_MOBILE_HOME,DSR_MOBILE_ROOMS,DSR_MOBILE_DEVICES,DSR_MOBILE_ROOM,DSR_MOBILE_DETAILS} dsr_mobile_page_t;
typedef enum {DSR_DEVICE_LIGHT,DSR_DEVICE_PLUG,DSR_DEVICE_RELAY} dsr_mobile_kind_t;
typedef struct {
    char name[48],model[40],host[16];
    uint8_t room;dsr_mobile_kind_t kind;
    bool online,output,busy,can_switch,has_power,has_voltage,has_current;
    double power_w,voltage_v,current_a;
} dsr_mobile_device_t;
typedef struct {char name[32];uint8_t icon;} dsr_mobile_room_t;
typedef struct {
    bool demo,wifi;
    char home_name[32];
    uint8_t device_count,room_count;
    dsr_mobile_device_t devices[DSR_UI_MAX_DEVICES];
    dsr_mobile_room_t rooms[DSR_UI_MAX_ROOMS];
} dsr_mobile_model_t;
typedef struct {
    void (*set_output)(unsigned index,bool on);
    void (*open_settings)(void);
    void (*add_device)(void);
} dsr_mobile_actions_t;

void dsr_mobile_bind(const dsr_mobile_actions_t *actions);
void dsr_mobile_update(const dsr_mobile_model_t *model);
void dsr_mobile_show(dsr_mobile_page_t page,unsigned index);
void dsr_mobile_hide(void);
bool dsr_mobile_active(void);
dsr_mobile_page_t dsr_mobile_page(void);
#endif
