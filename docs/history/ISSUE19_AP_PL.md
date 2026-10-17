# #19 — dodawanie Shelly przez fabryczne Wi-Fi AP

Kod wykonuje rzeczywisty skan radia, połączenie STA, żądania HTTP oraz odczyt
urządzenia po powrocie do LAN. Hostowe testy sterownika, protokołu i prawdziwego
LVGL nie zastępują odbioru na fizycznym Shelly. Nie uruchomiono operacji na
sprzęcie użytkownika ani nie zmieniono gałęzi Stable.

## Obsługiwany zakres

Ścieżka obsługuje **otwarty, fabryczny AP Shelly z lokalnym HTTP API bez hasła**,
odpowiadający na `192.168.33.1`. Gen1 i Gen2/3/4 są rozpoznawane z rzeczywistego
`GET /shelly`, a nie z tabeli modeli. AP wymagający klucza Wi-Fi, hasła API,
HTTPS/PKI lub przekierowania jest jawnie odrzucany. Nowe fabryczne firmware
mogą domyślnie wymagać enhanced security i mieć ograniczone okno konfiguracji;
ta wersja nie wyłącza zabezpieczeń, nie omija TLS i nie resetuje urządzenia.
Gen1 nie jest provisionowany do sieci działającej wyłącznie w WPA3.

Na LCD: **+ → Wi-Fi AP → Szukaj AP**. Skan odfiltrowuje kandydatów o nazwie
Shelly i końcówce MAC; chroniony AP jest oznaczony jako nieobsługiwany.
Wybranie wiersza nie przełącza radia ani nie wysyła danych sieci domowej.

1. Pierwszy ekran zgody opisuje czasową utratę domowego LAN, sterowania i panelu.
   Dopiero fizyczne **Połącz z AP** uruchamia połączenie przypięte do BSSID
   oraz kanału z rzeczywistego skanu i sprawdza lokalną tożsamość API.
2. Drugi ekran pokazuje odczytany model, ID i nazwę zapisanej sieci domowej.
   Ostrzega, że otwarty AP i HTTP nie zapewniają kryptograficznego
   uwierzytelnienia. Dopiero fizyczne **Przekaż Wi-Fi** zezwala na pojedynczy
   zapis danych do wybranego urządzenia. Hasło nie jest wyświetlane.
3. Pilot wraca do domowej sieci i szuka tego samego ID/MAC/generacji przez
   istniejący skaner LAN. Sprawdza również rzeczywisty stan STA, SSID i adres IP.
   Odczyt możliwości kieruje do istniejącego formularza nazwa/pomieszczenie/kanał.
   Zwykły BIND i jego trwałe potwierdzenia obu magazynów kończą dodawanie.

SSID, MAC, BSSID i publiczne API są sprawdzeniami spójności, **nie dowodem
kryptograficznej autentyczności**. Użytkownik wybiera własny sprzęt. Obie zgody
oraz błędy i przywracanie mają tłumaczenia PL/EN/DE. Podgląd ekranu WWW jest
wstrzymany przez cały kreator, także podczas ukrytego sprzątania.

## Uruchomienie z panelu WWW

Sparowany panel: **Urządzenia → Wyszukaj Shelly w LAN → Wi-Fi AP • kreator
na pilocie → Otwórz kreator na pilocie**. To rzeczywiste żądanie otwarcia LCD,
nie symulacja provisioningu w przeglądarce. Nie ma pola hasła Wi-Fi ani zgody
zastępującej dotyk. Wybór AP i obie zgody wykonuje się na pilocie. Panel jawnie
opisuje utratę LAN i powrót po jego przywróceniu; podgląd LCD jest wstrzymany.

| API v1 za istniejącym local/boot/auth gate | Wynik |
| --- | --- |
| `POST /api/v1/ap/open`, pusty body | 202: żądanie kolejkuje wyłącznie otwarcie LCD. Konfiguracja/credentials w body są odrzucane. Brak domowego Wi-Fi, busy kreator lub restart: odmowa. |
| `GET /api/v1/ap` | Publiczny stan handoff: 0 unavailable, 1 ready, 2 requested, 3 active, 4 expired. Bez SSID, hasła, MAC i BSSID. |

