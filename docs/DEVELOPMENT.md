# Development

## 0.7.63-dev — pełny Cyber Blue i kompletny atlas

Wydanie na żądanie właściciela z `work/cyber-blue-complete`.
`DEV_REQUEST`, `.github/dev-release-0.7.63.json` i `release.yml` wskazują
wyłącznie `dev-v0.7.63`, `0.7.63-dev` i tę gałąź. Tag powstaje dopiero
po CI 5/5 dla dokładnego commita. Build, audyty ELF, pakowanie i podpis
pozostają w istniejącym procesie; nie zmieniono kluczy ani bramki Stable.

Pełny atlas 434 klatek, zakres i odtwarzanie:
[cyber-blue/COMPLETE_AUDIT.md](cyber-blue/COMPLETE_AUDIT.md).
Fizyczny odbiór nowego UI, dotyku i OTA pozostaje oddzielnym etapem.

## 0.7.62-dev — zakończenie BLE i restart

Zgłoszenie właściciela: skan zwraca błąd, a kolejne połączenie może utknąć
na „Zamykanie połączenia”; restart pozostaje zajęty. Potwierdzona przyczyna
w kodzie: nieograniczone pętle `disconnect_peer()` w BLU i BLE RPC,
utrzymujące `dsr_activity_begin()` do nadejścia callbacku GAP.

Zamykanie ma 4 s limitu oczekiwania. `BLE_HS_ENOTCONN` jest potwierdzeniem,
że połączenia już nie ma. Brak potwierdzenia izoluje stary host w managerze
i zleca asynchroniczny `ble_hs_sched_reset()`; terminalny błąd zwalnia
aktywność. Nie wolno otworzyć nowej sesji, dopóki stop/deinit starego hosta
nie zakończy się poprawnie. Spóźniony sync nie znosi tej blokady. Stop SDK
może czekać na swój task; odbywa się po zwolnieniu aktywności i nie blokuje
admisji restartu. Nie niszczymy taska ani pamięci nadal używanej przez SDK.

BLU ma 120 s budżetu operacji; restart LCD anuluje BLE i czeka do 10 s na
normalną admisję, bez obchodzenia blokady Flash/OTA. Testy `test_blu.py`,
`test_ble_rpc.py`, scanner w `test.py` i `test_ble_cleanup_ui` obejmują brak
callbacków, anulowanie, późne zdarzenia i ponowne otwarcie po deinit.
`test_ota_fetch.py` obejmuje SDK EAGAIN przed i pomiędzy nagłówkami,
kontynuację tego samego klienta, timeout i anulowanie. Nie jest to odbiór RF.

Dokładny request Dev: `.github/dev-release-0.7.62.json`, `DEV_REQUEST`
i workflow dopuszczają tylko `dev-v0.7.62`, `0.7.62-dev` oraz
`work/shelly-hud-cyber-blue`, po CI 5/5 dla publikowanego SHA. Main i Stable
pozostają poza tą publikacją. Partycje i zapisane dane bez zmian.

## Layout

| Path | Contents |
| --- | --- |
| `main/` | `app_main.c`: boot order (board → splash → storage → workspace → UI → network) |
| `components/dsr_board` | ES3C35P display/touch/backlight, LVGL owner task (`dsr_ui_task.c`) |
| `components/dsr_platform` | Storage/SPIFFS, NVS registry, worker, HTTP/Shelly, Wi-Fi/AP, BLE, panel (HTTPS + `web/`), actions, SNTP clock, native OTA actor |
| `components/dsr_ui` | LVGL screens (`dsr_ui_*.c`), style/brand, fonts, icons |
| `components/dsr_ota` | Manifest (Ed25519), trust store, image stream, fetch/release chain, installer |
| `components/dsr_config` | Shared configuration document (cJSON), store, photo names |
| `components/dsr_core` | Portable bounded model, request builders, version policy |
| `components/dsr_crypto` | Vendored Monocypher (hash-pinned by `tools/verify_crypto_vendor.py`) |
| `boards/lcdwiki-es3c35p` | Partition table, sdkconfig defaults, [hardware reference](../boards/lcdwiki-es3c35p/README.md) |
| `tests/` | Host C tests (`test_*.c`), fixtures per area, UI tests (`tests/ui`), panel model tests (`*.cjs`) |
| `tools/` | Test runners (`test*.py`), target auditors (`check_*.py`), packaging and OTA signing |

## Host tests (no hardware)

