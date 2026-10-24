# SYS-02 — Backup, przywracanie i reset Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Umożliwić pełny backup szyfrowany hasłem oraz bezpieczne odtworzenie na tym samym pilocie po resecie, z fizycznymi zgodami LCD.

**Architecture:** Wersjonowany snapshot logiczny łączy istniejące magazyny przez
typowane adaptery. Wspólna warstwa utrzymania danych zapewnia admission, zgody,
szyfrowany staging i boot recovery. Przeglądarka szyfruje plik; LCD potwierdza
operacje. Reset korzysta z tego samego dziennika recovery i jawnej listy danych.

**Tech Stack:** C11, ESP-IDF 5.5.4, LVGL 9.3.0, NVS/SPIFFS, pinned NimBLE,
mbedTLS dostępny w SDK, Web Crypto PBKDF2/AES-GCM, istniejący panel HTTPS.

**Spec:** [Zatwierdzony projekt SYS-02](../specs/2026-10-24-system-data-design.md).
Niezależna część: [Aktywność LCD](2026-10-24-system-activity.md).

## Global Constraints

- ESP-IDF 5.5.4, LVGL 9.3.0 (`c033a98afddd65aaafeebea625382a94020fe4a7`), ST77922 2.1.1, mDNS 1.8.2; ES3C35P N16R8 320×480.
- Cyber Blue/Grafit/Jasny i PL/EN/DE; podwójne sfazowane turkusowe ramki, złote ważne teksty, istniejące kontrolki.
- Bez zmiany partycji, pożyczania OTA slotu, kluczy/pinów, worker RPC, schematów frozen 1/2 i limitu publicznego JSON 24576 B.
- Snapshot <=2097152 B, metadane <=65536 B, <=16 zdjęć po 115200 B; bufor I/O <=4096 B, PSRAM na żądanie.
- BSS, stos UI 8192 B i pula LVGL 131072 B bez podnoszenia budżetów; żadnego dużego obiektu automatycznego na stosie LVGL.
- Tylko ten sam fizyczny pilot, także po pełnym resecie; brak dawnych sesji WWW/TLS identity w kopii, nowe parowanie panelu po imporcie.
- PBKDF2-HMAC-SHA256 600000, AES-256-GCM, sól16/nonce12/tag16 B, nagłówek64 B jako AAD; hasło12 znaków..256 B UTF-8 bez normalizacji/przycinania.
- Jednorazowe zgody fizyczne120 s, oddzielne export/upload/commit/reset; brak GET mutującego dane.
- Reset przytrzymaniem3 s na LCD; commit importu/reset tylko przy świeżym >=20%, LOW odmawia, UNKNOWN wyłącznie po potwierdzeniu stabilnego USB.
- Nie wykonywać rzeczywistego resetu, flashowania ani komend do Shelly podczas prac bez osobnej zgody. Bez push, publikacji i merge main.
- Implementacja tutaj kolejno, jeden niezależny końcowy przegląd SYS-02. Ten plan wymaga odbioru przed kodem.

## Review Focus

1. STOP podczas zbierania snapshotu ma pierwszeństwo, a niekompletny snapshot nie może zostać wyeksportowany (Task 3: `stop_preempts_capture`).
2. Reset urywa zasilanie i usuwa zapis języka/stylu: recovery nadal ma czytelny ekran i nie startuje normalnych usług (Task 6: `cut_during_reset`).
3. Ta sama karta ID po podmianie fizycznego urządzenia nie otrzymuje z archiwum zgody automatyzacji (Task 2/6: `enrollment_identity_changed`).
4. Dwie karty przeglądarki lub spóźniony callback nie podmieniają kandydata i nie zużywają cudzej zgody (Task 7/8: `two_sessions_and_stale_callback`).
5. Zerwany download, błędne hasło i brak miejsca nie zmieniają istniejących danych ani nie pozostawiają klucza staging (Task 4/7/8: `cancel_cleanup`, `wrong_password`, `no_space`).

## Pliki, właściciele i wspólne typy

Nowe prywatne moduły w `components/dsr_platform/`:

