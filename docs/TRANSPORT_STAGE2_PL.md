# Etap 2: transport Shelly — 2026-10-27

## Korekta po logu z płytki 0.7.78 — 2026-11-01 (obowiązuje, kandydat 0.7.79-dev)

Log właściciela z 0.7.78: roleta „layout 4” nie odpowiedziała na żaden
`POST /rpc` (3/3, nagłówki wysłane, 0 bajtów przez 3 s, `sdk=0x7007`), a na
GET przy pełnym odczycie odpowiedziała. Te same trzy „WRITE, OFFLINE” z 0.7.76
dotyczyły tej rolety i komend POST `/rpc/Cover.*`, podczas gdy GET chwilę
później działał. Druga roleta i przekaźniki odpowiadają na POST. IDF 5.5.4
wysyła treść POST drugim zapisem TCP (`esp_http_client.c`), GET to jedno
zapytanie. W changelogu Shelly jedyny pasujący wpis to 2.2.0-beta1 (15.10,
tylko beta): „HTTP: Fix timeouts on incoming connection”; stabilne 2.1.1/2.0.3
zmieniają tylko certyfikaty CA. Związek jest prawdopodobny, nie potwierdzony.

Decyzja właściciela: GET dla Gen2+. Status i komendy przekaźnika, rolety,
dimmera i RGB to `GET /rpc/<Metoda>?<parametry>` (`rgb=[r,g,b]` w formie
z dokumentacji Shelly; parser URL IDF przepuszcza `[`, `]`, `,`). Usunięto
ramkę `rpc_status()` i dowód tożsamości przez `src`. Przed każdym zapisem
`/shelly`, potem GET statusu; STOP: `/shelly` + komenda. Zwykły odczyt w tle
to jeden GET statusu bez `/shelly`; pełny odczyt z tożsamością co
`DSR_FLEET_META_POLLS` i po każdym błędzie. Ustawienia urządzenia, provisioning
AP (hasło nigdy w URL) i BluTrv pozostają POST. Potwierdzenie `null` jest
przyjmowane tylko dla komend. Sesja Digest, spóźnione dane i polityka Wi-Fi
z tego dokumentu bez zmian; przy samych GET jedno wyzwanie 401 obsługuje całe
wywołanie.

| Operacja (z hasłem, 401 = 2 wymiany) | 0.7.77 | 0.7.78 | GET |
| --- | --- | --- | --- |
| Komenda rolety/dimmera/RGB bez odczytu | 5 | 3 | `/shelly` 1 + status 2 + komenda 1 = 4 |
| Light/RGB z odczytem | 7 | 4 | 1 + 2 + 1 + 1 = 5 |
| Przekaźnik z odczytem | 5 | 4 | 1 + 2 + 1 = 4 |
| Zwykły odczyt / pełny co 5 min | 5 | 2 / 4 | 2 / 4 |
| Bez hasła: zwykły odczyt / komenda rolety | 3 / 3 | 1 / 2 | 1 / 3 |

Testy: oczekiwania hosta wróciły do przepływu GET; kroki atrapy mają
`post=false`, więc każdy POST dla tych urządzeń przerywa test. Nowe kontrole:
kształt żądań w `test_core`, `test_get_transport` (nonce dla całego
wywołania, odczyt bez `/shelly`, spóźniona odpowiedź i ACK, `null` tylko dla
komendy). Poniższe opisy `rpc_status()`, `fixture_rpc()` i tabela wymian
dotyczą wydanego 0.7.78 i są historyczne.

Odbiór na płytce: roleta 4 bez `sdk=0x7007` przy odczytach i komendach;
w logu `leg=status`/`leg=command`, bez `leg=rpc`. Zmiana nazwy w aplikacji
Shelly jest widoczna do ok. 5 min później. Gdyby pod adresem IP urządzenia
pojawił się inny Shelly tego samego typu, zwykłe odczyty mogą do ok. 5 min
pokazywać jego stan; każdy zapis najpierw sprawdza `/shelly`.

**Wynik odbioru — 2026-11-02 (`dev-v0.7.80`, ten sam transport co 0.7.79):**
właściciel potwierdził, że rolety reagują płynnie w skórkach Grafitowy
i Cyber Blue. Ocena słowna, bez nowego logu USB; liczników `leg=`/`sdk=`
nie analizowano. Punkty 2–4 z odbioru 0.7.78 (tryb oszczędzania Wi-Fi,
zmiana nazwy, bateria) pozostają bez osobnego potwierdzenia.

Baza: `dev-v0.7.77` / `ee1a8ba`. Wydanie: kandydat `0.7.78-dev` (tag
`dev-v0.7.78`) na polecenie właściciela, przed odbiorem na płytce; bez
flashowania. Host i kompilacja nie potwierdzają działania na ES3C35P; odbiór
na płytce opisano na końcu.

## Decyzje właściciela

