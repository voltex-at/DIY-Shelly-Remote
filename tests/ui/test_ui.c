/* Actual LVGL + actual UI source. Only platform/radio are fixture providers.
 * Screenshots are rendered pixels, not HTML mockups or simulated device data
 * included in firmware. */
#include "dsr_ui.h"
#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_board.h"
#include "dsr_workspace.h"
#include "dsr_ble.h"
#include "esp_app_desc.h"
#include "src/widgets/buttonmatrix/lv_buttonmatrix_private.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static unsigned screenshots,requests;
void test_workspace_enable(void);
void test_workspace_disable(void);
void test_workspace_defer(bool enabled);
void test_workspace_finish(bool success);
void test_workspace_photos(bool enabled);
void test_workspace_fault(unsigned fault);
void test_panel_ready(bool ready);
void test_lan_found(const char *identity,const char *host);
void test_lan_inspected(dsr_result_t result,const char *components);
void test_ble_packet(const uint8_t *bytes,size_t length,unsigned address);
void test_ble_finish(dsr_ble_phase_t phase);
#define CHECK(x) do {checks++; if (!(x)) {fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); abort();}} while(0)
static dsr_job_t submitted;
static dsr_event_t response;
static bool in_flight, result_ready;
static dsr_result_t next_submit_result;
static dsr_settings_t stored = {.brightness = 70, .idle_seconds = 60};
static dsr_wifi_info_t wifi;
static unsigned char canvas[320 * 480 * 3];
static uint16_t draw_buffer[320 * 40];
static lv_display_t *display;
static const char *output;
static lv_indev_t *pointer;
static lv_point_t touch;
static bool pressed;
static unsigned ready_count;
static void read_pointer(lv_indev_t *input,lv_indev_data_t *data)
{(void)input;data->point=touch;data->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;}
static void tap(int x,int y)
{touch=(lv_point_t){x,y};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);pressed=false;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();lv_obj_update_layout(lv_screen_active());}
static void ready(lv_event_t *event){(void)event;ready_count++;}
static void key(lv_obj_t *kb,const char *text)
{
 lv_obj_update_layout(kb);const lv_buttonmatrix_t *matrix=(const lv_buttonmatrix_t *)kb;lv_area_t area;lv_obj_get_coords(kb,&area);
 for(unsigned i=0;i<matrix->btn_cnt;i++)if(!strcmp(lv_keyboard_get_button_text(kb,i),dsr_sc_tr(text))){
  lv_area_t b=matrix->button_areas[i];tap(area.x1+(b.x1+b.x2)/2,area.y1+(b.y1+b.y2)/2);return;
 }CHECK(!"Keyboard key missing");
}

const esp_app_desc_t *esp_app_get_description(void) {static const esp_app_desc_t d = {.version="0.3.0-wifi-dev"}; return &d;}
esp_err_t dsr_board_set_backlight(uint8_t value) {(void)value; return ESP_OK;}
void dsr_board_sleep_display(void) {}
bool dsr_board_touch_available(void) {return true;}
esp_err_t dsr_settings_load(dsr_settings_t *out) {*out=stored; return ESP_OK;}
esp_err_t dsr_connection_load(dsr_connection_t *out, bool *found) {memset(out,0,sizeof(*out)); *found=false; return ESP_OK;}
bool dsr_wifi_connected(void) {return wifi.connected;}
void dsr_wifi_get_info(dsr_wifi_info_t *out) {*out=wifi;}
void dsr_wifi_cancel_connect(void) {CHECK(submitted.kind==DSR_JOB_WIFI_CONNECT);}
dsr_result_t dsr_worker_submit(const dsr_job_t *job)
{if(next_submit_result!=DSR_OK){dsr_result_t result=next_submit_result;next_submit_result=DSR_OK;return result;}CHECK(!in_flight);requests++;submitted=*job; in_flight=true; return DSR_OK;}
bool dsr_worker_receive(dsr_event_t *out)
{if(!result_ready)return false; *out=response; result_ready=false; in_flight=false; return true;}