- `dsr_backup_format.{c,h}` — ograniczony format snapshotu, walidacja i indeks rekordów.
- `dsr_backup_sources.{c,h}` — koordynacja adapterów ustawień/konfiguracji/zdjęć/połączeń/reguł.
- `dsr_backup_bonds.{c,h}` — typowany adapter wszystkich aktywnych rodzin NimBLE store 5.5.4.
- `dsr_maintenance_model.{c,h}` — granty, generacje, fazy i codec dwóch rekordów journal.
- `dsr_maintenance_store.{c,h}` — NVS `dsr_maint`, zaszyfrowane pliki staging i recovery I/O.
- `dsr_maintenance.c`, `include/dsr_maintenance.h` i `include/dsr_maintenance_types.h` — publiczny status bez sekretów oraz asynchroniczna koordynacja.
- `dsr_panel_maintenance.inc` — mały handler w kontekście istniejącego `dsr_panel.c`, reużywa jego uwierzytelnienia i same-origin.
- `web/backup.js` — Web Crypto i osobny cykl życia panelu backupu.

Moduły źródłowe zachowują swoje magazyny: `dsr_settings.c`, `dsr_connection.c`,
`dsr_battery_profile.c`, `dsr_style_hint.c`, `dsr_photos.c`, `dsr_automation.c`,
`dsr_automation_store.c`, `dsr_ble.c`, `dsr_blu.c`, `dsr_ble_rpc.c`, `dsr_workspace.c`,
`dsr_config/dsr_config_store.c`. Jawne adaptery nie wywołują HTTP/LVGL.
`dsr_panel.c`, `dsr_tls.c`, `dsr_storage.c` uczestniczą w reset/invalidation.

UI: nowy `components/dsr_ui/dsr_ui_maintenance.c`, integracja w `dsr_ui.c`,
`dsr_ui_network.c`, `dsr_ui_internal.h`, `include/dsr_ui.h`, `dsr_ui_style.c`.
Boot: `main/app_main.c`; rejestracja źródeł i stack-usage w obu CMakeLists.
Testy: `tests/test_system_data_{format,sources,bonds,admission,stage,recovery,api}.c`,
`tests/system_data/` (ograniczone fake storage/scheduler), `tests/test_panel_backup.cjs`,
`tests/ui/test_system_data_ui.c`, `tools/test_system_data.py`.

Typy prywatne: opaque `dsr_backup_image_t` jest właścicielem bufora PSRAM i indeksu
zwalidowanych rekordów; opaque `dsr_maint_stage_t` jest uchwytem jednej operacji.
Publiczne `dsr_maint_op_t`: EXPORT, IMPORT_UPLOAD, IMPORT_COMMIT, RESET.
`dsr_maint_status_t`: op/phase/result, progress0..100, liczby składników, revision,
opaque request/candidate IDs, dostępność zgody i potrzeba USB; bez poświadczeń/kluczy/MAC.
`dsr_maint_generation_t` to monotonically advancing 64-bit generation całego zbioru
magazynów, nie sama publiczna revision. Overflow oznacza blokadę do restartu.
Prywatny `dsr_maint_lease_t` zawiera osobny niepowtarzalny reservation ID i uchwyconą
generation; po preemption stare release nie może zwolnić nowszej rezerwacji.
`dsr_maint_request_t`: op, nonce16 B, session binding32 B i candidate SHA-25632 B;
bez hasła backupu. Te nazwy/typy są wspólne dla niżej wymienionych tasków.

Wszystkie nowe testy C muszą kompilować właściwe pliki produkcyjne, kończyć się
znacznikiem PASS i niezerowym exit na błędzie; fake dotyczy sprzętu/IO, nie algorytmu.
Runner ma `--case format|sources|bonds|admission|stage|recovery|api|all`.
Windows: polecenia python3 uruchamiać przez istniejący `tools/dev/run_host.py` w
projektowym Cygwin; nie commitować lokalnego wrappera, .local ani dowodów renderu.

### Task 1: Kanoniczny snapshot i walidacja bez zapisów

**Files:** format module, shared `include/dsr_maintenance_types.h` oraz deklaracje prywatnego lease w `dsr_maintenance_model.h`, `tests/test_system_data_format.c`, runner; rejestracja odpowiednich źródeł CMake dopiero wraz z działającym modułem.

