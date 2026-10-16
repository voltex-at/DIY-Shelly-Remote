#include "dsr_ui_internal.h"
#include "dsr_native_ota.h"
#include "dsr_platform.h"
#include "dsr_ui_mobile.h"
#include "dsr_history.h"
#include "dsr_workspace.h"
#include "dsr_board.h"
#include "dsr_activity.h"
#include "dsr_fleet.h"
#include "esp_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dsr_settings_t settings;
static dsr_settings_t wifi_candidate;
static dsr_connection_t pending;
static dsr_fleet_t fleet;
_Static_assert(sizeof(fleet)<=8192,"Fleet fixed storage budget");
static bool busy,wifi_previous;
static dsr_job_kind_t operation;
static uint32_t serial,outstanding;
static unsigned active_slot;
static bool finishing_bind,bind_lease;
static dsr_result_t bind_transport_result;
static dsr_bind_stage_t bind_stage;
static bool storage_error;
static bool external_bind;
static bool lan_binding;
static uint32_t pending_room=UINT32_MAX;
static dsr_mobile_model_t mobile_model;
static lv_obj_t *form, *keyboard, *message, *fields[5],*endpoint_choice;
static enum { VIEW_NONE, VIEW_HOME, VIEW_DEVICE } view;

static const char *result_text(dsr_result_t result)
{
    switch (result) {
    case DSR_OK: return "Ready";
    case DSR_AUTH_REQUIRED: return "Authentication failed. Check login.";
    case DSR_RATE_LIMITED: return "Shelly limits logins. Wait, then Refresh.";
    case DSR_CONFLICT: return "Configuration changed; reload and retry adding the device";
    case DSR_IDENTITY_MISMATCH: return "Different device at this IP. Add again.";
    case DSR_OFFLINE: return "Shelly offline / Wi-Fi disconnected";
    case DSR_UNSUPPORTED: return "Not a supported relay/switch channel";
    case DSR_BAD_RESPONSE: return "Invalid response / state unconfirmed";
    case DSR_FULL: return "Limit: 16 saved Shelly endpoints";
    case DSR_BUSY: return "Operation in progress. Please wait.";
    case DSR_IO: return "Storage or memory error";
    default: return "Check IP, channel and field lengths";
    }
}

const char *dsr_network_bind_result_text(dsr_result_t result,dsr_bind_stage_t stage)
{
 if(stage==DSR_BIND_WORKSPACE)return result==DSR_CONFLICT?"Urządzenie zweryfikowane. Konfiguracja zmieniła się. Dodaj ponownie.":"Urządzenie zweryfikowane. Zapis konfiguracji niepotwierdzony. Dodaj ponownie.";
 if(stage==DSR_BIND_PRIVATE)return result==DSR_CONFLICT?"Konfiguracja zmieniła się przed zapisem powiązania. Dodaj ponownie.":"Konfiguracja zapisana. Nowe powiązanie niepotwierdzone. Dodaj ponownie.";
 return result_text(result);
}
void dsr_network_reset_view(void)
{
    dsr_ui_settings_hide();
    dsr_ui_battery_hide();dsr_ui_display_hide();
    dsr_ui_history_hide();
    dsr_wifi_ui_reset();
    dsr_ui_lan_reset();
    dsr_ui_ap_reset();
    dsr_ui_ble_reset();
    dsr_ui_native_ota_hide();
    dsr_ui_ble_rpc_reset();
    form = keyboard = message = endpoint_choice = NULL;
    memset(fields, 0, sizeof(fields)); view = VIEW_NONE;
}

