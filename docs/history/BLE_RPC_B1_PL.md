# #18 B1 — projekt i postęp rzeczywistego dodawania przez GATT

Pion DEV 0.7.27 ma rzeczywisty transport NimBLE GATT, dwa potwierdzenia na LCD,
pojedynczy zapis Wi-Fi, odczyt stanu oraz weryfikację tego samego urządzenia przez
HTTP w domowym LAN. Dopiero potem przechodzi do istniejącego formularza
nazwa/pomieszczenie/kanał i trwałego BIND. Nie jest to fizyczny odbiór sprzętu ani
pełne zamknięcie #18: bonding, urządzenia już skonfigurowane, BTHome i TLS-only
pozostają odrębnymi etapami istniejącego backlogu.

W LCD wybierz **+ → Bluetooth LE → wynik z RPC → Dodaj przez BLE**. Pierwsza
zgoda uruchamia sprawdzenie GATT; druga, po odczycie rzeczywistego modelu i ID,
zezwala na przekazanie zapisanej sieci. WWW może wskazać wynik bieżącego skanu i
otworzyć ten sam kreator na LCD. Nie przyjmuje adresu BLE ani hasła i nie wyraża
żadnej z obu fizycznych zgód. Podgląd WWW jest wstrzymany podczas kreatora.

## Zamrożony pierwszy zakres

Gen2/3/4 Wi-Fi z rzeczywistym kanałem switch, `GetDeviceInfo.provision` równym
`pending` lub `confirmed`, bez hasła API i z działającym obecnym transportem HTTP
na LAN. Brak `provision` jest nieobsługiwany; pole jest udokumentowane od 1.7.5.
BLU/BTHome/TRV, sensor bateryjny, Gen1, `locked`/`complete`, auth i HTTPS-only
nie są przedstawiane jako gotowe do dodania. Nie używamy nazwy reklamy jako
tożsamości. Adres BLE służy do połączenia; ID/MAC/generacja z RPC służą do
późniejszego porównania LAN. MAC różnych interfejsów nie muszą być takie same.

Pierwszy pion korzysta z oficjalnego otwartego okna setup. **Nie tworzy i nie
używa bondów.** SMP i trwały bond store są wyłączone. Błąd ATT wymagający security oraz callback
unsolicited pairing/encryption odmawia przed PSK.
Oddzielne zgody LCD przed połączeniem i przed danymi domowego Wi-Fi; ostrzeżenie
o braku kryptograficznego uwierzytelnienia otwartego kanału. Nie obiecujemy MITM
protection, sparowania ani pełnego bezpieczeństwa BLE firmware 2.0.

Przyszły bonded path wymaga odrębnego, udowodnionego adaptera trwałości. Dokładny
esp-nimble z IDF 5.5.4 (`5c1a43ca3d205c3d5183e57dedce335fb6a81155`) emituje
pairing complete przed zapisem, ignoruje błędy zapisu w SM i ma publiczny init
store typu void, który tylko loguje awarię odczytu. ENC_CHANGE ani RAM lookup nie
są dowodem trwałego bondingu. Nie kopiujemy prywatnego layoutu NVS, nie używamy
SDK linker-wrap, automatycznego usuwania/eviction, debug keys ani factory reset.

## Implementacja

- Istniejące `dsr_ble.c` i `dsr_ble_model.c` pozostają jedynym skanerem i modelem
  reklam. Ten sam manager obsługuje jedną operację scan albo GATT; jeden NimBLE
  host, CENTRAL/GATT_CLIENT obok OBSERVER. PERIPHERAL/BROADCASTER pozostają off.
  Brak dodatkowego workera i dużych pól w `dsr_job_t`/`dsr_event_t`.
- Prywatne, ograniczone bufory heap; callbacki nie wywołują LVGL. Każdy callback
  ma token sesji, numer procedury, connection i attribute handle. Cleanup czeka
  na terminalny ACK/disconnect; brak ACK utrzymuje kwarantannę, zanim uchwyt
  mógłby zostać użyty w nowej sesji. Cancel uszczelnia przyjmowanie wyników.
