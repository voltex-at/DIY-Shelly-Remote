#include "dsr_ui_internal.h"
#include "dsr_ui_cyber.h"
#include "dsr_ui_search.h"
#include "dsr_automation.h"
#include "dsr_workspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* LVGL owner only. Status refresh never rebuilds a focused form. */
static struct {
 cJSON *doc,*rule,*expected;
 uint32_t revision,id,next;
 unsigned page,step;
 bool active,saving,typing,deleting,dirty;
 char query[48],error[96],name_draft[192];
 lv_obj_t *body,*keyboard,*footer,*name,*status;
} form;
static const char *a(const char *pl,const char *en,const char *de)
{return dsr_sc_language()==1?en:dsr_sc_language()==2?de:pl;}
static const cJSON *get(const cJSON *v,const char *k){return cJSON_GetObjectItemCaseSensitive(v,k);}
static unsigned num(const cJSON *v,const char *k){const cJSON *n=get(v,k);return cJSON_IsNumber(n)?(unsigned)n->valueint:0;}
static const char *str(const cJSON *v,const char *k){const cJSON *s=get(v,k);return cJSON_IsString(s)?s->valuestring:"";}
static void draw(void);
static uint32_t gold(void){return dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xb16b00:0xffc107;}
static void fail(const char *text){snprintf(form.error,sizeof(form.error),"%s",text);}
static void clear(void){cJSON_Delete(form.doc);cJSON_Delete(form.expected);form.doc=form.rule=form.expected=NULL;}
void dsr_ui_automation_hide(void){clear();memset(&form,0,sizeof(form));}
static bool load(void)
{
 clear();char *json=dsr_workspace_snapshot();form.doc=json?cJSON_Parse(json):NULL;free(json);char error[96];
 if(!form.doc||!dsr_config_validate(form.doc,error)){clear();fail(a("Konfiguracja niedostępna","Configuration unavailable","Konfiguration nicht verfügbar"));return false;}
 form.revision=num(form.doc,"revision");return true;
}
static lv_obj_t *btn(lv_obj_t *p,const char *text,int x,int y,int w,lv_event_cb_t cb,uintptr_t value)
{
 lv_obj_t *b=dsr_sc_button(p,x,y,w,44,DSR_SC_PANEL,10,cb,(void *)value);
 lv_obj_t *l=dsr_sc_text(b,text,4,0,w-8,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(l);return b;
}
static int paragraph(lv_obj_t *p,const char *text,int y,bool important)
{
 lv_point_t size;lv_text_get_size(&size,text,dsr_sc_font(&dsr_roboto_14),0,0,260,LV_TEXT_FLAG_NONE);int h=size.y+24;if(h<48)h=48;
 lv_obj_t *box=dsr_sc_panel(p,4,y,280,h,DSR_SC_CARD,10);
 if(important){dsr_sc_border_color(box,lv_color_hex(gold()),0);dsr_sc_border_width(box,1,0);if(dsr_sc_style()==3)dsr_ui_cyber_warning(box,false);}
 dsr_sc_text(box,dsr_sc_user(text),10,12,260,&dsr_roboto_14,important?gold():DSR_SC_TEXT);return y+h+8;
}
static const char *state(unsigned state)
{
 switch(state){case DSR_AUTO_DISABLED:return a("Wyłączona","Disabled","Deaktiviert");case DSR_AUTO_ACTIVE:return a("Aktywna","Active","Aktiv");
 case DSR_AUTO_MANUAL:return a("Ręczna zmiana","Manual change","Manuell geändert");case DSR_AUTO_CONFIRMING:return a("Oczekiwanie na potwierdzenie","Waiting for confirmation","Bestätigung ausstehend");
 case DSR_AUTO_ERROR:return a("Błąd","Error","Fehler");default:return a("Oczekiwanie na dane","Waiting for data","Warte auf Daten");}
}
static const char *device_name(uint32_t id)
{const cJSON *d;cJSON_ArrayForEach(d,get(form.doc,"devices"))if(num(d,"id")==id)return str(d,"name");return "?";}
static void leave(lv_event_t *e)
{(void)e;if(form.saving)return;dsr_ui_automation_hide();dsr_ui_actions_open(1);}
static void list(lv_event_t *e)
{(void)e;if(form.saving)return;form.page=0;form.error[0]=0;load();draw();}
static void edit(lv_event_t *e)
{
 form.id=(uint32_t)(uintptr_t)lv_event_get_user_data(e);form.error[0]=0;
 if(!load()){draw();return;}cJSON *r;
 cJSON_ArrayForEach(r,get(form.doc,"automations"))if(num(r,"id")==form.id)form.rule=r;
 if(!form.id){
  cJSON *rules=cJSON_GetObjectItemCaseSensitive(form.doc,"automations");if(cJSON_GetArraySize(rules)>=8){fail(a("Limit 8 reguł","Limit: 8 rules","Maximal 8 Regeln"));draw();return;}
  uint32_t id=1;cJSON_ArrayForEach(r,rules)if(num(r,"id")>=id)id=num(r,"id")+1;
  if(!id||id>2147483647u)return;
  form.rule=cJSON_CreateObject();if(!form.rule)return;
  bool ok=cJSON_AddNumberToObject(form.rule,"id",id)&&cJSON_AddStringToObject(form.rule,"name","")&&cJSON_AddStringToObject(form.rule,"kind","contact_heating")&&
   cJSON_AddBoolToObject(form.rule,"enabled",false)&&cJSON_AddArrayToObject(form.rule,"sensors")&&cJSON_AddArrayToObject(form.rule,"targets")&&cJSON_AddNumberToObject(form.rule,"target_half",16);
  if(!ok||!cJSON_AddItemToArray(rules,form.rule)){cJSON_Delete(form.rule);form.rule=NULL;return;}form.id=id;
 }
 if(!form.rule)return;
 snprintf(form.name_draft,sizeof(form.name_draft),"%s",str(form.rule,"name"));
 form.page=1;form.step=0;form.deleting=form.dirty=false;form.query[0]=0;draw();
}
static bool selected(const cJSON *ids,uint32_t id)
{const cJSON *v;cJSON_ArrayForEach(v,ids)if(cJSON_IsNumber(v)&&(uint32_t)v->valueint==id)return true;return false;}
static bool occupied(uint32_t id)
{const cJSON *r;cJSON_ArrayForEach(r,get(form.doc,"automations"))if(r!=form.rule&&selected(get(r,"targets"),id))return true;return false;}
static bool typed(uint32_t id,bool trv)
{
 dsr_mobile_device_t d;if(!dsr_network_typed_device(id,&d)||!d.identity[0]||d.endpoint_kind!=DSR_ENDPOINT_BLU||d.generation!=DSR_GEN2_PLUS||!(d.blu.flags&DSR_BLU_VALID))return false;
 return trv?(d.blu.errors&DSR_BLU_TRV)!=0:(d.blu.flags&DSR_BLU_HAS_OPEN)&&!(d.blu.errors&DSR_BLU_TRV);
}
static void toggle(lv_event_t *e)
{
 if(form.saving)return;
 uint32_t id=(uint32_t)(uintptr_t)lv_event_get_user_data(e);cJSON *ids=cJSON_GetObjectItemCaseSensitive(form.rule,form.step==1?"sensors":"targets");
 if(selected(ids,id)){for(int i=0;i<cJSON_GetArraySize(ids);i++)if((uint32_t)cJSON_GetArrayItem(ids,i)->valueint==id){cJSON_DeleteItemFromArray(ids,i);break;}}
 else if(typed(id,form.step==2)&&!(form.step==2&&occupied(id))&&cJSON_GetArraySize(ids)<16){cJSON *v=cJSON_CreateNumber(id);if(!v||!cJSON_AddItemToArray(ids,v)){cJSON_Delete(v);fail(a("Błąd pamięci","Storage error","Speicherfehler"));}}
 form.dirty=true;draw();
}
static void changed(lv_event_t *e)
{
 if(form.saving)return;
 const char *text=lv_textarea_get_text(lv_event_get_target(e));snprintf(form.name_draft,sizeof(form.name_draft),"%s",text);form.dirty=true;if(strlen(text)>47){fail(a("Nazwa: maks. 47 bajtów","Name: max. 47 bytes","Name: max. 47 Bytes"));return;}
 cJSON *value=cJSON_CreateString(text);if(!value||!cJSON_ReplaceItemInObjectCaseSensitive(form.rule,"name",value)){cJSON_Delete(value);fail(a("Błąd pamięci","Storage error","Speicherfehler"));}
}
static void query_changed(lv_event_t *e){snprintf(form.query,sizeof(form.query),"%s",lv_textarea_get_text(lv_event_get_target(e)));}
static void keyboard_done(lv_event_t *e){(void)e;form.typing=false;draw();}
static void focus(lv_event_t *e)
{
 if(form.saving)return;
 form.typing=true;lv_obj_add_flag(form.footer,LV_OBJ_FLAG_HIDDEN);
 lv_obj_set_height(form.body,134);dsr_ui_keyboard_focus(form.keyboard,lv_event_get_target(e));
 lv_obj_set_size(form.keyboard,320,240);lv_obj_align(form.keyboard,LV_ALIGN_BOTTOM_MID,0,0);lv_obj_scroll_to_view(lv_event_get_target(e),LV_ANIM_OFF);
}
static void adjust(lv_event_t *e)
{
 if(form.saving)return;
 int half=(int)num(form.rule,"target_half")+(int)(intptr_t)lv_event_get_user_data(e);if(half<8||half>60)return;
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(form.rule,"target_half"),half);form.dirty=true;draw();
}
static void enabled(lv_event_t *e)
{if(!form.saving){cJSON_SetBoolValue(cJSON_GetObjectItemCaseSensitive(form.rule,"enabled"),lv_obj_has_state(lv_event_get_target(e),LV_STATE_CHECKED));form.dirty=true;}}
static void save(lv_event_t *e)
{
 (void)e;if(form.saving)return;char error[96];
 cJSON *candidate=cJSON_Duplicate(form.doc,true);
 if(!candidate){fail(a("Błąd pamięci","Storage error","Speicherfehler"));draw();return;}
 if(form.deleting){cJSON *rules=cJSON_GetObjectItemCaseSensitive(candidate,"automations");for(int i=0;i<cJSON_GetArraySize(rules);i++)if(num(cJSON_GetArrayItem(rules,i),"id")==form.id){cJSON_DeleteItemFromArray(rules,i);break;}}
 if(!dsr_config_validate(candidate,error)){cJSON_Delete(candidate);fail(a("Sprawdź nazwę i wybrane urządzenia.","Check the name and selected devices.","Name und ausgewählte Geräte prüfen."));draw();return;}
 if(!dsr_automation_config_allowed(candidate,error)){cJSON_Delete(candidate);fail(a("Przed zmianą aktywnej reguły wyłącz ją i zapisz.","Disable and save an active rule before changing it.","Aktive Regel vor Änderungen deaktivieren und speichern."));draw();return;}
 if(dsr_workspace_revision()!=form.revision){cJSON_Delete(candidate);fail(a("Konfiguracja zmieniona. Wróć i otwórz ponownie; szkic pozostaje.","Configuration changed. Reopen before saving; draft retained.","Konfiguration geändert. Neu öffnen; Entwurf bleibt erhalten."));draw();return;}
 char *json=cJSON_PrintUnformatted(candidate);if(!json){cJSON_Delete(candidate);fail(a("Błąd pamięci","Storage error","Speicherfehler"));draw();return;}
 cJSON_Delete(form.expected);form.expected=candidate;
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(form.expected,"revision"),form.revision+1);
 form.saving=dsr_workspace_submit(json,form.revision);free(json);
 fail(form.saving?a("Zapisywanie…","Saving…","Speichern…"):a("Nie zapisano. Szkic zachowany.","Not saved. Draft retained.","Nicht gespeichert. Entwurf erhalten."));draw();
}
static void next(lv_event_t *e)
{
 (void)e;if(form.saving)return;
 if(!form.step&&form.name&&strlen(form.name_draft)>47){fail(a("Nazwa: maks. 47 bajtów","Name: max. 47 bytes","Name: max. 47 Bytes"));draw();return;}
 if(!form.step&&(!*str(form.rule,"name")||strlen(str(form.rule,"name"))>47)){fail(a("Podaj nazwę (maks. 47 bajtów).","Enter a name (max. 47 bytes).","Namen eingeben (max. 47 Bytes)."));draw();return;}
 if((form.step==1||form.step==2)&&!cJSON_GetArraySize(get(form.rule,form.step==1?"sensors":"targets"))){fail(a("Wybierz urządzenie","Select a device","Gerät auswählen"));draw();return;}
 form.error[0]=0;if(form.step<3){form.step++;form.query[0]=0;draw();}else save(NULL);
}
static void previous(lv_event_t *e)
{(void)e;if(form.saving)return;if(form.page==2){list(NULL);return;}form.error[0]=0;if(form.deleting){form.deleting=false;draw();return;}if(form.step){form.step--;form.query[0]=0;draw();}else list(NULL);}
static void stay(lv_event_t *e){lv_obj_delete(lv_event_get_user_data(e));}
static void header_back(lv_event_t *e)
{
 if(form.saving)return;
 if(form.page!=1||form.step||!form.dirty){if(form.page)previous(e);else leave(e);return;}
 lv_obj_t *shade=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);lv_obj_set_style_bg_opa(shade,180,0);lv_obj_add_flag(shade,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *box=dsr_sc_panel(shade,12,140,296,180,DSR_SC_CARD,12);
 dsr_sc_text(box,a("Odrzucić niezapisane zmiany?","Discard unsaved changes?","Ungespeicherte Änderungen verwerfen?"),12,18,272,&dsr_roboto_16,gold());
 btn(box,dsr_sc_tr("Zostań"),8,122,132,stay,(uintptr_t)shade);btn(box,dsr_sc_tr("Odrzuć"),154,122,132,list,0);
}
static void deletion(lv_event_t *e)
{(void)e;if(!form.saving){form.deleting=true;draw();}}
static void remove_rule(lv_event_t *e)
{
 (void)e;if(form.saving)return;
 save(NULL);
}
static const char *read_reason(dsr_result_t result)
{
 switch(result){
 case DSR_UNSUPPORTED:return a("Bezpośredni odczyt TRV lub nastawa nieobsługiwane.","Direct TRV read or target is unsupported.","Direkte TRV-Abfrage oder Sollwert nicht unterstützt.");
 case DSR_AUTH_REQUIRED:return a("Wymagane logowanie do bramki.","Gateway authentication required.","Gateway-Anmeldung erforderlich.");
 case DSR_IDENTITY_MISMATCH:return a("Powiązanie termostatu zmienione. Sprawdź konfigurację.","Thermostat binding changed. Review configuration.","Thermostat-Zuordnung geändert. Konfiguration prüfen.");
 case DSR_RATE_LIMITED:return a("Limit zapytań. Oczekiwanie przed ponowieniem.","Rate limited. Waiting before retry.","Abfragelimit. Wartezeit vor Wiederholung.");
 case DSR_BUSY:return a("Oczekiwanie na dostęp do urządzenia.","Waiting for device access.","Warte auf Gerätezugriff.");
 case DSR_OFFLINE:return a("Brak odpowiedzi termostatu. Oczekiwanie na nowy odczyt.","Thermostat did not respond. Waiting for a new read.","Thermostat antwortet nicht. Warte auf neue Abfrage.");
 case DSR_IO:return a("Błąd pamięci.","Storage error.","Speicherfehler.");
 default:return a("Niepoprawny odczyt termostatu.","Invalid thermostat reading.","Ungültiger Thermostatwert.");
 }
}
static void details(lv_event_t *e){form.id=(uint32_t)(uintptr_t)lv_event_get_user_data(e);form.page=2;form.error[0]=0;draw();}
static void status_page(void)
{
 dsr_auto_snapshot_t s;dsr_automation_snapshot(&s);int y=4;const dsr_auto_rule_status_t *r=NULL;
 for(unsigned i=0;i<s.count;i++)if(s.rules[i].rule_id==form.id)r=&s.rules[i];
 const cJSON *rule;cJSON_ArrayForEach(rule,get(form.doc,"automations"))if(num(rule,"id")==form.id)break;
 bool target_error=false;for(unsigned i=0;i<s.target_count;i++)if(selected(get(rule,"targets"),s.targets[i].layout_id)&&r&&s.targets[i].result!=DSR_OK&&s.targets[i].result==r->result)target_error=true;
 y=paragraph(form.body,r?state(r->state):state(DSR_AUTO_WAITING),y,r&&(r->state>=DSR_AUTO_MANUAL));
 if(r&&r->missing_id){char text[128];snprintf(text,sizeof(text),"%s: %s (#%u)",a("Brak nowego raportu","Waiting for a new report","Neuer Bericht fehlt"),device_name(r->missing_id),(unsigned)r->missing_id);y=paragraph(form.body,text,y,true);}
 if(r&&r->result!=DSR_OK&&!target_error)y=paragraph(form.body,r->result==DSR_CONFLICT?a("Sprawdź powiązania. Wyłącz, zapisz i włącz regułę ponownie.","Review bindings. Disable, save, then enable the rule again.","Zuordnung prüfen. Regel deaktivieren, speichern und erneut aktivieren."):r->result==DSR_IO?a("Błąd dziennika. Automatyzacja wstrzymana.","Journal error. Automation paused.","Journalfehler. Automatik angehalten."):read_reason(r->result),y,true);
 for(unsigned i=0;i<s.target_count;i++)if(selected(get(rule,"targets"),s.targets[i].layout_id)){
  char text[192],baseline[24]="—",requested[24]="—",read[24]="—";
  if(s.targets[i].baseline)snprintf(baseline,24,"%.1f °C",s.targets[i].baseline/2.0);
  if(s.targets[i].requested)snprintf(requested,24,"%.1f °C",s.targets[i].requested/2.0);
  if(s.targets[i].read_half)snprintf(read,24,"%.1f °C",s.targets[i].read_half/2.0);
  snprintf(text,sizeof(text),"%s\n%s: %s\n%s: %s\n%s: %s",device_name(s.targets[i].layout_id),a("Poprzednia","Previous","Vorher"),baseline,a("Żądana","Requested","Angefordert"),requested,a("Odczytana","Read back","Ausgelesen"),read);y=paragraph(form.body,text,y,false);
  if(s.targets[i].result!=DSR_OK)y=paragraph(form.body,read_reason(s.targets[i].result),y,true);
 }
 paragraph(form.body,a("Wznowienie wymaga nowych raportów wszystkich kontaktów i odczytów TRV. Tryb beacon można włączyć samodzielnie w Shelly.","Resume requires new reports from all contacts and TRV reads. Beacon mode can be enabled manually in Shelly.","Fortsetzung nach neuen Berichten aller Kontakte und TRV-Abfragen. Beacon-Modus bei Bedarf selbst in Shelly aktivieren."),y,false);
}
static void selector(void)
{
 bool trv=form.step==2;const char *key=trv?"targets":"sensors";
 dsr_sc_text(form.body,trv?a("Termostaty","Thermostats","Thermostate"):a("Okna i drzwi","Windows and doors","Fenster und Türen"),4,4,280,&dsr_roboto_18,DSR_SC_TEXT);
 lv_obj_t *q=lv_textarea_create(form.body);dsr_ui_textarea(q);lv_obj_set_pos(q,4,34);lv_obj_set_size(q,280,44);lv_textarea_set_one_line(q,true);lv_textarea_set_max_length(q,47);lv_textarea_set_text(q,form.query);lv_textarea_set_placeholder_text(q,dsr_sc_tr("Szukaj"));
 lv_obj_add_event_cb(q,focus,LV_EVENT_FOCUSED,NULL);lv_obj_add_event_cb(q,query_changed,LV_EVENT_VALUE_CHANGED,NULL);
 const cJSON *d;int y=86;unsigned shown=0;cJSON_ArrayForEach(d,get(form.doc,"devices")){
  uint32_t id=num(d,"id");bool checked=selected(get(form.rule,key),id),eligible=typed(id,trv);
  if((!eligible&&!checked)||!dsr_ui_search_match(str(d,"name"),form.query))continue;
  lv_obj_t *b=btn(form.body,dsr_sc_user(str(d,"name")),4,y,280,toggle,id);
  lv_obj_set_style_pad_left(b,24,0);if(checked){dsr_sc_border_color(b,dsr_sc_color(DSR_SC_BLUE),0);dsr_sc_border_width(b,2,0);dsr_sc_text(b,"●",5,10,18,&dsr_roboto_14,DSR_SC_BLUE);}
  if(!checked&&trv&&occupied(id))lv_obj_add_state(b,LV_STATE_DISABLED);
  y+=52;shown++;
 }
 if(!shown)paragraph(form.body,a("Brak zweryfikowanych urządzeń. Najpierw dodaj czujnik BLU lub termostat TRV przez bramkę.","No verified devices. First add a BLU contact or TRV through its gateway.","Keine verifizierten Geräte. Zuerst BLU-Kontakt oder TRV über das Gateway hinzufügen."),y,true);
}
static void disable_tree(lv_obj_t *o)
{lv_obj_add_state(o,LV_STATE_DISABLED);for(unsigned i=0;i<lv_obj_get_child_count(o);i++)disable_tree(lv_obj_get_child(o,(int)i));}
static void draw(void)
{
 if(!form.active)return;
 form.typing=false;form.name=form.status=form.keyboard=NULL;
 lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);dsr_sc_background(root);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
 dsr_sc_header(root,"",false,dsr_wifi_connected(),header_back,NULL);
 dsr_sc_text(root,a("Automatyzacje","Automations","Automatisierungen"),12,50,296,&dsr_roboto_18,DSR_SC_TEXT);
 if(form.page==1){char step[48];snprintf(step,sizeof(step),"%u/4 | %s",form.step+1,a("Okno / drzwi - ogrzewanie","Window / door - heating","Fenster / Tür - Heizung"));dsr_sc_text(root,dsr_sc_user(step),12,78,296,&dsr_roboto_11,DSR_SC_MUTED);}
 form.body=dsr_sc_panel(root,12,96,296,308,DSR_SC_BG,0);lv_obj_set_style_bg_opa(form.body,LV_OPA_TRANSP,0);lv_obj_add_flag(form.body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(form.body,LV_DIR_VER);dsr_sc_scrollbar(form.body);
 form.footer=dsr_sc_panel(root,0,416,320,64,DSR_SC_BG,0);lv_obj_set_style_border_width(form.footer,0,0);
 if(!form.page){
  btn(form.body,a("Automatyzacja +","Automation +","Automatisierung +"),4,4,280,edit,0);int y=56;const cJSON *r;dsr_auto_snapshot_t s;dsr_automation_snapshot(&s);
  cJSON_ArrayForEach(r,get(form.doc,"automations")){
   uint32_t id=num(r,"id");unsigned st=cJSON_IsTrue(get(r,"enabled"))?DSR_AUTO_WAITING:DSR_AUTO_DISABLED;for(unsigned i=0;i<s.count;i++)if(s.rules[i].rule_id==id)st=s.rules[i].state;
   lv_obj_t *box=dsr_sc_panel(form.body,4,y,280,106,DSR_SC_CARD,10);lv_obj_t *name=dsr_sc_text(box,dsr_sc_user(str(r,"name")),10,8,210,&dsr_roboto_16,DSR_SC_TEXT);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);lv_obj_set_height(name,22);
   lv_obj_t *b=dsr_sc_button(box,228,4,44,44,DSR_SC_PANEL,8,edit,(void *)(uintptr_t)id);dsr_sc_icon_centered(b,DSR_ICON_EDIT,24,DSR_SC_BLUE);
   btn(box,state(st),8,50,264,details,id);y+=114;
  }
  btn(form.footer,dsr_sc_tr("Wstecz"),12,4,296,leave,0);
 }else if(form.page==2){status_page();btn(form.footer,dsr_sc_tr("Wstecz"),12,4,296,list,0);
 }else{
  if(form.deleting){paragraph(form.body,a("Usunąć regułę? Przyszłe polecenia zostaną anulowane. Temperatura nie zostanie automatycznie przywrócona.","Delete rule? Future commands will be cancelled. Temperature will not be restored automatically.","Regel löschen? Künftige Befehle werden abgebrochen. Die Temperatur wird nicht automatisch wiederhergestellt."),4,true);}
  else if(!form.step){
   dsr_sc_text(form.body,dsr_sc_tr("Nazwa"),4,4,280,&dsr_roboto_14,DSR_SC_TEXT);
   form.name=lv_textarea_create(form.body);dsr_ui_textarea(form.name);lv_obj_set_pos(form.name,4,30);lv_obj_set_size(form.name,280,44);lv_textarea_set_one_line(form.name,true);lv_textarea_set_max_length(form.name,47);lv_textarea_set_text(form.name,form.name_draft);lv_obj_add_event_cb(form.name,changed,LV_EVENT_VALUE_CHANGED,NULL);lv_obj_add_event_cb(form.name,focus,LV_EVENT_FOCUSED,NULL);
   dsr_sc_text(form.body,a("Temperatura przy otwarciu","Temperature while open","Temperatur bei Öffnung"),4,94,280,&dsr_roboto_14,DSR_SC_TEXT);
   btn(form.body,"−",4,128,48,adjust,(uintptr_t)(intptr_t)-1);char text[32];snprintf(text,sizeof(text),"%.1f °C",num(form.rule,"target_half")/2.0);lv_obj_t *v=dsr_sc_text(form.body,text,62,134,160,&dsr_roboto_22,DSR_SC_TEXT);lv_obj_set_style_text_align(v,LV_TEXT_ALIGN_CENTER,0);btn(form.body,"+",236,128,48,adjust,1);
   paragraph(form.body,a("Reakcja natychmiastowa. Powrót po zamknięciu wszystkich kontaktów. Ręczna zmiana zostaje zachowana.","Immediate response. Restore after all contacts close. Manual changes are preserved.","Sofortige Reaktion. Rückkehr nach Schließen aller Kontakte. Manuelle Änderungen bleiben erhalten."),188,false);
  }else if(form.step<3)selector();else{
   int y=paragraph(form.body,str(form.rule,"name"),4,false);char text[96];snprintf(text,sizeof(text),"%s: %d\n%s: %d\n%.1f °C",a("Kontakty","Contacts","Kontakte"),cJSON_GetArraySize(get(form.rule,"sensors")),a("Termostaty","Thermostats","Thermostate"),cJSON_GetArraySize(get(form.rule,"targets")),num(form.rule,"target_half")/2.0);y=paragraph(form.body,text,y,false);
   lv_obj_t *c=lv_checkbox_create(form.body);lv_checkbox_set_text(c,a("Włącz regułę","Enable rule","Regel aktivieren"));lv_obj_set_pos(c,12,y+8);lv_obj_set_size(c,264,44);lv_obj_set_style_text_font(c,&dsr_roboto_16,0);lv_obj_set_style_text_color(c,dsr_sc_color(DSR_SC_TEXT),0);if(cJSON_IsTrue(get(form.rule,"enabled")))lv_obj_add_state(c,LV_STATE_CHECKED);lv_obj_add_event_cb(c,enabled,LV_EVENT_VALUE_CHANGED,NULL);y+=60;
   y=paragraph(form.body,a("Wyłączenie lub usunięcie zatrzymuje kolejne polecenia. Zmiana czujników, celów lub temperatury aktywnej reguły wymaga jej wyłączenia i zapisu.","Disabling or deleting stops future commands. Disable and save an active rule before changing contacts, targets or temperature.","Deaktivieren oder Löschen stoppt künftige Befehle. Aktive Regel vor Änderungen an Kontakten, Zielen oder Temperatur deaktivieren und speichern."),y,true);
   btn(form.body,a("Usuń regułę","Delete rule","Regel löschen"),4,y,280,deletion,0);
  }
  btn(form.footer,dsr_sc_tr(form.step||form.deleting?"Wstecz":"Anuluj"),12,4,140,previous,0);
  lv_obj_t *b=btn(form.footer,form.deleting?dsr_sc_tr("Usuń"):dsr_sc_tr(form.step==3?"Zapisz":"Dalej"),168,4,140,form.deleting?remove_rule:next,0);dsr_ui_primary(b);if(form.saving)lv_obj_add_state(b,LV_STATE_DISABLED);

 }
 if(form.error[0]){lv_point_t size;lv_text_get_size(&size,form.error,dsr_sc_font(&dsr_roboto_12),0,0,276,LV_TEXT_FLAG_NONE);int height=size.y+20;if(height<60)height=60;int top=408-height;lv_obj_set_height(form.body,top-104);lv_obj_t *notice=dsr_sc_panel(root,12,top,296,height,DSR_SC_CARD,10);dsr_sc_border_color(notice,lv_color_hex(gold()),0);if(dsr_sc_style()==3)dsr_ui_cyber_warning(notice,false);form.status=dsr_sc_text(notice,dsr_sc_user(form.error),10,10,276,&dsr_roboto_12,gold());}
 if(form.saving){disable_tree(form.body);disable_tree(form.footer);}
 form.keyboard=lv_keyboard_create(root);dsr_ui_keyboard(form.keyboard);lv_obj_add_event_cb(form.keyboard,keyboard_done,LV_EVENT_READY,NULL);lv_obj_add_event_cb(form.keyboard,keyboard_done,LV_EVENT_CANCEL,NULL);lv_obj_add_flag(form.keyboard,LV_OBJ_FLAG_HIDDEN);
 lv_obj_update_layout(root);form.next=lv_tick_get()+1500;
}
void dsr_ui_automation_open(void)
{dsr_network_prepare_home();dsr_mobile_hide();dsr_ui_automation_hide();form.active=true;load();draw();}
void dsr_ui_automation_tick(void)
{
 if(!form.active)return;
 if(form.saving){bool busy;char message[96];dsr_workspace_status(&busy,message);if(busy)return;
  form.saving=false;char *json=dsr_workspace_snapshot();cJSON *actual=json?cJSON_Parse(json):NULL;free(json);
  bool ok=actual&&form.expected&&cJSON_Compare(actual,form.expected,true);cJSON_Delete(actual);
  if(ok){form.error[0]=0;form.page=0;load();}else fail(a("Nie zapisano. Szkic zachowany. Sprawdź konflikt lub pamięć.","Not saved. Draft retained. Check conflict or storage.","Nicht gespeichert. Entwurf bleibt erhalten. Konflikt oder Speicher prüfen."));draw();return;
 }
 bool held=false;for(lv_indev_t *i=lv_indev_get_next(NULL);i;i=lv_indev_get_next(i))if(lv_indev_get_type(i)==LV_INDEV_TYPE_POINTER&&(lv_indev_get_state(i)==LV_INDEV_STATE_PRESSED||lv_indev_get_scroll_obj(i)))held=true;
 if(!form.typing&&!held&&form.page!=1&&(int32_t)(lv_tick_get()-form.next)>=0){int scroll=lv_obj_get_scroll_y(form.body);if(dsr_workspace_revision()!=form.revision)load();draw();lv_obj_scroll_to_y(form.body,scroll,LV_ANIM_OFF);}
}