**Interfaces:** produces `dsr_result_t dsr_backup_image_open(uint8_t *owned,size_t size,const uint8_t device_id[32],dsr_backup_image_t **out);`, `dsr_result_t dsr_backup_manifest_validate(const cJSON *manifest,const uint8_t device_id[32]);`, `void dsr_backup_image_free(dsr_backup_image_t *image);`, `const uint8_t *dsr_backup_image_bytes(const dsr_backup_image_t *image,size_t *size);`. Open przejmuje owned również przy błędzie i zeruje/zwalnia; out=NULL przy błędzie. Free zeruje prywatne bufory. Widok bytes żyje do free. Manifest validator sprawdza wszystkie pola i referencje metadanych; image_open dodatkowo sprawdza fizyczną obecność/hash rekordów zdjęć. Recovery reużywa walidatora manifestu i sprawdza zdjęcia w staging/final osobno.

- [x] **RED:** `format_bounds`, `duplicate_keys_records`, `future_schema`, `foreign_device`, `photo_hash_and_refs`, `old_schema_upgrade` ze zdaniami `CHECK(max_size==2097152); CHECK(photo_bytes==115200); CHECK(writes==0); CHECK(out==NULL);` dla złego wejścia. Pełne16 zdjęć musi przejść, 17/metadata65537/trailing bytes/overflow nie.
- [x] **RED run:** `python3 tools/dev/run_host.py test_system_data.py --case format`; brak produkcyjnego walidatora ma dać FAIL.
- [x] **Implement:** Snapshot v1 ma magic8 `DSRSNP1\0`, u16version1/u16header24/u32manifest_length/u16photo_count/u16reserved0/u32total_size, potem manifest JSON bez NUL i rekordy zdjęć: raw content-id16 B, pełny SHA25632 B, u32length, u32reserved0, pixels. Wszystkie liczby LE, kolejność zdjęć identyczna z manifestem. Manifest whitelists: format/hardware/device/firmware/created/config/settings/connections/battery/style/bonds/enrollment/photos; dokładne pola wewnętrzne odzwierciedlają typowane źródła task2. Ścisłe typy, UTF-8, duplikaty i głębokość; konfigurację przekazać do istniejących frozen validators/upgrade w RAM. Metadane <=65536, cały snapshot <=2097152. Bez IO, komend, mutacji config i nieograniczonych alokacji.
- [x] **GREEN:** ten sam `--case format`, PASS; zapisać kanoniczny syntetyczny fixture bez prawdziwych sekretów do testów JS, a nie kopię urządzenia. Lokalny commit `feat: validate bounded logical system snapshots`.

### Task 2: Pełny snapshot źródeł, bonds i zgód reguł

**Files:** sources/bonds modules, wskazane wyżej magazyny i ich istniejące nagłówki, automation store/model, testy sources/bonds i runner.

**Interfaces:** consumes task1. Produces `dsr_result_t dsr_backup_sources_capture(const dsr_maint_lease_t *lease,dsr_backup_image_t **out);`, `dsr_result_t dsr_backup_sources_apply(const cJSON *validated_manifest);`, `dsr_result_t dsr_backup_bonds_capture(cJSON *manifest);`, `dsr_result_t dsr_backup_bonds_apply(const cJSON *manifest);`, `bool dsr_backup_sources_quiet(void);`. Capture wymaga `dsr_maintenance_lease_valid` z task3 (w tym izolowanym teście stub właściciela, nie walidatora/sources); apply tylko boot recovery przed żywymi czytelnikami i obejmuje prywatne źródła/enrollment, nie zdjęcia i finalne checkpointy publicznej konfiguracji (task6). Publiczny UI nie otrzymuje tych nagłówków. Własne małe adaptery modułów zachowują walidację i jawne pola, nie eksportują raw struct.

