# DIY Shelly Remote

An unofficial open-source ESP32 touchscreen remote controller for Shelly smart home devices, with local Wi-Fi and Bluetooth connectivity.

**Status: foundation, version `0.0.1-dev`. No supported board and no installable firmware release yet.**

This is a new, independent project for a custom 3.5-inch touchscreen remote. It does not modify the separate sensor remote firmware. Basic control will work locally without a phone, Home Assistant or Shelly Cloud. This project is not affiliated with or endorsed by Shelly.

## What is present

- Portable C core: separate Gen1 HTTP and Gen2+ HTTP RPC request builders, bounded device registry, numeric stable-version comparison and OTA metadata compatibility policy.
- ESP-IDF transport code: manual IPv4 probing through `/shelly`, relay/switch status, explicit ON/OFF and readback, limited Gen2+ power readings. Authentication-required responses are reported explicitly.
- Platform code: NVS settings, Wi-Fi station connection/reconnection and a bounded asynchronous network worker.
- Safe unconfigured board boundary and a serial-only diagnostic entry point. It does not start the display, touch, network or Bluetooth.
- Native tests and a GitHub Actions workflow for native tests and ESP-IDF diagnostic compilation.

The network/platform code is **written but not ESP-IDF compiled or tested on hardware**. Native tests cover the portable core. There is no functional UI, BLE scanner, discovery, authenticated Shelly control or OTA installer yet. See [PROJECT_STATUS.md](PROJECT_STATUS.md) for precise evidence.

## Start here

1. Confirm the board using [docs/HARDWARE.md](docs/HARDWARE.md).
2. Read [docs/BUILD.md](docs/BUILD.md) and run `python3 tools/test.py`.
3. Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) and [docs/API.md](docs/API.md).
4. Bring up display and touch before the first real relay test in [docs/TESTING.md](docs/TESTING.md).

Planned stack: ESP-IDF, C, LVGL, FreeRTOS, NimBLE where supported and NVS. ESP-IDF **v5.5.4** is the pinned baseline for diagnostic CI; final board support must validate it against the board vendor drivers. LVGL and NimBLE are not dependencies yet: they will be pinned/enabled with the board implementation.

## Intended experience

An original dark interface inspired by Shelly Smart Control: Home, Rooms, Devices, Scenes, Energy and Settings. Device capabilities determine the controls displayed. Unsupported features are not presented as available. See [docs/UI.md](docs/UI.md).

First hardware milestone: boot the confirmed board, show a responsive screen, configure Wi-Fi, add one Shelly by IPv4, read its relay state, issue ON/OFF, confirm state and handle disconnection. Support for authenticated devices must be added before claiming a generally usable first release.

Wi-Fi discovery, other component types, rooms/scenes, BLE/BTHome and signed OTA follow in tracked stages. OTA must preserve configuration, require user confirmation, check board compatibility, authenticate images and retain rollback; see [docs/OTA.md](docs/OTA.md).

## Repository and license

Intended repository: `voltex-at/DIY-Shelly-Remote` (not created at this checkpoint). The connected integration could read the account but does not expose repository creation. The archive contains the source tree and workflow ready for initial import.

Original project code: [MIT](LICENSE). Dependency licenses and review requirements: [NOTICE.md](NOTICE.md). No Shelly logos, proprietary icons, application source or graphical assets are included.