bool dsr_network_layout_reserved(uint32_t id)
{for(size_t i=0;i<fleet.count;i++)if(id&&fleet.slots[i].connection.layout_id==id)return true;return false;}
static int card_slot(unsigned index)
{
 if(index>=mobile_model.device_count)return -1;
 const dsr_mobile_device_t *card=&mobile_model.devices[index];
 for(size_t i=0;i<fleet.count;i++){
  const dsr_connection_t *c=&fleet.slots[i].connection;
  if(!c->layout_id&&!c->legacy_fallback)continue;
  if((c->device.endpoint_kind==DSR_ENDPOINT_COVER)!=(card->kind==DSR_DEVICE_COVER))continue;
  if(c->device.channel!=card->channel||strcmp(c->device.host,card->host))continue;
  if(c->layout_id?c->layout_id==card->id:!dsr_network_layout_reserved(card->id))return (int)i;
 }
 return -1;
}
static bool binding_target(const dsr_fleet_slot_t *s)
{
 if(!(finishing_bind||(busy&&operation==DSR_JOB_BIND)))return false;
 const dsr_device_t *d=&s->connection.device;
 return d->endpoint_kind==pending.device.endpoint_kind&&(!pending.device.generation||d->generation==pending.device.generation)&&d->channel==pending.device.channel&&(pending.device.device_id[0]?!strcmp(d->device_id,pending.device.device_id):!strcmp(d->host,pending.device.host));
}
static void publish_runtime(uint32_t now)
{
 for(size_t i=0;i<fleet.count;i++)dsr_history_binding(&fleet.slots[i].connection,fleet.slots[i].layout_enabled);
 size_t visible=0;for(size_t i=0;i<fleet.count;i++)if(fleet.slots[i].connection.layout_id||fleet.slots[i].connection.legacy_fallback)visible++;
 if(!dsr_workspace_runtime_publish_begin(visible))return;
 size_t out=0;for(size_t i=0;i<fleet.count;i++){
  if(!fleet.slots[i].connection.layout_id&&!fleet.slots[i].connection.legacy_fallback)continue;
  const dsr_fleet_slot_t *s=&fleet.slots[i];
  dsr_runtime_device_t r={.device=s->connection.device,.telemetry=s->connection.telemetry,.error=s->result,
   .pending=binding_target(s)||s->queued||s->mailbox_pending||s->urgent_stop||(s->in_flight&&(s->operation==DSR_JOB_SET_OUTPUT||s->operation==DSR_JOB_COVER)),.stop_pending=s->urgent_stop||s->mailbox_pending==2,
   .can_stop=s->connection.device.endpoint_kind==DSR_ENDPOINT_COVER&&s->layout_enabled&&dsr_wifi_connected()&&!binding_target(s)&&!s->blocked&&!s->urgent_stop&&s->mailbox_pending!=2,.has_sample_time=s->sample_valid,
   .sample_age_ms=s->sample_valid?now-s->sample_ms:0,.operation_id=s->command_id};
  r.device.online=r.device.online&&s->layout_enabled&&dsr_wifi_connected()&&s->sample_valid&&now-s->sample_ms<=15000;
  if(!r.device.online){r.device.has_power=false;memset(&r.telemetry,0,sizeof(r.telemetry));}
  dsr_workspace_runtime_publish_at(out++,visible,&r);
 }
 dsr_workspace_runtime_publish_end();
}
static void render(void)
{
 if(view!=VIEW_HOME)return;
 memset(&mobile_model,0,sizeof(mobile_model));mobile_model.wifi=dsr_wifi_connected();
 snprintf(mobile_model.home_name,sizeof(mobile_model.home_name),"Mój dom");
 char *configuration=dsr_workspace_snapshot();bool layout=configuration&&dsr_mobile_configure(&mobile_model,configuration);free(configuration);
 if(!layout){
  for(size_t i=0;i<fleet.count;i++){
   if(!fleet.slots[i].connection.layout_id&&!fleet.slots[i].connection.legacy_fallback)continue;
   dsr_mobile_device_t *d=&mobile_model.devices[mobile_model.device_count++];const dsr_device_t *c=&fleet.slots[i].connection.device;
   snprintf(d->name,sizeof(d->name),"%s",c->name);snprintf(d->host,sizeof(d->host),"%s",c->host);d->id=fleet.slots[i].connection.layout_id;
   snprintf(d->model,sizeof(d->model),"Shelly • kanał %u",c->channel);d->channel=c->channel;d->room=DSR_UI_NO_ROOM;d->kind=c->endpoint_kind==DSR_ENDPOINT_COVER?DSR_DEVICE_COVER:DSR_DEVICE_RELAY;
  }
 }
 for(unsigned i=0;i<mobile_model.device_count;i++){
  int at=card_slot(i);if(at<0)continue;
  dsr_mobile_device_t *d=&mobile_model.devices[i];const dsr_fleet_slot_t *s=&fleet.slots[at];const dsr_device_t *c=&s->connection.device;
  snprintf(d->identity,sizeof(d->identity),"%s",c->device_id);d->generation=c->generation;d->endpoint_kind=c->endpoint_kind;
  d->can_refresh=(d->id||(!layout&&mobile_model.device_count==1))&&s->layout_enabled&&!binding_target(s);
  snprintf(d->host,sizeof(d->host),"%s",c->host);d->online=mobile_model.wifi&&c->online&&s->layout_enabled&&s->sample_valid&&lv_tick_get()-s->sample_ms<=15000;d->output=c->output;
  d->busy=d->command_pending=binding_target(s)||s->queued||s->mailbox_pending||s->urgent_stop||(s->in_flight&&(s->operation==DSR_JOB_SET_OUTPUT||s->operation==DSR_JOB_COVER));
  d->has_sample_time=s->sample_valid;d->sample_ms=s->sample_ms;d->error=s->result;
  if(c->endpoint_kind==DSR_ENDPOINT_COVER){const dsr_cover_status_t *cover=&s->connection.telemetry.cover;
   d->can_cover=dsr_cover_can_move(cover,(dsr_cover_intent_t){.command=DSR_COVER_OPEN});
   d->can_position=dsr_cover_can_move(cover,(dsr_cover_intent_t){.command=DSR_COVER_GOTO,.position=0});
   d->can_stop_cover=mobile_model.wifi&&s->layout_enabled&&!binding_target(s)&&!s->blocked&&!s->urgent_stop&&s->mailbox_pending!=2;
   d->has_position=d->online&&(cover->flags&DSR_COVER_POSITION);d->position_pct=cover->current_pos;d->cover_errors=cover->errors;
   d->cover_state=cover->state==DSR_COVER_STATE_OPENING?DSR_COVER_OPENING:cover->state==DSR_COVER_STATE_CLOSING?DSR_COVER_CLOSING:cover->state==DSR_COVER_STATE_CALIBRATING?DSR_COVER_CALIBRATING:cover->state?DSR_COVER_STOPPED:DSR_COVER_UNKNOWN;
  }
  d->can_switch=c->has_switch&&!s->blocked&&s->layout_enabled;d->has_power=d->online&&c->has_power;d->power_w=c->power_w;
  d->has_voltage=d->online&&s->connection.telemetry.has_voltage;d->voltage_v=s->connection.telemetry.voltage_v;
  d->has_current=d->online&&s->connection.telemetry.has_current;d->current_a=s->connection.telemetry.current_a;
  d->has_energy=d->online&&s->connection.telemetry.has_energy;d->energy_wh=s->connection.telemetry.energy_wh;
 }
 dsr_mobile_update(&mobile_model);
}
static dsr_result_t submit(dsr_job_kind_t kind,bool on)
{
 if(busy||finishing_bind||dsr_worker_stop_pending())return DSR_BUSY;
 if(kind==DSR_JOB_BIND)for(size_t i=0;i<fleet.count;i++)if(fleet.slots[i].queued||fleet.slots[i].mailbox_pending||fleet.slots[i].urgent_stop)return DSR_BUSY;
 dsr_job_t job={.id=++serial,.kind=kind,.connection=pending,.settings=settings,.on=on};
 dsr_result_t result=dsr_worker_submit(&job);
 if(result==DSR_OK){busy=true;operation=kind;outstanding=job.id;if(kind==DSR_JOB_BIND){bind_stage=DSR_BIND_VERIFY;bind_lease=dsr_activity_begin();}}
 memset(&job,0,sizeof(job));render();return result;
}
static bool dispatch(uint32_t now,bool allow_poll)
{
 if(busy||dsr_worker_stop_pending()||!dsr_wifi_connected())return false;
 dsr_job_t job;unsigned index;
 if(!dsr_fleet_next(&fleet,now,allow_poll,&job,&index))return false;
 job.id=++serial;dsr_result_t result=dsr_worker_submit(&job);
 if(result==DSR_OK){busy=true;operation=job.kind;outstanding=job.id;active_slot=index;dsr_fleet_started(&fleet,index,&job);render();}
 memset(&job,0,sizeof(job));return true;
}
static bool dispatch_history(uint32_t now)
{
 if(busy||finishing_bind||dsr_worker_stop_pending())return false;
 bool clock=dsr_history_clock_pending();if(!clock&&!dsr_history_service_due(now))return false;
 dsr_job_t job={.id=++serial,.kind=clock?DSR_JOB_HISTORY_CLOCK:DSR_JOB_HISTORY_SERVICE};
 bool ok=dsr_worker_submit(&job)==DSR_OK;if(ok){busy=true;operation=job.kind;outstanding=job.id;}return ok;
}
bool dsr_network_ap_home(char ssid[33])
{if(ssid)snprintf(ssid,33,"%s",settings.ssid);return dsr_wifi_settings_valid(&settings)&&dsr_wifi_connected();}
dsr_result_t dsr_network_request_ap(dsr_job_kind_t kind,const dsr_wifi_ap_t *ap,const dsr_ap_identity_t *identity,const char *host,bool confirmed)
{
 if(busy||finishing_bind||dsr_worker_stop_pending()||kind<DSR_JOB_AP_SCAN||kind>DSR_JOB_AP_VERIFY)return DSR_BUSY;
 dsr_job_t job={.id=++serial,.kind=kind,.settings=settings,.ap_confirmed=confirmed};
 if(ap)job.ap=*ap;
 if(identity)job.ap_identity=*identity;
 if(host)snprintf(job.connection.device.host,sizeof(job.connection.device.host),"%s",host);
 dsr_result_t result=dsr_worker_submit(&job);if(result==DSR_OK){busy=true;operation=kind;outstanding=job.id;}memset(&job,0,sizeof(job));return result;
}
dsr_result_t dsr_network_request_station(const dsr_lan_device_t *identity)
{
 if(!identity||busy||finishing_bind||dsr_worker_stop_pending())return DSR_BUSY;
 dsr_job_t job={.id=++serial,.kind=DSR_JOB_LAN_STA_VERIFY,.settings=settings};
 snprintf(job.ap_identity.id,sizeof(job.ap_identity.id),"%s",identity->id);
 snprintf(job.ap_identity.mac,sizeof(job.ap_identity.mac),"%s",identity->mac);
 job.ap_identity.generation=identity->generation;job.ap_identity.auth_required=identity->auth_required;
 snprintf(job.connection.device.host,sizeof(job.connection.device.host),"%s",identity->host);
 dsr_result_t result=dsr_worker_submit(&job);if(result==DSR_OK){busy=true;operation=job.kind;outstanding=job.id;}memset(&job,0,sizeof(job));return result;
}
bool dsr_network_busy(void) { return busy||finishing_bind||dsr_worker_stop_pending(); }
bool dsr_network_ota_busy(void){return busy&&(operation==DSR_JOB_OTA_CHECK||operation==DSR_JOB_OTA_INSTALL);}
dsr_result_t dsr_network_request_ota(dsr_job_kind_t kind,unsigned channel,bool power_confirmed)
{
 if(busy||finishing_bind||dsr_worker_stop_pending()||dsr_ui_ap_active()||dsr_ui_ble_rpc_active()||dsr_ble_rpc_active()||dsr_wifi_temporary_active())return DSR_BUSY;
 dsr_job_t job={.id=++serial,.kind=kind};
 if(!job.id)job.id=++serial;
 dsr_result_t result=dsr_native_ota_prepare(job.id,kind,(dsr_ota_channel_t)channel,power_confirmed);
 if(result!=DSR_OK)return result;
 result=dsr_worker_submit(&job);
 if(result==DSR_OK){busy=true;operation=kind;outstanding=job.id;}
 else dsr_native_ota_reject(job.id);
 return result;
}
dsr_result_t dsr_network_request_display(uint8_t brightness,uint16_t seconds)
{
 if(busy)return DSR_BUSY;
 dsr_job_t job={.id=++serial,.kind=DSR_JOB_DISPLAY_SAVE};
 job.settings.brightness=brightness;job.settings.idle_seconds=seconds;
 dsr_result_t accepted=dsr_worker_submit(&job);
 if(accepted==DSR_OK){busy=true;operation=DSR_JOB_DISPLAY_SAVE;outstanding=job.id;}
 return accepted;
}
dsr_result_t dsr_network_request_battery(void)
{
 if(busy)return DSR_BUSY;
 const dsr_job_t job={.id=++serial,.kind=DSR_JOB_BATTERY_READ};
 dsr_result_t accepted=dsr_worker_submit(&job);
 if(accepted==DSR_OK){busy=true;operation=DSR_JOB_BATTERY_READ;outstanding=job.id;}
 return accepted;
}
void dsr_network_wifi_settings(dsr_settings_t *out) { *out = settings; }
dsr_result_t dsr_network_request_wifi(dsr_job_kind_t kind, const dsr_settings_t *candidate)
{
    if (busy || (kind != DSR_JOB_WIFI_SCAN && kind != DSR_JOB_WIFI_CONNECT)) return DSR_BUSY;
    if (candidate) wifi_candidate = *candidate;
    dsr_job_t job = {.id = ++serial, .kind = kind, .settings = wifi_candidate};
    dsr_result_t result = dsr_worker_submit(&job);
    if (result == DSR_OK) { busy = true; operation = kind; outstanding = job.id; }
    memset(&job, 0, sizeof(job)); return result;
}

