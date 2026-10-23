/* localtime_r/setenv for the status bar clock */
#define _POSIX_C_SOURCE 200809L
#include "dsr_ui_style.h"
#include "dsr_ui_cyber.h"
#include "dsr_ui_cyber_icons.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#ifndef DSR_UI_REVIEW_DEMO
void dsr_ui_battery_header(lv_obj_t *root);
#include "dsr_ui_brand.h"
#endif

static uint32_t accent=DSR_SC_BLUE;
static unsigned style;
static unsigned language=1; /* EN until the workspace (saved language) is loaded */
unsigned dsr_sc_language(void){return language;}
unsigned dsr_sc_style(void){return style;}
void dsr_sc_border_color(lv_obj_t *o,lv_color_t color,lv_style_selector_t selector)
{
 uint32_t c=lv_color_to_u32(color)&0xffffff;
 /* Preserve only intentional semantic warning/error contours. */
 bool semantic=c==0xffc107||c==0xffc65b||c==0xffbf24||c==0xf0a020||c==0xffb52e||c==0xe1bb70||c==DSR_CYBER_DANGER;
 lv_color_t target=style==DSR_SC_STYLE_CYBER_BLUE&&!semantic?lv_color_hex(DSR_CYBER_CYAN):color;
 if(selector==0&&lv_color_eq(lv_obj_get_style_border_color(o,0),target))return;
 lv_obj_set_style_border_color(o,target,selector);
}
void dsr_sc_border_width(lv_obj_t *o,int32_t width,lv_style_selector_t selector)
{int32_t target=style==DSR_SC_STYLE_CYBER_BLUE&&width>0?DSR_CYBER_BORDER_WIDTH:width;if(selector==0&&lv_obj_get_style_border_width(o,0)==target)return;lv_obj_set_style_border_width(o,target,selector);}
extern const lv_font_t dsr_roboto_medium_12,dsr_roboto_medium_14;
extern const lv_font_t dsr_roboto_bold_18,dsr_roboto_bold_22,dsr_roboto_bold_28;
const lv_font_t *dsr_sc_font(const lv_font_t *font)
{
 if(style!=DSR_SC_STYLE_CYBER_BLUE)return font;
 if(font==&dsr_roboto_12)return &dsr_roboto_medium_12;
 if(font==&dsr_roboto_14)return &dsr_roboto_medium_14;
 if(font==&dsr_roboto_18)return &dsr_roboto_bold_18;
 if(font==&dsr_roboto_22)return &dsr_roboto_bold_22;
 if(font==&dsr_roboto_28)return &dsr_roboto_bold_28;
 return font;
}
void dsr_sc_appearance(uint32_t color,unsigned theme,unsigned locale){accent=color;style=theme==DSR_SC_STYLE_LIGHT||theme==DSR_SC_STYLE_CYBER_BLUE?theme:DSR_SC_STYLE_GRAPHITE;language=locale<3?locale:1;}
unsigned dsr_sc_style_from_name(const char *name)
{return !name?DSR_SC_STYLE_GRAPHITE:!strcmp(name,"light")?DSR_SC_STYLE_LIGHT:!strcmp(name,"cyber_blue")?DSR_SC_STYLE_CYBER_BLUE:DSR_SC_STYLE_GRAPHITE;}
/* "Jasny": every neutral token of the dark design maps to a light twin.
 * Data colors (accent, RGB swatches, warnings) pass through unchanged. */
static uint32_t light(uint32_t c)
{
 static const uint32_t map[][2]={
  {DSR_SC_BG,0xeef2f6},{DSR_SC_PANEL,0xffffff},{DSR_SC_CARD,0xe3e8ed},{DSR_SC_EDGE,0xc3cbd3},
  {DSR_SC_TEXT,0x14202b},{DSR_SC_MUTED,0x4f5d6b},{DSR_SC_NAV,0xffffff},{DSR_SC_OFF,0x7d8892},
  {0xf0f4fb,0x14202b},{0x98aac0,0x7a8796},{0x45494d,0xffffff},{0x555b60,0xc9d0d7},{0x3c4246,0xdfe5eb},
  {0x1a2733,0xe2e8ee},{0x344f71,0xc6daf2},{0x30373d,0xdde2e7},{0x232b33,0xf5f7f9},{0x9db5d1,0x47617c},
  {0xa8bbd3,0x4d6782},{0x76e2b6,0x17855a},{0x72dfb0,0x17855a},{0xffb466,0xa95c0b},{0x5c492a,0xf3e2c0},
  {0xffd68a,0x7a4d00},{0x41474b,0xffffff},{0x292d30,0xe6ebf0},
  {0x252a2e,0xffffff},{0x244357,0xd6e6f8},{0x29343b,0xe3e8ed},{0x303943,0xdfe5eb},{0x25292b,0xffffff},
  {0x262a2d,0xeef1f4},{0x414548,0xffffff},{0x60646b,0xc8cfd6},{0xe8ecef,0xdfe5eb},{0x818991,0x6b7682},
  {0xe8edf2,0x14202b},{0xffc18c,0xa4500f},{0xe1bb70,0x8a6200},{0xffc077,0xa4500f},
  /* Shelly-style rows, rings and pills (0.7.49) */
  {0x2b2f33,0xffffff},{0x23272a,0xe6ebf0},{0x3a3f44,0xc3cbd3},{0x17191c,0xe3e8ed},{0x1d2124,0xe6ebf0},{0x3d4246,0xc9d0d7},
  {0xeceff1,0xf3f5f7},{0x5a6066,0xc9d0d7},{0x8a9096,0x6b7682},{0x1d3f73,0xd6e6f8},{0xf0a020,0x9a5a00}};
 for(size_t i=0;i<sizeof(map)/sizeof(map[0]);i++)if(map[i][0]==c)return map[i][1];
 return c;
}
/* Accent used AS TEXT on a light surface is darkened to stay readable;
 * accent fills (buttons, bars) keep the configured color. */
lv_color_t dsr_sc_text_color(uint32_t c)
{
 if(style==DSR_SC_STYLE_CYBER_BLUE&&c==DSR_SC_BLUE)return lv_color_hex(DSR_CYBER_CYAN);
 if(style==DSR_SC_STYLE_LIGHT&&c==DSR_SC_BLUE){
  const uint32_t r=(accent>>16)&0xff,g=(accent>>8)&0xff,b=accent&0xff;
  return lv_color_make((uint8_t)(r*3/5),(uint8_t)(g*3/5),(uint8_t)(b*3/5));
 }
 return dsr_sc_color(c);
}
lv_color_t dsr_sc_color(uint32_t c)
{
 if(style==DSR_SC_STYLE_CYBER_BLUE)return lv_color_hex(dsr_ui_cyber_color(c));
 if(c==DSR_SC_BLUE)return lv_color_hex(accent);
 if(style==DSR_SC_STYLE_LIGHT)c=light(c);
 return lv_color_hex(c);
}
/* User content (names of rooms, devices, scenes, dashboards) is shown as typed:
 * the next dsr_sc_tr of exactly this pointer returns it untranslated. */
