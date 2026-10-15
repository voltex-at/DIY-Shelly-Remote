# DIY Shelly Remote

> **0.4.0-config-dev — review branch, not a stable release.** `work/config-web-panel` contains the shared configuration, touch editor, local HTTPS panel, real LVGL preview and local development OTA installer. [Installation, pairing and limits](docs/CONFIG_WEB_PL.md) · [API](docs/CONFIG_API.md) · [actual 320×480 frames](docs/ui-review/README.md). Draft [PR #5](https://github.com/voltex-at/DIY-Shelly-Remote/pull/5) is awaiting review; `main` and `reference/0.2.0-local-dev` remain unchanged.

The configuration supports dashboards, rooms, devices, scenes, ordering, card sizes/fields and appearance in both editors. Revisions and two checksummed Flash checkpoints protect concurrent changes and interrupted saves. Room photos are resized in the browser and stored locally. Administrative APIs require physical pairing and authenticated HTTPS. No cloud services or CDN resources are used.

Live communication still controls **one verified Shelly switch/channel** through the preserved transport. Configured cover/sensor entries have no new transport implementation, and scenes have no executor yet. Populated review images are explicitly DEMO. Native/browser tests and target compilation pass; TLS, persistence and OTA on the physical board still require testing. The baseline description below records the preceding 0.2.0 milestone.

An unofficial open-source ESP32 touchscreen remote controller for Shelly smart home devices, with local Wi-Fi and Bluetooth connectivity.

**Preserved baseline: `0.2.0-local-dev`, on-device Wi-Fi and one local Shelly switch.** The preceding bring-up boots and responds to touch on the user’s ES3C35P; later network/control behavior awaits its physical test.

Project repository: [voltex-at/DIY-Shelly-Remote](https://github.com/voltex-at/DIY-Shelly-Remote). This independent firmware does not modify the separate sensor remote projects. It is not affiliated with or endorsed by Shelly.

## Baseline implementation (0.2.0)

- ESP-IDF v5.5.4 project with a documented LCDWiki ES3C35P N16R8 board profile: ST77922 QSPI display, 320x480 portrait, 16 MB Flash and 8 MB OPI PSRAM.
- LVGL 9.3.0 Home with confirmed relay state, explicit ON/OFF and optional actual Gen2+ power. On-screen Wi-Fi and Shelly forms with keyboard and scrolling. Hardware corner/color/brightness/off/wake tools remain available in Menu.
- Bounded asynchronous ST77922 touch polling, single-owner LVGL task, DMA display flushing with four-pixel alignment, and actual memory checks at boot.
- Manual IPv4 addition of one switch/channel, NVS persistence, Gen1 Basic / Gen2+ Digest via ESP-IDF, identity checks before requests, readback, offline handling and bounded background worker. No simulated devices. Prototype NVS is unencrypted; local Gen1 Basic HTTP has no transport confidentiality.
- Two 6 MiB OTA application slots planned into the confirmed 16 MiB Flash layout. There is **no OTA installer yet**.
- Native tests and GitHub Actions for diagnostic/board compilation and hardware-test artifacts. No stable release is published automatically.

Foundation validation: 94 portable assertions passed, including ASan/UBSan in GitHub Actions; ESP32 and ESP32-S3 diagnostic compilation passed in [run 38657250244](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38657250244). Board/UI changes have their own build evidence in [PROJECT_STATUS.md](PROJECT_STATUS.md). A successful compilation is not a physical board test.

## First installation and test

Read [the ES3C35P guide](boards/lcdwiki-es3c35p/README.md) before using a hardware-test artifact. Use only the ES3C35P variant; generic diagnostic artifacts are not firmware for this board. For this version follow [the Polish local-control installation/test guide](docs/LOCAL_CONTROL_PL.md). It covers Wi-Fi setup, one saved authenticated switch, ON/OFF, state refresh, restart and offline tests. Multi-device UI, rooms/scenes, energy history, BLE and OTA installation remain future work.

No phone, Home Assistant or cloud will be required for the final local-control functionality. All network/device states in production will come from real devices; no simulated devices are included in this bring-up build.

## Development

```sh
sudo apt-get install libcjson-dev
python3 tools/test.py
python3 tools/test.py --sanitize
```

See [BUILD.md](docs/BUILD.md), [ARCHITECTURE.md](docs/ARCHITECTURE.md), [API.md](docs/API.md), [UI.md](docs/UI.md), [OTA.md](docs/OTA.md), [COMPATIBILITY.md](docs/COMPATIBILITY.md) and [ROADMAP.md](docs/ROADMAP.md).

Original project code is [MIT](LICENSE). The panel initialization data derives from a separately attributed MIT project; driver/framework dependencies retain their own licenses. See [NOTICE.md](NOTICE.md). No official Shelly application code, graphics or logos are included.

Current development firmware: [verified CI/artifact](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38662049616/artifacts/11939312624), 94 + 371 checks with ASan/UBSan and all three ESP-IDF builds passed. See [validation record](docs/VALIDATION.md) for exact source/tested commits and hashes.