void dsr_network_home(lv_event_t *event)
{
    (void)event;
    dsr_ui_workspace_hide();
    dsr_network_reset_view(); view = VIEW_HOME;
    render(); dsr_mobile_show(DSR_MOBILE_HOME, 0);
}
void dsr_network_rooms(void)
{
    dsr_network_home(NULL); /* Re-read the published shared configuration first. */
    dsr_mobile_show(DSR_MOBILE_ROOMS,0);
}
void dsr_network_room_context(uint32_t id)
{
    dsr_network_home(NULL); /* Re-read published IDs; never reuse a room index. */
    for(unsigned i=0;i<mobile_model.room_count;i++)if(mobile_model.rooms[i].id==id){dsr_mobile_show(DSR_MOBILE_ROOM,i);return;}
    dsr_mobile_show(DSR_MOBILE_ROOMS,0);
}
void dsr_network_device_settings(uint32_t id)
{
    dsr_network_home(NULL); /* Fresh shared layout; never reuse the old index. */
    dsr_mobile_show_device_settings(id);
}
void dsr_network_device_control(uint32_t id)
{dsr_network_home(NULL);dsr_mobile_return_device(id);}
bool dsr_network_energy_at(unsigned index,dsr_mobile_device_t *out)
{if(!out||index>=mobile_model.device_count)return false;*out=mobile_model.devices[index];return true;}
bool dsr_network_history_device(uint32_t id,dsr_mobile_device_t *out)
{for(unsigned i=0;i<mobile_model.device_count;i++)if(id&&mobile_model.devices[i].id==id)return dsr_network_energy_at(i,out);return false;}