Mutex mailbox nie dotyka LVGL, Flash ani danych logowania. UI odbiera tylko
na ekranie HOME przy wolnym workerze; oczekiwanie wygasa po 10 sekundach.
Request nie trzyma długiego restart lease i może zniknąć przy restarcie,
ponieważ samo otwarcie niczego nie przełącza. Własny lease właściwego AP flow
zaczyna się dopiero przy fizycznej zgodzie na odejście z domu. Nie ma
samoczynnego ponawiania POST z WWW. Status GET jest odrzucany, jeśli przecina
okno POST; locale zachowuje DOM i stan żądania. Timeout/offline nie udaje
otwartego kreatora ani zakończonego provisioningu.

## Protokół i brak ponawiania zapisu

Bezpośrednio przed przekazaniem danych wykonywany jest ponowny, pozbawiony
credentials odczyt `/shelly`. Zmienione ID/MAC/generacja lub wymagane logowanie
blokują żądanie zapisu. Po potwierdzeniu:

| Generacja | Jedno żądanie zapisu | Późniejszy odczyt w LAN |
| --- | --- | --- |
| Gen1 | `POST /settings/sta`, form URL encoded: `enabled=1`, `ipv4_method=dhcp`, `ssid`, `key` | `/shelly`, następnie `/status` z `wifi_sta.connected`, `ssid`, `ip` |
| Gen2/3/4 | `POST /rpc/WiFi.SetConfig`, JSON params-only: `config.sta` z `ssid`, `pass`, `enable:true`, `ipv4mode:"dhcp"` | `/shelly`, następnie `/rpc/WiFi.GetStatus` z `status:"got ip"`, `ssid`, `sta_ip` |

Credentials nie trafiają do URL, logów, konfiguracji workspace ani podglądu.
Payload i odpowiedź transportu z POST są czyszczone przed zwolnieniem buforów.
Nie ma automatycznej autoryzacji, przekierowań, ponowienia POST ani reboot RPC.
Pinned ESP-IDF 5.5.4 dostaje `max_authorization_retries=-1` dla tego POST;
wartość `0` oznacza domyślne retry i nie służy do jego wyłączenia.
`restart_required:true` jest wynikiem nieobsługiwanym, bez automatycznego restartu.

Timeout lub zerwanie HTTP po zapisie oznacza niepewny wynik. Pilot przywraca
domowe Wi-Fi i sprawdza LAN, bez powtórnego wysyłania danych. Sam HTTP ACK nie
wyświetla sukcesu. Błąd po próbie zapisu nie obiecuje, że dane nie zostały wysłane.
Brak mDNS, izolacja klientów, VLAN, uśpienie, pełna lista albo brak urządzenia
w pasywnym ARP mogą uniemożliwić odnalezienie mimo skutecznego provisioningu;
po 45 sekundach wynik pozostaje niepotwierdzony. Nie ma skanowania całej podsieci.

Źródła protokołu:

