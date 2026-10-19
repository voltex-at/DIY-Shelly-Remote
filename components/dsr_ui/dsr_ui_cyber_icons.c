/* Original user-supplied Shelly HUD pack, CC0-1.0. A8, no PNG/SVG decoder.
 * Only the referenced symbol/size pairs are imported into cyber_icons/.
 * Several UI roles share the same descriptor, including plug/outlet. */
#include "dsr_ui_cyber_icons.h"
#include "cyber_icons/shelly_hud_icons.h"

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
 case DSR_ICON_WINDOW_CLOSED:return size>=48?&shud_window_closed_48:&shud_window_closed_24;
 case DSR_ICON_WINDOW_OPEN:return size>=48?&shud_window_open_48:&shud_window_open_24;
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
 default:return NULL;
 }
}