static void mobile_output(unsigned index,bool on)
{
 int at=card_slot(index);if(at<0)return;
 dsr_result_t result=binding_target(&fleet.slots[at])?DSR_BUSY:dsr_fleet_request(&fleet,(unsigned)at,on,dsr_wifi_connected(),lv_tick_get());
 if(result!=DSR_OK){fleet.slots[at].result=result;fleet.slots[at].command_error=true;}
 dispatch(lv_tick_get(),false);render();publish_runtime(lv_tick_get());
}
static void mobile_cover_send(unsigned index,dsr_cover_intent_t intent)
{
 if(index>=mobile_model.device_count)return;
 uint32_t id=mobile_model.devices[index].id;render();
 index=0;while(index<mobile_model.device_count&&mobile_model.devices[index].id!=id)index++;
 int at=card_slot(index);if(at<0||binding_target(&fleet.slots[at]))return;
 uint32_t operation_id;dsr_result_t result=dsr_workspace_cover_request(&fleet.slots[at].connection.device,intent,&operation_id);
 if(result==DSR_OK)fleet.slots[at].mailbox_pending=intent.command==DSR_COVER_STOP?2:1;
 else {fleet.slots[at].result=result;fleet.slots[at].command_error=true;}
 render();
}
static void mobile_cover(unsigned index,int direction){mobile_cover_send(index,(dsr_cover_intent_t){.command=direction>0?DSR_COVER_OPEN:DSR_COVER_CLOSE});}
static void mobile_stop(unsigned index){mobile_cover_send(index,(dsr_cover_intent_t){.command=DSR_COVER_STOP});}
static void mobile_position(unsigned index,uint8_t position){mobile_cover_send(index,(dsr_cover_intent_t){.command=DSR_COVER_GOTO,.position=position});}
static void mobile_add(void){dsr_network_device(NULL);}
static void mobile_edit_device(uint32_t id)
{if(id)dsr_ui_edit_local_device(id);}
static void mobile_refresh(uint32_t id)
{
 render(); /* Re-resolve the shared stable ID after a concurrent Web edit. */
 /* A manual read belongs to the selected bound endpoint, including when it
  * is offline. It must not clear another endpoint's auth/rate/command error. */
 /* Preserve the old read-only single-binding screen while shared storage is
  * unavailable. A zero ID never selects among multiple fallback endpoints. */
 if(!id&&(mobile_model.configured_layout||mobile_model.device_count!=1))return;
 unsigned index=0;while(index<mobile_model.device_count&&mobile_model.devices[index].id!=id)index++;
 int at=card_slot(index);if(at<0)return;
 dsr_fleet_slot_t *s=&fleet.slots[at];
 if(!s->layout_enabled||binding_target(s)||s->queued||s->mailbox_pending||s->urgent_stop||(s->in_flight&&(s->operation==DSR_JOB_SET_OUTPUT||s->operation==DSR_JOB_COVER)))return;
 s->blocked=s->command_error=false;s->result=DSR_OK;s->failures=0;s->next_poll=lv_tick_get();
 /* Existing in-flight GET is already the requested read. Queued SETs still
  * keep scheduler priority; ordinary polling keeps its original cursor. */
 if(!busy&&!finishing_bind){unsigned cursor=fleet.cursor;fleet.cursor=(unsigned)at;(void)dispatch(lv_tick_get(),true);if(!busy)fleet.cursor=cursor;}
 render();
}
bool dsr_network_endpoint_added(const char *id,dsr_generation_t generation,unsigned channel)
{for(size_t i=0;i<fleet.count;i++){const dsr_device_t *d=&fleet.slots[i].connection.device;if(!strcmp(d->device_id,id)&&d->generation==generation&&(channel>DSR_MAX_CHANNEL||d->channel==channel))return true;}return false;}
static void hide_keyboard(lv_event_t *event)
{
    (void)event;
    if (!keyboard || !form) return;
    lv_keyboard_set_textarea(keyboard, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_height(form, 300);
}

static void focus_field(lv_event_t *event)
{
    lv_obj_t *field = lv_event_get_target(event);
    if (!keyboard || !form) return;
    lv_obj_set_height(form, 121);
    dsr_ui_keyboard_focus(keyboard,field);
    lv_obj_scroll_to_view(field, LV_ANIM_OFF);
}

static lv_obj_t *field(unsigned index, const char *caption, const char *text, unsigned max, bool secret)
{
    int y = (int)index * 82;
    dsr_ui_label(form, caption, 0, y, 268);
    lv_obj_t *obj = lv_textarea_create(form);
    dsr_ui_textarea(obj);
    lv_obj_set_pos(obj, 0, y + 24); lv_obj_set_size(obj, 268, 44);
    lv_textarea_set_one_line(obj, true);
    lv_textarea_set_max_length(obj, max);
    lv_textarea_set_password_mode(obj, secret);
    lv_textarea_set_text(obj, text);
    lv_obj_add_event_cb(obj, focus_field, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj, focus_field, LV_EVENT_CLICKED, NULL);
    fields[index] = obj;
    return obj;
}

static void form_start(const char *title)
{
    lv_obj_t *root = dsr_ui_screen(title, true);
    message = dsr_ui_label(root, "Enter the device address and login.", 16, 77, 288);
    form = lv_obj_create(root); lv_obj_set_pos(form, 12, 115); lv_obj_set_size(form, 296, 300);
    lv_obj_set_style_pad_all(form, 8, 0); lv_obj_set_style_border_width(form, 0, 0);
    lv_obj_set_style_bg_color(form, dsr_sc_color(DSR_SC_BG), 0);
    lv_obj_set_style_text_color(form, lv_color_hex(0xf0f4fb), 0);
    keyboard = lv_keyboard_create(root); lv_obj_set_size(keyboard, 320, 210);
    dsr_ui_keyboard(keyboard);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(keyboard, hide_keyboard, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(keyboard, hide_keyboard, LV_EVENT_CANCEL, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

static bool copy_field(char *dest, size_t capacity, unsigned index)
{
    const char *text = lv_textarea_get_text(fields[index]);
    if (strlen(text) >= capacity) return false; /* UTF-8 byte limit, not character count */
    snprintf(dest, capacity, "%s", text); return true;
}

static void bind_device(lv_event_t *event)
{
    (void)event;
    if (busy||finishing_bind||dsr_worker_stop_pending()) { dsr_sc_label_set_text(message, result_text(DSR_BUSY)); return; }
    if (!dsr_wifi_connected()) { dsr_sc_label_set_text(message, "Connect Wi-Fi first"); return; }
    memset(&pending, 0, sizeof(pending));
    pending_room=UINT32_MAX;lan_binding=false;
    const char *channel = lv_textarea_get_text(fields[1]);
    char *end = NULL; long value = strtol(channel, &end, 10);
    if (!channel[0] || !end || *end || value < 0 || value > DSR_MAX_CHANNEL ||
        !copy_field(pending.device.host, sizeof(pending.device.host), 0) ||
        !copy_field(pending.device.name, sizeof(pending.device.name), 2) ||
        !copy_field(pending.credentials.username, sizeof(pending.credentials.username), 3) ||
        !copy_field(pending.credentials.password, sizeof(pending.credentials.password), 4)) {
        dsr_sc_label_set_text(message, "Invalid details. Channel range: 0-15"); return;
    }
    pending.device.channel = (uint8_t)value;
    if(endpoint_choice&&lv_dropdown_get_selected(endpoint_choice)==1){pending.device.endpoint_kind=DSR_ENDPOINT_COVER;pending.device.generation=DSR_GEN2_PLUS;}
    dsr_result_t result = submit(DSR_JOB_BIND, false);
    dsr_sc_label_set_text(message, result == DSR_OK ? "Checking device before saving..." : result_text(result));
}

static void manual_device(lv_event_t *event)
{
    (void)event; form_start("Zaawansowane • adres IP"); view = VIEW_DEVICE;
    char channel[4]="0";
    lv_obj_t *ip = field(0, "IPv4 address (same LAN)", "", 15, false);
    lv_textarea_set_accepted_chars(ip, "0123456789.");
    lv_obj_t *ch = field(1, "Kanał API (pierwszy to 0)", channel, 2, false);
    lv_textarea_set_accepted_chars(ch, "0123456789");
    field(2, "Name (optional)", "", 47, false);
    field(3, "Login (Gen2+: admin)", "admin", 32, false);
    field(4, "Shelly password (optional)", "", 64, true);
    dsr_ui_label(form,"Rodzina API",0,420,268);
    endpoint_choice=lv_dropdown_create(form);lv_obj_set_pos(endpoint_choice,0,444);lv_obj_set_size(endpoint_choice,268,44);lv_dropdown_set_options(endpoint_choice,"Switch / Relay\nCover Gen2+");
    dsr_ui_button(form, "Check & save device", 0, 508, 268, bind_device, NULL);
    dsr_ui_label(form, "Do 16 endpointów Switch/Cover.\nCover wymaga Gen2+ i aktywnego profilu rolety.\nZmiana profilu nie jest wykonywana.\nDane logowania w niezaszyfrowanym NVS.", 0, 572, 268);
}

void dsr_network_device(lv_event_t *event)
{
    (void)event;lv_obj_t *root=dsr_ui_screen("Dodaj urządzenie Shelly",true);
    dsr_ui_button(root,"Skanuj sieć Wi-Fi",12,96,296,dsr_ui_lan_open,NULL);
    dsr_ui_button(root,"Wi-Fi AP",12,152,296,dsr_ui_ap_open,NULL);
    dsr_ui_button(root,"Bluetooth LE",12,208,296,dsr_ui_ble_open,NULL);
    dsr_ui_button(root,"Dodaj ręcznie (IP)",12,264,296,manual_device,NULL);
    dsr_ui_label(root,"LAN, fabryczne Wi-Fi AP i skan BLE są dostępne.\nParowanie BLE nie jest jeszcze wdrożone.",16,329,288);
}
dsr_result_t dsr_network_bind_verified(const dsr_connection_t *connection,uint32_t room)
{
    if(!connection||busy||finishing_bind||dsr_worker_stop_pending())return DSR_BUSY;
    if(!dsr_wifi_connected())return DSR_OFFLINE;
    pending=*connection;pending_room=room;dsr_result_t result=submit(DSR_JOB_BIND,false);
    if(result==DSR_OK)lan_binding=true;else{memset(&pending,0,sizeof(pending));pending_room=UINT32_MAX;}
    return result;
}

/* Only a successfully parsed shared document can authorize publication or
 * targeted unbinding. An unavailable store never means an empty workspace. */
static cJSON *layout_snapshot(void)
{char *json=dsr_workspace_snapshot();cJSON *r=json?cJSON_Parse(json):NULL;free(json);char error[96];if(!r||!dsr_config_validate(r,error)){cJSON_Delete(r);return NULL;}return r;}
static uint32_t layout_find(const cJSON *r,const dsr_connection_t *c,bool *exists)
{
 *exists=false;const cJSON *v;cJSON_ArrayForEach(v,cJSON_GetObjectItemCaseSensitive(r,"devices")){
  const cJSON *id=cJSON_GetObjectItemCaseSensitive(v,"id"),*host=cJSON_GetObjectItemCaseSensitive(v,"host"),*channel=cJSON_GetObjectItemCaseSensitive(v,"channel");
  if(c->layout_id&&(uint32_t)id->valueint!=c->layout_id)continue;
  if(c->layout_id)*exists=true;
  const cJSON *kind=cJSON_GetObjectItemCaseSensitive(v,"kind");if(!cJSON_IsString(kind)||(!strcmp(kind->valuestring,"cover"))!=(c->device.endpoint_kind==DSR_ENDPOINT_COVER))continue;
  if(channel->valueint==c->device.channel&&!strcmp(host->valuestring,c->device.host)&&
    (c->layout_id||!dsr_network_layout_reserved((uint32_t)id->valueint)))return (uint32_t)id->valueint;
 }
 return 0;
}
static dsr_result_t maintenance_submit(dsr_job_kind_t kind,unsigned index,uint32_t id)
{
 if(dsr_worker_stop_pending())return DSR_BUSY;
 dsr_job_t job={.id=++serial,.kind=kind,.connection=fleet.slots[index].connection};job.connection.layout_id=id;
 dsr_result_t result=dsr_worker_submit(&job);if(result==DSR_OK){busy=true;operation=kind;outstanding=job.id;active_slot=index;}
 memset(&job,0,sizeof(job));return result;
}
static void reconcile_layout(bool allow_storage)
{
 cJSON *r=layout_snapshot();if(!r)return;
 for(unsigned i=0;i<fleet.count;i++){
  dsr_fleet_slot_t *s=&fleet.slots[i];bool exists=false;uint32_t id=layout_find(r,&s->connection,&exists);
  if(s->connection.layout_id){s->layout_enabled=id!=0;
   if(allow_storage&&(int32_t)(lv_tick_get()-s->next_maintenance)>=0&&!exists&&!busy&&!finishing_bind&&!s->queued&&!s->mailbox_pending&&!s->in_flight&&!s->urgent_stop){(void)maintenance_submit(DSR_JOB_UNBIND,i,s->connection.layout_id);break;}
  }else if(s->connection.legacy_fallback&&allow_storage&&(int32_t)(lv_tick_get()-s->next_maintenance)>=0&&id&&!busy&&!finishing_bind){(void)maintenance_submit(DSR_JOB_LAYOUT_PUBLISH,i,id);break;}
 }
 cJSON_Delete(r);
}
static void queue_verified_migrations(void)
{
 /* Stage workspace locators without mutating live siblings or their secrets. */
 for(size_t i=0;i<fleet.count;i++){
  const dsr_connection_t *saved=&fleet.slots[i].connection;
  if(!fleet.slots[i].layout_enabled||dsr_endpoint_equal(&saved->device,&pending.device))continue;
  if(saved->device.generation==pending.device.generation&&!strcmp(saved->device.device_id,pending.device.device_id)){
   dsr_connection_t sibling=*saved;memcpy(sibling.device.host,pending.device.host,sizeof(sibling.device.host));dsr_ui_workspace_migrate_bound(&sibling,UINT32_MAX,NULL);memset(&sibling,0,sizeof(sibling));
  }
 }
 dsr_ui_workspace_migrate_bound(&pending,pending_room,NULL);
}
static dsr_result_t publish_verified(void)
{
 if(dsr_worker_stop_pending())return DSR_BUSY;
 dsr_job_t job={.id=++serial,.kind=DSR_JOB_LAYOUT_PUBLISH,.connection=pending};
 dsr_result_t result=dsr_worker_submit(&job);if(result==DSR_OK){busy=true;operation=job.kind;outstanding=job.id;}
 memset(&job,0,sizeof(job));return result;
}
static void finish_bind(dsr_result_t result)
{
 if(result!=DSR_OK&&bind_stage==DSR_BIND_WORKSPACE)dsr_ui_workspace_migrate_cancel(&pending.device);
 finishing_bind=false;
 if(bind_lease){bind_lease=false;dsr_activity_end();}
 if(lan_binding){if(result!=DSR_OK)dsr_ui_lan_bind_result(result,bind_stage);lan_binding=false;}
 if(external_bind){dsr_workspace_bind_result_stage(result,bind_stage);external_bind=false;}
 else if(result==DSR_OK)dsr_network_home(NULL);
 if(message&&result!=DSR_OK)dsr_sc_label_set_text(message,dsr_network_bind_result_text(result,bind_stage));
 pending_room=UINT32_MAX;memset(&pending,0,sizeof(pending));
}
/* Keep the secret-free STOP snapshot in a separate bounded frame. The main
 * LVGL poll is already close to the pinned target's 2048-byte frame budget. */
static void __attribute__((noinline)) consume_stop(uint32_t now,bool outer_pending)
{
 dsr_stop_result_t stop;
 if(dsr_worker_stop_receive(&stop,outer_pending)){
  int at=dsr_fleet_find(&fleet,&stop.device);if(at>=0){dsr_fleet_slot_t *s=&fleet.slots[at];
   if(s->urgent_stop&&s->command_id==stop.operation_id&&s->connection.layout_id==stop.layout_id&&!strcmp(s->connection.device.host,stop.device.host)){
    s->urgent_stop=false;s->result=stop.result;s->command_error=stop.result!=DSR_OK;s->blocked=stop.result==DSR_AUTH_REQUIRED||stop.result==DSR_IDENTITY_MISMATCH||stop.result==DSR_RATE_LIMITED;
    s->connection.device=stop.device;s->connection.telemetry=stop.telemetry;if(stop.result==DSR_OK){s->sample_valid=true;s->sample_ms=now;}s->next_poll=now+1000;
   }
  }render();
 }
}
static void poll(lv_timer_t *timer)
{
 (void)timer;if(!dsr_activity_begin())return;uint32_t now=lv_tick_get();dsr_event_t event;
 if(dsr_worker_receive(&event)&&event.id==outstanding){
  busy=false;
  if(operation==DSR_JOB_UNBIND||operation==DSR_JOB_LAYOUT_PUBLISH){
   if(finishing_bind&&operation==DSR_JOB_LAYOUT_PUBLISH){if(!dsr_endpoint_equal(&pending.device,&event.device)||strcmp(pending.device.host,event.device.host)||pending.layout_id!=event.layout_id)event.result=DSR_IDENTITY_MISMATCH;}
   else {dsr_fleet_slot_t *s=&fleet.slots[active_slot];
    if(!dsr_endpoint_equal(&s->connection.device,&event.device)||strcmp(s->connection.device.host,event.device.host))event.result=DSR_IDENTITY_MISMATCH;
    s->next_maintenance=event.result==DSR_OK?0:now+5000;
   }
  }
  if(operation==DSR_JOB_OTA_CHECK||operation==DSR_JOB_OTA_INSTALL){/* Fixed synchronized updater snapshot is consumed by its LCD tick. */}
  else if(operation==DSR_JOB_LAN_STA_VERIFY)dsr_ui_ble_rpc_result(&event);
  else if(operation>=DSR_JOB_AP_SCAN&&operation<=DSR_JOB_AP_VERIFY)dsr_ui_ap_result(&event,operation);
  else if(operation==DSR_JOB_DISPLAY_SAVE){if(event.result==DSR_OK){settings.brightness=event.brightness;settings.idle_seconds=event.idle_seconds;}dsr_ui_display_result(&event);}
  else if(operation==DSR_JOB_BATTERY_READ)dsr_ui_battery_result(&event);
  else if(operation==DSR_JOB_WIFI_SCAN||operation==DSR_JOB_WIFI_CONNECT){
   if(operation==DSR_JOB_WIFI_CONNECT&&event.result==DSR_OK){settings=wifi_candidate;storage_error=false;dsr_fleet_wifi(&fleet,false,now);}
   dsr_wifi_ui_result(&event,operation);
  }else if(operation==DSR_JOB_UNBIND){
   if(event.result==DSR_OK){dsr_history_forget(&fleet.slots[active_slot].connection.device);memset(&fleet.slots[active_slot].connection.credentials,0,sizeof(dsr_credentials_t));memmove(&fleet.slots[active_slot],&fleet.slots[active_slot+1],(fleet.count-active_slot-1)*sizeof(fleet.slots[0]));memset(&fleet.slots[--fleet.count],0,sizeof(fleet.slots[0]));}
  }else if(operation==DSR_JOB_LAYOUT_PUBLISH){
   if(finishing_bind){
    cJSON *r=layout_snapshot();bool exists=false;bool valid=r&&layout_find(r,&pending,&exists)==pending.layout_id;cJSON_Delete(r);
    dsr_result_t result=event.result;if(result==DSR_OK&&!valid)result=DSR_CONFLICT;
    if(result==DSR_OK){pending.device=event.device;pending.telemetry=event.telemetry;pending.legacy_fallback=false;result=dsr_fleet_upsert(&fleet,&pending,now);}
    finish_bind(result);
   }else if(event.result==DSR_OK){fleet.slots[active_slot].connection.layout_id=event.layout_id;fleet.slots[active_slot].connection.legacy_fallback=false;}
  }else if(operation==DSR_JOB_BIND){
   bind_transport_result=event.result;
   if(event.result==DSR_OK){
    pending.device=event.device;pending.telemetry=event.telemetry;pending.legacy_fallback=false;
    int old=dsr_fleet_find(&fleet,&pending.device);pending.layout_id=old>=0?fleet.slots[old].connection.layout_id:0;
    if(old<0&&fleet.count>=DSR_MAX_DEVICES)bind_transport_result=DSR_FULL;
    else {bind_stage=DSR_BIND_WORKSPACE;queue_verified_migrations();finishing_bind=true;}
   }
   if(bind_transport_result!=DSR_OK)finish_bind(bind_transport_result);
  }else if(operation==DSR_JOB_HISTORY_SERVICE||operation==DSR_JOB_HISTORY_CLOCK){
   /* History/SNTP events are not endpoint samples. */
  }else {
   bool accepted=dsr_fleet_complete(&fleet,active_slot,&event,now);
   /* A Wi-Fi drop during the request cannot publish a late ONLINE sample. */
   if(!dsr_wifi_connected())dsr_fleet_wifi(&fleet,false,now);
   if(accepted&&event.result==DSR_OK&&fleet.slots[active_slot].layout_enabled&&fleet.slots[active_slot].connection.device.online)dsr_history_record(&fleet.slots[active_slot].connection,event.id);
  }
  render();
 }
 bool wifi=dsr_wifi_connected();if(wifi!=wifi_previous){wifi_previous=wifi;dsr_fleet_wifi(&fleet,wifi,now);render();}
 dsr_wifi_ui_tick();dsr_ui_lan_tick();dsr_ui_ap_tick();dsr_ui_ble_tick();dsr_ui_ble_rpc_tick();dsr_ui_battery_tick();dsr_ui_settings_tick();dsr_ui_native_ota_tick();dsr_ui_workspace_tick();dsr_panel_tick();dsr_ui_history_tick();
 uint32_t handoff_now=(uint32_t)(esp_timer_get_time()/1000);
 bool ap_available=view==VIEW_HOME&&!dsr_ui_ble_rpc_active()&&!dsr_ui_ap_active()&&dsr_network_ap_home(NULL);
 dsr_workspace_ap_publish(ap_available,dsr_ui_ap_active(),handoff_now);
 if(ap_available&&!busy&&!finishing_bind&&!dsr_worker_stop_pending()&&dsr_workspace_take_ap_open(handoff_now))dsr_ui_ap_open(NULL);
 bool ble_available=view==VIEW_HOME&&!dsr_ui_ap_active()&&!dsr_ui_ble_rpc_active()&&dsr_network_ap_home(NULL);
 dsr_ble_handoff_publish(ble_available,dsr_ui_ble_rpc_active(),handoff_now);
 dsr_ble_device_t candidate;
 if(ble_available&&!busy&&!finishing_bind&&!dsr_worker_stop_pending()&&dsr_ble_handoff_take(&candidate,handoff_now))dsr_ui_ble_rpc_open(&candidate);
 if(finishing_bind&&!busy){
  dsr_result_t result;bool pending_layout=dsr_ui_workspace_migrate_status(&result);
  if(!pending_layout){
   if(result!=DSR_OK)finish_bind(result);
   else {cJSON *r=layout_snapshot();bool exists=false;uint32_t id=r?layout_find(r,&pending,&exists):0;cJSON_Delete(r);
    if(!id)finish_bind(DSR_CONFLICT);else {pending.layout_id=id;bind_stage=DSR_BIND_PRIVATE;dsr_result_t admitted=publish_verified();if(admitted!=DSR_OK&&admitted!=DSR_BUSY)finish_bind(admitted);}
   }
  }
 }
 consume_stop(now,busy);
 dsr_output_request_t request;bool intent_changed=false;
 while(dsr_workspace_take_output(&request)){
  int at=dsr_fleet_find(&fleet,&request.device);
  if(at>=0){dsr_fleet_slot_t *s=&fleet.slots[at];s->mailbox_pending=0;dsr_result_t result;
   if(binding_target(s)||!s->layout_enabled||strcmp(request.device.host,s->connection.device.host))result=DSR_IDENTITY_MISMATCH;
   else if(request.device.endpoint_kind==DSR_ENDPOINT_COVER&&request.cover.command==DSR_COVER_STOP){
    result=s->blocked?s->result:dsr_worker_stop_request(&s->connection,request.operation_id);
    if(result==DSR_OK){dsr_fleet_cancel_queued(&fleet,(unsigned)at);s->urgent_stop=true;}
   }else result=request.device.endpoint_kind==DSR_ENDPOINT_COVER?dsr_fleet_cover_request(&fleet,(unsigned)at,request.cover,wifi,now):dsr_fleet_request(&fleet,(unsigned)at,request.on,wifi,now);
   s->command_id=request.operation_id;if(result!=DSR_OK){s->result=result;s->command_error=true;}intent_changed=true;
  }
 }
 if(intent_changed)render();
 if(!busy&&!finishing_bind&&!dsr_worker_stop_pending()){dsr_connection_t connection;uint32_t room;if(dsr_workspace_take_bind_with_room(&connection,&room)){
  pending=connection;pending_room=room;memset(&connection,0,sizeof(connection));dsr_result_t result=submit(DSR_JOB_BIND,false);
  if(result==DSR_OK)external_bind=true;else{memset(&pending,0,sizeof(pending));dsr_workspace_bind_result(result);}
 }}
 static uint32_t config_revision;uint32_t next_revision=dsr_workspace_revision();if(config_revision!=next_revision){config_revision=next_revision;render();}
 reconcile_layout(false);
 bool attempted=dispatch(now,false);
 if(!busy)reconcile_layout(true);
 if(dispatch_history(handoff_now))attempted=true;
 if(!attempted)(void)dispatch(now,!dsr_wifi_ui_active()&&!dsr_ui_lan_active()&&!dsr_ui_ap_active());
 /* Freshness can expire while the sole transport is still waiting. Update
  * the LCD once at that transition, rather than rebuilding it every tick. */
 if(view==VIEW_HOME)for(unsigned i=0;i<mobile_model.device_count;i++){
  int at=card_slot(i);if(at<0)continue;const dsr_fleet_slot_t *s=&fleet.slots[at];
  bool online=wifi&&s->layout_enabled&&s->connection.device.online&&s->sample_valid&&now-s->sample_ms<=15000;
  if(mobile_model.devices[i].online!=online){render();break;}
 }
 publish_runtime(now);dsr_activity_end();
}

void dsr_network_start_view(void)
{
    if (!settings.ssid[0]) dsr_network_wifi(NULL);
    else dsr_network_home(NULL);
}

void dsr_network_init(void)
{
    (void)dsr_history_init();
    const dsr_mobile_actions_t actions = {.set_cover=mobile_cover,.stop_cover=mobile_stop,.set_position=mobile_position,.set_output = mobile_output, .open_settings = dsr_ui_open_settings, .add_device = mobile_add,.edit_dashboard=dsr_ui_edit_dashboard,.select_dashboard=dsr_ui_select_dashboard,.refresh=mobile_refresh,.edit_device=mobile_edit_device,.add_room=dsr_ui_add_room,.edit_room=dsr_ui_edit_room,.open_charts=dsr_ui_history_open,.open_energy=dsr_ui_energy_open};
    dsr_mobile_bind(&actions);
    storage_error = dsr_settings_load(&settings) != ESP_OK;
    dsr_ui_display_init(settings.brightness,settings.idle_seconds);
    dsr_connection_t *loaded=calloc(DSR_MAX_DEVICES,sizeof(*loaded));size_t count=0;
    if(!loaded||dsr_connections_load(loaded,DSR_MAX_DEVICES,&count)!=ESP_OK)storage_error=true;
    if(loaded){for(size_t i=0;i<count;i++){(void)dsr_fleet_upsert(&fleet,&loaded[i],lv_tick_get()-5000);if(!loaded[i].layout_id&&loaded[i].legacy_fallback)dsr_ui_workspace_migrate(&loaded[i]);}memset(loaded,0,DSR_MAX_DEVICES*sizeof(*loaded));free(loaded);}
    lv_timer_create(poll,200,NULL);
}
