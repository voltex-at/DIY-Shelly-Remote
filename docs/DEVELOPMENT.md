# Development

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
