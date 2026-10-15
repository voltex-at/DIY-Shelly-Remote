# Project status

Checkpoint: 2026-10-15, Europe/Vienna. Current source version: `0.2.0-local-dev`.
Repository: https://github.com/voltex-at/DIY-Shelly-Remote

## Physical evidence received

The user reported “Działa” and supplied two photographs of the actual ES3C35P running the previously supplied `0.1.0-bringup` image. The Home screen is readable in native portrait and reports `Touch controller: responding`. The touch-test photograph shows `Corner 4 detected` and `Touch count: 4`.

This confirms physical boot/display rendering and touch responses. At 02:55 Europe/Vienna on 2026-10-15, the user additionally confirmed working colors, brightness adjustment and wake in `0.1.0-bringup`, before installing 0.2.0. These are user-reported physical test results, without measured color accuracy, brightness values or wake timing. Complete four-corner mapping, first-touch action suppression, long-duration operation, battery, Wi-Fi and Shelly control remain separate checks. No photos were copied into the public repository.

## Implementation and verification

| Area | Current implementation | Evidence / outstanding test |
| --- | --- | --- |
| Portable requests, registry, versions, OTA metadata policy | Implemented | 94 assertions; prior CI ASan/UBSan passed |
| ES3C35P hardware profile and ST77922 LCD/touch | Implemented | Previous full board CI passed; user boot/touch photos received |
| Test UI, colors/corners/brightness/off/wake | Implemented; tests retained in Menu | Readable UI and touch responses photographed; colors, brightness and wake user-confirmed on 0.1.0; full corner mapping/stability pending |
| On-device Wi-Fi entry and restart | Implemented | All four new CI jobs passed; physical Wi-Fi test pending |
| One saved manual IPv4 switch/channel | Implemented | Real cJSON + fake HTTP fixtures; actual Shelly test pending |
| Gen1 Basic / Gen2+ Digest authentication | ESP-IDF transport configured; both auth features enabled | Configuration/challenge-body fixture and board compilation passed; actual protected device test pending |
| Identity before read/write, explicit ON/OFF/readback, five-second polling | Implemented | Identity mismatch blocks command in fixture; physical network recovery pending |
| NVS settings and device binding | Validated schema; save after successful probe; credentials local | Fake-NVS adapter tests; persistence on actual board pending; NVS unencrypted |
| Multiple devices, rooms/scenes, energy history | Not implemented | Future milestone |
| BLE / BTHome / BLE RPC | Not implemented | Future milestone |
| OTA installation/signatures/health-confirmed rollback | No installer; metadata policy and dual-slot layout only | Future milestone |

Current local validation: **94 portable + 371 platform checks** passed both normally and with UBSan. Platform tests compile the real request/HTTP/connection/settings code and use real cJSON with mocked HTTP and NVS APIs. They check malformed/oversized JSON, authentication errors/429, challenge-body reset, explicit command/readback, identity mismatch, no command replay and invalid NVS schemas. They do not test IDF Digest cryptography, actual flash commits, Wi-Fi or radio coexistence. Full ASan/UBSan, both diagnostic builds and ES3C35P build/package passed in [run 38662049616](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38662049616). No compiler warnings/errors were reported by the ES3C35P build. The license packager retained 63 dependency notices plus its index.

## Hardware and artifact history

Profile: LCDWiki ES3C35P, ESP32-S3 N16R8, 16 MiB QSPI Flash, 8 MiB OPI PSRAM, ST77922 320x480 QSPI display and integrated I2C touch. GPIOs are manufacturer-sourced. Exact-board upstream initialization is attributed to its MIT source. Runtime checks enforce expected memory sizes. The display/touch port is preserved from the working bring-up.

PR #1 foundation passed [run 38657250244](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38657250244). PR #2 board bring-up passed all four jobs in [run 38658803879](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38658803879). The prior [0.1.0 artifact](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38658803879/artifacts/11938610486) was downloaded and its ZIP/BIN checksums, image metadata, Flash addresses and version verified. Exact values remain in [VALIDATION.md](docs/VALIDATION.md).

## Current limits and next test

Use [the Polish local-control guide](docs/LOCAL_CONTROL_PL.md) for the new development firmware; [the board reference](boards/lcdwiki-es3c35p/README.md) records hardware and original bring-up. No stable Release is claimed.

- One stored relay/switch channel, manual IPv4 only. No discovery, automatic DHCP address relocation, capability enumeration or notifications. Reserve the Shelly IP in DHCP; a different saved identity blocks control until explicitly added again.
- Local HTTP: Gen1 Basic has no transport confidentiality; Gen2+ uses IDF Digest SHA-256. No redirects, credentials in URLs/source/logs, or cloud dependency. Auth failure/429/identity mismatch pauses automatic polls; explicit Refresh or re-add resumes a user-directed attempt.
- Wi-Fi start is once per boot. The explicit Save & restart button applies a new network after NVS success. Reconnect uses 1–32 second backoff. Network operations run in a bounded worker; no LVGL call runs in that worker.
- NVS is not encrypted. Schema migration/production secure storage remain pending. NVS initialization/schema errors do not automatically erase settings. Brightness changes are temporary; screen off remains 60 seconds.
- Merged USB installation overwrites covered Flash contents including settings; it is not settings-preserving OTA. Both app slots are 6 MiB but installation and post-OTA health confirmation are not implemented.
- No battery, charging, RGB LED, deep sleep, audio, SD or pogo accessory behavior is claimed.

Next physical milestone: configure Wi-Fi on the screen, add an actual authenticated Shelly switch, verify ON/OFF and external state changes, reboot persistence and recovery after disconnection. Record exact Shelly model, firmware, generation, channel and auth mode without passwords. Colors, brightness and wake were confirmed on 0.1.0; check them for regressions after installing 0.2.0.

## Verified 0.2.0 development artifact

[PR #3](https://github.com/voltex-at/DIY-Shelly-Remote/pull/3) implements the one-switch milestone. Source commit `425682dadf7992e068afddf395f9527ae62e98e6`; tested PR merge `6fdbd9d604ca98cacdc234fa7ee7424f8007b874`. All four jobs passed in [run 38662049616](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38662049616). The actual [artifact 11939312624](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38662049616/artifacts/11939312624) was downloaded and its ZIP CRC/digest, both BIN hashes, ESP32-S3 image header/version, embedded app at 0x20000, Flash metadata and cJSON/lwIP notices were verified. Merged USB BIN: 1245776 bytes, flash at **0x0**. App: 1114704 bytes with 82% of the 6 MiB slot remaining. Exact hashes are in [VALIDATION.md](docs/VALIDATION.md). This is development firmware awaiting actual Wi-Fi/Shelly testing, not a stable Release.

Latest user checkpoint: 2026-10-15 02:55 Europe/Vienna. The user confirmed colors/brightness/wake in 0.1.0 and planned to install 0.2.0 next. No 0.2.0 physical result has been reported yet. OTA installation is intentionally unavailable in both versions; the 0.2.0 package is installed over USB.
