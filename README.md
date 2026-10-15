# DIY Shelly Remote

An unofficial open-source ESP32 touchscreen remote controller for Shelly smart home devices, with local Wi-Fi and Bluetooth connectivity.

**Current milestone: `0.1.0-bringup`, LCDWiki ES3C35P hardware test. Physical validation is pending.**

Project repository: [voltex-at/DIY-Shelly-Remote](https://github.com/voltex-at/DIY-Shelly-Remote). This independent firmware does not modify the separate sensor remote projects. It is not affiliated with or endorsed by Shelly.

## Current implementation

- ESP-IDF v5.5.4 project with a documented LCDWiki ES3C35P N16R8 board profile: ST77922 QSPI display, 320x480 portrait, 16 MB Flash and 8 MB OPI PSRAM.
- LVGL 9.3.0 hardware-test UI: Home, navigation shell for Rooms/Devices/Scenes/Energy, touch corner test, primary color test, brightness, backlight off and first-touch wake. The empty sections are explicitly identified as future work.
- Bounded asynchronous ST77922 touch polling, single-owner LVGL task, DMA display flushing with four-pixel alignment, and actual memory checks at boot.
- Portable C core and ESP-IDF HTTP/Wi-Fi/NVS/worker source for later local Shelly integration. These adapters are not yet connected to the test UI.
- Two 6 MiB OTA application slots planned into the confirmed 16 MiB Flash layout. There is **no OTA installer yet**.
- Native tests and GitHub Actions for diagnostic/board compilation and hardware-test artifacts. No stable release is published automatically.

Foundation validation: 94 portable assertions passed, including ASan/UBSan in GitHub Actions; ESP32 and ESP32-S3 diagnostic compilation passed in [run 38657250244](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38657250244). Board/UI changes have their own build evidence in [PROJECT_STATUS.md](PROJECT_STATUS.md). A successful compilation is not a physical board test.

## First installation and test

Read [the ES3C35P guide](boards/lcdwiki-es3c35p/README.md) before using a hardware-test artifact. Use only the ES3C35P variant; generic diagnostic artifacts are not firmware for this board. The first milestone checks colors, all touch corners, brightness, full backlight off and wake behavior. Wi-Fi credentials, actual Shelly control, rooms/scenes, energy, BLE and OTA will be integrated after this test.

No phone, Home Assistant or cloud will be required for the final local-control functionality. All network/device states in production will come from real devices; no simulated devices are included in this bring-up build.

## Development

```sh
python3 tools/test.py
python3 tools/test.py --sanitize
```

See [BUILD.md](docs/BUILD.md), [ARCHITECTURE.md](docs/ARCHITECTURE.md), [API.md](docs/API.md), [UI.md](docs/UI.md), [OTA.md](docs/OTA.md), [COMPATIBILITY.md](docs/COMPATIBILITY.md) and [ROADMAP.md](docs/ROADMAP.md).

Original project code is [MIT](LICENSE). The panel initialization data derives from a separately attributed MIT project; driver/framework dependencies retain their own licenses. See [NOTICE.md](NOTICE.md). No official Shelly application code, graphics or logos are included.
