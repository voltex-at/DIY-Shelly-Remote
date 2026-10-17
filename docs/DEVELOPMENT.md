# Development

## Layout

| Path | Contents |
| --- | --- |
| `main/` | `app_main.c`: boot order (board → splash → storage → workspace → UI → network) |
| `components/dsr_board` | ES3C35P display/touch/backlight, LVGL owner task (`dsr_ui_task.c`) |
| `components/dsr_platform` | Storage/SPIFFS, NVS registry, worker, HTTP/Shelly, Wi-Fi/AP, BLE, panel (HTTPS + `web/`), history, actions, native OTA actor |
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

`-D DSR_VERSION=x.y.z` overrides `PROJECT_VER` from `CMakeLists.txt`. The header clock uses the POSIX time zone `DSR_TIMEZONE_POSIX` (default `CET-1CEST,M3.5.0,M10.5.0/3`, Europe/Vienna); pass `-DDSR_TIMEZONE_POSIX=\"<TZ string>\"` to change it. After a
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