- [x] **RED:** round-trip każdego magazynu z syntetycznymi różnymi hasłami/kluczami, całe16 zdjęć/bonds, błędne połączenie/kanał/referencja, nieobsługiwana rodzina store, aktywny cykl/uncertain, `enrollment_identity_changed`. `CHECK(restored_connections==original_connections); CHECK(tls_exported==0 && panel_token_exported==0 && telemetry_exported==0); CHECK(device_commands==0);`.
- [x] **RED run:** `... test_system_data.py --case sources`, następnie `--case bonds`; FAIL przed adapterami.
- [x] **Implement sources:** konfiguracja i referencjonowane zdjęcia, `dsr_cfg`, `dsr_bindings`, `dsr_battery`, `dsr_ui` oraz enrollment. Nigdy fallback legacy jako drugi konkurencyjny rejestr. Zdjęcia pobierać z właściciela/cache lub mount bez side effect; nie dodawać eksportu dowolnej ścieżki. Enrollment eksportuje wyłącznie zatwierdzone tożsamości zgodne z definicjami i bindings. Apply tworzy nowy poprawny dziennik reguł z pustymi cycles/targets; nie wskrzesza baseline/override/manual intent. Wyłączony/aktywny/nieukończony enrollment według zatwierdzonej specyfikacji.
- [x] **Implement bonds:** audyt lokalnego `ble_store_nvs.c`/`ble_store_config.h` SDK5.5.4, jawny canonical codec pól każdej włączonej rodziny, max16 bonds i local identity/IRK. Eksport marshaled na właściciela NimBLE, z potwierdzoną barierą zapisu store; import boot-only przed uruchomieniem hosta/radia. Nie inicjalizować radia, by móc wykonać reset. Unknown enabled record/version odmawia pełnej kopii, bez pomijania. Adapter odtwarzania NVS respektuje faktyczny layout pinned store, nie dump ABI.
- [x] **GREEN:** ponowić tylko sources/bonds; przy zgodnych tożsamościach enabled rule przechodzi po fresh-contact/direct-TRV przez istniejący configure bez DSR_CONFLICT; zmieniona identity nie dostaje komendy. Lokalny commit `feat: capture and restore typed user data and BLE bonds`.

### Task 3: Wyłączność, generacje i bezpieczna koordynacja właścicieli

**Files:** maintenance public headers/coordinator/model; modify `dsr_activity.c`, `dsr_activity_internal.h`, workspace/config queue, sole worker oraz właściciele mutacji z task2; tests admission, istniejący restart/STOP.

**Interfaces:** `bool dsr_maintenance_acquire(dsr_maint_lease_t *lease);`, `void dsr_maintenance_release(const dsr_maint_lease_t *lease);`, `bool dsr_maintenance_lease_valid(const dsr_maint_lease_t *lease);`, `dsr_maint_generation_t dsr_maintenance_generation(void);`, `bool dsr_maintenance_generation_matches(dsr_maint_generation_t generation);`, `void dsr_maintenance_mutated(void);`, `void dsr_maintenance_snapshot(dsr_maint_status_t *out);`. Acquire/release to wewnętrzna para właściciela, nie permission z WWW. Release porównuje reservation ID; capture sprawdza ważność lease przed publikacją. Zwiększyć generation po każdej udanej zmianie któregokolwiek magazynu, łącznie z NimBLE i parowaniem; nie logować wartości tajnych.

- [x] **RED:** `count_not_mutex`, `busy_no_latch`, `config_queue_inflight`, `bond_write_race`, `stop_preempts_capture`, `restart_ota_conflict`, `generation_changed`, `all_failure_paths_release`. `CHECK(acquire_with_activity==false); CHECK(ordinary_restart_behavior_unchanged); CHECK(stop_admitted); CHECK(snapshot_sent==false);` gdy STOP przerwał capture. Sprawdzić brak podwójnego end i blokady UI.
- [x] **RED run:** `... test_system_data.py --case admission`; FAIL na obecnym liczniku, który nie daje wyłączności.
- [x] **Implement:** oddzielna rezerwacja maintenance w atomowym admission, do uzyskania tylko gdy operacje są rozstrzygnięte i count=0; busy odmawia, zamiast zamykać bramę przed follow-up aktualnego zadania. Potwierdzone bariery wszystkich właścicieli, w tym NimBLE store. Nie trzymać mutexów podczas IO/ADC/HTTP. Snapshot ma krótki token capture; pilny STOP unieważnia kandydat i ma pierwszeństwo, a wszyscy właściciele kończą bez publikacji starego snapshotu. Nie zwalniać właścicieli przed zabezpieczeniem ich kopiowanych buforów. Zbieranie i staged upload nie zajmują workera przez czas pobierania/KDF. Config task wykonuje maintenance etapami; nowe taski sieciowe/RPC nie powstają. Odrębny tryb boot recovery nie startuje normalnych aktorów.
- [x] **GREEN:** ten sam admission i istniejący `python3 tools/dev/run_host.py test_restart.py` raz po zmianie protokołu; tylko dotknięta regresja STOP z właściwego cover/worker case. PASS; commit `feat: serialize system maintenance with existing writers`.

