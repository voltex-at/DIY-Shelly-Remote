# Project status

Checkpoint: 2026-10-15, Europe/Vienna. Current source version: `0.1.0-bringup`.

## Verified repository and foundation

Public repository: https://github.com/voltex-at/DIY-Shelly-Remote

Foundation PR #1 was merged after all three jobs passed in [run 38657250244](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38657250244): 94 portable assertions with ASan/UBSan, ESP32 diagnostic compilation and ESP32-S3 diagnostic compilation. The HTTP/Wi-Fi/NVS/worker source therefore passed ESP-IDF v5.5.4 compilation. None of it has been tested against physical Shelly devices.

## Current evidence

| Area | Written | Compilation / automated validation | Physical test |
| --- | --- | --- | --- |
| Portable requests, registry, versions and OTA metadata policy | Yes | 94 assertions; native, UBSan locally; ASan/UBSan in CI passed | No |
| HTTP probe/status/ON/OFF/readback, NVS, Wi-Fi, worker | Yes | Foundation ESP32/S3 compilation passed; protocol fixture tests pending | No |
| LCDWiki ES3C35P memory/pin/Flash profile | Yes, vendor references documented | Board-specific build pending | No |
| ST77922 QSPI display and integrated I2C touch | Yes | Board-specific build pending | No |
| LVGL test UI, colors/corners/brightness/screen off/wake | Yes | Board-specific build pending | No |
| Wi-Fi onboarding and actual Shelly control in UI | No | Not applicable | No |
| Rooms/scenes, energy and full device screens | Navigation shells only | No functional feature claim | No |
| BLE scanning, BTHome, keys and BLE RPC | No | Not applicable | No |
| OTA installer, signature validation and health-confirmed rollback | No; metadata policy and dual-slot layout only | No installation validation | No |

## Hardware decision

The user supplied LCDWiki ES3C35P documentation. Profile: ESP32-S3, 16 MiB QSPI Flash, 8 MiB OPI PSRAM, ST77922 320x480 QSPI LCD and integrated touch block. GPIO mapping is sourced from the manufacturer. The module-specific initialization table and touch register protocol are attributed to an exact-board upstream MIT implementation. Firmware verifies actual memory sizes at startup. Physical board revision, colors, touch orientation and electrical behavior still need the user's test.

See [the board guide](boards/lcdwiki-es3c35p/README.md) for evidence, Flash layout, USB installation, expected screen, tests and logs.

## Implemented bring-up behavior

- Native portrait UI; no unverified hardware axis swap.
- Four-pixel horizontal invalidation alignment and bounded internal DMA draw buffer.
- Separate bounded touch polling; LVGL reads a synchronized sample without I2C calls.
- One LVGL owner task; creation occurs before that task starts.
- Backlight PWM, manual full off, 60-second inactivity off and first-touch-only wake. If touch probing fails, automatic off stays disabled.
- Actual Flash/PSRAM checks, amplifier disabled, no initialization of SD/audio/RGB/accessories and no guessed charge state.
- Two 6 MiB OTA slots within the documented 16 MiB, with no updater or authenticity claim yet.

## Known limitations

- No physical tests have occurred. Do not mark a compile success as hardware support validation or a stable release.
- The bring-up UI does not start Wi-Fi/BLE or control Shelly devices. Settings on the test screen are RAM-only; no credentials are collected.
- Local transport supports manual IPv4 and selected relay/switch channels only. No discovery, stable DHCP identity, WebSocket subscriptions, full capability enumeration or polling scheduler yet.
- Shelly authentication is not implemented: 401/403 report `DSR_AUTH_REQUIRED`. Do not disable device authentication to satisfy a support claim.
- Wi-Fi initialization is once per boot; failed partial initialization requires restart. Runtime reconfiguration/cleanup remains to be added.
- NVS schema 1 is validated but not encrypted. Migration and production secure-storage review remain outstanding. No automatic settings erasure is performed on NVS initialization errors.
- The OTA metadata helper does not verify an image digest/signature or install firmware. The bring-up version is intentionally outside the stable-only OTA policy. Post-OTA health confirmation is not implemented.
- Initial merged USB installation replaces the existing firmware and covered Flash contents; it is not a settings-preserving OTA update.
- No battery/charging, LED behavior, deep sleep or pogo accessory support is claimed.

## Next physical milestone

After board compilation succeeds, provide the actual hardware-test artifact and follow the board guide: boot photo/log, four touch corners, primary colors, brightness, off/wake and repeated operation without restarts. Use those results to fix the board port. Then integrate Wi-Fi setup and a real Shelly relay with state readback, authentication and offline recovery.