static const char *user_text;
const char *dsr_sc_user(const char *text){user_text=text;return text;}
/* Names the firmware itself created (defaults) stay localized. */
const char *dsr_sc_name(const char *name)
{return name&&strcmp(name,"Mój pulpit")&&strcmp(name,"Mój dom")?dsr_sc_user(name):name;}
const char *dsr_sc_tr(const char *text)
{
 if(text&&text==user_text){user_text=NULL;return text;}
 static const char *const words[][3]={
 {"Temperatura zadana","Target temperature","Soll"},
 {"Nastawa","Target","Soll"},
 {"Ustaw temperaturę","Set temperature","Temperatur einstellen"},
 {"Kolejność urządzeń","Device order","Reihenfolge der Geräte"},
 {"Edytuj dashboard","Edit dashboard","Dashboard bearbeiten"},
 {"Następny obiekt","Next object","Nächstes Objekt"},
 {"Tylko jeden obiekt","Only one object","Nur ein Objekt"},
 {"Typ: %s","Type: %s","Typ: %s"},
 {"Urządzenie","Device","Gerät"},{"Scena","Scene","Szene"},{"Grupa","Group","Gruppe"},
 {"Rozmiar: %s","Size: %s","Größe: %s"},
 {"Pełna szerokość","Full width","Volle Breite"},
 {"Połowa szerokości","Half width","Halbe Breite"},
 {"Dane na karcie","Card readings","Kartenwerte"},
 {"Usuń kartę","Remove card","Karte entfernen"},
 {"Znaleziono AP: %u","Access points found: %u","Zugangspunkte gefunden: %u"},
 {"Sprawdź tryb konfiguracji Shelly. Jeśli AP zniknął po przekazaniu Wi-Fi, wyszukaj urządzenie w sieci lokalnej.","Check Shelly setup mode. If its AP disappeared after sharing Wi-Fi, find the device on the local network.","Shelly-Konfigurationsmodus prüfen. Ist der AP nach der WLAN-Einrichtung verschwunden, das Gerät im lokalen Netzwerk suchen."},
 {"Poczekaj na zakończenie dodawania urządzenia.","Wait until device onboarding finishes.","Warten, bis das Hinzufügen abgeschlossen ist."},
 {"Usuwanie urządzenia","Removing device","Gerät entfernen"},
 {"Usuwane są tylko ta karta, jej powiązanie i odwołania. Pozostałe kanały, pokoje i sceny zostają zachowane.","Only this card, binding and references are removed. Other channels, rooms and scenes are preserved.","Nur diese Karte, Bindung und Verweise werden entfernt. Andere Kanäle, Räume und Szenen bleiben erhalten."},
 {"Nie udało się przygotować usunięcia. Dane zachowane.","Could not prepare removal. Data preserved.","Entfernen konnte nicht vorbereitet werden. Daten bleiben erhalten."},
 {"Usunąć ten kanał z pilota oraz jego karty i odwołania? Pozostałe kanały, pomieszczenia i sceny pozostaną. Shelly nie zostanie zresetowane.","Remove this channel, its cards and references from the remote? Other channels, rooms and scenes remain. Shelly will not be reset.","Diesen Kanal mit seinen Karten und Verweisen entfernen? Andere Kanäle, Räume und Szenen bleiben. Shelly wird nicht zurückgesetzt."},
 {"Nie potwierdzono zapisu usunięcia. Spróbuj ponownie.","Removal write not confirmed. Try again.","Speichern der Entfernung nicht bestätigt. Erneut versuchen."},
 {"Karta usunięta. Kończenie usuwania powiązania…","Card removed. Finishing binding removal…","Karte entfernt. Bindung wird entfernt…"},
 {"Karta usunięta. Błąd zapisu powiązania. Pilot ponowi próbę.","Card removed. Binding write failed. The remote will retry.","Karte entfernt. Bindung konnte nicht gespeichert werden. Erneuter Versuch folgt."},
 {"Urządzenie usunięte.","Device removed.","Gerät entfernt."},
 {"Usunięcie nie zostało zapisane.","Removal has not been saved.","Entfernung wurde nicht gespeichert."},

 {"Nazwa grupy","Group name","Gruppenname"},
 {"Nazwa sceny","Scene name","Szenenname"},
 {"np. Lampy na parterze","e.g. Downstairs lights","z. B. Lichter im Erdgeschoss"},
 {"np. Dobranoc","e.g. Good night","z. B. Gute Nacht"},
 {"Jedno dotknięcie uruchomi wybrane polecenia.","One tap runs the selected commands.","Ein Tippen startet die gewählten Befehle."},
 {"Wspólne włączanie i wyłączanie wybranych przekaźników i gniazdek.","Turn selected relays and plugs on or off together.","Gewählte Relais und Steckdosen gemeinsam ein- oder ausschalten."},
 {"Pokój i zdjęcie (opcjonalnie)","Room and photo (optional)","Raum und Bild (optional)"},
 {"Pomieszczenie (opcjonalnie)","Room (optional)","Raum (optional)"},
 {"Zdjęcie (opcjonalnie)","Photo (optional)","Bild (optional)"},
 {"Własne zdjęcie","Your photo","Eigenes Bild"},
 {"Opcjonalne ustawienia","Optional settings","Optionale Einstellungen"},
 {"1/3 • Nazwa","1/3 • Name","1/3 • Name"},
 {"2/3 • Urządzenia","2/3 • Devices","2/3 • Geräte"},
 {"2/3 • Polecenia","2/3 • Commands","2/3 • Befehle"},
 {"3/3 • Podsumowanie","3/3 • Review","3/3 • Prüfen"},
 {"Polecenia","Commands","Befehle"},
 {"Podsumowanie","Review","Prüfen"},
 {"Wybierz urządzenie","Choose device","Gerät wählen"},
 {"Wybierz polecenie","Choose command","Befehl wählen"},
 {"Zmień urządzenie","Change device","Gerät ändern"},
 {"Zmień nazwę i opcje","Change name and options","Name und Optionen ändern"},
 {"Co ma zrobić scena?","What should this scene do?","Was soll diese Szene tun?"},
 {"Dodaj urządzenie i wybierz dla niego polecenie.","Add a device and choose its command.","Gerät hinzufügen und Befehl wählen."},
 {"+ Dodaj polecenie","+ Add command","+ Befehl hinzufügen"},
 {"Sprawdź listę i zapisz.","Review the list and save.","Liste prüfen und speichern."},
 {"Zapisz scenę","Save scene","Szene speichern"},
 {"Zapisz grupę","Save group","Gruppe speichern"},
 {"Brak wyników. Zmień wyszukiwanie.","No matches. Change your search.","Keine Treffer. Suche ändern."},
 {"+ Nowa grupa","+ New group","+ Neue Gruppe"},
 {"+ Nowa scena","+ New scene","+ Neue Szene"},
 {"Uzupełnij nazwę i wybierz co najmniej jedno urządzenie lub polecenie.","Enter a name and choose at least one device or command.","Namen eingeben und mindestens ein Gerät oder einen Befehl wählen."},
 {"Poczekaj na zakończenie zapisu.","Wait for saving to finish.","Warten, bis das Speichern beendet ist."},
 {"Brak nazwy","No name","Kein Name"},
#include "dsr_ui_device_settings_words.inc"
 {"Poprzednie zdjęcia","Previous photos","Vorherige Bilder"},{"Następne zdjęcia","Next photos","Nächste Bilder"},
 {"Cały dom","Whole home","Ganzes Zuhause"},
 {"Szukaj","Search","Suchen"},
 {"Dodaj pierwszy wpis przyciskiem + u góry.","Add your first item using + above.","Ersten Eintrag oben mit + hinzufügen."},
 {"Sceny uruchamiane ręcznie","Manually started scenes","Manuell gestartete Szenen"},
 {"Nowa scena","New scene","Neue Szene"},
 {"Edytuj scenę","Edit scene","Szene bearbeiten"},
 {"Nowa grupa","New group","Neue Gruppe"},
 {"Edytuj grupę","Edit group","Gruppe bearbeiten"},
 {"Typ grupy","Group type","Gruppentyp"},
 {"Przekaźniki i gniazdka","Relays and plugs","Relais und Steckdosen"},
 {"Światła • wkrótce","Lights • coming later","Lichter • später verfügbar"},
 {"Rolety i termostaty • wkrótce","Covers and thermostats • later","Rollläden und Thermostate • später"},
 {"Grupa steruje zgodnymi kanałami Switch.","The group controls compatible Switch channels.","Die Gruppe steuert kompatible Switch-Kanäle."},
 {"Przycisk uruchamia wybrane działania po kolei. Warunki i harmonogramy nie są jeszcze obsługiwane.","The button runs selected actions in order. Conditions and schedules are not supported yet.","Die Taste führt Aktionen nacheinander aus. Bedingungen und Zeitpläne werden noch nicht unterstützt."},
 {"Wybrano: %u","Selected: %u","Ausgewählt: %u"},
 {"Brak zgodnych urządzeń. Dodaj je na stronie Urządzenia.","No compatible devices. Add them on the Devices page.","Keine kompatiblen Geräte. Auf der Seite Geräte hinzufügen."},
 {"Wybierz pomieszczenie","Choose a room","Raum wählen"},
 {"Wybierz zdjęcie","Choose a photo","Bild wählen"},
 {"Bez zdjęcia","No photo","Ohne Bild"},
 {"Własne zdjęcia dodasz w Web Panelu.","Add your own photos in the Web Panel.","Eigene Bilder im Web-Panel hinzufügen."},
 {"Podaj nazwę (maks. 47 bajtów).","Enter a name (max. 47 bytes).","Namen eingeben (max. 47 Bytes)."},
 {"Zapisz, aby usunąć wpis.","Save to delete this item.","Zum Löschen dieses Eintrags speichern."},
 {"Otwórz\nZamknij\nStop","Open\nClose\nStop","Öffnen\nSchließen\nStopp"},
 {"Włącz\nWyłącz","On\nOff","Ein\nAus"},
 {"Firmware: %s","Firmware: %s","Firmware: %s"},
 {"Firmware nieodczytany","Firmware not read","Firmware nicht ausgelesen"},
 {"Użyj nazwy z Shelly","Use name from Shelly","Namen von Shelly verwenden"},
 {"Stan: włączone","State: on","Status: ein"},
 {"Stan: wyłączone","State: off","Status: aus"},
 {"Zmiana stanu","State change","Statusänderung"},
 {"Odczyt stanu","State observation","Statusablesung"},
 {"Potwierdzono","Confirmed","Bestätigt"},
 {"Błąd kroku","Step failed","Schritt fehlgeschlagen"},
 {"Wynik niepewny • sprawdź stan","Uncertain result • check state","Ergebnis unsicher • Status prüfen"},
 {"Anulowano pozostałe kroki","Remaining steps cancelled","Übrige Schritte abgebrochen"},
 {"Oczekuje","Waiting","Wartet"},
 {"Uruchomienie","Run","Ausführung"},
 {"Anuluj pozostałe kroki","Cancel remaining steps","Übrige Schritte abbrechen"},
 {"Nie przyjęto. Odśwież i sprawdź urządzenia.","Not accepted. Refresh and check the devices.","Nicht angenommen. Aktualisieren und Geräte prüfen."},
 {"Wykonywanie kolejno. Możliwy częściowy wynik.","Running sequentially. Partial results are possible.","Nacheinander ausgeführt. Teilergebnisse sind möglich."},
 {"Zakończono. Wyniki dotyczą każdego kroku osobno.","Completed. Results apply to each individual step.","Abgeschlossen. Ergebnisse gelten für jeden einzelnen Schritt."},
 {"Brak uruchomienia","No run","Keine Ausführung"},
 {"Bez automatycznego ponawiania poleceń.","Commands are never replayed automatically.","Befehle werden nicht automatisch wiederholt."},
 {"Aktywność niedostępna","Activity unavailable","Aktivität nicht verfügbar"},
 {"Brak UTC • %u s od startu","UTC unset • %u s since boot","UTC fehlt • %u s seit Start"},
 {"Brak aktywności w tym uruchomieniu","No activity since this boot","Keine Aktivität seit diesem Start"},
 {"Nowsze","Newer","Neuere"},
 {"Starsze","Older","Ältere"},
 {"Wyczyść dziennik RAM","Clear RAM activity log","RAM-Aktivität löschen"},
 {"Cały dom • każde urządzenie ma własny wynik.","Whole home • each device has its own result.","Ganzes Zuhause • jedes Gerät hat ein eigenes Ergebnis."},
 {"Uruchom scenę","Run scene","Szene starten"},
 {"Włącz grupę","Turn group on","Gruppe einschalten"},
 {"Wyłącz grupę","Turn group off","Gruppe ausschalten"},
 {"Dodaj wpis w edytorze konfiguracji.","Add an entry in the configuration editor.","Eintrag im Konfigurationseditor hinzufügen."},
 {"Pokaż bieżące wykonanie","Show current run","Aktuelle Ausführung anzeigen"},
 {"Polecenia wymagają zgodnego powiązania i uprawnień.","Commands require a matching binding and authorization.","Befehle erfordern passende Bindung und Berechtigung."},
 {"Wykonanie","Execution","Ausführung"},
 {"Do 16 kanałów Switch. Grupa ma jawne Włącz/Wyłącz.","Up to 16 Switch channels. Groups use explicit On/Off.","Bis zu 16 Switch-Kanäle. Gruppen verwenden explizit Ein/Aus."},
 {"+ Grupa","+ Group","+ Gruppe"},
 {"Sprawdzanie wydania i podpisu…","Checking release and signature…","Version und Signatur werden geprüft…"},
 {"Instalowanie. Nie odłączaj zasilania.","Installing. Keep power connected.","Installation läuft. Stromversorgung angeschlossen lassen."},
 {"Podpis i zgodność manifestu potwierdzone.","Manifest signature and compatibility verified.","Manifest-Signatur und Kompatibilität bestätigt."},
 {"Instalacja zakończona. Uruchom pilot ponownie.","Installation complete. Restart the remote.","Installation abgeschlossen. Fernbedienung neu starten."},
 {"Automatyczny restart za %u s","Automatic restart in %u s","Automatischer Neustart in %u s"},
 {"Automatyczny restart za %u s","Automatic restart in %u s","Automatischer Neustart in %u s"},
 {"Później","Later","Später"},
 {"Anulowano. Nie uruchomiono nowego firmware.","Cancelled. New firmware has not been started.","Abgebrochen. Neue Firmware wurde nicht gestartet."},
 {"Nie potwierdzono wyboru startowego firmware. Dalsze zapisy zablokowane do restartu.","Boot firmware selection is unconfirmed. Further writes are blocked until restart.","Start-Firmware nicht bestätigt. Weitere Schreibvorgänge bis zum Neustart gesperrt."},
 {"Połącz pilot z domowym Wi-Fi.","Connect the remote to home Wi-Fi.","Fernbedienung mit dem Heim-WLAN verbinden."},
 {"Nie ustawiono czasu. Sprawdź dostęp do internetu i spróbuj ponownie.","Time is not set. Check internet access and retry.","Uhrzeit nicht gesetzt. Internetzugang prüfen und erneut versuchen."},
 {"Brak wydania w wybranym kanale.","No release in the selected channel.","Keine Version im gewählten Kanal."},
 {"Brak wydania Stable. Wersje testowe: Zmień kanał, potem Dev.","No Stable release yet. Test versions: Change channel, then Dev.","Noch keine Stable-Version. Testversionen: Kanal ändern, dann Dev."},
 {"GitHub chwilowo ogranicza zapytania. Spróbuj ponownie za godzinę.","GitHub is limiting requests for now. Try again in an hour.","GitHub begrenzt gerade Anfragen. In einer Stunde erneut versuchen."},
 {"Nie pobrano manifestu wydania. Sprawdź internet i spróbuj ponownie.","Could not download the release manifest. Check the internet and try again.","Release-Manifest nicht geladen. Internet prüfen und erneut versuchen."},
 {"Kod błędu: %d/%d/%d/%d","Error code: %d/%d/%d/%d","Fehlercode: %d/%d/%d/%d"},
 {"Brak zatwierdzonego klucza wydawcy. Instalacja zablokowana.","No approved publisher key. Installation is blocked.","Kein freigegebener Herausgeberschlüssel. Installation gesperrt."},
 {"Nieprawidłowy podpis wydania. Instalacja zablokowana.","Invalid release signature. Installation is blocked.","Ungültige Versionssignatur. Installation gesperrt."},
 {"Brak zgodnej nowszej wersji z poprawnym manifestem.","No compatible newer version with a valid manifest.","Keine kompatible neuere Version mit gültigem Manifest."},
 {"Nie zweryfikowano manifestu. Spróbuj ponownie później.","Manifest verification failed. Try again later.","Manifest konnte nicht geprüft werden. Später erneut versuchen."},
 {"Układ partycji jest niezgodny lub OTA jest zajęte.","Partition layout is incompatible or OTA is busy.","Partitionslayout inkompatibel oder OTA belegt."},
 {"Pilot nie jest gotowy do aktualizacji.","The remote is not ready for an update.","Fernbedienung ist noch nicht bereit für ein Update."},
 {"Zgoda wygasła. Sprawdź wydanie ponownie.","Consent expired. Check the release again.","Zustimmung abgelaufen. Version erneut prüfen."},
 {"Wydanie zmieniło się po potwierdzeniu. Sprawdź je ponownie.","The release changed after confirmation. Check it again.","Version wurde nach Bestätigung geändert. Erneut prüfen."},
 {"Brak pamięci. Spróbuj ponownie później.","Out of memory. Try again later.","Nicht genügend Speicher. Später erneut versuchen."},
 {"Nie rozpoznano lokalnej wersji firmware.","Local firmware version is not recognized.","Lokale Firmware-Version nicht erkannt."},
 {"Nie ukończono instalacji. Zachowano bieżący firmware.","Installation did not complete. Current firmware retained.","Installation nicht abgeschlossen. Bisherige Firmware bleibt erhalten."},
 {"Przerwano pobieranie. Sprawdź Wi-Fi i spróbuj ponownie.","Download interrupted. Check Wi-Fi and try again.","Download unterbrochen. WLAN prüfen und erneut versuchen."},
 {"Nie udało się zapisać firmware. Bieżąca wersja pozostaje aktywna.","Firmware could not be written. Current version remains active.","Firmware konnte nicht gespeichert werden. Bisherige Version bleibt aktiv."},
 {"Nie pobrano poprawnego katalogu wydań. Sprawdź internet i spróbuj ponownie.","Could not fetch a valid release catalogue. Check internet and retry.","Kein gültiger Versionskatalog geladen. Internet prüfen und erneut versuchen."},
 {"Sprawdź dostępne podpisane wydanie.","Check for an available signed release.","Nach einer signierten Version suchen."},
 {"Zainstalować podpisane wydanie?","Install the signed release?","Signierte Version installieren?"},
 {"Podłącz stabilne zasilanie USB. Nie odłączaj go podczas zapisu. Instalacja nie uruchamia pilota ponownie automatycznie.","Connect stable USB power and keep it connected during writes. Installation does not restart the remote automatically.","Stabile USB-Stromversorgung anschließen und beim Schreiben angeschlossen lassen. Installation startet die Fernbedienung nicht automatisch neu."},
 {"Potwierdzenie jest jednorazowe i ważne przez 120 s od sprawdzenia.","Confirmation is one-time and valid for 120 s after checking.","Bestätigung gilt einmalig und 120 s ab der Prüfung."},
 {"USB podłączone • instaluj","USB connected • install","USB angeschlossen • installieren"},
 {"Wybierz kanał aktualizacji","Choose update channel","Update-Kanal wählen"},
 {"Dev zawiera wersje testowe. Wybór kanału nie rozpoczyna pobierania ani instalacji.","Dev contains test builds. Choosing a channel does not start downloading or installation.","Dev enthält Testversionen. Die Kanalwahl startet weder Download noch Installation."},
 {"Dev • wersje testowe","Dev • test builds","Dev • Testversionen"},
 {"Kanał: Dev • wersje testowe","Channel: Dev • test builds","Kanal: Dev • Testversionen"},
 {"Kanał: Stable","Channel: Stable","Kanal: Stable"},
 {"Opis wydania z GitHuba • niezweryfikowany","GitHub release notes • unverified","GitHub-Versionshinweise • ungeprüft"},
 {"Sprawdź aktualizacje","Check for updates","Nach Updates suchen"},
 {"Zmień kanał","Change channel","Kanal ändern"},
 {"Zainstaluj","Install","Installieren"},
 {"Energia","Energy","Energie"},
 {"Anuluj synchronizację","Cancel synchronization","Synchronisierung abbrechen"},
 {"Synchronizuj czas UTC","Synchronize UTC time","UTC-Zeit synchronisieren"},
 {"Wczytywanie historii…","Loading history…","Verlauf wird geladen…"},
 {"Nowszy format historii. Pliki zachowane.","Newer history format. Files preserved.","Neueres Verlaufsformat. Dateien erhalten."},
 {"Historia niedostępna: brak pamięci.","History unavailable: insufficient memory.","Verlauf nicht verfügbar: zu wenig Speicher."},
 {"Błąd zapisu historii. Dane mogą być tylko w RAM.","History storage error. Data may exist only in RAM.","Verlauf-Speicherfehler. Daten eventuell nur im RAM."},
 {"Synchronizacja czasu UTC…","Synchronizing UTC time…","UTC-Zeit wird synchronisiert…"},
 {"Brak poprawnego czasu UTC. W historii pozostaje przerwa.","No valid UTC time. History remains incomplete.","Keine gültige UTC-Zeit. Verlauf bleibt lückenhaft."},
 {"Brak próbek dla tego pomiaru","No samples for this measurement","Keine Stichproben für diese Messgröße"},
 {"Wh: odczyt licznika Shelly, nie suma W.","Wh: Shelly counter reading, not a sum of W.","Wh: Shelly-Zählerstand, keine Summe von W."},
 {"Punkty: rzeczywiste średnie z okien 5 min.","Points: actual 5-minute window means.","Punkte: echte Mittelwerte aus 5-Minuten-Fenstern."},
 {"Przerwy lub reset licznika. Bez interpolacji.","Gaps or counter reset. No interpolation.","Lücken oder Zähler-Reset. Keine Interpolation."},
 {"Brak próbek. Historia zbiera rzeczywiste odczyty.","No samples. History collects actual readings.","Keine Stichproben. Verlauf sammelt echte Messwerte."},
 {"Historia lokalna • zapis potwierdzony","Local history • storage confirmed","Lokaler Verlauf • Speicherung bestätigt"},
 {"Bieżące okno RAM. Restart może utracić do 5 min.","Current window in RAM. Restart may lose up to 5 min.","Aktuelles Fenster im RAM. Neustart verliert ggf. bis zu 5 Min."},
 {"%.1f W • świeże liczniki: %u/%u","%.1f W • fresh meters: %u/%u","%.1f W • aktuelle Zähler: %u/%u"},
 {"Brak świeżych pomiarów mocy","No fresh power measurements","Keine aktuellen Leistungsmesswerte"},
 {"Suma tylko porównywalnych W. Braki nie są zerem.","Sum of comparable W only. Missing is not zero.","Nur vergleichbare W summiert. Fehlend ist nicht null."},
 {"licznik Shelly","Shelly counter","Shelly-Zähler"},
 {"24 h lokalnie • 5 min • oś czasu UTC","24 h locally • 5 min • UTC timeline","24 h lokal • 5 Min. • UTC-Zeitachse"},
 {"Odczyt starszy niż 15 s","Reading older than 15 s","Messwert älter als 15 s"},

 {"Już dodane","Already added","Bereits hinzugefügt"},
 {"Dodanie zachowuje poprzednie urządzenia i ich kanały.","Adding preserves previous devices and their channels.","Beim Hinzufügen bleiben bisherige Geräte und deren Kanäle erhalten."},
 {"Do 16 niezależnych kanałów przekaźników.\nDodanie zachowuje poprzednie połączenia.\nLokalne HTTP; Gen1 Basic / Gen2+ Digest.\nDane logowania w nieszyfrowanym NVS.","Up to 16 independent relay/switch endpoints.\nAdding another retains earlier bindings.\nLocal HTTP; Gen1 Basic / Gen2+ Digest.\nCredentials in unencrypted NVS.","Bis zu 16 unabhängige Relais-/Schaltkanäle.\nWeitere Geräte ersetzen keine Verbindungen.\nLokales HTTP; Gen1 Basic / Gen2+ Digest.\nZugangsdaten im unverschlüsselten NVS."},
 {"Ustawienia pilota","Remote settings","Fernbedienung einstellen"},
 {"Sieć i połączenia","Network and connections","Netzwerk und Verbindungen"},
 {"Ekran i komfort","Display and comfort","Bildschirm und Komfort"},
 {"Panel WWW / bezpieczeństwo","Web panel / security","Webpanel / Sicherheit"},
 {"Bluetooth LE • tylko skan","Bluetooth LE • scan only","Bluetooth LE • nur Scan"},
 {"Parowanie BLE i onboarding AP nie są wdrożone.","BLE pairing and AP onboarding are not implemented.","BLE-Kopplung und AP-Einrichtung sind nicht implementiert."},
 {"Wi-Fi AP • niedostępne","Wi-Fi AP • unavailable","WLAN-AP • nicht verfügbar"},
 {"Wi-Fi: połączono","Wi-Fi: connected","WLAN: verbunden"},
 {"Wi-Fi: brak połączenia","Wi-Fi: disconnected","WLAN: nicht verbunden"},
 {"Wygeneruj kod parowania","Generate pairing code","Kopplungscode erstellen"},
 {"Nie utworzono kodu. HTTPS niedostępny lub zajęty.","No code created. HTTPS is unavailable or busy.","Kein Code erstellt. HTTPS ist nicht verfügbar oder belegt."},
 {"Jednorazowy kod • 120 s • 5 prób","One-time code • 120 s • 5 attempts","Einmalcode • 120 s • 5 Versuche"},
 {"Sesje i unieważnianie: brak widoku LCD. Kod nie jest stałym hasłem.","Sessions and revocation: no LCD view. This code is not a permanent password.","Sitzungen und Widerruf: keine LCD-Ansicht. Der Code ist kein dauerhaftes Passwort."},
 {"Lokalne OTA • Web Panel","Local OTA • Web panel","Lokales OTA • Webpanel"},
 {"Nie udzielono zgody OTA. HTTPS niedostępny lub zajęty.","OTA not allowed. HTTPS is unavailable or busy.","OTA nicht freigegeben. HTTPS ist nicht verfügbar oder belegt."},
 {"To lokalny tryb deweloperski, nie podpisany updater z pilota.","This is local developer mode, not a signed updater on the remote.","Dies ist ein lokaler Entwicklermodus, kein signierter Updater auf der Fernbedienung."},
 {"Kanał: Dev • wersja testowa","Channel: Dev • test build","Kanal: Dev • Testversion"},
 {"Natywne podpisane OTA: niewdrożone (#24). Brak zweryfikowanego wydania.","Native signed OTA: not implemented (#24). No verified release.","Natives signiertes OTA: nicht implementiert (#24). Keine verifizierte Version."},
 {"Sprawdź aktualizacje • niedostępne","Check updates • unavailable","Updates prüfen • nicht verfügbar"},
 {"Zainstaluj • niedostępne","Install • unavailable","Installieren • nicht verfügbar"},
 {"Informacje i diagnostyka","Information and diagnostics","Informationen und Diagnose"},
 {"Restart z LCD: niewdrożony. Eksport i reset: niewdrożone; dane zachowane.","LCD restart: not implemented. Export and reset: not implemented; data preserved.","LCD-Neustart: nicht implementiert. Export und Reset: nicht implementiert; Daten erhalten."},
 {"Konfiguracja wpisów i scen","Entry and scene configuration","Einträge und Szenen konfigurieren"},
 {"Konfiguracja wpisów","Entry configuration","Einträge konfigurieren"},
 {"Restart • niedostępny","Restart • unavailable","Neustart • nicht verfügbar"},
 {"Restart pilota","Restart remote","Fernbedienung neu starten"},
 {"Czy uruchomić pilot ponownie?","Restart the remote?","Fernbedienung neu starten?"},
 {"Ustawienia i zdjęcia pozostaną. Niezapisane zmiany zostaną utracone.","Settings and photos will remain. Unsaved changes will be lost.","Einstellungen und Fotos bleiben erhalten. Ungespeicherte Änderungen gehen verloren."},
 {"Potwierdź restart przyciskiem poniżej.","Confirm restart using the button below.","Neustart mit der Schaltfläche unten bestätigen."},
 {"Uruchom ponownie","Restart now","Jetzt neu starten"},
 {"Anuluj","Cancel","Abbrechen"},
 {"Usuń pulpit","Delete dashboard","Dashboard löschen"},
 {"Usunąć \"%s\"?","Delete \"%s\"?","\"%s\" löschen?"},
 {"Po potwierdzeniu wybierz Zapisz. Dane zostaną zmienione dopiero po udanym zapisie.","After confirming, choose Save. Data will change only after a successful save.","Nach Bestätigung Speichern wählen. Daten ändern sich erst nach erfolgreichem Speichern."},
 {"Stan parowania niedostępny.","Pairing state unavailable.","Kopplungsstatus nicht verfügbar."},
 {"Trwałe unieważnienie: niepotwierdzone.","Persistent revocation: unconfirmed.","Dauerhafter Widerruf: unbestätigt."},
 {"Dostęp WWW: aktywny.","Web access: active.","Webzugriff: aktiv."},
 {"Dostęp WWW: nieaktywny.","Web access: inactive.","Webzugriff: inaktiv."},
 {"Wspólne parowanie, nie lista aktywnych sesji.","Shared pairing, not an active session list.","Gemeinsame Kopplung, keine Liste aktiver Sitzungen."},
 {"Unieważnij parowanie WWW","Revoke Web pairing","Web-Kopplung widerrufen"},
 {"Unieważnij parowanie","Revoke pairing","Kopplung widerrufen"},
 {"Stare cookie utracą dostęp. Trwające operacje mogą się zakończyć. Wi-Fi i dane pozostaną.","Old cookies will lose access. Running operations may finish. Wi-Fi and data will remain.","Alte Cookies verlieren den Zugriff. Laufende Vorgänge dürfen enden. WLAN und Daten bleiben erhalten."},
 {"Potwierdź unieważnienie przyciskiem poniżej.","Confirm revocation using the button below.","Widerruf mit der Schaltfläche unten bestätigen."},
 {"Unieważnianie parowania…","Revoking pairing…","Kopplung wird widerrufen…"},
 {"Parowanie unieważniono. Nowy dostęp wymaga nowego kodu.","Pairing revoked. New access requires a new code.","Kopplung widerrufen. Neuer Zugriff erfordert einen neuen Code."},
 {"Nie potwierdzono trwałego unieważnienia. Spróbuj ponownie.","Persistent revocation unconfirmed. Try again.","Dauerhafter Widerruf unbestätigt. Erneut versuchen."},
 {"Restart jest niedostępny. Spróbuj później.","Restart is unavailable. Try again later.","Neustart nicht verfügbar. Später erneut versuchen."},
 {"Przyjęto żądanie restartu.","Restart request accepted.","Neustart-Anfrage angenommen."},
 {"Restart: w menu System, po potwierdzeniu. Eksport i reset: niewdrożone.","Restart: in System, after confirmation. Export and reset: not implemented.","Neustart: im Systemmenü, nach Bestätigung. Export und Reset: nicht implementiert."},
 {"Pierwszy dotyk wybudza bez wykonania akcji.","The first touch wakes without executing an action.","Die erste Berührung weckt das Gerät, ohne eine Aktion auszuführen."},
 {"System","System","System"},
 {"Odśwież","Refresh","Aktualisieren"},
 {"HTTPS niedostępny","HTTPS unavailable","HTTPS nicht verfügbar"},
 {"SHA-256 certyfikatu tego pilota","This remote's certificate SHA-256","SHA-256 des Zertifikats dieser Fernbedienung"},
 {"Odrzucić zmiany?","Discard changes?","Änderungen verwerfen?"},
 {"Zostań","Stay","Bleiben"},{"Odrzuć","Discard","Verwerfen"},
 {"Niezapisane zmiany tego formularza zostaną utracone.","Unsaved changes in this form will be lost.","Ungespeicherte Änderungen in diesem Formular gehen verloren."},
 {"Kod parowania wygasł.","Pairing code expired.","Kopplungscode abgelaufen."},
 {"Zgoda na OTA wygasła.","OTA permission expired.","OTA-Freigabe abgelaufen."},
 {"RAM wolna: %lu B\nNajwiększy blok: %lu B\nPSRAM wolna: %lu B\nFlash dostępna: %lu B / 0x%x","Free RAM: %lu B\nLargest block: %lu B\nFree PSRAM: %lu B\nAvailable Flash: %lu B / 0x%x","RAM frei: %lu B\nGrößter Block: %lu B\nPSRAM frei: %lu B\nFlash verfügbar: %lu B / 0x%x"},
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
 {"Nowy profil","New profile","Neues Profil"},
 {"Utworzyć nowy profil?","Create a new profile?","Neues Profil erstellen?"},
 {"Tylko profil baterii zostanie zastąpiony po naciśnięciu Zapisz.","Only the battery profile will be replaced after you press Save.","Nur das Akkuprofil wird nach dem Speichern ersetzt."},
 {"Utwórz","Create","Erstellen"},
 {"Nowy profil jest wyłączony. Sprawdź dane i zapisz.","The new profile is disabled. Check the data and save.","Das neue Profil ist deaktiviert. Daten prüfen und speichern."},
 {"Animacja z trendu napięcia","Voltage trend animation","Animation aus Spannungstrend"},
 {"Po min. 10 s wzrostu i min. 20 mV: prawdopodobne ładowanie. Spadek, zastój lub brak pomiarów wygasza animację.","After at least 10 s of rise and 20 mV: likely charging. A fall, plateau or missing readings stops the animation.","Nach mindestens 10 s Anstieg und 20 mV: vermutlich Laden. Abfall, Stillstand oder fehlende Messungen stoppen die Animation."},
 {"Prawdopodobne ładowanie • trend min. 10 s","Likely charging • rising for at least 10 s","Vermutlich Laden • Anstieg seit min. 10 s"},
 {"Kalibracja baterii","Battery calibration","Akku-Kalibrierung"},
 {"Typ ogniwa","Cell type","Zelltyp"},
 {"Wyłącz szacowanie","Disable estimate","Schätzung ausschalten"},
 {"Tylko ogniwo 1S Li-Po / Li-ion 3,6–3,7 V, ładowane do 4,2 V. Profil nie zmienia ładowarki.","Only 1S Li-Po / Li-ion 3.6–3.7 V cells, charged to 4.2 V. This profile does not control the charger.","Nur 1S Li-Po / Li-Ion mit 3,6–3,7 V, geladen bis 4,2 V. Das Profil steuert das Ladegerät nicht."},
 {"Napięcie nominalne (3600 / 3700 mV)","Nominal voltage (3600 / 3700 mV)","Nennspannung (3600 / 3700 mV)"},
 {"Pojemność z etykiety (1–20000 mAh)","Label capacity (1–20000 mAh)","Kapazität laut Etikett (1–20000 mAh)"},
 {"Napięcie dla 0% (2800–3600 mV)","Voltage at 0% (2800–3600 mV)","Spannung bei 0% (2800–3600 mV)"},
 {"Napięcie dla 50% (3500–4100 mV)","Voltage at 50% (3500–4100 mV)","Spannung bei 50% (3500–4100 mV)"},
 {"Napięcie dla 100% (4000–4250 mV)","Voltage at 100% (4000–4250 mV)","Spannung bei 100% (4000–4250 mV)"},
 {"Korekta pomiaru","Measurement correction","Messkorrektur"},
 {"Równoczesny odczyt multimetru (mV)","Simultaneous multimeter reading (mV)","Zeitgleiche Multimeter-Messung (mV)"},
 {"Pomiar bazowy: %u mV • %lu s","Base reading: %u mV • %lu s","Basismessung: %u mV • %lu s"},
 {"Brak świeżego pomiaru bazowego.","No fresh base reading.","Keine aktuelle Basismessung."},
 {"Oblicz korektę","Calculate correction","Korrektur berechnen"},
 {"Korekta 1:1","Correction 1:1","Korrektur 1:1"},
 {"~% to szacunek z trzech punktów napięcia. Obciążenie, temperatura i USB wpływają na wynik. Pojemność nie mierzy prądu ani czasu pracy. Animacja z trendu nie potwierdza ładowania.","~% is an estimate from three voltage points. Load, temperature and USB affect it. Capacity does not measure current or run time. The trend animation does not confirm charging.","~% ist eine Schätzung aus drei Spannungspunkten. Last, Temperatur und USB beeinflussen sie. Kapazität misst weder Strom noch Laufzeit. Die Trendanimation bestätigt kein Laden."},
 {"Sprawdź dane ogniwa i progi. Wartości wstępne są przybliżone.","Check the cell data and thresholds. Prefilled values are approximate.","Zelldaten und Schwellen prüfen. Vorgaben sind Näherungswerte."},
 {"Sprawdź liczby i progi: 0% < 50% < 100%. Odstęp min. 100 mV.","Check numbers and thresholds: 0% < 50% < 100%. Gap at least 100 mV.","Zahlen und Schwellen prüfen: 0% < 50% < 100%. Abstand mind. 100 mV."},
 {"Odczyt profilu baterii…","Reading battery profile…","Akkuprofil wird gelesen…"},
 {"Zapis lub odczyt niepotwierdzony. Wczytaj profil ponownie.","Save or readback unconfirmed. Reload the profile.","Speichern oder Rücklesen unbestätigt. Profil neu laden."},
 {"Wpisz równoczesny odczyt multimetru w mV.","Enter the simultaneous multimeter reading in mV.","Zeitgleiche Multimeter-Messung in mV eingeben."},
 {"Zmierz ponownie. Wpisz 2500–4350 mV; korekta maks. ±10%.","Measure again. Enter 2500–4350 mV; maximum correction ±10%.","Erneut messen. 2500–4350 mV eingeben; Korrektur maximal ±10%."},
 {"~%: szacunek. Brak detekcji baterii i ładowania.","Estimated %. No battery-presence or charging signal.","Geschätzte %. Kein Signal für Akkupräsenz oder Laden."},
 {"Szacunek z profilu • wpływ obciążenia i temperatury","Profile estimate • affected by load and temperature","Profilschätzung • abhängig von Last und Temperatur"},
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
 {"Sterowanie","Controls","Steuerung"},{"Zużycie","Usage","Verbrauch"},{"Ustawienia","Settings","Einstellungen"},
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
 {"Shelly • konfiguracja BLE","Shelly • BLE setup","Shelly • BLE-Einrichtung"},
 {"Shelly BLU • urządzenie BLE","Shelly BLU • BLE device","Shelly BLU • BLE-Gerät"},
 {"BTHome • producent nieznany","BTHome • unknown vendor","BTHome • Hersteller unbekannt"},
 {"Nierozpoznane urządzenie BLE","Unrecognized BLE device","Unbekanntes BLE-Gerät"},
 {"Wykryto • nie sparowano","Detected • not paired","Erkannt • nicht gekoppelt"},
 {"BTHome v2 • szyfrowane","BTHome v2 • encrypted","BTHome v2 • verschlüsselt"},
 {"BTHome v2 • jawne","BTHome v2 • unencrypted","BTHome v2 • unverschlüsselt"},
 {"Skan nie dodaje do Wi-Fi. Parowanie i zapis BLE nie są jeszcze wdrożone.","Scanning does not join Wi-Fi. BLE pairing and saving are not implemented yet.","Scannen verbindet kein WLAN. BLE-Kopplung und Speicherung sind noch nicht implementiert."},
 {"Szukaj w LAN","Search LAN","LAN durchsuchen"},
 {"Shelly • RPC / konfiguracja","Shelly • RPC / setup","Shelly • RPC / Einrichtung"},
 {"Shelly BLU","Shelly BLU","Shelly BLU"},
 {"Pozostałe urządzenia","Other devices","Weitere Geräte"},
 {"Wykryto • %d dBm • bez parowania","Detected • %d dBm • not paired","Erkannt • %d dBm • nicht gekoppelt"},
 {"Bluetooth niedostępny","Bluetooth unavailable","Bluetooth nicht verfügbar"},
 {"Nie znaleziono urządzeń BLE","No BLE devices found","Keine BLE-Geräte gefunden"},
 {"Skan przerwany","Scan cancelled","Scan abgebrochen"},
 {"Sprawdź firmware i spróbuj ponownie.","Check firmware and try again.","Firmware prüfen und erneut versuchen."},
 {"BLU może wymagać krótkiego naciśnięcia przycisku.","BLU may need a short button press to advertise.","BLU benötigt eventuell einen kurzen Tastendruck."},
 {"Wyszukiwanie urządzeń Bluetooth…","Searching for Bluetooth devices…","Bluetooth-Geräte werden gesucht…"},
 {"Skan trwa do 20 s. Działa także bez Wi-Fi. Naciśnij krótko przycisk BLU, aby wykryć urządzenie.","Scanning takes up to 20 s, also without Wi-Fi. Briefly press the BLU button to make the device discoverable.","Scan bis zu 20 s, auch ohne WLAN. BLU-Taste kurz drücken, damit das Gerät erkannt wird."},
 {"Wyszukaj urządzenia w pobliżu","Find nearby devices","Geräte in der Nähe suchen"},
 {"Urządzenia Shelly ogłaszające RPC oraz Shelly BLU / BTHome. Wykrycie nie oznacza sparowania ani dodania do LAN.","Shelly devices announcing RPC and Shelly BLU / BTHome. Discovery does not mean pairing or LAN setup.","Shelly-Geräte mit angekündigtem RPC und Shelly BLU / BTHome. Erkennung bedeutet keine Kopplung oder LAN-Einrichtung."},
 {"Ostatni sygnał urządzenia %u s temu. Możliwy brak zasięgu.","Last device signal %u s ago. Possibly out of range.","Letztes Gerätesignal vor %u s. Eventuell außer Reichweite."},
 {"Ostatni sygnał urządzenia %u s temu.","Last device signal %u s ago.","Letztes Gerätesignal vor %u s."},
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

/* Screen root: solid fallback color plus the owner's background artwork. */
void dsr_sc_background(lv_obj_t *root)
{
    dsr_ui_cyber_background(root,style==DSR_SC_STYLE_CYBER_BLUE);
    lv_obj_set_style_bg_color(root,dsr_sc_color(DSR_SC_BG),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
#ifndef DSR_UI_REVIEW_DEMO
    lv_obj_set_style_bg_image_src(root,dsr_ui_brand_background(),0);
#endif
}
/* Shared styles (0.7.50): a panel used to carry 13 local style properties and
 * a button three more state blocks, each a separate LVGL heap allocation. A
 * 24-tile dashboard of mixed devices then left <2 KB of the 128 KB pool and the
 * first repaint stalled in the shadow allocator. Identical looks now share one
 * lv_style_t; only position and size stay per object. Entries are never freed
 * (objects may reference them); when the table is full the old local path is
 * used, so the look never depends on the cache. */
enum{SHARED_PANEL,SHARED_TEXT,SHARED_PRESSED,SHARED_DISABLED,SHARED_OUTLINE,SHARED_GLOW,SHARED_SCROLLBAR};
typedef struct{lv_style_t style;uint32_t a;uintptr_t b;uint8_t kind;bool used;} shared_style_t;
#ifdef ESP_PLATFORM
#include "esp_attr.h"
/* Same PSRAM as the LVGL pool that holds every other style and object. */
EXT_RAM_BSS_ATTR
#endif
static shared_style_t shared_styles[96];
static lv_style_t *shared(uint8_t kind,uint32_t a,uintptr_t b)
{
 for(unsigned i=0;i<sizeof(shared_styles)/sizeof(shared_styles[0]);i++){
  shared_style_t *e=&shared_styles[i];
  if(e->used){if(e->kind==kind&&e->a==a&&e->b==b)return &e->style;continue;}
  lv_style_init(&e->style);e->kind=kind;e->a=a;e->b=b;e->used=true;lv_style_t *t=&e->style;
  switch(kind){
   case SHARED_PANEL:lv_style_set_bg_color(t,lv_color_hex(a));lv_style_set_bg_opa(t,LV_OPA_COVER);lv_style_set_radius(t,(int32_t)(b&0xffff));lv_style_set_text_color(t,lv_color_hex((uint32_t)(b>>16)));break;
   case SHARED_TEXT:lv_style_set_text_font(t,(const lv_font_t *)b);lv_style_set_text_color(t,lv_color_hex(a));break;
   case SHARED_PRESSED:lv_style_set_bg_color(t,lv_color_hex(a));break;
   case SHARED_DISABLED:lv_style_set_bg_color(t,lv_color_hex(a));lv_style_set_opa(t,LV_OPA_COVER);break;
   case SHARED_OUTLINE:lv_style_set_border_width(t,1);lv_style_set_border_color(t,lv_color_hex(a));break;
   case SHARED_SCROLLBAR:lv_style_set_bg_color(t,lv_color_hex(a));lv_style_set_bg_opa(t,LV_OPA_70);lv_style_set_width(t,2);lv_style_set_length(t,36);lv_style_set_radius(t,1);lv_style_set_pad_right(t,-9);lv_style_set_pad_ver(t,6);break;
   default:lv_style_set_shadow_width(t,14);lv_style_set_shadow_spread(t,1);lv_style_set_shadow_color(t,lv_color_hex(a));lv_style_set_shadow_opa(t,LV_OPA_70);break;
  }
  return t;
 }
 return NULL;
}
static uint32_t hex(lv_color_t c){return lv_color_to_u32(c)&0xffffff;}
lv_obj_t *dsr_sc_panel(lv_obj_t *p,int x,int y,int w,int h,uint32_t color,int radius)
{
    lv_obj_t *o=lv_obj_create(p);lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    /* remove_style_all leaves padding at its default 0. */
    lv_style_t *s=radius>=0&&radius<=0xffff?shared(SHARED_PANEL,hex(dsr_sc_color(color)),(uintptr_t)radius|((uintptr_t)hex(dsr_sc_color(DSR_SC_TEXT))<<16)):NULL;
    if(s)lv_obj_add_style(o,s,0);
    else{lv_obj_set_style_bg_color(o,dsr_sc_color(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
     lv_obj_set_style_radius(o,radius,0);lv_obj_set_style_text_color(o,dsr_sc_color(DSR_SC_TEXT),0);}
    if(style==DSR_SC_STYLE_CYBER_BLUE&&radius>=6&&radius<LV_RADIUS_CIRCLE&&h>=20&&!(w==h&&radius>=w/2))dsr_ui_cyber_panel(o,color==DSR_SC_BLUE);
    else if(style==DSR_SC_STYLE_CYBER_BLUE&&radius>=6)dsr_ui_cyber_contour(o);
    if(style==DSR_SC_STYLE_CYBER_BLUE&&color==DSR_SC_BG&&radius==0&&w>=280&&h>60)lv_obj_set_style_bg_opa(o,LV_OPA_TRANSP,0);
    return o;
}
lv_obj_t *dsr_sc_text(lv_obj_t *p,const char *s,int x,int y,int w,const lv_font_t *font,uint32_t color)
{
    font=dsr_sc_font(font);
    lv_obj_t *o=lv_label_create(p);lv_obj_remove_style_all(o);lv_label_set_text(o,dsr_sc_tr(s));
    lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);
    lv_style_t *t=shared(SHARED_TEXT,hex(dsr_sc_text_color(color)),(uintptr_t)font);
    if(t)lv_obj_add_style(o,t,0);else{lv_obj_set_style_text_font(o,font,0);lv_obj_set_style_text_color(o,dsr_sc_text_color(color),0);}
    lv_label_set_long_mode(o,LV_LABEL_LONG_MODE_WRAP);
    return o;
}
lv_obj_t *dsr_sc_button(lv_obj_t *p,int x,int y,int w,int h,uint32_t color,int radius,lv_event_cb_t cb,void *ctx)
{
    lv_obj_t *o=dsr_sc_panel(p,x,y,w,h,color,radius);
    lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);
    lv_style_t *pressed=shared(SHARED_PRESSED,hex(dsr_sc_color(0x344f71)),0),*disabled=shared(SHARED_DISABLED,hex(dsr_sc_color(0x30373d)),0);
    if(style==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_button(o);
    else if(pressed)lv_obj_add_style(o,pressed,LV_STATE_PRESSED);else lv_obj_set_style_bg_color(o,dsr_sc_color(0x344f71),LV_STATE_PRESSED);
    if(disabled)lv_obj_add_style(o,disabled,LV_STATE_DISABLED);
    else{lv_obj_set_style_bg_color(o,dsr_sc_color(0x30373d),LV_STATE_DISABLED);lv_obj_set_style_opa(o,LV_OPA_COVER,LV_STATE_DISABLED);}
    /* Consistent outlines distinguish controls from the background in all themes. */
    if(style!=DSR_SC_STYLE_CYBER_BLUE&&radius>=6&&color!=DSR_SC_BLUE){lv_style_t *e=shared(SHARED_OUTLINE,hex(dsr_sc_color(DSR_SC_EDGE)),0);
     if(e)lv_obj_add_style(o,e,0);else{dsr_sc_border_width(o,1,0);dsr_sc_border_color(o,dsr_sc_color(DSR_SC_EDGE),0);}}
    if(cb){
     /* Compact +/- and group controls keep their layout but have a minimum
      * 44 px touch target, including the space immediately around them. */
     int shortest=w<h?w:h;if(shortest<44)lv_obj_set_ext_click_area(o,(44-shortest+1)/2);
     lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,ctx);
    }
    return o;
}

/* Apache-2.0 Material Symbols Rounded, pre-filtered to coverage masks.
 * No SVG parser, web font, or integer-grid line approximation at runtime. */
extern const lv_image_dsc_t dsr_symbol_atlas[40];
static void symbol_line(lv_layer_t *layer,const lv_area_t *a,int size,lv_color_t color,int x1,int y1,int x2,int y2)
{
 lv_draw_line_dsc_t line;lv_draw_line_dsc_init(&line);line.color=color;line.width=size>=48?3:2;line.round_start=line.round_end=1;
 line.p1=(lv_point_precise_t){a->x1+x1*size/64,a->y1+y1*size/64};line.p2=(lv_point_precise_t){a->x1+x2*size/64,a->y1+y2*size/64};lv_draw_line(layer,&line);
}
static lv_color_t icon_color(lv_obj_t *o)
{return style==DSR_SC_STYLE_CYBER_BLUE?lv_obj_get_style_image_recolor(o,0):lv_obj_get_style_line_color(o,0);}
static void state_symbol(lv_layer_t *layer,unsigned kind,lv_area_t a,int size,lv_color_t color)
{
#define LINE(x1,y1,x2,y2) symbol_line(layer,&a,size,color,x1,y1,x2,y2)
 if(kind==DSR_ICON_BATTERY){LINE(20,14,44,14);LINE(44,14,44,58);LINE(44,58,20,58);LINE(20,58,20,14);LINE(26,14,26,6);LINE(26,6,38,6);LINE(38,6,38,14);LINE(36,22,27,38);LINE(27,38,35,38);LINE(35,38,29,50);}
 else if(kind==DSR_ICON_VALVE){LINE(10,32,24,32);LINE(40,32,54,32);LINE(24,23,40,23);LINE(40,23,40,42);LINE(40,42,24,42);LINE(24,42,24,23);LINE(32,23,32,12);LINE(22,12,42,12);LINE(10,26,10,38);LINE(54,26,54,38);}
 else if(kind==DSR_ICON_BLUETOOTH){LINE(31,6,31,58);LINE(31,6,46,19);LINE(46,19,16,45);LINE(16,19,46,45);LINE(46,45,31,58);}
 else if(kind==DSR_ICON_REFRESH){
  lv_draw_arc_dsc_t arc;lv_draw_arc_dsc_init(&arc);arc.center=(lv_point_t){a.x1+size/2,a.y1+size/2};arc.radius=(uint16_t)(size*23/64);arc.width=size>=40?3:2;arc.color=color;arc.start_angle=30;arc.end_angle=300;lv_draw_arc(layer,&arc);
  LINE(43,6,44,19);LINE(44,19,31,18);
 }
 else{
  bool door=kind==DSR_ICON_DOOR_CLOSED||kind==DSR_ICON_DOOR_OPEN;bool open=kind==DSR_ICON_DOOR_OPEN||kind==DSR_ICON_WINDOW_OPEN;
  LINE(12,8,52,8);LINE(52,8,52,56);LINE(52,56,12,56);LINE(12,56,12,8);
  if(open){LINE(17,12,37,20);LINE(37,20,37,60);LINE(37,60,17,52);LINE(17,52,17,12);LINE(32,34,32,38);if(!door){LINE(17,30,37,38);LINE(42,12,47,12);LINE(47,12,47,51);}}
  else{LINE(18,14,46,14);LINE(46,14,46,50);LINE(46,50,18,50);LINE(18,50,18,14);if(door){LINE(39,33,39,36);}else{LINE(32,14,32,50);LINE(18,32,46,32);}}
 }
#undef LINE
}
static void draw_icon_at(lv_obj_t *obj,lv_layer_t *layer,unsigned kind,lv_area_t area,lv_color_t color,lv_opa_t opacity)
{
    if(kind>=DSR_ICON_COUNT)return;
    int side=lv_area_get_width(&area);
    if(style==DSR_SC_STYLE_CYBER_BLUE||kind>=DSR_ICON_CPU){
     const lv_image_dsc_t *asset=dsr_ui_cyber_icon((dsr_ui_icon_t)kind,(unsigned)side);
     if(asset){
      unsigned pixels=asset->header.w;
      unsigned shown=(unsigned)side<pixels?(unsigned)side:pixels;
      lv_draw_image_dsc_t d;lv_draw_image_dsc_init(&d);d.base.obj=obj;d.src=asset;d.recolor=color;
      d.recolor_opa=opacity;d.antialias=1;
      d.scale_x=d.scale_y=shown*256/pixels;d.pivot=(lv_point_t){0,0};
      int x=area.x1+(side-(int)shown)/2,y=area.y1+(side-(int)shown)/2;
      lv_area_t source={x,y,x+(int)pixels-1,y+(int)pixels-1};
      lv_draw_image(layer,&d,&source);return;
     }
    }
    if(kind>=DSR_ICON_WINDOW_CLOSED||(style==DSR_SC_STYLE_CYBER_BLUE&&kind==DSR_ICON_BATTERY)){state_symbol(layer,kind,area,side,color);return;}
    lv_draw_image_dsc_t draw;lv_draw_image_dsc_init(&draw);
    draw.base.obj=obj;draw.src=&dsr_symbol_atlas[kind];draw.recolor=color;
    draw.recolor_opa=LV_OPA_COVER;draw.antialias=1;
    draw.scale_x=draw.scale_y=(uint32_t)side*256/64;
    draw.pivot=(lv_point_t){0,0};
    lv_area_t source={area.x1,area.y1,area.x1+63,area.y1+63};
    lv_draw_image(layer,&draw,&source);
}
static void icon_draw(lv_event_t *event)
{
 lv_obj_t *o=lv_event_get_target(event);lv_area_t area;lv_obj_get_coords(o,&area);
 draw_icon_at(o,lv_event_get_layer(event),(unsigned)(uintptr_t)lv_event_get_user_data(event),area,icon_color(o),lv_obj_get_style_image_recolor_opa(o,0));
}
void dsr_sc_draw_icon(lv_obj_t *owner,lv_layer_t *layer,dsr_ui_icon_t kind,const lv_area_t *area,uint32_t color)
{
 draw_icon_at(owner,layer,(unsigned)kind,*area,style==DSR_SC_STYLE_CYBER_BLUE?dsr_sc_text_color(color):dsr_sc_color(color),LV_OPA_COVER);
}
uint32_t dsr_sc_icon_accent(void)
{return style==DSR_SC_STYLE_CYBER_BLUE?DSR_SC_GLOW:DSR_SC_TEXT;}
lv_obj_t *dsr_sc_icon(lv_obj_t *p,dsr_ui_icon_t kind,int x,int y,int size,uint32_t color)
{
    /* Native 24/32 px masks stay crisp in compact controls. Parent hit targets
     * and positions are unchanged; smaller dropdown arrows still fit their slot. */
    if(style==DSR_SC_STYLE_CYBER_BLUE&&size>=20&&size<=32)size=size<=28?24:32;
    lv_obj_t *o=lv_obj_create(p);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,size,size);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    if(style==DSR_SC_STYLE_CYBER_BLUE){lv_obj_set_style_image_recolor(o,dsr_sc_text_color(color),0);lv_obj_set_style_image_recolor_opa(o,LV_OPA_COVER,0);}
    else lv_obj_set_style_line_color(o,dsr_sc_color(color),0);
    lv_obj_add_event_cb(o,icon_draw,LV_EVENT_DRAW_MAIN,(void *)(uintptr_t)kind);return o;
}
lv_obj_t *dsr_sc_icon_centered(lv_obj_t *p,dsr_ui_icon_t kind,int size,uint32_t color)
{
    lv_obj_t *o=dsr_sc_icon(p,kind,0,0,size,color);lv_obj_center(o);return o;
}
const dsr_ui_icon_t dsr_sc_device_icons[DSR_SC_DEVICE_ICONS]={
 DSR_ICON_LIGHT,DSR_ICON_FLOOR_LAMP,DSR_ICON_WALL_LAMP,DSR_ICON_PLUG,DSR_ICON_OUTLET,DSR_ICON_COVER,DSR_ICON_GARAGE,
 DSR_ICON_DOOR,DSR_ICON_CONTACT,DSR_ICON_FAN,DSR_ICON_THERMOSTAT,DSR_ICON_BOILER,DSR_ICON_AC,DSR_ICON_TV,
 DSR_ICON_SPEAKER,DSR_ICON_ROUTER,DSR_ICON_KETTLE,DSR_ICON_COFFEE,DSR_ICON_LAUNDRY,DSR_ICON_EV};
void dsr_sc_icon_color(lv_obj_t *o,uint32_t color)
{if(style==DSR_SC_STYLE_CYBER_BLUE)lv_obj_set_style_image_recolor(o,dsr_sc_text_color(color),0);else lv_obj_set_style_line_color(o,lv_color_hex(color),0);lv_obj_invalidate(o);}

/* Status bar clock (0.7.51): local Central European time (CET/CEST, the
 * owner's zone) once SNTP has set a plausible UTC; hidden until then. */
static lv_obj_t *clock_label;
static void clock_deleted(lv_event_t *event){if(lv_event_get_target(event)==clock_label)clock_label=NULL;}
void dsr_sc_clock_tick(void)
{
 if(!clock_label)return;
 static bool zone;if(!zone){setenv("TZ","CET-1CEST,M3.5.0,M10.5.0/3",1);tzset();zone=true;}
 time_t now=time(NULL);char text[8]="";struct tm local;
 if(now>=(time_t)1735689600&&localtime_r(&now,&local))snprintf(text,sizeof(text),"%02d:%02d",local.tm_hour,local.tm_min);
 if(strcmp(lv_label_get_text(clock_label),text))lv_label_set_text(clock_label,text);
}
static const char back_tag;
static lv_event_cb_t header_callback;
static void header_back_icon(lv_event_t *event)
{
 lv_obj_t *icon=lv_event_get_target(event);lv_area_t a;lv_obj_get_coords(icon,&a);
 lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);d.color=icon_color(icon);d.width=3;d.round_start=d.round_end=1;
 /* Keep the 24 px slot; only the chevron's stroke becomes heavier. */
 int x=(a.x1+a.x2)/2,y=(a.y1+a.y2)/2;
 d.p1=(lv_point_precise_t){x+4,y-8};d.p2=(lv_point_precise_t){x-4,y};lv_draw_line(lv_event_get_layer(event),&d);
 d.p1=d.p2;d.p2=(lv_point_precise_t){x+4,y+8};lv_draw_line(lv_event_get_layer(event),&d);
}
void dsr_sc_header_back(lv_obj_t *root,lv_event_cb_t back,void *context)
{
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *b=lv_obj_get_child(root,(int)i);if(lv_obj_get_user_data(b)!=(void *)&back_tag)continue;
  if(header_callback)lv_obj_remove_event_cb(b,header_callback);
  header_callback=back;
  if(back)lv_obj_add_event_cb(b,back,LV_EVENT_CLICKED,context);
  return;
 }
}
void dsr_sc_header(lv_obj_t *root,const char *title,bool demo,bool wifi,lv_event_cb_t back,void *context)
{
 if(back){
  header_callback=back;lv_obj_t *b=dsr_sc_button(root,12,0,44,DSR_SC_STATUS_H,DSR_SC_BG,8,back,context);lv_obj_set_user_data(b,(void *)&back_tag);
  lv_obj_t *icon=dsr_sc_icon_centered(b,DSR_ICON_BACK,26,dsr_sc_icon_accent());
  if(style==DSR_SC_STYLE_CYBER_BLUE){dsr_ui_cyber_header_back(b);lv_obj_remove_event_cb(icon,icon_draw);lv_obj_add_event_cb(icon,header_back_icon,LV_EVENT_DRAW_MAIN,NULL);}
 }
 lv_obj_t *name=dsr_sc_text(root,title,back?56:12,14,back?192:232,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_height(name,24);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_CLIP);
 if(demo){lv_obj_t *badge=dsr_sc_panel(root,256,9,54,21,0x5c492a,5);dsr_sc_border_width(badge,1,0);dsr_sc_border_color(badge,lv_color_hex(0xe1bb70),0);dsr_sc_text(badge,"DEMO",10,4,40,&dsr_roboto_12,0xffd68a);}
 else {
  lv_obj_set_x(name,back?128:92);lv_obj_set_width(name,back?92:124);
  /* A device name between clock and Wi-Fi: one line, else two smaller lines. */
  if(back&&title&&title[0]){
   lv_point_t size;lv_text_get_size(&size,lv_label_get_text(name),dsr_sc_font(&dsr_roboto_14),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
   if(size.x>92){lv_obj_set_style_text_font(name,dsr_sc_font(&dsr_roboto_12),0);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_DOTS);lv_obj_set_y(name,6);lv_obj_set_height(name,2*lv_font_get_line_height(dsr_sc_font(&dsr_roboto_12)));}
   else lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_DOTS);
  }
  clock_label=dsr_sc_text(root,"",back?64:12,9,68,&dsr_roboto_22,DSR_SC_TEXT);lv_obj_set_style_text_align(clock_label,LV_TEXT_ALIGN_LEFT,0);
  lv_obj_add_event_cb(clock_label,clock_deleted,LV_EVENT_DELETE,NULL);dsr_sc_clock_tick();
  dsr_sc_icon(root,DSR_ICON_WIFI,224,10,24,wifi?DSR_SC_BLUE:DSR_SC_OFF);
#ifndef DSR_UI_REVIEW_DEMO
  dsr_ui_battery_header(root);
#endif
 }
}
/* Compact Cyber BLU header: keep the shared clock and its delete lifetime. */
void dsr_sc_blu_header(lv_obj_t *root,const char *title,bool demo,bool wifi,lv_event_cb_t back,void *context,lv_event_cb_t help)
{
 /* The full device name is the control tab below this shared status row. */
 (void)title;dsr_sc_header(root,"",demo,wifi,back,context);
 lv_obj_t *b=dsr_sc_button(root,164,0,44,44,DSR_SC_BG,8,help,NULL);dsr_sc_icon_centered(b,DSR_ICON_INFO,24,dsr_sc_icon_accent());
}
static void plus_feedback(lv_event_t *event)
{
 lv_obj_t *button=lv_event_get_target(event);lv_obj_t *circle=lv_obj_get_child(button,0);
 bool down=lv_event_get_code(event)==LV_EVENT_PRESSED;
 lv_obj_set_style_opa(circle,down?LV_OPA_70:LV_OPA_COVER,0);
}
void dsr_sc_navigation(lv_obj_t *root,unsigned current,unsigned enabled,lv_event_cb_t callback)
{
 lv_obj_t *bar=dsr_sc_panel(root,0,DSR_SC_NAV_TOP,320,52,DSR_SC_NAV,0);
 if(style==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_navigation(bar,current);
 dsr_sc_panel(bar,0,0,320,1,0x60646b,0);
 lv_obj_set_style_bg_grad_color(bar,dsr_sc_color(0x232b33),0);lv_obj_set_style_bg_grad_dir(bar,LV_GRAD_DIR_VER,0);
 if(current<5)dsr_sc_panel(bar,16+(int)current*64,0,32,3,DSR_SC_BLUE,2);
 const dsr_ui_icon_t icons[]={DSR_ICON_HOME,DSR_ICON_GRID,DSR_ICON_CPU,DSR_ICON_SETTINGS,DSR_ICON_PLUS};
 for(unsigned i=0;i<5;i++){
  bool available=(enabled&(1u<<i))!=0;
  uint32_t icon_color=!available?DSR_SC_OFF:i==4?dsr_sc_icon_accent():style==DSR_SC_STYLE_CYBER_BLUE?DSR_SC_GLOW:i==current?dsr_sc_icon_accent():DSR_SC_MUTED;
  lv_obj_t *b=dsr_sc_button(bar,(int)i*64,4,64,47,DSR_SC_NAV,0,callback,(void *)(uintptr_t)i);lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,0);
  /* Leaving a navigation target cancels the tap instead of retaining it. */
  lv_obj_remove_flag(b,LV_OBJ_FLAG_PRESS_LOCK);
  if(i==4){lv_obj_add_event_cb(b,plus_feedback,LV_EVENT_PRESSED,NULL);lv_obj_add_event_cb(b,plus_feedback,LV_EVENT_RELEASED,NULL);lv_obj_add_event_cb(b,plus_feedback,LV_EVENT_PRESS_LOST,NULL);lv_obj_t *circle=dsr_sc_panel(b,0,0,42,42,DSR_SC_BLUE,LV_RADIUS_CIRCLE);lv_obj_center(circle);if(style==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_power(circle,true,true);dsr_sc_icon_centered(circle,icons[i],28,icon_color);}
  else dsr_sc_icon_centered(b,icons[i],26,icon_color);
  if(!available){lv_obj_add_state(b,LV_STATE_DISABLED);lv_obj_set_style_opa(b,LV_OPA_40,LV_STATE_DISABLED);}
 }
}
lv_obj_t *dsr_sc_chip(lv_obj_t *parent,const char *text,int x,int y,int max_width,uint32_t color)
{
 text=dsr_sc_tr(text);
 const lv_font_t *font=dsr_sc_font(&dsr_roboto_12);int border=style==DSR_SC_STYLE_CYBER_BLUE?2:0;
 /* Measure the same skin font that is rendered, including its frame inset. */
 lv_point_t size;lv_text_get_size(&size,text,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 int width=size.x+14+2*border;if(width>max_width)width=max_width;
 int line=lv_font_get_line_height(font);
 lv_obj_t *chip=dsr_sc_panel(parent,x,y,width,line+6+2*border,0x292d30,6);
 lv_obj_t *label=dsr_sc_text(chip,dsr_sc_user(text),7,3,width-14-2*border,&dsr_roboto_12,color);lv_obj_set_height(label,line);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_DOTS);return chip;
}

/* Owner 2026-10-18: a slim bar on the right shows the position on long pages.
 * remove_style_all() also removed LVGL's scrollbar look, so lists had none. */
static void scrollbar_margin(lv_event_t *e)
{
 if(lv_event_get_code(e)==LV_EVENT_REFR_EXT_DRAW_SIZE){lv_event_set_ext_draw_size(e,9);return;}
 /* LVGL clips DRAW_POST to the list's child viewport. Draw the margin bar
  * before that clip, while keeping rows clipped inside their normal bounds. */
 lv_obj_t *o=lv_event_get_target(e);if(lv_obj_get_style_pad_right(o,LV_PART_SCROLLBAR)>=0)return;lv_area_t h,v;lv_obj_get_scrollbar_area(o,&h,&v);
 if(lv_area_get_width(&v)<=0||lv_area_get_height(&v)<=0)return;
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);lv_obj_init_draw_rect_dsc(o,LV_PART_SCROLLBAR,&d);lv_draw_rect(lv_event_get_layer(e),&d,&v);
}
void dsr_sc_scrollbar(lv_obj_t *o)
{
 /* A scroll flag alone is not hit-testable after dsr_sc_panel removed
  * CLICKABLE. Empty space and passive cards must admit real finger drags. */
 lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_set_scrollbar_mode(o,LV_SCROLLBAR_MODE_AUTO);
 lv_obj_add_event_cb(o,scrollbar_margin,LV_EVENT_REFR_EXT_DRAW_SIZE,NULL);
 lv_obj_add_event_cb(o,scrollbar_margin,LV_EVENT_DRAW_MAIN_END,NULL);
 lv_style_t *t=shared(SHARED_SCROLLBAR,hex(dsr_sc_color(style==DSR_SC_STYLE_CYBER_BLUE?DSR_SC_GLOW:0x8a9096)),0);
 if(t)lv_obj_add_style(o,t,LV_PART_SCROLLBAR);
 if(lv_obj_get_style_width(o,0)>=310)lv_obj_set_style_pad_right(o,1,LV_PART_SCROLLBAR);
 lv_obj_refresh_ext_draw_size(o);
}
void dsr_sc_glow(lv_obj_t *o,bool on)
{
 if(style==DSR_SC_STYLE_CYBER_BLUE){dsr_ui_cyber_glow(o,on);return;}
 /* Off costs nothing: default shadow width is 0. */
 lv_style_t *g=shared(SHARED_GLOW,hex(dsr_sc_color(DSR_SC_GLOW)),0);
 if(g){if(on)lv_obj_add_style(o,g,0);else lv_obj_remove_style(o,g,0);return;}
 lv_obj_set_style_shadow_width(o,on?14:0,0);lv_obj_set_style_shadow_spread(o,on?1:0,0);
 lv_obj_set_style_shadow_color(o,dsr_sc_color(DSR_SC_GLOW),0);lv_obj_set_style_shadow_opa(o,on?LV_OPA_70:LV_OPA_TRANSP,0);
}
lv_obj_t *dsr_sc_power_button(lv_obj_t *parent,int x,int y,int size,bool on,bool enabled,dsr_ui_icon_t icon,uint32_t icon_color,lv_event_cb_t cb,void *context)
{
 if(style==DSR_SC_STYLE_CYBER_BLUE){
  lv_obj_t *ring=dsr_sc_button(parent,x,y,size,size,DSR_SC_PANEL,LV_RADIUS_CIRCLE,cb,context);
  dsr_ui_cyber_power(ring,on,enabled);
  dsr_sc_icon_centered(ring,icon,size>=80?48:32,enabled?DSR_SC_GLOW:0x71879e);
  if(!enabled)lv_obj_add_state(ring,LV_STATE_DISABLED);
  return ring;
 }
 /* One object: light face, thick ring as its border (blue + glow when on).
  * Two objects per row button matter: a 24-tile dashboard shares 128 KB LVGL heap. */
 lv_obj_t *ring=dsr_sc_button(parent,x,y,size,size,enabled?0xeceff1:0x5a6066,LV_RADIUS_CIRCLE,cb,context);
 dsr_sc_border_width(ring,5,0);dsr_sc_border_color(ring,dsr_sc_color(on&&enabled?DSR_SC_GLOW:0x3a3f44),0);
 dsr_sc_glow(ring,on&&enabled);
 dsr_sc_icon_centered(ring,icon,(size-10)*52/100,enabled?icon_color:0x8a9096);
 if(!enabled)lv_obj_add_state(ring,LV_STATE_DISABLED);
 return ring;
}
static int segments(lv_obj_t *pill,const char *const *texts,unsigned count,int max_width)
{
 /* One pill + one recoloured label (separators inline): two objects per row.
  * 14 px since 0.7.51 (owner: larger, more readable values). */
 if(count>6)count=6;
 char line[240];int total=0;lv_point_t size;const lv_font_t *font=&dsr_roboto_14;
 /* Measure the exact joined string with the actual skin font, including
  * separators, border and padding. The old sum undercounted all of these. */
 do{
  line[0]=0;for(unsigned i=0;i<count;i++){if(i)strncat(line,"  |  ",sizeof(line)-strlen(line)-1);strncat(line,dsr_sc_tr(texts[i]),sizeof(line)-strlen(line)-1);}
  lv_text_get_size(&size,line,dsr_sc_font(font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);total=size.x+20;
  if(total>max_width&&font==&dsr_roboto_14){font=&dsr_roboto_12;continue;}
  if(total<=max_width||count<=1)break;
  count--;
 }while(count);
 if(total>max_width)total=max_width;
 lv_obj_set_width(pill,total);lv_obj_t *label=lv_obj_get_child(pill,0);lv_obj_set_width(label,total-20);lv_obj_set_style_text_font(label,dsr_sc_font(font),0);
 if(strcmp(lv_label_get_text(label),line))lv_label_set_text(label,line);
 lv_obj_set_height(label,18);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_DOTS);
 return total;
}
int dsr_sc_segments(lv_obj_t *parent,const char *const *texts,unsigned count,int x,int y,int max_width,uint32_t color)
{
 lv_obj_t *pill=dsr_sc_panel(parent,x,y,1,24,0x23272a,12);dsr_sc_text(pill,"",8,4,1,&dsr_roboto_14,color);
 if(style==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_inset(pill);
 return segments(pill,texts,count,max_width);
}
void dsr_sc_segments_update(lv_obj_t *pill,const char *const *texts,unsigned count,int max_width)
{(void)segments(pill,texts,count,max_width);}
void dsr_sc_gradient_arc(lv_obj_t *parent,int cx,int cy,int r,int width,int start,int span,const uint32_t *stops,unsigned n_stops,unsigned n,bool enabled)
{
 for(unsigned i=0;i<n;i++){
  double t=n>1?(double)i/(double)(n-1)*(double)(n_stops-1):0;unsigned k=(unsigned)t;if(k>=n_stops-1)k=n_stops-2;double f=t-k;uint32_t c=0;
  for(int sh=16;sh>=0;sh-=8){int a=(int)((stops[k]>>sh)&255),b=(int)((stops[k+1]>>sh)&255);c=c<<8|(uint32_t)(a+(b-a)*f+0.5);}
  lv_obj_t *seg=lv_arc_create(parent);lv_obj_remove_flag(seg,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_size(seg,2*r,2*r);lv_obj_set_pos(seg,cx-r,cy-r);
  int a0=start+span*(int)i/(int)n,a1=start+span*(int)(i+1)/(int)n+(i+1<n?1:0);
  lv_arc_set_bg_angles(seg,(lv_value_precise_t)(a0%360),(lv_value_precise_t)(a1%360));
  lv_obj_set_style_arc_width(seg,width,LV_PART_MAIN);lv_obj_set_style_arc_color(seg,dsr_sc_color(enabled?c:0x4a5056),LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(seg,i==0||i==n-1,LV_PART_MAIN);lv_obj_set_style_arc_opa(seg,LV_OPA_TRANSP,LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(seg,LV_OPA_TRANSP,LV_PART_KNOB);lv_obj_set_style_pad_all(seg,0,LV_PART_KNOB);
 }
}

/* Shared compact status/measurement modules. No click handler or hidden state. */
lv_obj_t *dsr_sc_badge(lv_obj_t *parent,dsr_ui_icon_t icon,const char *text,int x,int y,int width,uint32_t color)
{
 lv_obj_t *badge=dsr_sc_panel(parent,x,y,width,28,DSR_SC_PANEL,7);
 uint32_t contour=color==DSR_SC_TEXT?DSR_SC_GLOW:color;
 dsr_sc_border_color(badge,dsr_sc_color(contour),0);dsr_sc_border_width(badge,1,0);
 /* 20 px requests become 24 px in Cyber Blue. Use an explicit 16 px glyph
  * and an 8 px gap, so icon and caption never share the same pixels. */
 dsr_sc_icon(badge,icon,8,4,16,contour);
 lv_obj_t *label=dsr_sc_text(badge,text,32,5,width-44,&dsr_roboto_12,color);
 lv_point_t size;lv_text_get_size(&size,lv_label_get_text(label),dsr_sc_font(&dsr_roboto_12),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 if(size.x>width-44)lv_obj_set_style_text_font(label,dsr_sc_font(&dsr_roboto_11),0);
 lv_text_get_size(&size,lv_label_get_text(label),lv_obj_get_style_text_font(label,0),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 lv_obj_set_height(label,17);lv_label_set_long_mode(label,size.x>width-44?LV_LABEL_LONG_MODE_SCROLL_CIRCULAR:LV_LABEL_LONG_MODE_CLIP);
 return badge;
}
