#include "dsr_ui_style.h"
#include <stdint.h>
#include <string.h>
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
void dsr_sc_appearance(uint32_t color,unsigned theme,unsigned locale){accent=color;style=theme<3?theme:DSR_SC_STYLE_GRAPHITE;language=locale<3?locale:1;}
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
 if(style==DSR_SC_STYLE_LIGHT&&c==DSR_SC_BLUE){
  const uint32_t r=(accent>>16)&0xff,g=(accent>>8)&0xff,b=accent&0xff;
  return lv_color_make((uint8_t)(r*3/5),(uint8_t)(g*3/5),(uint8_t)(b*3/5));
 }
 return dsr_sc_color(c);
}
lv_color_t dsr_sc_color(uint32_t c)
{
 if(c==DSR_SC_BLUE)return lv_color_hex(accent);
 if(style==DSR_SC_STYLE_MIDNIGHT&&c==DSR_SC_BG)c=0x080f19;
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
 {"Historia 1 h / 24 h jest w zakładce Wykresy. Oś czasu UTC.","1 h / 24 h history is in Charts. UTC timeline.","1 h / 24 h Verlauf unter Diagramme. UTC-Zeitachse."},
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
    lv_obj_set_style_bg_color(root,dsr_sc_color(DSR_SC_BG),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
#ifndef DSR_UI_REVIEW_DEMO
    lv_obj_set_style_bg_image_src(root,dsr_ui_brand_background(),0);
#endif
}
lv_obj_t *dsr_sc_panel(lv_obj_t *p,int x,int y,int w,int h,uint32_t color,int radius)
{
    lv_obj_t *o=lv_obj_create(p);lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(o,dsr_sc_color(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
    lv_obj_set_style_radius(o,radius,0);lv_obj_set_style_pad_all(o,0,0);
    lv_obj_set_style_text_color(o,dsr_sc_color(DSR_SC_TEXT),0);
    return o;
}
lv_obj_t *dsr_sc_text(lv_obj_t *p,const char *s,int x,int y,int w,const lv_font_t *font,uint32_t color)
{
    lv_obj_t *o=lv_label_create(p);lv_obj_remove_style_all(o);lv_label_set_text(o,dsr_sc_tr(s));
    lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);lv_obj_set_style_text_font(o,font,0);
    lv_obj_set_style_text_color(o,dsr_sc_text_color(color),0);lv_label_set_long_mode(o,LV_LABEL_LONG_MODE_WRAP);
    return o;
}
lv_obj_t *dsr_sc_button(lv_obj_t *p,int x,int y,int w,int h,uint32_t color,int radius,lv_event_cb_t cb,void *ctx)
{
    lv_obj_t *o=dsr_sc_panel(p,x,y,w,h,color,radius);
    lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_bg_color(o,dsr_sc_color(0x344f71),LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(o,dsr_sc_color(0x30373d),LV_STATE_DISABLED);
    lv_obj_set_style_opa(o,LV_OPA_COVER,LV_STATE_DISABLED);
    /* Light surfaces need an outline to read as tappable on the light art. */
    if(style==DSR_SC_STYLE_LIGHT&&color!=DSR_SC_BLUE){lv_obj_set_style_border_width(o,1,0);lv_obj_set_style_border_color(o,dsr_sc_color(DSR_SC_EDGE),0);}
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
 lv_obj_set_style_bg_grad_color(bar,dsr_sc_color(0x232b33),0);lv_obj_set_style_bg_grad_dir(bar,LV_GRAD_DIR_VER,0);
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
 text=dsr_sc_tr(text);
 lv_point_t size;lv_text_get_size(&size,text,&dsr_roboto_12,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 int width=size.x+14;if(width>max_width)width=max_width;
 lv_obj_t *chip=dsr_sc_panel(parent,x,y,width,20,0x292d30,6);
 lv_obj_t *label=dsr_sc_text(chip,dsr_sc_user(text),7,3,width-14,&dsr_roboto_12,color);lv_obj_set_height(label,15);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_DOTS);return chip;
}

void dsr_sc_glow(lv_obj_t *o,bool on)
{
 lv_obj_set_style_shadow_width(o,on?14:0,0);lv_obj_set_style_shadow_spread(o,on?1:0,0);
 lv_obj_set_style_shadow_color(o,dsr_sc_color(DSR_SC_GLOW),0);lv_obj_set_style_shadow_opa(o,on?LV_OPA_70:LV_OPA_TRANSP,0);
}
lv_obj_t *dsr_sc_power_button(lv_obj_t *parent,int x,int y,int size,bool on,bool enabled,dsr_ui_icon_t icon,uint32_t icon_color,lv_event_cb_t cb,void *context)
{
 /* One object: light face, thick ring as its border (blue + glow when on).
  * Two objects per row button matter: a 24-tile dashboard shares 128 KB LVGL heap. */
 lv_obj_t *ring=dsr_sc_button(parent,x,y,size,size,enabled?0xeceff1:0x5a6066,LV_RADIUS_CIRCLE,cb,context);
 lv_obj_set_style_border_width(ring,5,0);lv_obj_set_style_border_color(ring,dsr_sc_color(on&&enabled?DSR_SC_GLOW:0x3a3f44),0);
 dsr_sc_glow(ring,on&&enabled);
 dsr_sc_icon_centered(ring,icon,(size-10)*52/100,enabled?icon_color:0x8a9096);
 if(!enabled)lv_obj_add_state(ring,LV_STATE_DISABLED);
 return ring;
}
int dsr_sc_segments(lv_obj_t *parent,const char *const *texts,unsigned count,int x,int y,int max_width,uint32_t color)
{
 /* One pill + one recoloured label (separators inline): two objects per row. */
 int widths[6],total=0;if(count>6)count=6;
 for(unsigned i=0;i<count;i++){lv_point_t size;lv_text_get_size(&size,dsr_sc_tr(texts[i]),&dsr_roboto_12,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);widths[i]=size.x+16;total+=widths[i];}
 /* Values that do not fit are left out (never squeezed to "..."); the first
  * one is kept and truncated so the row always says something. */
 while(count>1&&total>max_width)total-=widths[--count];
 if(total>max_width){widths[0]=max_width;total=max_width;}
 char line[200];line[0]=0;
 for(unsigned i=0;i<count;i++){if(i)strncat(line,"  #5a6066 |#  ",sizeof(line)-strlen(line)-1);strncat(line,dsr_sc_tr(texts[i]),sizeof(line)-strlen(line)-1);}
 lv_obj_t *pill=dsr_sc_panel(parent,x,y,total,22,0x23272a,11);
 lv_obj_t *label=dsr_sc_text(pill,"",8,4,total-12,&dsr_roboto_12,color);lv_label_set_recolor(label,true);lv_label_set_text(label,line);
 lv_obj_set_height(label,15);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_DOTS);
 return total;
}
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