Requires a C11 compiler, `libcjson-dev`, OpenSSL (`libssl-dev`) and Python 3.

```sh
python3 tools/test.py --sanitize          # all portable C suites (ASan/UBSan)
python3 tools/test_config.py --sanitize
python3 tools/test_ui.py --sanitize       # genuine LVGL 9.3.0 screens, needs .test-deps/lvgl
python3 tools/test_ui.py --ubsan-only --case test_owner_ux_ui # photo/video LCD regressions
python3 tools/test_ui.py --review         # renders 320x480 frames + review ZIP into build-ui/
python3 tools/test_idf_spiffs.py --sanitize --cjson-source <esp-idf>/components/json/cJSON
node tests/test_panel*.cjs                # panel JS models
```

`--ubsan-only` exists for runtimes without LeakSanitizer; it is not a substitute
for ASan. Negative tests (rejections, fault injection) must not be removed.

## Firmware build

ESP-IDF **v5.5.4**, LVGL 9.3.0 and ST77922 2.1.1 (pinned in `dependencies.lock`).

```sh
idf.py -B build-board -D DSR_BOARD=lcdwiki-es3c35p set-target esp32s3
idf.py -B build-board -D DSR_BOARD=lcdwiki-es3c35p build
```

`-D DSR_VERSION=x.y.z` overrides `PROJECT_VER` from `CMakeLists.txt`. After a
build the `tools/check_*.py` auditors verify stack/BSS budgets, symbols and the
OTA trust key against the real ELF (see `.github/workflows/ci.yml`). Memory
limits are not raised to make CI pass.

## CI (`.github/workflows/ci.yml`)

| Job | What it proves |
| --- | --- |
| `native` | Host C suites with ASan/UBSan, config, SPIFFS, OTA parsers |
| `ui-review` | Genuine LVGL screens, panel assets, review frames (artifact) |
| `diagnostic` (esp32, esp32s3) | Compile coverage only, not a supported board |
| `es3c35p` | Board firmware, target auditors, FACTORY 16 MiB + UPDATE packages |

CI is compile/host evidence only. Physical confirmation on the ES3C35P is
reported separately.

## Releases (`.github/workflows/release.yml`)

The owner publishes a GitHub Release (`dev-vX.Y.Z` as pre-release, `vX.Y.Z` as
Stable). The workflow builds, signs the v2 manifest with the `DSR_OTA_SIGNING_KEY`
secret ([OTA_MANIFEST_PL.md](OTA_MANIFEST_PL.md)) and attaches the app BIN,
manifest and FACTORY ZIP. Stable and merges to `main` require explicit owner
approval. Never commit keys, tokens, SSIDs or backups.

### Wydanie 0.7.53-dev

Jednorazowe żądanie `.github/dev-release-0.7.53.json` na
`work/codex-p0-review` uruchamia istniejący `Signed release`.
`release_tool.py` akceptuje wyłącznie tę parę gałęzi i wersji; tag tworzy
dopiero po CI 5/5 dla dokładnego SHA. Tag i opublikowane zasoby są niezmienne.
Podpis pozostaje w chronionym środowisku `release`; ścieżka Stable zachowuje
osobną zgodę właściciela. Oba archiwa instalacyjne przechodzą kontrolę
pakietów, FACTORY dodatkowo rzeczywisty montaż SPIFFS IDF 5.5.4 na kopii NOR.

### Kontrola katalogu OTA przed publikacją

`release_tool.py publish` publikuje tylko krótki nagłówek bieżącej wersji
(do 320 bajtów UTF-8) i link do pełnej historii w tagu. Po uploadzie sprawdza
cały obiekt GitHub Release wraz z metadanymi zasobów: limit 16 KiB oraz
`dsr_ota_discovery_parse` z firmware. Dzięki temu jedna strona z jednym
wydaniem nie może zostać opublikowana z opisem przepełniającym klienta.

`repair-dev-notes.yml` jest jednorazową korektą opisu istniejącego 0.7.53
na zgłoszenie fizycznego błędu 0.7.52 `5/7/200/1`. Żądanie przypina stary
hash opisu, release id, źródłowy commit i wszystkie digesty. Zmienia tylko
pole `body`; tag, kanał, pliki i podpis pozostają bez zmian. Hostowy checker
jest read-only i nie wykonuje operacji na urządzeniu.

### Wydanie 0.7.54-dev i metadane Draft