- Oszczędzanie energii Wi-Fi: wyłączone, gdy ekran świeci i nie działa host BLE.
- Tożsamość Shelly Gen2+ (przekaźnik, roleta, dimmer, RGB) na szybkiej ścieżce:
  identyfikator z pola `src` odpowiedzi JSON-RPC; pełne `/shelly` i nazwa
  okresowo oraz po błędach.

## Ustalone fakty, na których oparto zmiany

| Źródło | Fakt |
| --- | --- |
| Shelly Gen2 RPC Channels | Kanał HTTP nie obsługuje keep-alive (`Connection: close`). Ramka `POST /rpc` odpowiada pełną ramką z `src` urządzenia. |
| Shelly Gen2 Authentication (2.0.0+) | Nonce Digest ważny 1 h, do 30 000 żądań przy rosnącym `nc`. Błędny digest liczy się jako nieudane logowanie; po 10 w 10 min opóźnienia i HTTP 429. Bufor 32 nonce; przy zapełnieniu nowe nonce dostają 429. |
| `esp_http_client.c` ESP-IDF v5.5.4 | Wynik handlera `HTTP_EVENT_ON_DATA` jest ignorowany. Nonce przechowuje metodę żądania z wyzwania 401. `esp_http_client_prepare()` jest publiczne i liczy nagłówek dla nowego URI. Połączony klient nie liczy nagłówka ponownie bez `prepare`. |
| `esp_wifi.h` / `wifi.rst` v5.5.4 | Domyślnie `WIFI_PS_MIN_MODEM`; `WIFI_PS_NONE` dopuszczalne, w koegzystencji radio i tak śpi w szczelinach BT. |

Wniosek: ponowne użycie połączenia TCP nic nie daje; ponowne użycie klienta
SDK w obrębie jednego zadania pozwala użyć nonce bez kolejnej rundy 401, ale
wyłącznie dla tej samej metody HTTP.

## Zmiany

| Obszar | Zmiana | Pliki |
| --- | --- | --- |
| Szybka ścieżka statusu | `rpc_status()`: `POST /rpc` z `{"id":1,"method":"<X>.GetStatus",...}`; ramka musi mieć `id`=1, obiekt `result` i unikalne klucze. `src` równe zapisanemu ID dowodzi tożsamości. Brak/inna pisownia `src` → `/shelly` + dotychczasowy GET. | `dsr_http.c` |
| Komendy | Cover (bez STOP), Light, RGB: status przez ramkę zamiast `/shelly` + GET (3 → 2 transakcje). Odczyt zwrotny przekaźnika/Light/RGB: ramka. Przekaźnik zachowuje wstępne `/shelly`; STOP zachowuje `/shelly` + POST. Zasada: tożsamość udowodniona przed każdym zapisem, bez ponawiania niepewnych zapisów. | `dsr_http.c` |
| Sesja Digest | Jeden klient SDK na wywołanie i host, na stosie (wątek LAN ma własny). Nonce oferowany tylko żądaniu z tą samą metodą; inne żądanie startuje bez poświadczeń i dostaje nowe wyzwanie. Odpowiedź inna niż 200 zwalnia klienta. | `dsr_http.c` |
| Spóźnione dane | Bajty po terminie nie są odrzucane (SDK i tak nie przerywa wymiany); `late` tylko w logu. Spóźniony ACK rolety jest teraz ACK, a nie `BAD_RESPONSE`. | `dsr_http.c` |
| Harmonogram metadanych | `meta_polls` w istniejącym wyrównaniu slotu (flota nadal 8080 B). Zwykłe odczyty Gen2+ są telemetryczne; pełny odczyt: pierwszy, co `DSR_FLEET_META_POLLS`=60 (5 min przy 5 s), po błędzie, zmianie Wi-Fi, nowym powiązaniu i ręcznym odświeżeniu. Gen1/BLU/H&T bez zmian. | `dsr_fleet.c/.h`, `dsr_worker.c`, `dsr_platform.h` |
| Wi-Fi | `dsr_wifi_power_display()` z pętli UI i `dsr_wifi_power_ble()` przed startem i po zwolnieniu NimBLE. `esp_wifi_set_ps()` tylko przy zmianie; błąd ponawiany. | `dsr_wifi.c`, `dsr_ble.c`, `dsr_ui_network.c` |
| Diagnostyka | Log `Cover HTTP` ma dodatkowo `auth=none/open/pre/chal` i `reuse`, etap `rpc`; zmiana trybu oszczędzania i fallback `src` są logowane bez hostów i ID. | `dsr_http.c`, `dsr_wifi.c` |

Liczba wymian dla urządzenia z hasłem (wyzwanie 401 = 2 wymiany):