### Task 4: Trwały zaszyfrowany staging i journal

**Files:** maintenance model/store, `dsr_storage.c`, test stage, fake NVS/SPIFFS i źródła task1/2.

**Interfaces:** `dsr_result_t dsr_maint_stage_prepare(const dsr_backup_image_t *image,dsr_maint_stage_t **out);`, `dsr_result_t dsr_maint_stage_commit(dsr_maint_stage_t *stage,const dsr_maint_lease_t *lease);`, `dsr_result_t dsr_maint_stage_cancel(dsr_maint_stage_t *stage);`, `void dsr_maint_stage_free(dsr_maint_stage_t *stage);`. Prepare zapisuje zaufaną lokalną `dsr_maintenance_generation()` na wejściu; nie bierze bieżącej generation z importowanego manifestu. Commit sprawdza aktualny lease i dataset generation stage pod tą samą wyłącznością. Free uchwytu nie usuwa danych COMMITTED. Stage ma operation ID, digest, generation i liczby; klucz tylko prywatnie. Fazy journal: PREPARING, COMMITTED, APPLYING, COMPLETE, FAULT; op IMPORT/RESET i applied bitmap16.

- [x] **RED:** `empty_full_photo_budget`, `no_space`, `nvs_full`, `psram_fragmented`, `short_write`, `tag_mismatch`, `cut_before_commit`, `cancel_cleanup`. `CHECK(active_store_writes==0); CHECK(ota_partition_writes==0); CHECK(peak_spiffs<=total*3/4);` przed commit; po cancel brak owned stage/key. Zły/jednostronny journal nigdy nie otwiera normalnego bootu.
- [x] **RED run:** `... test_system_data.py --case stage`; FAIL przed journal/staging.
- [x] **Implement:** dwa wersjonowane rekordy `dsr_maint` z CRC/seq, jawny boot-latch. PREPARING utrwalić przed pierwszym plikiem. Niezależny random key32 i nonce prefix8+record-index4, AES-256-GCM tag16, AAD op/device/type/index/len/manifest digest. Nowy key/op dla każdej operacji; bez reuse nonce przy retry. Osobne owned pliki metadanych i zdjęć z deterministycznymi nazwami wewnętrznymi. Weryfikować wszystkie tagi/hash i readback przed COMMITTED. Preflight old+stage+onephoto+FS overhead+65536 reserve poniżej75%, a także wymagane NVS entries/PSRAM. IO chunks<=4096. Nie formatować ani kasować zwykłych zdjęć, gdy brakuje miejsca. Restart PREPARING sprząta tylko owned staging, nie dane.
- [x] **GREEN:** ten sam stage; dowód, że pełne16 zdjęć na pustym pilocie mieści się bez OTA slotu, a pełny stary+nowy zestaw odmawia bez zmian. Commit `feat: persist authenticated restore staging and intent`.

### Task 5: Granty i reset jako operacje właściciela

**Files:** maintenance model/coordinator, power integration, panel pairing invalidation; test admission/api modele, worker/config task; UI i HTTP dołączą dopiero task7/8.

**Interfaces:** `dsr_result_t dsr_maintenance_request(const dsr_maint_request_t *request);`, `dsr_result_t dsr_maintenance_confirm(const uint8_t request_id[16],bool stable_usb);`, `bool dsr_maintenance_cancel(const uint8_t request_id[16]);`. Confirm jest wywołaniem wyłącznie fizycznego LVGL ownera; nie istnieje publiczny HTTP odpowiednik. request/confirm są nonblocking, wynik przez snapshot; powiązanie z sesją ustawia zaufany handler, nie pole JSON.