Żądanie `.github/dev-release-0.7.54.json` dotyczy wyłącznie tej wersji na
`work/p0-spiffs-ota-brand`; firmware przypina tag do `aaff1d1` po CI 5/5.
Pierwszą publikację zatrzymał parser katalogu przy metadanych Draft. Kontrola
przed publikacją rozpoznaje lokalne tymczasowe ścieżki `untagged-*` i sprawdza
ich docelowe URL tagu. Po publikacji sprawdza również rzeczywistą odpowiedź
publiczną bez poprawiania URL; błąd ponownie ukrywa wydanie.

Jednorazowy `complete-dev-054.yml` wznawia tylko widoczność istniejących pięciu
zasobów: wymaga CI 5/5 firmware i kodu narzędzia, stałego tagu/SHA/uruchomienia,
zgodnych digestów i odczytanego podpisu produkcyjnego. Nie podpisuje ponownie,
nie wysyła i nie zastępuje plików. Wymaga dokładnego żądania
`.github/complete-dev-0.7.54.json`.

### Wydanie 0.7.55-dev

`.github/dev-release-0.7.55.json` jest następnym, jawnie zleconym wydaniem DEV.
Dla tego wydania `release.yml` i `DEV_REQUEST` wskazywały tylko 0.7.55 oraz gałąź P0. Tag powstał
dopiero po pełnym CI 5/5 dokładnego SHA, bez zmiany poprzedniego tagu lub
zasobów 0.7.54. `main` i Stable pozostają osobne. Testy konfiguracji:
`tools/test_device_settings.py`, sterownika GATT: `tools/test_blu.py`, LCD:
`tools/test_ui.py --case test_shelly_settings_ui`. Pełne CI nadal uruchamia
wszystkie dotychczasowe testy, ASan/UBSan i nowe audyty stosów/BSS.
NimBLE ma teraz SMP/NVS dla BLU, ale stary przepływ B1 nadal odmawia parowania.
Nie interpretować testów fixture jako pomiaru SMP/RF ani trwałości Flash na sprzęcie.

### Wydanie 0.7.56-dev

Właściciel jawnie zlecił publikację OTA Dev 2026-10-19 (Europe/Vienna).
`.github/dev-release-0.7.56.json`, `release.yml` i `DEV_REQUEST` dopuszczały
wyłącznie nowy tag `dev-v0.7.56`, wersję `0.7.56-dev` i gałąź P0.
Istniejące tagi i zasoby pozostają niezmienne. Gate czeka na CI 5/5 dokładnego
commitu przed utworzeniem tagu. Podpis używa zatwierdzonego klucza w środowisku
`release`; klucz nie jest odczytywany do workspace. Nie zmienia to main ani
Stable. Zakres: uproszczone sceny/grupy i poprawki zgłoszeń LCD, Cover, TRV,
priorytetu dotyku oraz postępu OTA. Odbiór fizyczny pozostaje osobnym krokiem.

### Wydanie 0.7.57-dev

Właściciel zlecił publikację OTA Dev 2026-10-19 (Europe/Vienna), po korekcie
położenia głównych przycisków aktualizacji. `.github/dev-release-0.7.57.json`,
`release.yml` i `DEV_REQUEST` dopuszczały wyłącznie nowy tag `dev-v0.7.57`,
wersję `0.7.57-dev` i gałąź P0. Gate tworzy tag po CI 5/5 dokładnego commitu;
podpis pozostaje w chronionym środowisku `release` z dotychczasowym kluczem.
Poprzednie wydania są niezmienne. Zakres obejmuje również wcześniejsze poprawki
dotyku, wspólnej nawigacji grup/scen i widoków BLU/TRV. Testy hostowe i audyty
targetu nie potwierdzają fizycznej płynności ani szczytu pamięci SDK.

### Wydanie 0.7.58-dev — Cyber Blue Premium, etap 3

Właściciel jawnie zlecił publikację Dev 2026-10-19 po zakończeniu etapu 3.
`.github/dev-release-0.7.58.json`, `release.yml` i `DEV_REQUEST` dopuszczają
wyłącznie tag `dev-v0.7.58`, wersję `0.7.58-dev` i gałąź
`work/shelly-hud-cyber-blue`. To aktualizacja istniejącego mechanizmu żądania
wydania, z pełnym CI 5/5 dla dokładnego SHA przed utworzeniem niezmiennego tagu.
Podpis pozostaje w chronionym środowisku `release`; klucz nie jest pobierany
do workspace. Poprzednie tagi/zasoby, `main` i ścieżka Stable pozostają bez zmian.