| Operacja | Przed | Po |
| --- | --- | --- |
| Komenda rolety/dimmera/RGB (bez odczytu) | `/shelly` 1 + GET 2 + POST 2 = 5 | ramka 2 + POST 1 = 3 |
| Komenda Light/RGB z odczytem | 1 + 2 + 2 + 2 = 7 | 2 + 1 + 1 = 4 |
| Przekaźnik z odczytem | 1 + 2 + 2 = 5 | 1 + 2 + 1 = 4 |
| Odczyt po ruchu rolety | `/shelly` 1 + GET 2 = 3 | ramka 2 |
| Zwykły odczyt (co 5 s) | 1 + GET 2 + nazwa 2 = 5 | ramka 2; pełny co 5 min: 1 + 2 + 1 = 4 |
| Bez hasła: zwykły odczyt / komenda rolety | 3 / 3 | 1 / 2 |

## Testy (host, Cygwin; bez sanitizerów)

| Polecenie | Wynik |
| --- | --- |
| zestawy `tools/test.py`: `platform`, `actions_http_uncertainty`, `actions_worker`, `battery`, `wifi`, `ble` | PASS: 53312 / 228 / 1436 / 4615 / 532 / 2239 kontroli |
| `tools/test_automation.py --case http`, `--case runtime`; `tools/test_fleet_target.py`; `tools/test_system_data.py --case ble_owner`; `tools/check_docs.py` | PASS |
| `tools/test_ht_core_review.py` (lokalna kopia bez UBSan, którego Cygwin nie ma) | PASS; z UBSan tylko w CI |
| `tools/test_ui.py --cases test_cover_readback_priority_ui test_cover_ack_ui test_priority_ui test_cover_refresh_ui test_group_stop_ui test_screen_work_ui test_cover_ui test_light_ui test_rgb_ui test_blu_ui test_ht_ui` | PASS (LVGL 9.3) |

Atrapa `esp_http_client` modeluje teraz semantykę IDF 5.5.4: kilka klientów,
`prepare()` przy każdym połączeniu (Shelly zamyka), nonce związany z metodą.
Każdy nagłówek Digest z inną metodą lub nieaktualny nagłówek przy `auth=NONE`
przerywa test. `fixture()` sprawdza, że żaden klient nie wycieka.
`fixture_rpc()` zamienia dotychczasowe odpowiedzi urządzenia na ramki JSON-RPC.

Nowe regresje: walidacja ramki, fallback `src`, nonce bez drugiego 401 i bez
przekazania innej metodzie, brak poświadczeń, spóźniona odpowiedź i ACK,
harmonogram metadanych, trasowanie PROBE w workerze, polityka Wi-Fi/BLE.
Dowód „czerwony”: te same testy na wariantach kodu w katalogu tymczasowym
padają — odrzucanie spóźnionych bajtów (`test_platform.c:640`), brak sesji
(`:271`), nonce dla dowolnej metody (atrapa, `:135`).

Świadomie zmienione oczekiwania: brak `/shelly` przed statusem na szybkiej
ścieżce (mniej żądań, jedna granica pilnego STOP zamiast dwóch; dwie zostają w
fallbacku — nowy test), zwykły odczyt po ruchu jest telemetryczny, zmiana
profilu RGB jest wykrywana przez błąd RPC na komendzie i przez `identity()` w
pełnym odświeżaniu.

## Kompilacja docelowa

`tools/dev/Build-Firmware.ps1` (ESP-IDF 5.5.4, ES3C35P, bez flashowania): PASS,
bez ostrzeżeń. Aplikacja 0x442d10 B (+3296 B wobec 0.7.77), 29% wolnej
partycji. Na tym ELF: `check_boot_memory.py` (BSS wewnętrzny 61904 B, pula LVGL
131072 B w PSRAM, stos UI 8192 B), `check_fleet_target.py`, `check_ap_target.py`,
`check_native_ota_target.py`, `check_ble_target.py`, `check_actions_target.py` —
PASS. Największe ramki `dsr_http.c`: `dsr_http_lan_inspect` 928 B (zadanie LAN),
`fetch_json_payload` 496 B; `power_apply` 48 B. To statyczne ramki kompilatora,
nie pomiar high-water mark na płytce.

## Odbiór na płytce (właściciel)

1. Aktualizacja 0.7.77→0.7.78 i 20 komend rolety: w logu `Cover HTTP` liczba `leg=identity`
   powinna spaść, `leg=rpc` pojawić się przed `command`; przy haśle `auth=pre`
   na POST. Liczba `WRITE, OFFLINE` i czasy `ms` do porównania z 0.7.76/0.7.77.
2. `power save=none` po włączeniu ekranu, `min_modem` po wygaszeniu i przed BLE.
3. Zmiana nazwy w aplikacji Shelly: widoczna na pilocie do ok. 5 min.
4. Zużycie baterii przy włączonym ekranie (modem-sleep wyłączony) — ocena właściciela.

Ograniczenia: brak potwierdzenia formatu `src` na wszystkich modelach Gen2/3/4
właściciela (dlatego fallback), brak pomiaru radia i baterii, sanitizery tylko
w CI Linux.