- [x] **RED:** typowanie zgód EXPORT/UPLOAD/COMMIT/RESET, expiry120000 ms/wrap, replay, obcy request/session, nowy digest/generation, wyjście, timeout power oraz LOW19/READY20/UNKNOWN+USB/BUSY. `CHECK(commit_without_lcd==false); CHECK(low_battery_writes==0); CHECK(busy_is_not_unknown); CHECK(no_flash_before_confirm);`.
- [x] **RED run:** `... test_system_data.py --case admission` z nowym zestawem grantów; FAIL przed handlerem koordynatora.
- [x] **Implement:** świeży odczyt przez sole worker jak istniejący `dsr_ota_power_read/acquire`, nie cache LCD. Zwolnić worker lease zanim maintenance próbuje count=0; po uzyskaniu barier sprawdzić nadal świeży wynik i generation. LOW nigdy nie ma obejścia USB, UNKNOWN wymaga potwierdzenia. Reset przygotowuje i readbackuje własny RESET COMMITTED, zanim restart zostanie zlecony. Zmiana sesji lub magazynu unieważnia precommit grant. Zgoda na import jawnie obejmuje enrollment tylko zapisanych tożsamości.
- [x] **GREEN:** tylko grant/power/admission cases, regresja istniejącego OTA power bez zmiany jego polityki. Commit `feat: require physical one-shot consent for data maintenance`.

### Task 6: Boot recovery importu i pełnego resetu

**Files:** store/coordinator, `main/app_main.c`, source adapters, storage/config/auto store; create recovery tests, extend `tests/test_startup.c` i `tests/startup/` minimalnym stubem.

**Interfaces:** `typedef void (*dsr_maint_progress_fn)(const dsr_maint_status_t *status);`; `dsr_result_t dsr_maintenance_recover(dsr_maint_progress_fn progress);` returns DSR_OK tylko gdy wolno wejść w normalny startup. Inny wynik zatrzymuje ordinary boot. Callback publikuje status do istniejącego LVGL ownera, sam nie rysuje z taska storage. Retry/reset recovery korzystają z tej samej fizycznej zgody, nie trasy HTTP.

- [x] **RED:** `cut_after_every_persisted_step`, `cut_during_reset`, `both_checkpoints_newer`, `old_fallback_resurrection`, `missing_stage_or_final`, `complete_cleanup_cut`, `enrollment_identity_changed`. `CHECK(normal_wifi_start==0 && normal_workspace_start==0 && normal_ble_start==0);` przy pending/fault; `CHECK(reset_erased_ota==0 && reset_erased_phy==0);` zawsze. Po dowolnym naprawialnym cut wynik identyczny z jednym pełnym wykonaniem, bez komend Shelly.
- [x] **RED run:** `... test_system_data.py --case recovery`; FAIL przed boot hook.
- [x] **Implement import:** hook po NVS/board i splash, przed settings/worker/Wi-Fi/workspace/GC/TLS/BLE. Zamontować bez format-on-error. Photo per-record: decrypt → final write/flush/hash/readback → journal bitmap → unlink owned stage. Powtórka weryfikuje final i stage; przy brakach fail closed. Apply ustawień/bonds/enrollment i obu checkpointów z revision większą od obu starych; overflow wykryć przed commit. Skasować fallback `dsr_device` i stary panel token, zachować bieżący TLS identity. Sprzątanie klucza/journal: oba key-bearing sloty zastąpić readbackowanymi keyless COMPLETE receipts, usunąć owned stage, a boot-latch wyczyścić dopiero po pełnej weryfikacji. COMPLETE z latch wznawia tylko cleanup, nigdy import lub zwykły boot z mieszaniną.
- [x] **Implement reset:** jawna lista namespaces ze spec (`dsr_cfg`, `dsr_bindings`, `dsr_device`, `dsr_battery`, `dsr_ui`, `dsr_auto`, `dsr_panel`, `dsr_tls`, `nimble_bond`), tylko format `dsrdata`. `dsr_maint` przetrwa do readback/COMPLETE. Bez `nvs_flash_erase`, OTA/PHY/eFuse lub RPC. Uszkodzony journal/stage zostawia recovery, bez automatycznego resetu; ręczne Reset z recovery musi najpierw utrwalić nową potwierdzoną intencję.
- [x] **GREEN:** ten sam recovery i właściwy startup fixture; PASS. Zachować failpoints w host fixtures, nie włączone w target produkcyjny. Commit `feat: resume data restore and factory reset before normal boot`.

### Task 7: API HTTPS i szyfrowany plik w panelu

**Files:** panel include, `dsr_panel.c`, `web/backup.js`, `web/panel.js`, `web/panel.css`, platform CMake/assets, API C tests, JS tests; `tools/serve_panel_test.py` wyłącznie model bez urządzenia.