- [Gen1 HTTP dialect, settings/sta i status](https://shelly-api-docs.shelly.cloud/gen1/)
- [WiFi.SetConfig i WiFi.GetStatus](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/WiFi/)
- [RPC przez HTTP, params-only POST](https://shelly-api-docs.shelly.cloud/gen2/General/RPCChannels/)
- [Sys, enhanced security i okna konfiguracji](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Sys/)
- [ESP-IDF 5.5.4 HTTP client](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/protocols/esp_http_client.html)

## Zachowanie sieci domowej i lifecycle

Driver zachowuje zapisane domowe ustawienia i używa `WIFI_STORAGE_RAM`.
Tymczasowe połączenie AP nigdy nie wywołuje `dsr_settings_save` ani nie podmienia
`current` domowego profilu. Dotyczy to również uzyskania DHCP z Shelly AP.
Po utracie zasilania boot wczytuje poprzedni profil z `dsr_cfg`; nie ma nowego
sekretu ani stanu sesji AP w NVS. Istniejące bindingi pozostają zachowane.

Przed przełączeniem radia worker ustawia barierę niezależnego zadania LAN:
odmawia nowych operacji, anuluje skan i czeka na zakończenie istniejącego HTTP
oraz wyczyszczenie jego credentials. Timeout bariery 12 s lub anulowanie
odmawiają przełączenia. Dzięki temu odczyt uwierzytelniony, rozpoczęty jeszcze
w domu, nie wysyła późniejszej części żądania do urządzenia w AP.

Po sukcesie, błędzie, upływie zgody lub anulowaniu pilot próbuje przywrócić
domowy profil. Błąd `stop/set_config/start` zachowuje flagę temporary, wyłącza
automatyczne łączenie i nie przedstawia AP jako domowego LAN. Widoczny błąd
oferuje ponowienie, a ukryte sprzątanie ponawia przywrócenie z odstępem 1 s.
Brak potwierdzenia `STA_STOPPED` jest pamiętany niezależnie od tego, czy SDK
już przyjął `stop`. Kolejne próby nie czyszczą oczekiwanego ACK i nie instalują
nowego profilu, dopóki rzeczywista bariera event loop nie zostanie potwierdzona.
Publiczny odczyt informacji Wi-Fi podczas temporary również maskuje
`connected` i IP; adres przydzielony przez Shelly AP nie jest domowym połączeniem.
Jeśli domowy profil został już zainstalowany, ale nie uzyskał IP, flaga temporary
jest zwalniana; zwykły reconnect może używać wyłącznie domowego profilu.

Jest jeden worker z dotychczasowym stosem 10240 B. Połączenie/restore czeka do
20 s na IP; operacje SDK i HTTP również mają własne ograniczenia. Cancel jest
obsługiwany po zakończeniu trwającego żądania, bez replay. Nie obiecujemy
natychmiastowego anulowania ani maksymalnego całkowitego czasu równego samej
sumie timeoutów. Lease obejmuje sesję aż do bezpiecznego przywrócenia profilu.
Ukrycie ekranu i powrót do niego nie otwierają drugiej sesji ani drugiego lease.

## Sprawdzenia i odbiór

- `python3 tools/test.py --ubsan-only`: rzeczywisty driver Wi-Fi z fault injection,
  rzeczywisty HTTP/cJSON i orkiestracja AP, router workera, NVS oraz istniejące testy.
- `python3 tools/test_lan_barrier.py --ubsan-only`: rzeczywisty kod zadania LAN,
  drain/wipe aktywnego inspect przed handoff, blokada enqueue, timeout/cancel/resume.
- `python3 tools/test_ui.py --ubsan-only`: rzeczywiste LVGL, oba consenty PL/EN/DE,
  brak POST po samym wyborze, opóźnione ACK, hide/reopen, błąd SDK restore,
  ukryte retry, DHCP-only failure, niepewny POST, BUSY verify, LAN handoff i
  deadlines po ponad 24,9 dnia uptime. Kadry hostowe są oznaczone TEST / DEMO.

Snapshot LAN do odszukania urządzenia jest alokowany na heap, nie kopiowany
na stos UI 8192 B. Job/event współdzielą warianty AP przez union. Pliki AP,
HTTP, Wi-Fi, worker i UI mają `.su` do oceny rzeczywistych ramek ESP32-S3;
hostowe wyniki nie są pomiarem stosu SDK ani runtime peak heap.
`python3 tools/check_ap_target.py --build-dir build-board --out build-board/AP.json`
sprawdza rzeczywiste pliki `.su` i tablice symboli obiektów, zachowując limity
2048 B workera/network, 8192 B stosu UI oraz 80 KiB internal BSS. Dodatkowo
ogranicza cały stały stan AP UI do 2048 B; snapshot LAN pozostaje na heap.
`python3 tools/test_ap_target.py` sprawdza odrzucanie przekroczeń i brakujących
raportów na syntetycznych danych, co nie jest dowodem pamięci rzeczywistego MCU.

Przed zamknięciem #19 wymagane są exact target CI/ELF oraz odbiór właściciela:
Gen1 i co najmniej jeden obsługiwany Gen2+, poprawna/błędna sieć, anulowanie
przed i po POST, reset zasilania w AP, ponowny start z dawnym domowym profilem,
zachowanie minimum dwóch bindingów, osobne nieobsługiwane auth/HTTPS i rzeczywisty
powrót do LAN. Bez takiego testu nie deklarujemy fizycznej zgodności modelu.
