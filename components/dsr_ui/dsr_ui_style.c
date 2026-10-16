#include "dsr_ui_style.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#ifndef DSR_UI_REVIEW_DEMO
void dsr_ui_battery_header(lv_obj_t *root);
#endif

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
 {"Bateria i zasilanie","Battery and power","Akku und Stromversorgung"},
 {"Napięcie wejścia BAT","BAT input voltage","Spannung am BAT-Eingang"},
 {"Ekran","Display","Bildschirm"},
 {"Błąd sterownika ekranu.","Display driver failed.","Bildschirmtreiber fehlgeschlagen."},
 {"Wygaszanie ekranu","Screen timeout","Bildschirm-Timeout"},
 {"Zapisywanie ustawień…","Saving settings…","Einstellungen speichern…"},
 {"Zmiany obowiązują po zapisaniu.","Changes apply after saving.","Änderungen gelten nach dem Speichern."},
 {"Nie potwierdzono zapisu. Spróbuj ponownie.","Save not confirmed. Please retry.","Speichern nicht bestätigt. Erneut versuchen."},
 {"Zapisano. Błąd sterownika ekranu.","Saved. Display driver failed.","Gespeichert. Bildschirmtreiber fehlgeschlagen."},
 {"Ustawienia zapisane.","Settings saved.","Einstellungen gespeichert."},
 {"Pomiar na żądanie","Measurement on request","Messung auf Anfrage"},
 {"Brak pomiaru","No measurement","Keine Messung"},
 {"Odczyt napięcia…","Reading voltage…","Spannung wird gemessen…"},
 {"Pomiar niedostępny. Spróbuj ponownie.","Measurement unavailable. Try again.","Messung nicht verfügbar. Erneut versuchen."},
 {"Kalibracja ADC niedostępna. Napięcie nieznane.","ADC calibration unavailable. Voltage unknown.","ADC-Kalibrierung fehlt. Spannung unbekannt."},
 {"Napięcie BAT • dzielnik nominalny 2:1","BAT voltage • nominal 2:1 divider","BAT-Spannung • nominaler Teiler 2:1"},
 {"Pomiar nieaktualny. Zmierz ponownie.","Measurement is stale. Measure again.","Messung veraltet. Erneut messen."},
 {"Ostatni odczyt: %lu s temu","Last reading: %lu s ago","Letzte Messung: vor %lu s"},
 {"Ładowanie i obecność akumulatora: brak wiarygodnych danych.","Charging and battery presence: no reliable data.","Ladestatus und Akku-Erkennung: keine verlässlichen Daten."},
 {"Procent baterii wymaga potwierdzenia typu ogniwa i pomiaru multimetrem.","Battery percentage needs the cell type and a multimeter check.","Für den Akkustand sind Zelltyp und Multimeterprüfung erforderlich."},
 {"Zmierz napięcie","Measure voltage","Spannung messen"},
 {"Pilot jest zajęty. Spróbuj ponownie.","Remote is busy. Try again.","Fernbedienung beschäftigt. Erneut versuchen."},
 {"Konfiguracja niedostępna","Configuration unavailable","Konfiguration nicht verfügbar"},
 {"Nie można otworzyć edytora","Cannot open editor","Editor kann nicht geöffnet werden"},
 {"Diagnostyka","Diagnostics","Diagnose"},{"Diagnostyka konfiguracji","Configuration diagnostics","Konfigurationsdiagnose"},
 {"Tylko odczyt • dane zachowane","Read only • data preserved","Nur Lesen • Daten erhalten"},
 {"SPIFFS: zamontowany","SPIFFS: mounted","SPIFFS: eingebunden"},
 {"SPIFFS: niepusty, niezamontowany","SPIFFS: nonempty, not mounted","SPIFFS: belegt, nicht eingebunden"},
 {"SPIFFS: błąd odczytu","SPIFFS: read error","SPIFFS: Lesefehler"},
 {"SPIFFS: błąd inicjalizacji","SPIFFS: provisioning error","SPIFFS: Initialisierungsfehler"},
 {"SPIFFS: niedostępny","SPIFFS: unavailable","SPIFFS: nicht verfügbar"},
 {"HTTPS uruchomiony","HTTPS running","HTTPS gestartet"},
 {"HTTPS: błąd uruchomienia","HTTPS: startup failed","HTTPS: Start fehlgeschlagen"},
 {"HTTPS nie został uruchomiony","HTTPS was not started","HTTPS wurde nicht gestartet"},
 {"USB 115200 • zachowaj kopię partycji przed naprawą.","USB 115200 • back up partitions before repair.","USB 115200 • Partitionen vor der Reparatur sichern."},
 {"Nie kasuj i nie formatuj pamięci.","Do not erase or format Flash.","Flash nicht löschen oder formatieren."},
 {"Serwer HTTPS jest niedostępny. Kod parowania nie został utworzony.","HTTPS is unavailable. No pairing code was generated.","HTTPS ist nicht verfügbar. Es wurde kein Kopplungscode erzeugt."},
 {"Magazyn konfiguracji niedostępny: niepusta partycja SPIFFS nie została zamontowana.","Configuration storage unavailable: nonempty SPIFFS partition could not be mounted.","Konfigurationsspeicher nicht verfügbar: belegte SPIFFS-Partition konnte nicht eingebunden werden."},
 {"Magazyn konfiguracji jest niedostępny.","Configuration storage is unavailable.","Konfigurationsspeicher ist nicht verfügbar."},
 {"Nie można odczytać poprawnej konfiguracji. Sprawdź pamięć i kopię danych.","Cannot read a valid configuration. Check storage and the backup.","Keine gültige Konfiguration lesbar. Speicher und Sicherung prüfen."},
 {"Brak pamięci do otwarcia edytora. Ustawienia pozostają zachowane.","Not enough memory to open the editor. Settings are preserved.","Nicht genug Speicher für den Editor. Einstellungen bleiben erhalten."},
 {"Dane nie zostały skasowane. Szczegóły są w diagnostyce i logu USB.","Data was not erased. Details are in diagnostics and the USB log.","Daten wurden nicht gelöscht. Details in Diagnose und USB-Protokoll."},
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
 {"Informacje o pulpicie","Dashboard information","Dashboard-Informationen"},
 {"Urządzenia: %u • online: %u","Devices: %u • online: %u","Geräte: %u • online: %u"},
 {"Pomieszczenia: %u","Rooms: %u","Räume: %u"},
 {"Stan online dotyczy ostatnich odczytów.","Online state reflects the latest readings.","Online-Status gemäß den letzten Messungen."},
 {"Bluetooth LE","Bluetooth LE","Bluetooth LE"},
 {"Wi-Fi offline • BLE dostępne","Wi-Fi offline • BLE available","WLAN offline • BLE verfügbar"},
 {"Nieznany model","Unknown model","Unbekanntes Modell"},
 {"Zatrzymywanie skanu…","Stopping scan…","Scan wird gestoppt…"},
 {"Shelly • reklamowane RPC","Shelly • advertised RPC","Shelly • RPC angekündigt"},
 {"Shelly BLU • reklama BLE","Shelly BLU • BLE advertisement","Shelly BLU • BLE-Werbung"},
 {"BTHome • producent nieznany","BTHome • unknown vendor","BTHome • Hersteller unbekannt"},
 {"Niezidentyfikowana reklama","Unidentified advertisement","Nicht identifizierte Werbung"},
 {"Wykryto • nie sparowano","Detected • not paired","Erkannt • nicht gekoppelt"},
 {"BTHome v2 • szyfrowane","BTHome v2 • encrypted","BTHome v2 • verschlüsselt"},
 {"BTHome v2 • jawne","BTHome v2 • unencrypted","BTHome v2 • unverschlüsselt"},
 {"Skan nie dodaje do Wi-Fi. Parowanie i zapis BLE nie są jeszcze wdrożone.","Scanning does not join Wi-Fi. BLE pairing and saving are not implemented yet.","Scannen verbindet kein WLAN. BLE-Kopplung und Speicherung sind noch nicht implementiert."},
 {"Szukaj w LAN","Search LAN","LAN durchsuchen"},
 {"Shelly • RPC / konfiguracja","Shelly • RPC / setup","Shelly • RPC / Einrichtung"},
 {"Shelly BLU","Shelly BLU","Shelly BLU"},
 {"Pozostałe reklamy","Other advertisements","Weitere Werbung"},
 {"Wykryto • %d dBm • bez parowania","Detected • %d dBm • not paired","Erkannt • %d dBm • nicht gekoppelt"},
 {"Bluetooth niedostępny","Bluetooth unavailable","Bluetooth nicht verfügbar"},
 {"Nie znaleziono urządzeń BLE","No BLE devices found","Keine BLE-Geräte gefunden"},
 {"Skan przerwany","Scan cancelled","Scan abgebrochen"},
 {"Sprawdź firmware i spróbuj ponownie.","Check firmware and try again.","Firmware prüfen und erneut versuchen."},
 {"BLU może wymagać krótkiego naciśnięcia przycisku.","BLU may need a short button press to advertise.","BLU benötigt eventuell einen kurzen Tastendruck."},
 {"Odbieranie reklam Bluetooth…","Receiving Bluetooth advertisements…","Bluetooth-Werbung wird empfangen…"},
 {"Skan trwa do 20 s. Działa także bez Wi-Fi. Naciśnij krótko przycisk BLU, aby wysłać reklamę.","Scanning takes up to 20 s, also without Wi-Fi. Briefly press the BLU button to advertise.","Scan bis zu 20 s, auch ohne WLAN. BLU-Taste kurz drücken, um Werbung zu senden."},
 {"Wyszukaj urządzenia w pobliżu","Find nearby devices","Geräte in der Nähe suchen"},
 {"Shelly z reklamowanym RPC oraz Shelly BLU / BTHome. Wykrycie nie oznacza sparowania ani dodania do LAN.","Shelly advertising RPC and Shelly BLU / BTHome. Discovery does not mean pairing or LAN setup.","Shelly mit RPC-Werbung und Shelly BLU / BTHome. Erkennung bedeutet keine Kopplung oder LAN-Einrichtung."},
 {"Ostatnia reklama %u s temu. Możliwy brak zasięgu.","Last advertisement %u s ago. Possibly out of range.","Letzte Werbung vor %u s. Eventuell außer Reichweite."},
 {"Ostatnia reklama %u s temu.","Last advertisement %u s ago.","Letzte Werbung vor %u s."},
 {"%u / 20 s • wykryto %u","%u / 20 s • found %u","%u / 20 s • %u erkannt"},
 {"LAN i skan BLE są dostępne.\nWi-Fi AP i parowanie BLE nie są jeszcze wdrożone.","LAN and BLE scan are available.\nWi-Fi AP and BLE pairing are not implemented yet.","LAN- und BLE-Scan sind verfügbar.\nWLAN-AP und BLE-Kopplung sind noch nicht implementiert."},
 {"Odczyt potwierdzony %u s temu","Reading confirmed %u s ago","Messung vor %u s bestätigt"},
 {"Brak potwierdzonego odczytu","No confirmed reading","Keine bestätigte Messung"},
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
 else {
  lv_obj_set_width(name,back?184:216);
  dsr_sc_icon(root,DSR_ICON_WIFI,244,10,22,wifi?DSR_SC_BLUE:DSR_SC_OFF);
#ifndef DSR_UI_REVIEW_DEMO
  dsr_ui_battery_header(root);
#endif
 }
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