**Interfaces:** authenticated routes `POST /api/v1/backup/request` (EXPORT lub IMPORT_UPLOAD), `POST /api/v1/backup/export` (one-shot binary snapshot), `POST /api/v1/backup/import` (binary upload), `GET /api/v1/maintenance/status` (redacted), `POST /api/v1/maintenance/cancel`, `POST /api/v1/reset/request` (tylko prośba LCD). Commit dopiero LCD task8. `backup.js` exports `createBackupPanel({request,translate,onNotice})` returning `{root,dispose}`; panel shell ustawia request/session, dispose abortuje callbacks i czyści pola/bufory.

- [x] **RED API:** brak cookie/origin/header, GET export/import/reset, duże/krótkie/chunked body, grant misuse/replay, `two_sessions_and_stale_callback`; `CHECK(secret_bytes_sent==0); CHECK(flash_changes==0);` bez grantów. Transfer bounded2 MiB/chunks4096 i osobny timeout; no-store również na błędach.
- [x] **RED JS:** Web Crypto round-trip ze wspólnym syntetycznym fixture, Unicode/no trim, nagłówek AAD, `wrong_password`, zły tag/len/iterations/reserved, maxsize, dwa okna, zerwany export, abort, secure context absent; `assert.equal(passwordRequests,0); assert.equal(plaintextDownloads,0); assert.equal(importRequestsAfterBadTag,0);`.
- [x] **RED runs:** `... test_system_data.py --case api`; `node tests/test_panel_backup.cjs`; FAIL przed podłączeniem.
- [x] **Implement:** reużyć dotychczasowe local_request/authorized, rate limits i header `X-DSR-Request`, nie tworzyć bypass ani globalnego backup tokenu. Request/transfer nie trzyma locka HTTP podczas storage. Download jednorazowy po LCD zgodzie, export data tylko w body POST, nie URL. `backup.js` używa dokładnie header64 i krypto ze spec, sprawdza tanie bounds przed KDF, decrypt przed prośbą upload. Tylko zaszyfrowany `.dsrbak` jest pobierany; hasła ani jawnego snapshotu nie cache/log/store. Publiczny status nie ujawnia device hash lub składników prywatnych. Nowy asset obsługiwany przez embedded route/CSP self i fixture server. Panel nie gubi niezapisanej konfiguracji przy odmowie/anulowaniu/timeout; powodzenie jasno wymaga restartu/ponownego parowania.
- [x] **GREEN:** tylko API i JS; przejść Chromium desktop/mobile na localhost modelu z rzeczywistym Web Crypto, bez komend do urządzenia. Weryfikacja nagłówków HTTPS pozostaje w produkcyjnym handlerze C i odbiorze fizycznym. Commit `feat: export and import password-encrypted backups through paired web panel`.

### Task 8: LCD zgód, importu, resetu i recovery

**Files:** new UI module i istniejące hooki/menu/tłumaczenia, `include/dsr_ui.h`, `dsr_ui_internal.h`, CMake, `tests/ui/test_system_data_ui.c`, fixture status/grants, runner UI.

**Interfaces:** `void dsr_ui_maintenance_open(lv_event_t *event);`, `void dsr_ui_maintenance_hide(void);`, `void dsr_ui_maintenance_tick(void);` oraz `void dsr_ui_maintenance_boot_publish(const dsr_maint_status_t *status);`. Boot publish jest bounded mailbox; render/potwierdzenia należą do istniejącego LVGL ownera ze splash, nie do app_main/storage callbacku. Zwykła nawigacja nie jest startowana w recovery.

