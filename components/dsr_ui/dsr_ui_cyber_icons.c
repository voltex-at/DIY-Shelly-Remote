/* Original user-supplied Shelly HUD pack, CC0-1.0. A8, no PNG/SVG decoder.
 * Only the referenced symbol/size pairs are imported into cyber_icons/.
 * Several UI roles share the same descriptor, including plug/outlet. */
#include "dsr_ui_cyber_icons.h"
#include "cyber_icons/shelly_hud_icons.h"
#include "cyber_icons/shelly_hud_extra.h"
#include "cyber_icons/shelly_hud_blu.h"


const lv_image_dsc_t *dsr_ui_cyber_icon(dsr_ui_icon_t kind,unsigned size)
{
 switch(kind){
 case DSR_ICON_DASHBOARD:return &shud_dashboard_24;
 case DSR_ICON_HOME:return size>=48?&shud_home_48:size>=29?&shud_home_32:&shud_home_24;
 case DSR_ICON_ENERGY:return size>=29?&shud_chart_32:&shud_chart_24;
 case DSR_ICON_SETTINGS:return &shud_settings_24;
 case DSR_ICON_PLUS:return &shud_plus_24;
 case DSR_ICON_POWER:return size>=48?&shud_power_48:&shud_power_24;
 case DSR_ICON_BACK:return &shud_back_24;
 case DSR_ICON_EDIT:return &shud_edit_24;
 case DSR_ICON_INFO:return size>=29?&shud_info_32:&shud_info_24;
 case DSR_ICON_PLUG:case DSR_ICON_OUTLET:return size>=29?&shud_plug_32:&shud_plug_24;
 case DSR_ICON_LIGHT:return size>=29?&shud_lightbulb_32:&shud_lightbulb_24;
 case DSR_ICON_WIFI:return size>=29?&shud_wifi_32:&shud_wifi_24;
 case DSR_ICON_GRID:return size>=48?&shud_grid_48:&shud_grid_24;
 case DSR_ICON_CHEVRON:return &shud_chevron_down_24;
 case DSR_ICON_UP:return &shud_shutter_up_24;
 case DSR_ICON_DOWN:return &shud_shutter_down_24;
 case DSR_ICON_CONTACT:return size>=48?&shud_sensor_48:size>=29?&shud_sensor_32:&shud_sensor_24;
 case DSR_ICON_COVER:return size>=29?&shud_shutter_32:&shud_shutter_24;
 case DSR_ICON_BATTERY:return &shud_battery_24;
 case DSR_ICON_SCENES:return size>=48?&shud_layers_48:&shud_layers_32;
 case DSR_ICON_THERMOSTAT:return size>=29?&shud_thermometer_32:&shud_thermometer_24;
 case DSR_ICON_TV:return size>=29?&shud_display_32:&shud_display_24;
 case DSR_ICON_DOOR:return size>=29?&shud_door_32:&shud_door_24;
 case DSR_ICON_AC:return size>=29?&shud_snowflake_32:&shud_snowflake_24;
 case DSR_ICON_ROUTER:return size>=29?&shud_access_point_32:&shud_access_point_24;
 case DSR_ICON_WINDOW_CLOSED:return size>=64?&shudb_window_closed_80:size>=40?&shudb_window_closed_48:size>=29?&shudb_window_closed_32:&shudb_window_closed_24;
 case DSR_ICON_WINDOW_OPEN:return size>=64?&shudb_window_open_80:size>=40?&shudb_window_open_48:size>=29?&shudb_window_open_32:&shudb_window_open_24;
 case DSR_ICON_BLUETOOTH:return &shud_bluetooth_24;
 case DSR_ICON_REFRESH:return &shud_refresh_24;
 case DSR_ICON_CPU:return size>=29?&shud_cpu_32:&shud_cpu_24;
 case DSR_ICON_GIT_BRANCH:return &shud_git_branch_24;
 case DSR_ICON_PALETTE:return &shud_palette_24;
 case DSR_ICON_DISPLAY:return &shud_display_24;
 case DSR_ICON_SHIELD:return &shud_shield_24;
 case DSR_ICON_NEXT:return &shud_chevron_right_24;
 case DSR_ICON_STOP:return &shud_stop_24;
 case DSR_ICON_SAVE:return &shud_save_24;
 case DSR_ICON_TRASH:return &shud_trash_24;
 case DSR_ICON_WARNING:return &shud_warning_24;
 case DSR_ICON_ACTIVITY:return &shud_activity_24;
 case DSR_ICON_MEMORY:return &shud_memory_24;
 case DSR_ICON_POWER_SUPPLY:return &shud_battery_charging_48;
 case DSR_ICON_SOFA:return size>=48?&shudx_sofa_48:size>=29?&shudx_sofa_32:&shudx_sofa_24;
 case DSR_ICON_BED:return size>=48?&shudx_bed_48:size>=29?&shudx_bed_32:&shudx_bed_24;
 case DSR_ICON_KITCHEN:return size>=48?&shudx_kitchen_48:size>=29?&shudx_kitchen_32:&shudx_kitchen_24;
 case DSR_ICON_BATH:return size>=48?&shudx_bathroom_48:size>=29?&shudx_bathroom_32:&shudx_bathroom_24;
 case DSR_ICON_FAN:return size>=48?&shudx_fan_48:size>=29?&shudx_fan_32:&shudx_fan_24;
 case DSR_ICON_BOILER:return size>=48?&shudx_boiler_48:size>=29?&shudx_boiler_32:&shudx_boiler_24;
 case DSR_ICON_GARAGE:return size>=48?&shudx_garage_48:size>=29?&shudx_garage_32:&shudx_garage_24;
 case DSR_ICON_FLOOR_LAMP:return size>=48?&shudx_floor_lamp_48:size>=29?&shudx_floor_lamp_32:&shudx_floor_lamp_24;
 case DSR_ICON_WALL_LAMP:return size>=48?&shudx_wall_lamp_48:size>=29?&shudx_wall_lamp_32:&shudx_wall_lamp_24;
 case DSR_ICON_EV:return size>=48?&shudx_ev_charger_48:size>=29?&shudx_ev_charger_32:&shudx_ev_charger_24;
 case DSR_ICON_KETTLE:return size>=48?&shudx_kettle_48:size>=29?&shudx_kettle_32:&shudx_kettle_24;
 case DSR_ICON_COFFEE:return size>=48?&shudx_coffee_machine_48:size>=29?&shudx_coffee_machine_32:&shudx_coffee_machine_24;
 case DSR_ICON_LAUNDRY:return size>=48?&shudx_washing_machine_48:size>=29?&shudx_washing_machine_32:&shudx_washing_machine_24;
 case DSR_ICON_SPEAKER:return size>=48?&shudx_speaker_48:size>=29?&shudx_speaker_32:&shudx_speaker_24;
 case DSR_ICON_DOOR_CLOSED:return size>=64?&shudb_door_closed_80:size>=40?&shudb_door_closed_48:size>=29?&shudb_door_closed_32:&shudb_door_closed_24;
 case DSR_ICON_DOOR_OPEN:return size>=64?&shudb_door_open_80:size>=40?&shudb_door_open_48:size>=29?&shudb_door_open_32:&shudb_door_open_24;
 case DSR_ICON_VALVE:return size>=48?&shudx_valve_48:size>=29?&shudx_valve_32:&shudx_valve_24;
 case DSR_ICON_WINDOW_TILTED:return size>=64?&shudb_window_tilted_80:size>=40?&shudb_window_tilted_48:size>=29?&shudb_window_tilted_32:&shudb_window_tilted_24;
 case DSR_ICON_WINDOW_NO_DATA:return size>=64?&shudb_window_no_data_80:size>=40?&shudb_window_no_data_48:size>=29?&shudb_window_no_data_32:&shudb_window_no_data_24;
 case DSR_ICON_DOOR_NO_DATA:return size>=64?&shudb_door_no_data_80:size>=40?&shudb_door_no_data_48:size>=29?&shudb_door_no_data_32:&shudb_door_no_data_24;
 case DSR_ICON_ILLUMINANCE:return &shudb_illuminance_24;
 case DSR_ICON_SIGNAL:return &shudb_signal_24;
 case DSR_ICON_CLOCK:return &shudb_clock_24;
 case DSR_ICON_ECO:return &shudx_eco_32;
 case DSR_ICON_LOCATION:return &shudx_location_32;
 case DSR_ICON_TRV_VALVE:return &shudx_trv_valve_32;
 default:return NULL;
 }
}
