#include "dsr_ui_style.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static uint32_t accent=DSR_SC_BLUE;
static bool midnight;
static unsigned language;
unsigned dsr_sc_language(void){return language;}
void dsr_sc_appearance(uint32_t color,bool night,unsigned locale){accent=color;midnight=night;language=locale<3?locale:0;}
lv_color_t dsr_sc_color(uint32_t c)
{if(c==DSR_SC_BLUE)c=accent;if(midnight&&c==DSR_SC_BG)c=0x080f19;return lv_color_hex(c);}
const char *dsr_sc_tr(const char *text)
{
 static const char *const words[][3]={
 {"Mój dom","My home","Mein Zuhause"},{"Mój pulpit","My dashboard","Mein Dashboard"},{"Aktywność","Activity","Aktivität"},
 {"Pomieszczenia","Rooms","Räume"},{"Urządzenia","Devices","Geräte"},{"Grupy","Groups","Gruppen"},{"Sceny","Scenes","Szenen"},
 {"Sterowanie","Controls","Steuerung"},{"Wykresy","Charts","Diagramme"},{"Ustawienia","Settings","Einstellungen"},
 {"Główne sterowanie","Main control","Hauptsteuerung"},{"Włączone","On","Ein"},{"Wyłączone","Off","Aus"},
 {"Włącz","Turn on","Einschalten"},{"Wyłącz","Turn off","Ausschalten"},{"Otwarte","Open","Offen"},{"Zamknięte","Closed","Geschlossen"},
 {"Brak połączenia","No connection","Keine Verbindung"},{"Nieprzypisane","Unassigned","Nicht zugeordnet"},{"Roleta","Cover","Rollladen"},
 {"Oczekiwanie…","Waiting…","Warten…"},{"Moc chwilowa","Current power","Aktuelle Leistung"},{"Pomiar mocy niedostępny","Power not available","Keine Leistungsmessung"},
 {"Napięcie","Voltage","Spannung"},{"Natężenie","Current","Stromstärke"},{"Brak urządzeń","No devices","Keine Geräte"},
 {"Brak pomieszczeń","No rooms","Keine Räume"},{"DEMO • dane przykładowe","DEMO • sample data","DEMO • Beispieldaten"},
 {"Wstecz","Back","Zurück"},{"Zapisz","Save","Speichern"},{"Edytuj pilot","Edit remote","Fernbedienung bearbeiten"},
 {"Pulpity","Dashboards","Dashboards"},{"Wygląd i język","Appearance & language","Design und Sprache"},
 {"+ Dodaj wpis","+ Add item","+ Eintrag hinzufügen"},{"Usuń wpis","Delete item","Eintrag löschen"},{"Edytuj konfigurację","Edit configuration","Konfiguration bearbeiten"},
 {"Edycja • wspólna konfiguracja","Edit • shared configuration","Gemeinsame Konfiguration"},
 {"Nazwa","Name","Name"},{"Ikona","Icon","Symbol"},{"Typ","Type","Typ"},{"Adres IPv4 Shelly","Shelly IPv4 address","Shelly IPv4-Adresse"},
 {"Kanał (0–15)","Channel (0–15)","Kanal (0–15)"},{"ID pomieszczenia (0 = brak)","Room ID (0 = none)","Raum-ID (0 = keiner)"},
 {"Obiekt","Object","Objekt"},{"Rozmiar","Size","Größe"},{"Dane","Data","Daten"},{"Góra","Up","Hoch"},{"Dół","Down","Runter"},{"Usuń","Remove","Entfernen"},
 {"+ Urządzenie","+ Device","+ Gerät"},{"+ Pomieszczenie","+ Room","+ Raum"},{"+ Scena","+ Scene","+ Szene"},
 {"Web Panel • parowanie","Web panel • pairing","Webpanel • Kopplung"},{"Parowanie Web Panelu","Web panel pairing","Webpanel koppeln"},
 {"Jednorazowy kod • ważny 2 minuty","One-time code • valid 2 minutes","Einmalcode • 2 Minuten gültig"},
 {"Zezwól na OTA • 2 minuty","Allow OTA • 2 minutes","OTA erlauben • 2 Minuten"},
 {"Zmiany zostaną zastosowane po zapisaniu","Changes apply after saving","Änderungen nach dem Speichern"},
 {"Bateria","Battery","Batterie"},{"Pozycja","Position","Position"},
 {"Otwórz","Open","Öffnen"},{"Zamknij","Close","Schließen"},
 {"Otwieranie","Opening","Wird geöffnet"},{"Zamykanie","Closing","Wird geschlossen"},
 {"Zatrzymano","Stopped","Gestoppt"},{"Kalibracja","Calibration","Kalibrierung"},
 {"Nieznana pozycja","Position unknown","Position unbekannt"},{"Stan nieznany","State unknown","Zustand unbekannt"},
 {"Brak obsługi Cover RPC","Cover RPC not implemented","Cover RPC nicht implementiert"},
 {"Roleta w salonie","Living room cover","Rollladen Wohnzimmer"},
 {"Podgląd rolety • DEMO","Cover preview • DEMO","Rollladen-Vorschau • DEMO"},
 {"Podgląd grafiki bez sterowania sprzętem.","Visual preview without hardware control.","Designvorschau ohne Gerätesteuerung."},
 {"Offline • stan niepotwierdzony","Offline • state unconfirmed","Offline • Zustand unbestätigt"},
 {"Włączone • Online","On • Online","Ein • Online"},{"Wyłączone • Online","Off • Online","Aus • Online"},
 {"Oczekiwanie na odczyt…","Waiting for reading…","Messwert wird abgerufen…"},
 {"Brak dodatkowych pomiarów","No additional readings","Keine weiteren Messwerte"},
#include "dsr_ui_strings.inc"
 };
 for(unsigned i=0;i<sizeof(words)/sizeof(words[0]);i++)for(unsigned key=0;key<3;key++)if(!strcmp(text,words[i][key]))return words[i][language];
 return text;
}
void dsr_sc_label_set_text(lv_obj_t *label,const char *text){lv_label_set_text(label,dsr_sc_tr(text));}
void dsr_sc_label_set_text_fmt(lv_obj_t *label,const char *format,...)
{
 char text[512];va_list args;va_start(args,format);vsnprintf(text,sizeof(text),dsr_sc_tr(format),args);va_end(args);lv_label_set_text(label,text);
}