- [x] **RED:** nowy case `test_system_data_ui`: export consent, upload consent, candidate summary/enrollment notice, busy/no space/LOW/UNKNOWN, cancel/expiry, reset hold2999 vs3000 ms, recovery/retry/complete, `two_sessions_and_stale_callback`. `CHECK(reset_commits==0);` dla tap/krótkiego hold/leave; `CHECK(reset_commits==1);` po jednym pełnym hold. Brak hasła/SSID/MAC/private key w całym drzewie tekstowym i podglądzie WWW.
- [x] **RED run:** `python3 tools/dev/run_host.py test_ui.py --case test_system_data_ui --jobs 4`; FAIL przed widokiem.
- [x] **Implement:** System → Kopia i reset, znane wspólne kontrolki, czytelne liczby składników i komunikat ponownego parowania. Na LCD jasne „urządzenia Shelly zachowają obecny stan”; reset nie wygląda jak restart. Oddzielne Anuluj/Potwierdź, złote komunikaty i podwójne turkusowe ramki w Cyber Blue; trzy skórki/PLENDE. Staging/commit mają prawdziwy postęp, nie obietnicę sukcesu. Recovery używa dostępnego hint stylu i języka bez odczytu usuwanej konfiguracji; po usunięciu języka fallback EN jak boot splash, ze wszystkimi wariantami tłumaczeń dostępnymi normalnie. W pending nie dopuszczać zwykłych kontrolek ani zdjęć. Dodać maintenance do sensitive-preview guard, aby WWW nie potwierdzało fizycznej zgody przez obraz LCD; nie osłabiać istniejących blokad.
- [x] **GREEN:** tylko ten case, wybrane kadry rzeczywistego LVGL9.3 dla3 skórek/3 języków, pressed/disabled/long strings; oglądnięcie odstępów i ramek, bez całego atlasu. Pokazać ankietę etapami export/import/reset; zapisać akceptację wyglądu osobno od sprzętu. Commit `feat: add physical backup restore and reset consent screens`.

### Task 9: Integracja, jeden przegląd końcowy i odbiór

**Files:** `docs/PROJECT_STATUS_PL.md`, `docs/DEVELOPMENT.md`, test runner integration `tools/test.py`, CI wywołujące nowe cases zgodnie z istniejącym modelem; stack/target auditor source lists tylko jeśli wymagane, bez podnoszenia budżetów.

**Interfaces:** consumes wszystkie kroki i plan Aktywności; produces evidence table implementation / host / target build / preview acceptance / physical verification. Brak produkcyjnych API nowych poza powyższymi.

- [ ] Po połączeniu wykonać raz `... test_system_data.py --case all`, JS backup oraz właściwe dotknięte regresje startup/workspace/restart/bonds/panel/automations. Każdy fail naprawiać i powtarzać tylko jego zestaw oraz zależność, nie całą serię. Nie modyfikować frozen schema1 fixtures ani nie wyciszać sanitizerów.
- [ ] Raz zbudować ES3C35P przez `tools/dev/Build-Firmware.ps1`, uruchomić właściwe audytory targetu i stosu dla nowego call graph; sprawdzić pinned dependencies, PSRAM path, BSS, UI8192/LVGL131072 i app/partition. Nie flashować. Build uruchamiać ponownie dopiero po poprawkach wpływających na target.
- [ ] Jeden świeży niezależny przegląd kodu/bezpieczeństwa całego SYS-02: format, key/nonce lifecycle, logical reset whitelist, admission/STOP, two-phase consent, NimBLE ownership, generacje/CAS, recovery przed GC i obie kopie journal. Naprawić potwierdzone defekty z minimalnym testem.
- [ ] Uzgodnić z właścicielem osobny konkretny odbiór urządzenia: aktualna kopia → potwierdzony reset → reconnect/pair → restore na tym samym pilocie, hasła/BLE/bonds/zdjęcia/kalibracja/reguły, kontrolowane przerwania zasilania i odzyskanie. Bez tej zgody nie wykonywać resetu ani komend. Utrata zdalnej strony BLE bond nie jest błędem odszyfrowania pliku; pokazać uczciwe ponowne parowanie.
- [ ] Zaktualizować status i #74 z dokładnymi dowodami i otwartymi brakami, `python3 -X utf8 tools/check_docs.py`, `git diff --check`; commit wyłącznie kod/testy/docs. Po zakończeniu całej ankiety dopiero pełny przegląd UI/CI dla commita wydania i osobna autoryzacja nowego DEV. Nie oznaczać SYS-02 sprzętowo odebranego na podstawie hosta/builda.

## Samokontrola planu przed przekazaniem

Pokrycie spec: Aktywność w osobnym planie; snapshot/limits1; wszystkie prywatne
źródła, bonds i enrollment2; spójność/STOP3; staging/klucze/miejsce4; granty/zasilanie5;
import/reset/power-loss6; hasło/Web Crypto/HTTPS7; trzy skórki/zgody/recovery8;
walidacja docelowa i granice publikacji9. Nazwy i własność image/stage/grantów
są wspólne dla tasków; pięć klas Review Focus ma testy przy właścicielu.
Nie rozpoczęto implementacji ani nie uruchomiono opisanych przyszłych testów.
