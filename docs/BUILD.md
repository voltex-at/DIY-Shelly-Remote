# Build and installation

## Portable tests

Requires Python 3 and a C11 compiler (`cc` or `CC` environment variable).

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

`esp32s3` is the second CI compile target. Neither is a confirmed physical board. The diagnostic build uses ESP-IDF defaults where a chip build requires them; those defaults are not accepted production hardware parameters. CMake refuses a normal build until a real board port is supplied.

The GitHub Actions workflow runs portable tests and both diagnostic builds. It retains ELF/map files for debugging. It does not publish installable BIN assets or create Releases. The workflow has not run at the initial checkpoint.

## Installation

There is currently **no firmware to install on the user's board**. Do not flash diagnostic outputs to an unidentified board. After bring-up, the board-specific guide must give the exact target, Flash layout, USB driver, port and install command. Use the generated `flasher_args.json` or `idf.py flash` with that board profile instead of guessing offsets.

First installation will use USB. OTA requires a later verified dual-slot board profile and installer. USB recovery must remain documented. Do not erase all Flash as a routine troubleshooting step: it removes settings.
