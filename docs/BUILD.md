# Build and installation

## Portable tests

Requires Python 3, a C11 compiler (`cc` or `CC` environment variable), and cJSON development headers/library (`sudo apt-get install libcjson-dev` on Debian/Ubuntu). The runner executes portable tests plus actual HTTP/settings adapters with mocked transport/NVS and real cJSON.

```sh
python3 tools/test.py
python3 tools/test.py --sanitize
```

`--sanitize` enables AddressSanitizer and UndefinedBehaviorSanitizer. In a restricted runtime without usable process metadata, run `--ubsan-only` and report that limitation; it does not replace ASan validation.

## ESP-IDF diagnostic compilation

Install ESP-IDF **v5.5.4** using the [official instructions](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32/get-started/index.html). These commands compile only the unconfigured diagnostic app:

```sh
. "$IDF_PATH/export.sh"
idf.py -B build -D DSR_DIAGNOSTIC_BUILD=ON set-target esp32
idf.py -B build -D DSR_DIAGNOSTIC_BUILD=ON build
```

`esp32s3` is the second CI compile target. These are generic chip targets; only the separate ES3C35P profile has a physical boot/touch report. The diagnostic build uses ESP-IDF defaults where a chip build requires them; those defaults are not accepted production hardware parameters. CMake refuses a normal build until a real board port is supplied.

The GitHub Actions workflow runs native tests and both diagnostic builds. Diagnostic jobs retain ELF/map files only; the separate board job retains installable development BIN assets. No job creates a stable Release. The foundation workflow passed in run 38657250244. Board-specific changes require their own successful CI run.

## ES3C35P hardware build and installation

The [LCDWiki ES3C35P board guide](../boards/lcdwiki-es3c35p/README.md) now provides the confirmed profile, pin map, partition table, exact build selection, USB first-install procedure and physical test checklist. Select `-DDSR_BOARD=lcdwiki-es3c35p`; never combine it with `DSR_DIAGNOSTIC_BUILD`.

The additional `es3c35p` CI job builds an actual local-switch development application and merged first-install image, computes SHA-256 sums, and retains the resolved dependency lockfile. Its artifact is labeled `es3c35p-local-switch-NOT-STABLE`. A successful job produces a real compiled image, not proof of physical operation. It is not published automatically as a stable Release.

First installation uses USB. Subsequent OTA requires the still-unimplemented verified installer. The app-only BIN and merged first-install BIN have different meanings; follow the board guide and do not guess offsets.

For current USB image naming and the on-screen workflow see [LOCAL_CONTROL_PL.md](LOCAL_CONTROL_PL.md). The board reference preserves the original bring-up names as historical installation context.
