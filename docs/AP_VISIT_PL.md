# #19: tymczasowe połączenie z AP — warstwa kontrolera, nie gotowy provisioning

Pierwszy niezależny etap istniejącego [Issue19](https://github.com/voltex-at/DIY-Shelly-Remote/issues/19).
Nie włącza przycisku Wi-Fi AP, nie zastępuje działającego kreatora LAN i nie
udaje obsługi wielu Shelly z #20. Backend `dsr_wifi_visit_ap` jest kompilowany,
ale **nie ma jeszcze produkcyjnego wywołującego kontrolera/worker job/UI**;
linker może usunąć nieużyte publiczne funkcje. Nie wymuszamy ich retencji.

## Kontrakt

- Wywołanie wyłącznie z kontrolera poza LVGL/HTTP task, po jawnym potwierdzeniu
  utraty domowego LAN. Wymaga poprawnych danych Wi-Fi i niezerowego unicast BSSID
  wybranego AP. BSSID pinning ogranicza wybór radia; NIE uwierzytelnia Shelly.
- Trzyma istniejącą bramkę `control` przez stop/start/DHCP, callback i powrót.
  Scan/connect/nested visit zwracają BUSY, restart nie uzyska rezerwacji activity.
  Callback jest zaufanym kodem przyszłego kontrolera, nie sandboxem uprawnień.
  Musi być ograniczony czasowo, sprawdzać `dsr_wifi_ap_cancelled()` i nie wywoływać
  ponownie lifecycle Wi-Fi. Nie ma gwarancji twardego całkowitego deadline.
- `dsr_wifi_prepare_connect` ma wyczyścić stary CANCEL **przed zakolejkowaniem**
  nowej operacji. CANCEL oczekujący przed wejściem nie odłącza LAN. Po rozpoczęciu
  operacji anulowanie nie pomija powrotu. Zakończona akcja callbacka nie jest
  cofana; `action_called/action_result` pozostają dostępne także po cancel/błędzie
  recovery. Nie ponawiać automatycznie provisioning write po niepewnym wyniku.
- Target ustawiany wyłącznie przez istniejące `WIFI_STORAGE_RAM`. Funkcja nie
  wywołuje `dsr_settings_save`, nie zmienia `current`, NVS, bindingów, workspace,
  zdjęć, TLS, partycji, ani ustawień urządzenia Shelly. Callback nie otrzymuje
  zapisanych domowych credentials. Nie logujemy SSID/BSSID/IP/haseł.
- Podczas obcej sieci `dsr_wifi_connected` jest false. Snapshot publiczny nie
  publikuje foreign DHCP jako połączenia domowego; IP jest puste, status
  CONNECTING podczas operacji / DRIVER_ERROR po nieudanej restauracji.
  Już rozpoczęte połączenia sieciowe nie są anulowane przez ten prymityw.
  Przed jego podłączeniem kontroler musi zapewnić wyłączność transportu #20,
  zatrzymanie nowych LAN/HTTPS akcji i uczciwy komunikat utraty panelu WWW.
- Po każdej próbie przełączenia następuje próba stop→fence→start zapisanej sieci,
  także po błędzie/cancel callbacka. `home_config_restored` oznacza pozytywne
  zakończenie stop fence/config/start/connect-admission; `home_connected` osobno
  wymaga DHCP. Bez zapisanej sieci przywracany jest stan idle, nie fikcyjny LAN.
  `switch_attempted=false` oznacza, że odmowa/prekancel nie zmieniła sieci.
- Po braku zdarzenia STOP zapamiętujemy niedokończoną barierę także między
  próbami. Nie zmieniamy credentials tylko dlatego, że driver_started=false.
  Brak potwierdzonego powrotu blokuje automatyczny reconnect i kolejne visit/scan,
  aby nie powrócić automatycznie do obcego AP. Jawny zwykły connect zapisanej
  sieci umożliwia recovery. Nie obiecujemy, że uszkodzony sterownik zawsze wróci.
- Po zaniku zasilania ustawienia aplikacji nadal wskazują starą sieć: brak
  zapisu targetu. Świeży proces testuje inicjalizację z domowym rekordem MODEL;
  rzeczywisty power-cut i zachowanie sterownika/NVS wymagają testu sprzętowego.

## Dowody i bramki

Testy kompilują rzeczywiste `dsr_wifi.c`, politykę danych Wi-Fi i atomowy activity.
SDK/event loop/radio/persistence/boot są jawnie MODEL. Przypadki: brak zgody,
niepoprawny BSSID/SSID/hasło, niegotowy startup, brak domowej konfiguracji,
BSSID+PMF, serializacja/nested BUSY/restart admission, callback failures,
cancel przed/w trakcie, każde stop/config/start/connect failure, brak START/STOP,
DHCP/auth/not-found/timeout dla AP i home, old GOT_IP i odzyskanie po błędzie.
`AP_VISIT.json` sprawdza rzeczywiste docelowe własne ramki kompilatora i raportuje
retencję ELF oddzielnie; NIE mierzy stosu SDK/callbacka ani radia.
Lokalne UBSan i CI ASan/UBSan/LSan są osobnymi bramkami; ograniczony lokalny
`/proc` może uniemożliwić LSan i nie jest wynikiem PASS.

Następne etapy #19: kandydaci skanu z BSSID, potwierdzenie w #32, wyłączność
transportu, rzeczywista tożsamość API i obsługiwany model/generacja, świadome
przekazanie credentials (bez logów), ograniczona pojedyncza próba konfiguracji,
powrót LAN i rediscovery z tym samym stabilnym ID, dopiero potem binding #20.
SSID nie jest dowodem tożsamości. Nie wykonujemy tutaj `WiFi.SetConfig`.

Źródła: [ESP-IDF5.5.4 Wi-Fi](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/network/esp_wifi.html)
(`WIFI_STORAGE_RAM`, start/stop/connect, BSSID),
[Shelly WiFi API](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/WiFi/)
(zmiana STA może zerwać połączenie i wymaga ponownego odkrycia urządzenia).
Pełny #19 pozostaje **Not implemented jako funkcja użytkowa**. Wymagany realny
ES3C35P+Shelly AP test, powrót LAN/ON-OFF/restart/retencja/Wi-Fi/BLE; testy modelu
nie są physical pass. Bez erase/reset/main/auto-merge/Stable.