- Service `5f6d4f53-5f52-5043-5f53-56435f49445f`, DATA `...-5f64-6174615f5f5f`,
  TX_CTL `...-5f74-785f63746c5f`, RX_CTL `...-5f72-785f63746c5f` wymagane wraz
  z właściwościami. TX_CTL to uint32 big endian długości UTF-8, DATA pisane
  kolejno z response, fragmenty do MTU−3. RX_CTL musi mieć 4 bajty i długość
  do 8192; zerowe ogłoszenie może jedynie oczekiwać na ramkę w ograniczonym czasie.
  DATA czytane do dokładnej długości, zero/overflow/truncation oznacza błąd.
- Request ≤1024, response ≤8192, jedna transakcja ≤15 s. ACK ATT nie dowodzi
  sukcesu RPC. Wymagane dokładne `id`, `dst` i `src`, result XOR error, walidacja
  duplicate keys i escaped NUL. Notification nigdy nie potwierdza komendy.
- Przed PSK ponowny rzeczywisty odczyt identity, setup i enhanced-security policy.
  Jeden `WiFi.SetConfig` na fizyczną zgodę. Po możliwym DATA write nie powtarzamy
  zapisu: timeout jest niepewny, dalsze operacje wyłącznie readback.
- Pilot pozostaje na domowym Wi-Fi, nie zapisuje `dsr_cfg`. Po GATT STA readback:
  ten sam ID/MAC/gen i home SSID/IP przez istniejący LAN HTTP, możliwości,
  name/room/channel i istniejący multi BIND z ACK obu magazynów.
- WWW wybiera wyłącznie istniejący rekord przez serial skanu+index, rozwiązywany
  i walidowany po stronie firmware; brak MAC/PSK przyjmowanego od przeglądarki.
  TTL/mutex handoff kieruje na LCD, obie zgody fizyczne; podgląd wstrzymany.
  AP i GATT provisioning nie mogą jednocześnie używać radia/sekretów sesji.

## Granice wykonania i pamięci

Jedna sesja central, manager BLE 4096 B i dotychczasowy host NimBLE 4096 B;
brak nowego taska, powiększenia `dsr_job_t`/`dsr_event_t`, zmian UI8192 ani
network worker10240. Polling HTTP innych Shelly pozostaje niezależny od GATT.
AP i BLE provisioning mają wzajemną blokadę. Końcowy HTTP STA verify korzysta
z istniejącego pojedynczego workera; może czekać na bieżący bounded request,
a odmowa BUSY ponawia tylko admission odczytu, nigdy WiFi.SetConfig.

Connect ma SDK timeout10 s i owner12 s, discovery oraz każda transakcja limit15 s,
zgoda na przekazanie120 s, STA i poszukiwanie LAN45 s. Terminy poszczególnych
etapów mogą się sumować. Przy braku terminalnego GAP ACK po4 s jest kwarantanna:
nie zwalniamy uchwytu/lease i nie uruchamiamy nowej sesji na pozornie wolnym
połączeniu. Trwała awaria hosta wymaga diagnozy sprzętu; UI nie ogłasza sukcesu.

Ramka TX≤1024, RX≤8192 +NUL, jeden bufor read516 B. Żywa transakcja ma dodatkowo
kopię parsera≤8193 B i drzewo cJSON. Błędy alokacji są fail closed; request z PSK
jest wymazywany również po nieudanym calloc RX. Nie zmieniamy domowego NVS ani
nie tworzymy/używamy bondów. Flash format i schema pozostają dotychczasowe.

`check_ble_target.py` ocenia rzeczywiste `.su` wszystkich nowych units, publiczne
API, BSS obiektów oraz dotychczasowy limit całego internalBSS81920 B.
Nie jest to pomiar transitive SDK stack ani runtime heap peak. Bazowy AP miał
66552 B; dokładny koszt central/GATT musi pochodzić z artefaktu CI tego źródła.

## Testy i pozostały odbiór

- `python3 tools/test_ble_rpc.py --ubsan-only`:152554 framing/parser/policy/OOM,
  20203 rzeczywisty klient GATT ze SDK callbacks MODEL oraz30 handler WWW.
  Obejmuje ACK fragmentów, id/src/dst, notification, MTU23/247/517,
  prequeued cancel bez connect, stare callbacks, identity/setup recheck,
  auth/enhanced/locked, utratę ACK zapisu bez replay, opóźniony disconnect i
  wymazanie całego serialized request przed free po RX allocation failure.
