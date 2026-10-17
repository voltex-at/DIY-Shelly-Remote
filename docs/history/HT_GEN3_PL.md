# #23 — Shelly H&T Gen3: LAN, tylko odczyt

Pion dotyczy wyłącznie fizycznego **Shelly H&T Gen3, S3SN-0U12A**, z działającym
istniejącym transportem HTTP. Bazą jest RGB PR63 `e21e066b45ebd59d87969fa7a992b63c14ef8ca6`.
Nie wykonano testu fizycznego H&T, zapisu Flash pilota ani zmiany ustawień
urządzenia. Zintegrowana wersja projektu: `0.7.35-ht-dev`.

## Źródła i zakres

Oficjalne źródła producenta sprawdzone 2026-10-16:

- [Model i komponenty](https://shelly-api-docs.shelly.cloud/gen2/Devices/Gen3/ShellyHTG3/).
- [Kod S3SN-0U12A, zasilanie i uśpienie](https://kb.shelly.cloud/knowledge-base/shelly-h-t-gen3).
- [Temperature.GetStatus, tC/tF i błędy](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Temperature/).
- [Humidity.GetStatus i rh](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Humidity/).
- [DevicePower: battery.percent, V, external.present](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/DevicePower/).
- [Shelly.GetStatus / GetDeviceInfo / ListMethods](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly/).
- [WiFi.SetConfig / GetStatus](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/WiFi/),
  [BLE setup/security](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/BLE/).

Kanał0 jest wiązany dopiero po rzeczywistym statusie zawierającym trzy obiekty
`temperature:0`, `humidity:0`, `devicepower:0`, każdy z `id:0`. Dokument modelu
wymienia komponenty; adapter nie przyjmuje samych nazw/reklamy jako dowodu
ich identyfikatorów. Brak jednego komponentu odmawia BIND. Brak/null samych
pomiarów pozwala dodać potwierdzone urządzenie z jawnie nieznanymi wartościami.
Każdy odczyt sprawdza publiczną identity `/shelly` bez credentials i dokładny
model oraz fizyczne `gen:3`, przed autoryzowanym `GET /rpc/Shelly.GetStatus`.
Prywatna/publiczna generation2 nadal oznacza transport **Gen2+**, a nie zmianę
fizycznej generacji. Prywatny `endpoint_kind:4` oznacza tę jedną rodzinę H&T.

Nie ma sterowania wyjściem, ustawień progów/offsetów, HT_UI.SetConfig,
Temperature.SetConfig, Humidity.SetConfig, kalibracji ani wymuszania wake-up.
Gen1 H&T, Plus H&T, H&T Gen4, BTHome, BLU, TRV i inne sensory nie są aliasami.
Historia pomiarów pozostaje jawnie nieobsługiwana; Switch0 historia nie dostaje
próbek kind4. Grupy/sceny zachowują istniejący zakres Switch/Cover.

## Wartości i znaczenie czasu

Jedna odpowiedź Shelly.GetStatus tworzy jeden spójny bundle. `tC`, `rh` oraz
`battery.percent` są osobnymi liczbami lub wartościami nieznanymi; 0°C, 0% RH,
0% baterii i `external.present:false` pozostają prawdziwymi wartościami. Ułamki
są dozwolone. Nie wylicza się temperatury z tF, baterii z V ani mocy/energii.

API Temperature nie podaje numerycznego zakresu tC. Adapter stosuje defensywną
granicę −273.15..1000°C i opcjonalnego tF −459.67..1832°F. To granice adaptera,
nie zakres producenta ani deklarowana dokładność. Zakres pracy modelu0..40°C
nie odrzuca poprawnej ujemnej temperatury. RH i procent baterii przyjmują0..100.
Opcjonalne V jest tylko sprawdzane jako finite0..20V, nie jest prezentowane jako
napięcie sieciowe ani używane do szacowania procentu baterii.

Missing/null/error i błędny typ/zakres unieważniają pomiar danej sekcji. Błąd
sekcji DevicePower (także niepoprawne opcjonalne V) unieważnia baterię oraz
zewnętrzne zasilanie, zachowując temperaturę i RH z tej samej odpowiedzi.
Zewnętrzne zasilanie może być true/false/unknown. Nie oznacza ładowania baterii.
Duplikaty kluczy, złe identyfikatory lub niepoprawny cały bundle nie tworzą
nowej próbki. Nowy poprawny bundle **zastępuje całość**: własne missing/null
nie pożyczają starych wartości z poprzednim znacznikiem czasu.

Niepowodzenie całej transakcji (offline, IO/OOM, limit, malformed/bad response,
unsupported bundle) zachowuje ostatni poprawny bundle i **pierwotny sample_ms**,
z online=false i widocznym błędem. Zmieniona identity albo błąd autoryzacji
unieważniają dawną próbkę. Restart zaczyna od unknown, bez odtwarzania pomiarów
z NVS. LCD/WWW opisują czas **odebrania odpowiedzi**, nie czas pomiaru; czas
pomiaru pozostaje nieznany. Nie zaokrągla się brakujących pól do0.

H&T nie utrzymuje stale otwartego LAN. Producent opisuje uśpienie również na
USB: wybudzenie, pomiar i raport co5min, następnie ponowny sen. Timeout nie
jest dowodem, że czujnik śpi. UI mówi „brak odpowiedzi / możliwe uśpienie”.
Samo odpytywanie może nie trafić w krótkie okno aktywności, także przy zasilaniu
USB. Krótki przycisk urządzenia może wybudzić je do konfiguracji zgodnie z KB;
pilot nie wykonuje resetu. Nie obiecuje się ciągłego strumienia ani odbioru
push/MQTT/WebSocket. Poll po sukcesie60s, wykładniczy backoff do15min;
auth/identity/rate-limit blokują automatyczny poll do świadomego odświeżenia.
Świeże online jest ograniczone do15s od odebrania odpowiedzi, ostatnie liczby
pozostają widoczne z rosnącym wiekiem. Wi-Fi pilota musi działać do Refresh.

## BIND i odświeżanie

LAN/manualny BIND wskazuje jawnie H&T. Publiczna kategoria workspace to
istniejące schema2 `sensor`; prywatny kind4 wybiera transport i renderer.
Stara karta sensor w tym samym host/channel ani zapisany Switch0 nie dostają
credentials/konwersji. Nowe H&T dostaje nowy numericID dopiero po dokładnym
workspace ACK; prywatna publikacja sprawdza ten sam pełny klucz i ownership.
Kind4 layout0 jest w kwarantannie bez host fallback. Usunięcie karty, zmiana
lokatora, opóźnione ACK i retry metadata zachowują istniejące guardy P0 BIND.

LCD ma wyłącznie pomiary, wiek, informację o zasilaniu i „Odśwież status”. WWW
ma ten sam read-only widok PL/EN/DE. Uwierzytelniony
`POST /api/v1/devices/refresh` przyjmuje dokładnie
`{device_id,generation:2,endpoint_kind:4,channel:0}`; nie przyjmuje hosta,
credentials, on ani komend ustawień. Istniejący bounded mailbox daje
operation_id/boot_id, rozwiązuje prywatny lokator i kolejkuje **PROBE/GET**.
Duplikat pending jest BUSY; błędy transportu nie wywołują ponownego POST.
Jawny Refresh może sprawdzić offline/stary/auth stan ponownie, zawsze przez
identity guard przed credentialed GetStatus. Nie wymaga świeżej próbki aktuatora.
Jeden serialny worker zachowuje opóźnienie aktualnego requestu; Refresh nie
przerywa radio/HTTP i nie tworzy dodatkowego tasku. HTTP legs mają istniejący
limit3s oraz SDK Digest; to nie deklaracja twardego łącznego czasu wszystkich
prób/challenge.

B1 pozostaje otwartym setup bez pairing/bonds. Tylko exact model/gen3 może
ominąć dotychczasowy veto baterii, i tylko po rzeczywistych ListMethods
WiFi.SetConfig/GetStatus, Sys.GetConfig, Shelly.GetDeviceInfo/GetStatus oraz
Temperature/Humidity/DevicePower.GetStatus. Obowiązują >=1.7.5,
provision pending/confirmed, kontrola enhanced_security i obie zgody fizyczne.
Jedno WiFi.SetConfig, brak replay po niepewnym wyniku, exact id/MAC/gen/SSID/IP
LAN readback i typed BIND pozostają bez zmian. Brak metody/okna/HTTP wsparcia
odmawia, bez sztucznej flagi. Fabryczne HTTPS2.0/encrypted BLE nadal poza B1;
nie ma wyłączenia enhanced security, skip-TLS ani resetu bondów.

## Trwałość, pamięć, dowody

Prywatny raw registry **v6**, nadal4104B, zachowuje byte layout v1/v2 Switch,
v3 Cover, v4 Light, v5 RGB. Prawidłowe starsze rekordy importuje tylko do RAM,
bez boot write. Pierwsza jawna mutacja zapisuje v6 z exact readback. Nieznana
przyszła wersja, corrupt/OOM/IO odmawiają bez erase/fallback do legacy.
Starszy binary znający do v5 zachowuje v6 jako future, bez interpretacji H&T
jako Switch. Workspace schema2 i jego whitelist nie zmieniają się. Downgrade
nie pokaże nowych pomiarów, zachowuje pliki/ID; pozostałe znane ograniczenia
legacy prywatnego blobu z poprzednich pionów pozostają.

Telemetria32B wykorzystuje typed union trzech presence/double oraz status5B;
każdy konsument rozróżnia kind4 przed W/V/A/Wh. Connection296, job560,
event1192, mobile device320, mobile model6944 i fleet8080B pozostają bez zmian.
Nie dodano tasku, dużego static snapshotu, formatu historii ani nowych buforów
stałych. Target BSS/.su/ELF i fizyczny odbiór to osobne bramki integracji.

Host UBSan: rzeczywisty parser/HTTP/NVS/fleet, niezależny RED→GREEN dla
malformed bundle retaining last-good, exhaustive cJSON OOM parser/serializer,
mailbox i worker read-only route. Genuine LVGL sprawdza dwie zgody B1→LAN→
nowy typed numericID obok legacy sensor, zera/null/retained czas, Refresh GET,
PL/EN/DE i40 cykli bez zmiany heap. Actual JS model sprawdza brak actuator
kontrolek, jeden refresh, pending, rosnący wiek i zachowany DOM/locale.
Playwright scenariusz jest dodany do istniejącego browser CI; nie deklaruje
się lokalnego browser PASS ani rzeczywistego sprzętu na podstawie fixture.