lv_obj_t *dsr_sc_panel(lv_obj_t *p,int x,int y,int w,int h,uint32_t color,int radius)
{
    lv_obj_t *o=lv_obj_create(p);lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(o,dsr_sc_color(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
    lv_obj_set_style_radius(o,radius,0);lv_obj_set_style_pad_all(o,0,0);
    lv_obj_set_style_text_color(o,lv_color_hex(DSR_SC_TEXT),0);
    return o;
}
lv_obj_t *dsr_sc_text(lv_obj_t *p,const char *s,int x,int y,int w,const lv_font_t *font,uint32_t color)
{
    lv_obj_t *o=lv_label_create(p);lv_obj_remove_style_all(o);lv_label_set_text(o,dsr_sc_tr(s));
    lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);lv_obj_set_style_text_font(o,font,0);
    lv_obj_set_style_text_color(o,dsr_sc_color(color),0);lv_label_set_long_mode(o,LV_LABEL_LONG_MODE_WRAP);
    return o;
}
lv_obj_t *dsr_sc_button(lv_obj_t *p,int x,int y,int w,int h,uint32_t color,int radius,lv_event_cb_t cb,void *ctx)
{
    lv_obj_t *o=dsr_sc_panel(p,x,y,w,h,color,radius);
    lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_bg_color(o,lv_color_hex(0x344f71),LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(o,lv_color_hex(0x30373d),LV_STATE_DISABLED);
    lv_obj_set_style_opa(o,LV_OPA_COVER,LV_STATE_DISABLED);
    if(cb)lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,ctx);
    return o;
}

/* Apache-2.0 Material Symbols Rounded, pre-filtered to coverage masks.
 * No SVG parser, web font, or integer-grid line approximation at runtime. */
extern const lv_image_dsc_t dsr_symbol_atlas[24];
static void icon_draw(lv_event_t *event)
{
    lv_obj_t *obj=lv_event_get_target(event);
    unsigned kind=(unsigned)(uintptr_t)lv_event_get_user_data(event);
    if(kind>=24)return;
    lv_area_t area;lv_obj_get_coords(obj,&area);
    int size=lv_obj_get_width(obj);
    lv_draw_image_dsc_t draw;lv_draw_image_dsc_init(&draw);
    draw.src=&dsr_symbol_atlas[kind];draw.recolor=lv_obj_get_style_line_color(obj,0);
    draw.recolor_opa=LV_OPA_COVER;draw.antialias=1;
    draw.scale_x=draw.scale_y=(uint32_t)size*256/64;
    draw.pivot=(lv_point_t){0,0};
    lv_area_t source={area.x1,area.y1,area.x1+63,area.y1+63};
    lv_draw_image(lv_event_get_layer(event),&draw,&source);
}
lv_obj_t *dsr_sc_icon(lv_obj_t *p,dsr_ui_icon_t kind,int x,int y,int size,uint32_t color)
{
    lv_obj_t *o=lv_obj_create(p);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,size,size);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_line_color(o,dsr_sc_color(color),0);
    lv_obj_add_event_cb(o,icon_draw,LV_EVENT_DRAW_MAIN,(void *)(uintptr_t)kind);return o;
}
lv_obj_t *dsr_sc_icon_centered(lv_obj_t *p,dsr_ui_icon_t kind,int size,uint32_t color)
{
    lv_obj_t *o=dsr_sc_icon(p,kind,0,0,size,color);lv_obj_center(o);return o;
}
void dsr_sc_icon_color(lv_obj_t *o,uint32_t color){lv_obj_set_style_line_color(o,lv_color_hex(color),0);lv_obj_invalidate(o);}