static void flush(lv_display_t *d,const lv_area_t *area,uint8_t *data)
{
    const uint16_t *pixels=(const uint16_t *)data;
    for(int y=area->y1;y<=area->y2;y++)for(int x=area->x1;x<=area->x2;x++) {
        uint16_t p=*pixels++;
        if(x<0||x>=320||y<0||y>=480)continue;
        unsigned at=(unsigned)(y*320+x)*3;
        canvas[at]=(unsigned char)(((p>>11)&31)*255/31);
        canvas[at+1]=(unsigned char)(((p>>5)&63)*255/63);
        canvas[at+2]=(unsigned char)((p&31)*255/31);
    }
    lv_display_flush_ready(d);
}
static void tick(void) {lv_tick_inc(250);lv_timer_handler();lv_refr_now(display);}
static void screenshot(const char *name)
{
    lv_obj_update_layout(lv_screen_active());lv_refr_now(display);
    lv_obj_t *badge=NULL;
    if(strtoul(name,NULL,10)>=50){badge=dsr_sc_panel(lv_layer_top(),226,8,84,23,0x5c492a,5);dsr_sc_text(badge,"TEST / DEMO",4,5,76,&dsr_roboto_11,0xffd68a);}
    lv_obj_update_layout(lv_screen_active());lv_obj_invalidate(lv_screen_active());lv_refr_now(display);
    if(badge){unsigned at=(20*320+230)*3;CHECK(canvas[at]>60&&canvas[at+1]>40&&canvas[at+2]<70);}
    char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",output,name);
    FILE *f=fopen(path,"wb");CHECK(f!=NULL);fprintf(f,"P6\n320 480\n255\n");
    CHECK(fwrite(canvas,1,sizeof(canvas),f)==sizeof(canvas));fclose(f);screenshots++;if(badge)lv_obj_delete(badge);
}
static lv_obj_t *find_label(lv_obj_t *root,const char *text)
{
    if(lv_obj_check_type(root,&lv_label_class)&&!strcmp(lv_label_get_text(root),dsr_sc_tr(text)))return root;
    for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *f=find_label(lv_obj_get_child(root,(int)i),text);if(f)return f;}
    return NULL;
}
static lv_obj_t *find_type(lv_obj_t *root,const lv_obj_class_t *type)
{
    if(lv_obj_check_type(root,type))return root;
    for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *f=find_type(lv_obj_get_child(root,(int)i),type);if(f)return f;}
    return NULL;
}
static void click(const char *text)
{
    lv_obj_t *label=find_label(lv_screen_active(),text);if(!label)fprintf(stderr,"Missing UI label: %s\n",text);CHECK(label!=NULL);
    (void)lv_obj_send_event(lv_obj_get_parent(label),LV_EVENT_CLICKED,NULL);
    lv_obj_update_layout(lv_screen_active());
}
static lv_obj_t *power_target(lv_obj_t *root)
{
 if(lv_obj_get_width(root)==86&&lv_obj_get_height(root)==86&&lv_obj_has_flag(root,LV_OBJ_FLAG_CLICKABLE))return root;
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *o=power_target(lv_obj_get_child(root,(int)i));if(o)return o;}return NULL;
}
static lv_obj_t *nth_type(lv_obj_t *root,const lv_obj_class_t *type,unsigned *skip)
{
    if(lv_obj_check_type(root,type)){if(!*skip)return root;(*skip)--;}
    for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *o=nth_type(lv_obj_get_child(root,(int)i),type,skip);if(o)return o;}return NULL;
}
static void deliver(dsr_result_t result,dsr_wifi_status_t detail)
{response.id=submitted.id;response.result=result;response.wifi_detail=detail;result_ready=true;tick();}
static cJSON *room_document(void)
{char *json=dsr_workspace_snapshot();CHECK(json);cJSON *r=cJSON_Parse(json);free(json);CHECK(r);return r;}
static void publish_document(cJSON *r)
{char *json=cJSON_PrintUnformatted(r);CHECK(json);CHECK(dsr_workspace_submit(json,dsr_workspace_revision()));free(json);cJSON_Delete(r);}
static void room_name(const char *name)
{lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_textarea_set_text(field,name);}
static void room_delete_button(void)
{
 lv_obj_t *label=find_label(lv_screen_active(),"Usuń pomieszczenie");CHECK(label);
 lv_obj_t *button=lv_obj_get_parent(label);lv_obj_scroll_to_view(button,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());
 lv_area_t a;lv_obj_get_coords(button,&a);CHECK(a.x1>=0&&a.x2<320&&a.y1>=118&&a.y2<420);CHECK(lv_obj_get_height(button)>=44);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static unsigned json_allocations,json_fail_at;
static void *json_alloc(size_t size){json_allocations++;return json_fail_at&&json_allocations==json_fail_at?NULL:malloc(size);}
static void storage_error_regression(void)
{
 test_workspace_disable();test_workspace_enable();wifi.connected=false;tick();
 dsr_config_health_storage(DSR_STORAGE_NONEMPTY,ESP_FAIL);dsr_config_health_workspace(DSR_WORKSPACE_MOUNTING,ESP_FAIL);
 unsigned before=requests;
 for(unsigned fault=1;fault<=4;fault++)for(unsigned surface=0;surface<3;surface++){
  test_workspace_fault(0);dsr_network_rooms();test_workspace_fault(fault);
  tap(surface==2?280:160,surface==0?116:surface==1?380:450);
  CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));CHECK(find_label(lv_screen_active(),"Nie można otworzyć edytora"));CHECK(!find_label(lv_screen_active(),"Zapisz"));
  if(fault==1&&surface==0){screenshot("120-storage-unavailable-room");click("Diagnostyka");CHECK(find_label(lv_screen_active(),"SPIFFS: niepusty, niezamontowany"));CHECK(find_label(lv_screen_active(),"Nie kasuj i nie formatuj pamięci."));screenshot("121-storage-read-only-diagnostics");}
  test_workspace_fault(0);click("Wstecz");CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);
 }
 for(unsigned fault=1;fault<=4;fault++){
  dsr_network_home(NULL);test_workspace_fault(fault);tap(224,116);CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));if(fault==1)screenshot("122-storage-pencil-error");test_workspace_fault(0);click("Wstecz");CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);
 }
 /* Actual cJSON allocator failures: parse, duplicate, new object and field. */
 cJSON_Hooks hooks={.malloc_fn=json_alloc,.free_fn=free};char *json=dsr_workspace_snapshot();CHECK(json);cJSON_InitHooks(&hooks);json_fail_at=json_allocations=0;cJSON *parsed=cJSON_Parse(json);CHECK(parsed);unsigned parse_count=json_allocations;json_allocations=0;cJSON *duplicate=cJSON_Duplicate(parsed,true);CHECK(duplicate);unsigned duplicate_count=json_allocations;cJSON_Delete(parsed);cJSON_Delete(duplicate);cJSON_InitHooks(NULL);free(json);
 unsigned faults[]={1,parse_count+1,parse_count+duplicate_count+1,parse_count+duplicate_count+2};
 for(unsigned i=0;i<4;i++){
  dsr_network_rooms();json_allocations=0;json_fail_at=faults[i];cJSON_InitHooks(&hooks);tap(160,116);cJSON_InitHooks(NULL);json_fail_at=0;
  CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));CHECK(!find_label(lv_screen_active(),"Zapisz"));if(i==1)screenshot("123-storage-baseline-memory-error");click("Wstecz");
 }
 /* All three real pointer targets still open the form with healthy storage. */
 dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);dsr_config_health_workspace(DSR_WORKSPACE_READY,ESP_OK);
 for(unsigned surface=0;surface<3;surface++){dsr_network_rooms();tap(surface==2?280:160,surface==0?116:surface==1?380:450);CHECK(find_label(lv_screen_active(),"Nowe pomieszczenie"));click("Wstecz");}
 dsr_network_home(NULL);tap(224,116);CHECK(find_label(lv_screen_active(),"Edytuj pilot"));test_panel_ready(false);click("Web Panel • parowanie");CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));CHECK(!find_label(lv_screen_active(),"0123456789abcdef"));screenshot("124-storage-https-unavailable");click("Diagnostyka");CHECK(find_label(lv_screen_active(),"HTTPS: błąd uruchomienia"));test_panel_ready(true);click("Wstecz");
 dsr_ui_edit_dashboard();click("Web Panel • parowanie");CHECK(find_label(lv_screen_active(),"0123456789abcdef"));click("Wstecz");click("Wstecz");CHECK(requests==before);test_workspace_disable();
 test_workspace_enable();
 for(unsigned language=1;language<3;language++){
  cJSON *r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(language==1?"en":"de"));publish_document(r);dsr_network_rooms();test_workspace_fault(1);tap(160,116);CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));screenshot(language==1?"125-storage-error-english":"127-storage-error-german");click("Diagnostyka");CHECK(find_label(lv_screen_active(),"Nie kasuj i nie formatuj pamięci."));screenshot(language==1?"126-storage-diagnostics-english":"128-storage-diagnostics-german");test_workspace_fault(0);click("Wstecz");
 }
 test_workspace_disable();dsr_sc_appearance(DSR_SC_BLUE,false,0);
}
static void rooms_regression(void)
{
 /* The network connection stays intact; only shared layout starts empty. */
 wifi.connected=false;tick();unsigned before=requests;dsr_connection_t bound=submitted.connection;
 test_workspace_disable();test_workspace_enable();test_workspace_photos(true);dsr_network_rooms();
 CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);CHECK(find_label(lv_screen_active(),"Brak pomieszczeń"));screenshot("90-rooms-empty");
 tap(160,380);CHECK(find_label(lv_screen_active(),"Nowe pomieszczenie"));screenshot("91-room-new");
 click("Zapisz");CHECK(find_label(lv_screen_active(),"Podaj nazwę pomieszczenia."));
 room_name("Salon");tap(236,442);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano • zsynchronizowano"));tap(78,442);
 CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);CHECK(find_label(lv_screen_active(),"Salon"));screenshot("92-rooms-created");
 /* Real pointer on the card's 44x44 edit target; it must not open the room. */
 tap(134,168);CHECK(!dsr_mobile_active());CHECK(find_label(lv_screen_active(),"Edytuj pomieszczenie"));room_name("Salon 2");
 lv_obj_t *icon=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(icon);lv_dropdown_set_selected(icon,2);lv_obj_send_event(icon,LV_EVENT_VALUE_CHANGED,NULL);
 click("Wybierz zdjęcie z pamięci");test_workspace_defer(true);tap(236,442);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisywanie…"));screenshot("93-room-saving");
 tap(78,442);CHECK(find_label(lv_screen_active(),"Poczekaj na zakończenie zapisu."));
 cJSON *r=room_document();CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"rooms")->child,"name")->valuestring,"Salon"));cJSON_Delete(r);
 test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze."));screenshot("94-room-save-failed");
 click("Zapisz");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano • zsynchronizowano"));test_workspace_defer(false);click("Wstecz");
 tap(160,116);room_name("Łazienka Küche");click("Wybierz zdjęcie z pamięci");click("Zmień zdjęcie z pamięci");click("Zapisz");dsr_ui_workspace_tick();click("Wstecz");screenshot("95-rooms-photos");
 r=room_document();cJSON *rooms=cJSON_GetObjectItemCaseSensitive(r,"rooms");CHECK(cJSON_GetArraySize(rooms)==2);
 CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(rooms->child,"name")->valuestring,"Salon 2"));CHECK(cJSON_GetObjectItemCaseSensitive(rooms->child,"icon")->valueint==2);
 CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(rooms->child,"photo")->valuestring,dsr_photo_id(0)));
 /* Reordering from Web changes list positions, not which ID gets edited. */
 cJSON *moved=cJSON_DetachItemFromArray(rooms,0);cJSON_AddItemToArray(rooms,moved);publish_document(r);dsr_network_rooms();tap(134,168);
 CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Łazienka Küche"));room_name("Pokój 2");click("Zapisz");dsr_ui_workspace_tick();click("Wstecz");
 r=room_document();rooms=cJSON_GetObjectItemCaseSensitive(r,"rooms");CHECK(cJSON_GetObjectItemCaseSensitive(rooms->child,"id")->valueint==2);CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(rooms->child->next,"name")->valuestring,"Salon 2"));cJSON_Delete(r);
 /* An already-open room also follows ID when a Web reorder is published. */
 tap(60,228);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM);r=room_document();rooms=cJSON_GetObjectItemCaseSensitive(r,"rooms");moved=cJSON_DetachItemFromArray(rooms,0);cJSON_AddItemToArray(rooms,moved);publish_document(r);
 char *json=dsr_workspace_snapshot();dsr_mobile_model_t view_model={0};CHECK(dsr_mobile_configure(&view_model,json));free(json);dsr_mobile_update(&view_model);CHECK(find_label(lv_screen_active(),"Pokój 2"));tap(160,116);CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Pokój 2"));click("Wstecz");
 /* Single-room contextual entry and unsaved-change protection. */
 tap(215,228);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM);tap(160,116);room_name("Nie zapisuj");click("Wstecz");CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));screenshot("96-room-discard");click("Zostań");CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Nie zapisuj"));click("Wstecz");click("Odrzuć");CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);
 /* Pending save A / later edit B, followed by an external Web edit. */
 dsr_ui_edit_room(2);room_name("Wersja A");test_workspace_defer(true);click("Zapisz");room_name("Wersja B");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano wcześniejszą wersję • są niezapisane zmiany"));
 test_workspace_defer(false);r=room_document();publish_document(r);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));screenshot("97-room-conflict");click("Zapisz");click("Wstecz");click("Odrzuć");
 /* A Web write may supersede our completed write before the next LCD tick.
  * It must neither report our draft as saved nor trap Back in pending state. */
 dsr_ui_edit_room(2);room_name("Zapis LCD");test_workspace_defer(true);click("Zapisz");test_workspace_finish(true);test_workspace_defer(false);r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"rooms")->child->next,"name",cJSON_CreateString("Zapis Web"));publish_document(r);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));click("Wstecz");CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));click("Odrzuć");
 /* Deleted IDs cannot accidentally edit whichever room occupies that index. */
 dsr_ui_edit_room(999);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);
 dsr_ui_edit_room(2);room_name("ąąąąąąąąąąąąąąąąąąąąąąąą");click("Zapisz");CHECK(find_label(lv_screen_active(),"Podaj krótszą nazwę pomieszczenia."));click("Wstecz");click("Odrzuć");
 /* Reference protection includes a real binding-shaped definition and scene. */
 r=room_document();cJSON *devices=cJSON_GetObjectItemCaseSensitive(r,"devices");cJSON *d=cJSON_CreateObject();cJSON_AddNumberToObject(d,"id",7);cJSON_AddStringToObject(d,"name","TEST Shelly");cJSON_AddStringToObject(d,"host",bound.device.host);cJSON_AddNumberToObject(d,"channel",bound.device.channel);cJSON_AddStringToObject(d,"kind","relay");cJSON_AddNumberToObject(d,"room",2);cJSON_AddItemToArray(devices,d);
 cJSON *scene=cJSON_Parse("{\"id\":1,\"name\":\"TEST scene\",\"actions\":[{\"device\":7,\"command\":\"on\"}]}");cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"scenes"),scene);
 cJSON *tiles=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles");cJSON_AddItemToArray(tiles,cJSON_Parse("{\"id\":1,\"type\":\"room\",\"ref\":2,\"span\":1,\"height\":1,\"fields\":255}"));publish_document(r);
 dsr_ui_edit_room(2);room_delete_button();CHECK(find_label(lv_screen_active(),"Pomieszczenie jest używane"));screenshot("98-room-delete-blocked");tap(160,329);click("Wstecz");
 /* Removing dashboard refs alone still leaves the device/scene protected. */
 r=room_document();cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles"),0);publish_document(r);dsr_ui_edit_room(2);room_delete_button();CHECK(find_label(lv_screen_active(),"Pomieszczenie jest używane"));tap(160,329);click("Wstecz");
 /* Explicitly move the device first, then confirm deletion, then durable Save. */
 r=room_document();cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"devices")->child,"room"),1);publish_document(r);
 dsr_ui_edit_room(2);room_delete_button();CHECK(find_label(lv_screen_active(),"Usunąć pomieszczenie?"));screenshot("99-room-delete-confirm");click("Anuluj");room_delete_button();click("Usuń");CHECK(find_label(lv_screen_active(),"Usunięcie pomieszczenia"));
 r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==2);cJSON_Delete(r);click("Zapisz");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Pomieszczenie usunięte"));CHECK(!find_label(lv_screen_active(),"Zapisz"));click("Wstecz");
 r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==1);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"devices"))==1);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"scenes"))==1);d=cJSON_GetObjectItemCaseSensitive(r,"devices")->child;CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(d,"host")->valuestring,bound.device.host));CHECK(cJSON_GetObjectItemCaseSensitive(d,"channel")->valueint==bound.device.channel);cJSON_Delete(r);
 /* New rooms appear in the existing LAN wizard and dashboard tile selector. */
 dsr_ui_edit_dashboard();click("Pulpity");click("Mój pulpit");click("+ Pomieszczenie");click("Zapisz");dsr_ui_workspace_tick();r=room_document();tiles=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles");CHECK(tiles->child&&cJSON_GetObjectItemCaseSensitive(tiles->child,"ref")->valueint==1);cJSON_Delete(r);
 wifi.connected=true;dsr_ui_lan_open(NULL);test_lan_found("{\"gen\":2,\"id\":\"shellyplus1-test\",\"mac\":\"aabbccddeeff\",\"model\":\"TEST model\",\"auth_en\":false}","192.168.1.88");dsr_ui_lan_tick();click("TEST model");test_lan_inspected(DSR_OK,"{\"switch:1\":{\"id\":1,\"output\":false}}");dsr_ui_lan_tick();lv_obj_t *room=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(room);CHECK(strstr(lv_dropdown_get_options(room),"Salon 2"));CHECK(lv_dropdown_get_option_count(room)==2);dsr_ui_lan_reset();wifi.connected=false;
 dsr_network_rooms();for(unsigned i=1;i<8;i++){dsr_ui_add_room();room_name("Pokój TEST");click("Zapisz");dsr_ui_workspace_tick();click("Wstecz");}
 dsr_ui_add_room();CHECK(find_label(lv_screen_active(),"Limit: 8 pomieszczeń. Edytuj lub usuń istniejący pokój."));screenshot("100-rooms-limit");click("Wstecz");
 /* PL/EN/DE use the same production form and reference warning. */
 for(unsigned language=1;language<3;language++){
  r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(language==1?"en":"de"));publish_document(r);dsr_network_rooms();screenshot(language==1?"101-rooms-english":"103-rooms-german");dsr_ui_edit_room(1);screenshot(language==1?"102-room-edit-english":"104-room-edit-german");room_delete_button();screenshot(language==1?"105-room-blocked-english":"106-room-blocked-german");tap(160,329);click("Wstecz");
 }
 CHECK(requests==before);CHECK(!memcmp(&submitted.connection,&bound,sizeof(bound)));
 dsr_network_rooms();dsr_ui_edit_room(1);click("Wstecz");lv_refr_now(display);lv_mem_monitor_t memory_before,memory_after;lv_mem_monitor(&memory_before);
 for(unsigned i=0;i<40;i++){dsr_ui_edit_room(1);click("Wstecz");}lv_refr_now(display);lv_mem_monitor(&memory_after);CHECK(memory_after.free_size==memory_before.free_size);CHECK(memory_after.free_biggest_size>32000);
 printf("Rooms UI heap before=%u after=%u largest=%u; 40 edit/back cycles, 8 rooms\n",(unsigned)memory_before.free_size,(unsigned)memory_after.free_size,(unsigned)memory_after.free_biggest_size);
 test_workspace_photos(false);dsr_network_home(NULL);tap(188,116);CHECK(find_label(lv_screen_active(),"Bluetooth LE"));dsr_network_home(NULL);
}
static void polling_regression(void)
{
 dsr_network_home(NULL);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *name=find_label(lv_screen_active(),"Desk lamp");CHECK(name);
 unsigned before=requests;
 for(unsigned i=0;i<2;i++){
  lv_tick_inc(5000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);
  CHECK(find_label(lv_screen_active(),"Desk lamp")==name);CHECK(find_label(lv_screen_active(),"Nieprzypisane • Włączone"));CHECK(!find_label(lv_screen_active(),"Odczyt stanu…"));
  response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Desk lamp")==name);
 }
 CHECK(requests==before+2);screenshot("70-home-poll-stable");
 lv_tick_inc(5000);tick();CHECK(submitted.kind==DSR_JOB_PROBE);before=requests;
 tap(274,178);CHECK(submitted.kind==DSR_JOB_PROBE&&requests==before);CHECK(find_label(lv_screen_active(),"Nieprzypisane • Wysyłanie polecenia…"));screenshot("71-command-queued-behind-read");
 tap(274,178);CHECK(requests==before); /* disabled duplicate, never lost/replayed */
 deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(in_flight&&submitted.kind==DSR_JOB_SET_OUTPUT&&!submitted.on);CHECK(requests==before+1);CHECK(!find_label(lv_screen_active(),"Nieprzypisane • Wyłączone"));
 response.device.output=false;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Nieprzypisane • Wyłączone"));
 tap(274,178);CHECK(submitted.kind==DSR_JOB_SET_OUTPUT&&submitted.on);response.device.output=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 /* A failed read cancels the queued intent, rather than blindly sending SET. */
 lv_tick_inc(5000);tick();before=requests;tap(274,178);response.device.online=false;deliver(DSR_AUTH_REQUIRED,DSR_WIFI_IDLE);CHECK(requests==before);CHECK(find_label(lv_screen_active(),"Błąd logowania"));
 click("Desk lamp");click("Odśwież stan");response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 lv_obj_t *label=find_label(lv_screen_active(),"230.4 V");CHECK(label);
 lv_tick_inc(5000);tick();CHECK(submitted.kind==DSR_JOB_PROBE);CHECK(find_label(lv_screen_active(),"230.4 V")==label);CHECK(find_label(lv_screen_active(),"Włączone • Online"));
 before=requests;click("Odśwież stan");CHECK(requests==before); /* joins read */
 deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"230.4 V")==label);screenshot("72-details-poll-freshness");
 lv_tick_inc(1000);tick();CHECK(find_label(lv_screen_active(),"Odczyt potwierdzony 1 s temu"));CHECK(find_label(lv_screen_active(),"230.4 V")==label);
 /* Busy dispatch is reported and never retried as a SET by automatic polling. */
 lv_tick_inc(5000);tick();before=requests;tap(160,262);CHECK(submitted.kind==DSR_JOB_PROBE);next_submit_result=DSR_BUSY;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(!in_flight&&requests==before);
 lv_tick_inc(5000);tick();CHECK(submitted.kind==DSR_JOB_PROBE);deliver(DSR_OK,DSR_WIFI_IDLE);click("Odśwież stan");deliver(DSR_OK,DSR_WIFI_IDLE);
 /* Losing Wi-Fi while queued cancels intent even if the read later succeeds. */
 lv_tick_inc(5000);tick();before=requests;tap(160,262);wifi.connected=false;tick();CHECK(find_label(lv_screen_active(),"Brak odpowiedzi"));deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(requests==before&&!in_flight);
 wifi.connected=true;tick();CHECK(submitted.kind==DSR_JOB_PROBE);click("Odśwież stan");response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(!find_label(lv_screen_active(),"Brak odpowiedzi"));
 dsr_network_home(NULL);
}
static void ble_regression(void)
{
 dsr_network_device(NULL);click("Bluetooth LE");CHECK(find_label(lv_screen_active(),"Wyszukaj urządzenia w pobliżu"));screenshot("80-ble-start");click("Szukaj");tick();CHECK(find_label(lv_screen_active(),"Odbieranie reklam Bluetooth…"));screenshot("81-ble-scanning");
 const uint8_t rpc[]={17,7,0x5f,0x44,0x49,0x5f,0x43,0x56,0x53,0x5f,0x43,0x50,0x52,0x5f,0x53,0x4f,0x6d,0x5f};
 const uint8_t blu[]={6,0xff,0xa9,0x0b,0x0b,2,0},bthome[]={6,0x16,0xd2,0xfc,0x40,1,92};
 /* The native worker may keep polling the relay while BLE owns its own task. */
 wifi.connected=true;tick();if(in_flight){response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 lv_tick_inc(5000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(dsr_ui_ble_active());
 test_ble_packet(rpc,sizeof(rpc),1);test_ble_packet(blu,sizeof(blu),2);test_ble_packet(bthome,sizeof(bthome),3);test_ble_finish(DSR_BLE_DONE);tick();
 CHECK(find_label(lv_screen_active(),"Shelly • RPC / konfiguracja"));CHECK(find_label(lv_screen_active(),"Shelly BLU DoorWindow"));CHECK(find_label(lv_screen_active(),"Pozostałe reklamy"));screenshot("82-ble-results");
 click("Shelly BLU DoorWindow");tick();CHECK(find_label(lv_screen_active(),"Wykryto • nie sparowano"));screenshot("83-ble-detail");click("Wstecz");click("Ponów");click("Anuluj");tick();CHECK(find_label(lv_screen_active(),"Skan przerwany"));screenshot("84-ble-cancelled");
 click("Ponów");test_ble_finish(DSR_BLE_DONE);tick();CHECK(find_label(lv_screen_active(),"Nie znaleziono urządzeń BLE"));screenshot("85-ble-empty");
 click("Ponów");test_ble_finish(DSR_BLE_UNAVAILABLE);tick();CHECK(find_label(lv_screen_active(),"Bluetooth niedostępny"));screenshot("86-ble-unavailable");
 for(unsigned language=1;language<3;language++){dsr_sc_appearance(DSR_SC_BLUE,false,language);dsr_ui_ble_open(NULL);click("Ponów");test_ble_packet(blu,sizeof(blu),2);test_ble_finish(DSR_BLE_DONE);tick();screenshot(language==1?"87-ble-english":"88-ble-german");}
 dsr_sc_appearance(DSR_SC_BLUE,false,0);dsr_network_home(NULL);lv_mem_monitor_t a,b;lv_mem_monitor(&a);
 for(unsigned i=0;i<10;i++){dsr_ui_ble_open(NULL);click("Ponów");click("Anuluj");tick();click("Wstecz");dsr_network_home(NULL);tick();}
 lv_mem_monitor(&b);CHECK(b.free_size+1024>=a.free_size);CHECK(!dsr_ui_ble_active());
 if(in_flight){CHECK(submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 wifi.connected=false;tick();
 printf("BLE UI heap before=%u after=%u; 10 scan/cancel/exit cycles; host radio fixture only\n",(unsigned)a.free_size,(unsigned)b.free_size);
}
static void bounds(lv_obj_t *obj,int max_y)
{lv_area_t a;lv_obj_get_coords(obj,&a);CHECK(a.x1>=0&&a.x2<320&&a.y1>=0&&a.y2<max_y);}
static void keyboard_regression(void)
{
 for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);lv_obj_t *root=dsr_ui_screen("Connect to Wi-Fi",false);
  dsr_ui_label(root,"SSID: TEST / DEMO",16,92,288);
  lv_obj_t *field=lv_textarea_create(root);dsr_ui_textarea(field);lv_obj_set_pos(field,16,130);lv_obj_set_size(field,288,44);lv_textarea_set_one_line(field,true);lv_textarea_set_password_mode(field,true);
  lv_obj_t *kb=lv_keyboard_create(root);dsr_ui_keyboard(kb);dsr_ui_keyboard_focus(kb,field);lv_obj_add_event_cb(kb,ready,LV_EVENT_READY,NULL);
  lv_obj_update_layout(root);CHECK(lv_obj_get_height(kb)==240);CHECK(lv_obj_get_style_text_font(kb,LV_PART_ITEMS)==&dsr_roboto_22);bounds(field,240);
  const lv_buttonmatrix_t *matrix=(const lv_buttonmatrix_t *)kb;for(unsigned i=0;i<matrix->btn_cnt;i++){lv_area_t b=matrix->button_areas[i];CHECK(b.x1>=0&&b.x2<320);CHECK(lv_area_get_height(&b)>=40);}
  lv_textarea_set_text(field,"abcdef123!xyz");lv_textarea_set_cursor_pos(field,LV_TEXTAREA_CURSOR_LAST);
  for(unsigned i=0;i<4;i++){key(kb,LV_SYMBOL_LEFT);}
  CHECK(lv_textarea_get_cursor_pos(field)==9);key(kb,"x");CHECK(!strcmp(lv_textarea_get_text(field),"abcdef123x!xyz"));key(kb,LV_SYMBOL_BACKSPACE);CHECK(!strcmp(lv_textarea_get_text(field),"abcdef123!xyz"));
  lv_textarea_set_cursor_pos(field,0);key(kb,LV_SYMBOL_LEFT);CHECK(lv_textarea_get_cursor_pos(field)==0);lv_textarea_set_cursor_pos(field,LV_TEXTAREA_CURSOR_LAST);key(kb,LV_SYMBOL_RIGHT);CHECK(lv_textarea_get_cursor_pos(field)==13);
  lv_textarea_set_text(field,"");key(kb,LV_SYMBOL_UP);key(kb,language==2?"Z":"Y");CHECK(!strcmp(lv_textarea_get_text(field),language==2?"Z":"Y"));key(kb,LV_SYMBOL_UP);
  lv_textarea_set_text(field,"ąäóabc");lv_textarea_set_cursor_pos(field,3);key(kb,"123");key(kb,"ąä");key(kb,"Ż");CHECK(!strcmp(lv_textarea_get_text(field),"ąäóŻabc"));key(kb,LV_SYMBOL_BACKSPACE);CHECK(!strcmp(lv_textarea_get_text(field),"ąäóabc"));key(kb,"ABC");
  bool ascii[127]={0};ascii[' ']=true;const lv_keyboard_mode_t modes[]={LV_KEYBOARD_MODE_TEXT_LOWER,LV_KEYBOARD_MODE_TEXT_UPPER,LV_KEYBOARD_MODE_SPECIAL,LV_KEYBOARD_MODE_USER_1};
  for(unsigned m=0;m<4;m++){lv_keyboard_set_mode(kb,modes[m]);matrix=(const lv_buttonmatrix_t *)kb;for(unsigned i=0;i<matrix->btn_cnt;i++){const unsigned char *s=(const unsigned char *)lv_keyboard_get_button_text(kb,i);if(s[0]>=33&&s[0]<127&&!s[1])ascii[s[0]]=true;}}
  for(unsigned i=32;i<127;i++)CHECK(ascii[i]);
  lv_keyboard_set_mode(kb,LV_KEYBOARD_MODE_TEXT_LOWER);screenshot(language==0?"30-keyboard-pl":language==1?"31-keyboard-en":"32-keyboard-de");
  lv_textarea_set_accepted_chars(field,"0123456789.");lv_textarea_set_text(field,"192.168.1.7");dsr_ui_keyboard_focus(kb,field);CHECK(lv_keyboard_get_mode(kb)==LV_KEYBOARD_MODE_NUMBER);key(kb,LV_SYMBOL_LEFT);key(kb,"0");CHECK(!strcmp(lv_textarea_get_text(field),"192.168.1.07"));screenshot(language==0?"33-keyboard-ip":language==1?"34-keyboard-ip-en":"35-keyboard-ip-de");
  unsigned before=ready_count;key(kb,"Gotowe");CHECK(ready_count==before+1);CHECK(lv_textarea_get_password_mode(field));
 }
 dsr_sc_appearance(DSR_SC_BLUE,false,0);lv_obj_clean(lv_screen_active());
}

int main(int argc,char **argv)
{
    CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);keyboard_regression();
    dsr_ui_init();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_WIFI_SCAN);screenshot("01-scanning");
    response.scan=(dsr_wifi_scan_t){.count=4,.items={
        {.ssid="Home Wi-Fi",.rssi=-39,.security=DSR_WIFI_WPA2},
        {.ssid="A very long network name 12345678",.rssi=-58,.security=DSR_WIFI_WPA2_WPA3},
        {.ssid="Guest network",.rssi=-64,.security=DSR_WIFI_OPEN},
        {.ssid="Office enterprise",.rssi=-72,.security=DSR_WIFI_UNSUPPORTED}}};
    deliver(DSR_OK,DSR_WIFI_IDLE);screenshot("02-network-list");
    click("Home Wi-Fi");screenshot("03-password-keyboard");
    lv_obj_t *password=find_type(lv_screen_active(),&lv_textarea_class);CHECK(password!=NULL);
    CHECK(lv_textarea_get_password_mode(password));bounds(password,208);
    lv_obj_t *button=find_label(lv_screen_active(),"Connect");CHECK(button!=NULL);bounds(lv_obj_get_parent(button),272);
    lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb!=NULL);bounds(kb,480);
    lv_color_t foreground=lv_obj_get_style_text_color(password,LV_PART_MAIN);
    lv_color_t background=lv_obj_get_style_bg_color(password,LV_PART_MAIN);
    CHECK(foreground.red>200&&foreground.green>200&&foreground.blue>200);
    CHECK(background.red<60&&background.green<60&&background.blue<80);
    click("Connect");CHECK(!in_flight);CHECK(find_label(lv_screen_active(),"Use 8-63 characters or a 64-digit hex key.")!=NULL);
    lv_textarea_set_text(password,"not-a-real-password");unsigned joins=requests;key(kb,"Gotowe");CHECK(requests==joins+1);CHECK(in_flight&&submitted.kind==DSR_JOB_WIFI_CONNECT);
    CHECK(!strcmp(submitted.settings.ssid,"Home Wi-Fi"));screenshot("04-connecting");
    deliver(DSR_OFFLINE,DSR_WIFI_BAD_PASSWORD);CHECK(find_type(lv_screen_active(),&lv_keyboard_class)!=NULL);
    CHECK(find_label(lv_screen_active(),"Authentication failed. Check the password.")!=NULL);screenshot("05-password-error");
    click("Connect");wifi=(dsr_wifi_info_t){.connected=true,.status=DSR_WIFI_CONNECTED,.ssid="Home Wi-Fi",.ip="192.168.1.42"};
    deliver(DSR_OK,DSR_WIFI_CONNECTED);char ip_label[64];snprintf(ip_label,sizeof(ip_label),dsr_sc_tr("IP address: %s"),"192.168.1.42");CHECK(find_label(lv_screen_active(),ip_label)!=NULL);screenshot("06-connected");
    click("Continue");CHECK(find_label(lv_screen_active(),"Brak urządzeń")!=NULL);
    CHECK(find_label(lv_screen_active(),"ON")==NULL);screenshot("07-home");
    dsr_network_device(NULL);screenshot("14-add-device-wizard");click("Dodaj ręcznie (IP)");screenshot("08-device-form");
    password=find_type(lv_screen_active(),&lv_textarea_class);CHECK(password!=NULL);
    foreground=lv_obj_get_style_text_color(password,LV_PART_MAIN);CHECK(foreground.red>200);
    dsr_network_wifi(NULL);tick();deliver(DSR_OK,DSR_WIFI_IDLE);click("Guest network");
    CHECK(find_type(lv_screen_active(),&lv_keyboard_class)==NULL);screenshot("09-open-network");
    click("Connect");CHECK(!submitted.settings.password[0]);click("Cancel");deliver(DSR_OFFLINE,DSR_WIFI_CANCELLED);
    CHECK(find_label(lv_screen_active(),"Home Wi-Fi")!=NULL);screenshot("10-cancelled");
    dsr_wifi_scan_t many={.count=20};
    for(unsigned i=0;i<20;i++){snprintf(many.items[i].ssid,33,"Network %02u",i);many.items[i].security=DSR_WIFI_WPA2;many.items[i].rssi=-40-(int)i;}
    lv_mem_monitor_t before,after;
    dsr_network_home(NULL);tick();
    lv_mem_monitor(&before);
    for(unsigned i=0;i<60;i++) {
        dsr_network_wifi(NULL);tick();response.scan=many;deliver(DSR_OK,DSR_WIFI_IDLE);
        click("Network 00");click("Back");click("Home");
    }
    tick();lv_mem_monitor(&after);
    printf("UI heap before=%u after=%u largest=%u\n",(unsigned)before.free_size,(unsigned)after.free_size,(unsigned)after.free_biggest_size);
    fflush(stdout);CHECK(after.free_size+1024>=before.free_size);
    dsr_network_device(NULL);click("Dodaj ręcznie (IP)");password=find_type(lv_screen_active(),&lv_textarea_class);
    lv_textarea_set_text(password,"192.168.1.77");click("Check & save device");CHECK(submitted.kind==DSR_JOB_BIND);
    response.device=(dsr_device_t){.host="192.168.1.77",.name="Desk lamp",.device_id="shellyplus1-AABBCCDDEEFF",
        .generation=DSR_GEN2_PLUS,.online=true,.has_switch=true,.output=true,.has_power=true,.power_w=4.1};
    response.telemetry=(dsr_telemetry_t){.has_voltage=true,.has_current=true,.voltage_v=230.4,.current_a=0.018};
    deliver(DSR_OK,DSR_WIFI_IDLE);screenshot("11-configured-switch");
    polling_regression();
    click("Desk lamp");CHECK(find_label(lv_screen_active(),"230.4 V"));CHECK(find_label(lv_screen_active(),"0.018 A"));screenshot("50-real-metrics-fixture");
    lv_obj_t *power=power_target(lv_screen_active());CHECK(power);lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);CHECK(in_flight&&submitted.kind==DSR_JOB_SET_OUTPUT);
    response.device.online=false;deliver(DSR_AUTH_REQUIRED,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Błąd logowania"));CHECK(!find_label(lv_screen_active(),"230.4 V"));screenshot("51-command-auth-error");
    dsr_network_home(NULL);screenshot("53-card-auth-error");CHECK(find_label(lv_screen_active(),"Błąd logowania"));click("Desk lamp");
    click("Odśwież stan");CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"230.4 V"));
    power=power_target(lv_screen_active());CHECK(power);lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);response.device.online=false;deliver(DSR_OFFLINE,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Brak odpowiedzi"));screenshot("52-command-timeout");
    lv_tick_inc(5000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Brak odpowiedzi"));
    click("Odśwież stan");CHECK(submitted.kind==DSR_JOB_PROBE);deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(!find_label(lv_screen_active(),"Brak odpowiedzi"));
    power=power_target(lv_screen_active());CHECK(power);lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);response.device.online=false;deliver(DSR_BAD_RESPONSE,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Stan niepotwierdzony"));screenshot("54-readback-unconfirmed");click("Odśwież stan");CHECK(submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);
    dsr_network_home(NULL);
    wifi.connected=false;tick();screenshot("12-switch-offline");
    CHECK(find_label(lv_screen_active(),"Desk lamp")!=NULL);
    click("Desk lamp");CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS);
    CHECK(find_label(lv_screen_active(),"Offline • stan niepotwierdzony")!=NULL);
    ble_regression();
    dsr_ui_open_settings();screenshot("40-settings-menu");
    click("Podgląd rolety • DEMO");CHECK(find_label(lv_screen_active(),"Pozycja 65%"));CHECK(find_label(lv_screen_active(),"DEMO • dane przykładowe"));screenshot("42-cover-preview-on-device");
    unsigned before_preview=submitted.id;click("Otwórz");CHECK(find_label(lv_screen_active(),"Otwarte 100%"));click("Zamknij");CHECK(find_label(lv_screen_active(),"Zamknięte 0%"));click("Stop");CHECK(submitted.id==before_preview);tap(20,20);CHECK(!find_label(lv_screen_active(),"DEMO • dane przykładowe"));CHECK(find_label(lv_screen_active(),"Desk lamp"));
    dsr_ui_open_settings();click("Settings");screenshot("41-display-settings");
    test_workspace_enable();dsr_ui_edit_dashboard();
    CHECK(find_label(lv_screen_active(),"Edytuj pilot"));screenshot("13-workspace-editor");
    click("Pomieszczenia");click("+ Dodaj wpis");
    lv_obj_t *name=find_type(lv_screen_active(),&lv_textarea_class);CHECK(name);lv_textarea_set_text(name,"Salon testowy");click("Zapisz");
    char *configuration=dsr_workspace_snapshot();CHECK(configuration);CHECK(dsr_config_revision(configuration)==2);
    dsr_mobile_model_t model={0};CHECK(dsr_mobile_configure(&model,configuration));free(configuration);
    CHECK(model.room_count==1);CHECK(!strcmp(model.rooms[0].name,"Salon testowy"));
    dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano • zsynchronizowano"));
    test_workspace_defer(true);lv_textarea_set_text(name,"Wersja A");click("Zapisz");
    lv_textarea_set_text(name,"Wersja B");click("Zapisz");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisywanie…"));
    test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano wcześniejszą wersję • są niezapisane zmiany"));
    screenshot("20-workspace-save-race");
    CHECK(!strcmp(lv_textarea_get_text(name),"Wersja B"));configuration=dsr_workspace_snapshot();CHECK(strstr(configuration,"Wersja A"));CHECK(!strstr(configuration,"Wersja B"));free(configuration);
    click("Zapisz");test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze."));CHECK(!strcmp(lv_textarea_get_text(name),"Wersja B"));
    click("Zapisz");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano • zsynchronizowano"));configuration=dsr_workspace_snapshot();CHECK(dsr_config_revision(configuration)==4);CHECK(strstr(configuration,"Wersja B"));free(configuration);
    test_workspace_defer(false);configuration=dsr_workspace_snapshot();CHECK(dsr_workspace_submit(configuration,4));free(configuration);dsr_ui_workspace_tick();
    CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));
    dsr_network_home(NULL);dsr_ui_workspace_tick();
    wifi.connected=false;dsr_ui_lan_open(NULL);tick();CHECK(find_label(lv_screen_active(),"Najpierw połącz Wi-Fi"));screenshot("19-lan-no-wifi");
    wifi.connected=true;dsr_network_device(NULL);click("Skanuj sieć Wi-Fi");tick();CHECK(dsr_ui_lan_active());screenshot("15-lan-scanning");
    const char info[]="{\"gen\":2,\"id\":\"shellyplus2pm-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-002P16EU\",\"auth_en\":true}";
    test_lan_found(info,"192.168.1.88");tick();CHECK(find_label(lv_screen_active(),"SNSW-002P16EU"));screenshot("16-lan-results");click("SNSW-002P16EU");
    unsigned skip=1;password=nth_type(lv_screen_active(),&lv_textarea_class,&skip);CHECK(password);
    lv_obj_send_event(password,LV_EVENT_CLICKED,NULL);lv_obj_update_layout(lv_screen_active());bounds(password,240);screenshot("36-lan-password-keyboard");key(find_type(lv_screen_active(),&lv_keyboard_class),"Gotowe");
    lv_textarea_set_text(password,"wrong-password");click("Sprawdź");
    test_lan_inspected(DSR_AUTH_REQUIRED,"{}");tick();CHECK(find_label(lv_screen_active(),"Błąd logowania. Sprawdź dane."));screenshot("17-lan-auth-error");
    skip=1;password=nth_type(lv_screen_active(),&lv_textarea_class,&skip);CHECK(password);lv_textarea_set_text(password,"correct-password");click("Sprawdź");
    const char components[]="{\"switch:1\":{\"id\":1,\"output\":false},\"switch:4\":{\"id\":4,\"output\":true}}";
    test_lan_inspected(DSR_OK,components);tick();CHECK(find_label(lv_screen_active(),"Kanał z odpowiedzi API"));screenshot("18-lan-components");
    name=find_type(lv_screen_active(),&lv_textarea_class);CHECK(name);lv_textarea_set_text(name,"Światło testowe");
    skip=0;lv_obj_t *room=nth_type(lv_screen_active(),&lv_dropdown_class,&skip);CHECK(room);lv_dropdown_set_selected(room,1);
    skip=1;lv_obj_t *channels=nth_type(lv_screen_active(),&lv_dropdown_class,&skip);CHECK(channels);CHECK(lv_dropdown_get_option_count(channels)==2);
    click("Zapisz");CHECK(in_flight&&submitted.kind==DSR_JOB_BIND&&submitted.connection.device.channel==1);CHECK(!strcmp(submitted.connection.device.device_id,"shellyplus2pm-aabbccddeeff"));
    response.device=submitted.connection.device;response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);
    configuration=dsr_workspace_snapshot();CHECK(configuration);CHECK(dsr_mobile_configure(&model,configuration));free(configuration);CHECK(model.device_count==1&&model.devices[0].room==0&&model.devices[0].channel==1);
    dsr_ui_lan_reset();storage_error_regression();rooms_regression();test_workspace_disable();
    printf("PASS: %u real LVGL UI checks; %u screenshots; keyboard, LAN, BLE, contextual rooms, stable IDs, references, CAS/save regression; 60 navigation + 40 room edit cycles\n",checks,screenshots);
    lv_deinit();return 0;
}