Kod etapu 3 (`e2687d6`) przeszedł CI 5/5 w uruchomieniu `39234794754`.
Commit wydania z nowym numerem wersji przechodzi ponownie pełne CI i osobny
build wydania. Wyniki LVGL i porównania znajdują się w
[cyber-blue/stage3/README.md](cyber-blue/stage3/README.md).
Odbiór fizyczny na ESP32-S3 pozostaje do wykonania przez właściciela.

### Wydanie 0.7.59-dev — nowy ekran Sterowanie BLU w Cyber Blue

Właściciel jawnie zlecił publikację Dev 2026-10-19 (Europe/Vienna), po
zakończeniu przebudowy BLU i potwierdzeniu CI 5/5 dla `719edb0`
(uruchomienie `39293997480`). `.github/dev-release-0.7.59.json`, `release.yml`
i ówczesny `DEV_REQUEST` dopuszczały wyłącznie `dev-v0.7.59`, `0.7.59-dev` oraz gałąź
`work/shelly-hud-cyber-blue`. Pełne CI jest powtarzane dla dokładnego commitu
wydania przed utworzeniem tagu. Podpis pozostaje w istniejącym chronionym
środowisku `release`; klucz nie jest pobierany do workspace. Poprzednie
tagi i zasoby są niezmienne, `main` i Stable pozostają bez zmian.

Zakres i wyniki renderera LVGL: [cyber-blue/blu-control/README.md](cyber-blue/blu-control/README.md).
Rzeczywiste odczyty radiowe i odbiór na fizycznym ESP32-S3 pozostają do
wykonania przez właściciela; testy hosta i CI ich nie zastępują.

### Wydanie 0.7.61-dev — integracja referencji Cyber Blue

Właściciel 2026-10-20 dostarczył `Text.txt`, `Text2.txt`, `Text1(1).txt`,
pełny plik edytora „Ogólne” i sześć renderów. Zlecił stworzenie nowego Dev
i porównanie renderów. Same opisy zawierały wyniki dawnych sesji, bez ich
większości źródeł; obecna integracja ma własne testy, a nie przejęte liczniki.
Włączono załączony edytor oraz odtworzono przekaźnik, TRV i ustawienia
na istniejącym modelu/aktorze. Powstał komponent rzeczywistego pancerza rolety.
`DEV_REQUEST`, `.github/dev-release-0.7.61.json` i workflow wskazują tylko
`dev-v0.7.61`, `0.7.61-dev`, `work/shelly-hud-cyber-blue`.
Publikacja czeka na 5/5 CI tego samego commitu. Klucz pozostaje w chronionym
środowisku `release`. Nie scala się do `main` ani nie publikuje Stable.

Test: `python3 tools/test_ui.py --case test_cyber_recovered_ui` i pełne UI;
`python3 tools/render_recovered.py` eksportuje niezmienione piksele LVGL.
Większy przycisk powrotu z 0.7.60 pozostaje, a dwie zakładki zastępują starszą
referencję przekaźnika z trzema. Wybrane testowe identyfikatory w eksporcie
są przykładowe. Raport i klatki: `docs/cyber-blue/recovered/`.

### Wydanie 0.7.60-dev — dotyk i pobieranie OTA

Właściciel 2026-10-19 jawnie zlecił poprawkę dotyku, sprawdzenie błędu OTA
ze zdjęcia i publikację Dev. `.github/dev-release-0.7.60.json`, `release.yml`
i `DEV_REQUEST` dopuszczają wyłącznie `dev-v0.7.60`, `0.7.60-dev` oraz gałąź
`work/shelly-hud-cyber-blue`. Tag powstaje po CI 5/5 dokładnego commitu.
Podpis korzysta z dotychczasowego chronionego środowiska `release`.

Lokalne sprawdzenie: `python3 tools/test_ui.py --ubsan-only --case test_touch_ui`
oraz testy `tools/test.py`, `tools/test_ota_download.py` i `tools/test_ota_install.py`.
CI uruchamia pełny zestaw z ASan/UBSan, prawdziwy renderer LVGL, panel WWW
oraz buildy ESP32/ESP32-S3/ES3C35P. Surowy błąd ze zdjęcia nie zastępuje
logu fizycznego urządzenia. Do krótkiego odbioru: pojedyncze i mocniejsze
naciśnięcia, długie przytrzymanie, przewijanie kart w obu kierunkach,
przeciąganie rolety/termostatu i aktualizacja przez Dev przy stabilnym USB.