- `python3 tools/test.py --ubsan-only`: pełny portable suite PASS, w tym
  platform1384, Wi-Fi441, scanner/mailbox2173 i worker3613. Rzeczywisty kod z
  dostawcami HTTP/FreeRTOS/NVS MODEL; zero operacji na fizycznym sprzęcie.
- `python3 tools/test_ui.py --ubsan-only`:3964 genuine LVGL checks,134 kadry,
  dwie zgody PL/EN/DE, hidden/reopen i late ACK, timeout po2^31 ms,
  uncertain write→same-ID LAN→HTTP STA→istniejący formularz BIND.
- `node tests/test_panel_ble_model.cjs`:28 actual shipped view checks; model DOM,
  stare GET, utrata odpowiedzi POST, serial/index zamiast MAC/PSK, brak replay.
  Dictionary2083 checks/263 wpisy PL/EN/DE. Playwright i target ELF mają osobne
  bramki CI; host nie dowodzi fizycznego dołączenia do Wi-Fi.

Odbiór fizyczny musi potwierdzić własny wspierany Shelly w pending/confirmed,
minimum MTU23, zerwane połączenie przed/po zapisie, odmowę locked/auth/enhanced,
przyjęcie obu zgód, ten sam ID/MAC/gen i homeSSID/IP w LAN, wybór kanału/nazwy/
pokoju i trwały drugi binding po restarcie. Równolegle sprawdzić zwykłe ON/OFF,
AP cancel/home recovery, BLE scan bez Wi-Fi, brak danych Wi-Fi w logach/WWW oraz
wolny/largest internal heap i stabilny LCD. Niespełniona bramka sprzętowa nie jest
opisywana jako PASS; stary pojedynczy binding pozostaje zachowany przy błędzie.

## Checkpoint z natywnym OTA

Zintegrowano z natywnym OTA od `e69e24a9be67cc81b9beec3547d4dfef0d703193`
w oddzielnym worktree. Kolejność końca enum to `AP_VERIFY`, `OTA_CHECK`,
`OTA_INSTALL`, `LAN_STA_VERIFY`. OTA oraz STA mają jawny dispatch przed relaysSET;
walidacja AP obejmuje wyłącznie zakres AP. Struktury job/event nie zmieniły pól
ani rozmiaru. Zachowane są native init, powiadomienia UI, consent/power TTL,
legacy OTA/auth normalization, oba target checkery i oba zestawy testów LCD.

UI nie rozpoczyna OTA podczas BLE consent/cleanup ani GATT podczas pracującego
zadania OTA. Zwykłe poll/SET pozostają niezależne od managera GATT. Testy
rzeczywistego LCD sprawdzają obie odmowy admission z zerem dodatkowych zadań.
Full native UBSan PASS; integrated worker3637, platform1384, GATT20203;
genuine LVGL4099 checks/142 kadry, WebBLE28/AP46/runtime47/dictionary2083 PASS.
Są to testy hosta, bez deklaracji targetu ani sprzętu; source CI ma osobno zmierzyć
Xtensa i całe BSS po złożeniu obu pionów.

## Źródła pierwotne

- [Shelly GATT UUID i framing](https://kb.shelly.cloud/knowledge-base/kbsa-communicating-with-shelly-devices-via-bluetoo)
- [RPC request/response/notification](https://shelly-api-docs.shelly.cloud/gen2/General/RPCProtocol/)
- [GetDeviceInfo i provision](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly/)
- [BLE setup window i bonding](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/BLE/)
- [Enhanced security](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Sys/)
- [Factory 2.0 HTTPS i Shelly PKI](https://shelly-api-docs.shelly.cloud/gen2/General/CustomHTTPSCertificates/)
- [IDF 5.5.4 Kconfig](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/kconfig-reference.html)
- [Pinned blecent example](https://github.com/espressif/esp-idf/blob/v5.5.4/examples/bluetooth/nimble/blecent/main/main.c)
- [Pinned SM persistence](https://github.com/espressif/esp-nimble/blob/5c1a43ca3d205c3d5183e57dedce335fb6a81155/nimble/host/src/ble_sm.c)

- [WiFi.SetConfig i WiFi.GetStatus](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/WiFi/)
- [Pinned GATT API](https://github.com/espressif/esp-nimble/blob/5c1a43ca3d205c3d5183e57dedce335fb6a81155/nimble/host/include/host/ble_gatt.h)