void dsr_sc_header(lv_obj_t *root,const char *title,bool demo,bool wifi,lv_event_cb_t back,void *context)
{
 if(back){lv_obj_t *b=dsr_sc_button(root,0,0,44,44,DSR_SC_BG,8,back,context);dsr_sc_icon_centered(b,DSR_ICON_BACK,24,DSR_SC_TEXT);}
 lv_obj_t *name=dsr_sc_text(root,title,back?44:12,9,back?204:232,&dsr_roboto_18,DSR_SC_TEXT);lv_obj_set_height(name,24);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_DOTS);
 if(demo){lv_obj_t *badge=dsr_sc_panel(root,256,9,54,21,0x5c492a,5);lv_obj_set_style_border_width(badge,1,0);lv_obj_set_style_border_color(badge,lv_color_hex(0xe1bb70),0);dsr_sc_text(badge,"DEMO",10,4,40,&dsr_roboto_12,0xffd68a);}
 else dsr_sc_icon(root,DSR_ICON_WIFI,280,10,22,wifi?DSR_SC_BLUE:DSR_SC_OFF);
}
void dsr_sc_navigation(lv_obj_t *root,unsigned current,unsigned enabled,lv_event_cb_t callback)
{
 lv_obj_t *bar=dsr_sc_panel(root,0,428,320,52,DSR_SC_NAV,0);
 dsr_sc_panel(bar,0,0,320,1,0x60646b,0);
 lv_obj_set_style_bg_grad_color(bar,lv_color_hex(0x232b33),0);lv_obj_set_style_bg_grad_dir(bar,LV_GRAD_DIR_VER,0);
 if(current<5)dsr_sc_panel(bar,16+(int)current*64,0,32,3,DSR_SC_BLUE,2);
 const dsr_ui_icon_t icons[]={DSR_ICON_DASHBOARD,DSR_ICON_HOME,DSR_ICON_ENERGY,DSR_ICON_SETTINGS,DSR_ICON_PLUS};
 for(unsigned i=0;i<5;i++){
  lv_obj_t *b=dsr_sc_button(bar,(int)i*64,4,64,47,DSR_SC_NAV,0,callback,(void *)(uintptr_t)i);lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,0);
  if(i==4){lv_obj_t *circle=dsr_sc_panel(b,0,0,36,36,DSR_SC_BLUE,LV_RADIUS_CIRCLE);lv_obj_center(circle);dsr_sc_icon_centered(circle,icons[i],20,DSR_SC_TEXT);}
  else dsr_sc_icon_centered(b,icons[i],26,i==current?DSR_SC_TEXT:DSR_SC_MUTED);
  if(!(enabled&(1u<<i)))lv_obj_add_state(b,LV_STATE_DISABLED);
 }
}
lv_obj_t *dsr_sc_chip(lv_obj_t *parent,const char *text,int x,int y,int max_width,uint32_t color)
{
 lv_point_t size;lv_text_get_size(&size,dsr_sc_tr(text),&dsr_roboto_12,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 int width=size.x+14;if(width>max_width)width=max_width;
 lv_obj_t *chip=dsr_sc_panel(parent,x,y,width,20,0x292d30,6);
 lv_obj_t *label=dsr_sc_text(chip,text,7,3,width-14,&dsr_roboto_12,color);lv_obj_set_height(label,15);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_DOTS);return chip;
}
