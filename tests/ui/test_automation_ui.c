/* Actual LVGL requested/readback state. SDK/workspace providers are MODELS. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static void capture_trv(unsigned index,uint8_t half){(void)index;(void)half;CHECK(false);}
void dsr_ui_automation_open(void);
void test_automation_guard_error(bool enabled);
void test_automation_status(const dsr_auto_snapshot_t *value);
static const char *lang(unsigned l,const char *pl,const char *en,const char *de){return l==1?en:l==2?de:pl;}
static void warning_flow(unsigned language){
 const char *info=lang(language,"Wyłączenie lub usunięcie zatrzymuje kolejne polecenia. Zmiana czujników, celów lub temperatury aktywnej reguły wymaga jej wyłączenia i zapisu.","Disabling or deleting stops future commands. Disable and save an active rule before changing contacts, targets or temperature.","Deaktivieren oder Löschen stoppt künftige Befehle. Aktive Regel vor Änderungen an Kontakten, Zielen oder Temperatur deaktivieren und speichern.");
 const char *error=lang(language,"Nie zapisano. Szkic zachowany. Sprawdź konflikt lub pamięć.","Not saved. Draft retained. Check conflict or storage.","Nicht gespeichert. Entwurf bleibt erhalten. Konflikt oder Speicher prüfen.");
 lv_obj_t *first=find_label(lv_screen_active(),info),*second=find_label(lv_screen_active(),error);CHECK(first&&second);
 lv_obj_update_layout(lv_screen_active());lv_area_t a,b,text;lv_obj_get_coords(lv_obj_get_parent(first),&a);lv_obj_get_coords(lv_obj_get_parent(second),&b);
 CHECK(a.y2+8<b.y1); /* whole frames, not only their clipped visible fragments */
 CHECK(a.y1>=96&&b.y2<404); /* both messages readable above the fixed actions */
 lv_obj_get_coords(second,&text);CHECK(text.y1>=b.y1+8&&text.y2<=b.y2-8);
}
static void seed(void){
 cJSON *r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Window A\",\"host\":\"192.0.2.1\",\"channel\":0,\"room\":0,\"kind\":\"contact\"},{\"id\":2,\"name\":\"Thermostat A\",\"host\":\"192.0.2.1\",\"channel\":1,\"room\":0,\"kind\":\"sensor\"},{\"id\":3,\"name\":\"Relay A\",\"host\":\"192.0.2.1\",\"channel\":2,\"room\":0,\"kind\":\"relay\"}]"));publish_document(r);
 boot_loaded_count=3;for(unsigned i=0;i<3;i++){boot_loaded[i]=(dsr_connection_t){.layout_id=i+1,.device={.generation=DSR_GEN2_PLUS,.host="192.0.2.1",.endpoint_kind=i==2?DSR_ENDPOINT_SWITCH:DSR_ENDPOINT_BLU,.channel=i,.online=true}};snprintf(boot_loaded[i].device.device_id,64,"shellyblugwg3-test_%012u",i+1);boot_loaded[i].telemetry.blu.flags=DSR_BLU_VALID|(i==0?DSR_BLU_HAS_OPEN:0);if(i==1)boot_loaded[i].telemetry.blu.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET;}
}
static void saved_edit(void){
 lv_obj_t *name=find_label(lv_screen_active(),"Heating downstairs");CHECK(name);lv_obj_t *box=lv_obj_get_parent(name),*edit=NULL;
 for(unsigned i=0;i<lv_obj_get_child_count(box);i++){lv_obj_t *b=lv_obj_get_child(box,(int)i);if(lv_obj_get_x(b)==228)edit=b;}
 CHECK(edit);lv_obj_send_event(edit,LV_EVENT_CLICKED,NULL);
}
static void form_checks(unsigned theme,unsigned language){
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"automations",cJSON_CreateArray());cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(theme==3?"cyber_blue":theme==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(language==1?"en":language==2?"de":"pl"));publish_document(doc);dsr_network_home(NULL);
 unsigned controls=requests;dsr_ui_automation_open();click(lang(language,"Automatyzacja +","Automation +","Automatisierung +"));CHECK(find_label(lv_screen_active(),"8.0 °C"));
 lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_textarea_set_text(field,"Heating downstairs");lv_obj_send_event(field,LV_EVENT_VALUE_CHANGED,NULL);lv_obj_send_event(field,LV_EVENT_FOCUSED,NULL);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb&&!lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN));lv_area_t f,k;lv_obj_get_coords(field,&f);lv_obj_get_coords(kb,&k);CHECK(f.y2<k.y1);lv_obj_send_event(kb,LV_EVENT_READY,NULL);
 field=find_type(lv_screen_active(),&lv_textarea_class);lv_tick_inc(2000);dsr_ui_automation_tick();CHECK(field==find_type(lv_screen_active(),&lv_textarea_class));
 const char *next=lang(language,"Dalej","Next","Weiter"),*save=lang(language,"Zapisz","Save","Speichern");click(next);
 CHECK(find_label(lv_screen_active(),"Window A")&&!find_label(lv_screen_active(),"Thermostat A")&&!find_label(lv_screen_active(),"Relay A"));click("Window A");click(next);
 CHECK(find_label(lv_screen_active(),"Thermostat A")&&!find_label(lv_screen_active(),"Window A"));click("Thermostat A");click(next);
 lv_obj_t *enable=find_type(lv_screen_active(),&lv_checkbox_class);CHECK(enable&&!lv_obj_has_state(enable,LV_STATE_CHECKED));CHECK(requests==controls);
 lv_obj_t *save_label=find_label(lv_screen_active(),save);CHECK(save_label);lv_area_t action;lv_obj_get_coords(lv_obj_get_parent(save_label),&action);CHECK(action.y1==420&&action.y2==463&&lv_area_get_height(&action)>=44);
 if(theme==3){test_workspace_defer(true);click(save);test_workspace_finish(false);test_workspace_defer(false);dsr_ui_automation_tick();CHECK(find_type(lv_screen_active(),&lv_checkbox_class));char error_shot[64];snprintf(error_shot,sizeof(error_shot),"automation-summary-error-%u-%u",theme,language);screenshot(error_shot);warning_flow(language);}
 char shot[64];snprintf(shot,sizeof(shot),"automation-summary-%u-%u",theme,language);if(language==theme%3)screenshot(shot);
 click(save);dsr_ui_automation_tick();doc=room_document();const cJSON *r=cJSON_GetObjectItemCaseSensitive(doc,"automations")->child;CHECK(r&&!cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(r,"enabled")));CHECK(cJSON_GetObjectItemCaseSensitive(r,"target_half")->valueint==16);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"sensors"))==1);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"targets"))==1);cJSON_Delete(doc);
 if(language==0){
  saved_edit();click(next);click(next);click(next);enable=find_type(lv_screen_active(),&lv_checkbox_class);CHECK(enable);lv_obj_add_state(enable,LV_STATE_CHECKED);lv_obj_send_event(enable,LV_EVENT_VALUE_CHANGED,NULL);click(save);dsr_ui_automation_tick();
  doc=room_document();CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"automations")->child,"enabled")));cJSON_Delete(doc);
  saved_edit();click("+");click(next);click(next);click(next);test_automation_guard_error(true);click(save);CHECK(find_label(lv_screen_active(),"Przed zmianą aktywnej reguły wyłącz ją i zapisz."));test_automation_guard_error(false);
  enable=find_type(lv_screen_active(),&lv_checkbox_class);lv_obj_remove_state(enable,LV_STATE_CHECKED);lv_obj_send_event(enable,LV_EVENT_VALUE_CHANGED,NULL);click(save);dsr_ui_automation_tick();
  dsr_auto_snapshot_t s={.count=1,.target_count=1,.rules={{.rule_id=1,.state=DSR_AUTO_WAITING,.missing_id=1}}};s.targets[0].layout_id=2;s.targets[0].baseline=40;s.targets[0].requested=16;s.targets[0].read_half=44;test_automation_status(&s);dsr_ui_automation_hide();dsr_ui_automation_open();click("Oczekiwanie na dane");
  CHECK(find_label(lv_screen_active(),"Brak nowego raportu: Window A (#1)"));CHECK(find_label(lv_screen_active(),"Thermostat A\nPoprzednia: 20.0 °C\nŻądana: 8.0 °C\nOdczytana: 22.0 °C"));if(theme==3)screenshot("automation-status-cyber");click("Wstecz");
  s.rules[0].state=DSR_AUTO_ERROR;s.rules[0].result=DSR_UNSUPPORTED;s.targets[0].result=DSR_UNSUPPORTED;test_automation_status(&s);dsr_ui_automation_hide();dsr_ui_automation_open();click("Błąd");CHECK(find_label(lv_screen_active(),"Bezpośredni odczyt TRV lub nastawa nieobsługiwane."));click("Wstecz");
  s.rules[0].result=DSR_IO;test_automation_status(&s);dsr_ui_automation_hide();dsr_ui_automation_open();click("Błąd");CHECK(find_label(lv_screen_active(),"Błąd dziennika. Automatyzacja wstrzymana."));CHECK(find_label(lv_screen_active(),"Bezpośredni odczyt TRV lub nastawa nieobsługiwane."));if(theme==3)screenshot("automation-read-journal-error-cyber");click("Wstecz");
  saved_edit();click(next);click(next);click(next);click("Usuń regułę");test_workspace_defer(true);click("Usuń");test_workspace_finish(false);test_workspace_defer(false);dsr_ui_automation_tick();click("Wstecz");click(save);dsr_ui_automation_tick();doc=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"automations"))==1);cJSON_Delete(doc);
  saved_edit();click(next);click(next);click(next);click("Usuń regułę");click("Usuń");dsr_ui_automation_tick();doc=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"automations"))==0);cJSON_Delete(doc);memset(&s,0,sizeof(s));test_automation_status(&s);
 }
 CHECK(requests==controls);dsr_ui_automation_hide();
}
int main(int argc,char **argv){
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");test_workspace_enable();seed();dsr_ui_init();
 dsr_mobile_actions_t controls={.set_trv=capture_trv};dsr_mobile_bind(&controls);
 const unsigned themes[]={0,2,3};
 for(unsigned t=0;t<3;t++){
  dsr_sc_appearance(DSR_SC_BLUE,themes[t],1);dsr_ui_apply_theme();
  dsr_mobile_model_t m={.wifi=true,.device_count=1};
  m.devices[0]=(dsr_mobile_device_t){.id=3,.online=true,.has_trv_target=true,.trv_target_c=20,.has_temperature=true,.temperature_c=21.5,.endpoint_kind=DSR_ENDPOINT_BLU,.generation=2,.blu={.flags=DSR_BLU_VALID,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET}};
  strcpy(m.devices[0].name,"Thermostat");strcpy(m.devices[0].host,"192.0.2.1");strcpy(m.devices[0].identity,"gateway_trv");
  dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);dsr_mobile_trv_request_state(3,44,1234,true);
  m.devices[0].busy=m.devices[0].command_pending=true;dsr_mobile_update(&m);lv_refr_now(display);
  CHECK(find_label(lv_screen_active(),"22.0 °C"));CHECK(m.devices[0].trv_target_c==20);
  char shot[64];snprintf(shot,sizeof(shot),"automation-trv-wait-%u",themes[t]);screenshot(shot);
  m.devices[0].busy=m.devices[0].command_pending=false;m.devices[0].error=DSR_IO;dsr_mobile_update(&m);
  CHECK(find_label(lv_screen_active(),"22.0 °C"));CHECK(find_label(lv_screen_active(),"Storage error"));snprintf(shot,sizeof(shot),"automation-trv-error-%u",themes[t]);screenshot(shot);
  dsr_mobile_trv_request_state(3,44,1233,false);CHECK(find_label(lv_screen_active(),"22.0 °C"));
  dsr_mobile_trv_request_state(3,44,1234,false);m.devices[0].error=DSR_OK;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"20.0 °C"));
 }
 for(unsigned t=0;t<3;t++)for(unsigned l=0;l<3;l++)form_checks(themes[t],l);
 printf("automation LCD: %u checks; actual LVGL, no control call\n",checks);lv_deinit();return 0;
}
