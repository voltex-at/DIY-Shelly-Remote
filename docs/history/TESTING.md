# Verification and physical test plan

## Automated now

`python3 tools/test.py` compiles and runs the actual portable C module with strict warnings. The 94 assertions cover:

- Endpoint validation and rejection of malformed/URL-injected IPv4 inputs.
- Exact Gen1/Gen2+ read and explicit ON/OFF request format and boundary channels.
- Registry updates, multi-channel separation, capacity and invalid fixed buffers.
- Numeric stable-version ordering, overflow, suffixes and malformed versions.
- OTA metadata policy: wrong board, oversized image, downgrade/equal/prerelease, non-HTTPS URL and malformed digest.

Native and UBSan-only execution passed locally. ASan/UBSan also passed in GitHub Actions run 38657250244, together with ESP32/ESP32-S3 diagnostic compilation. The initial local runtime could not run LeakSanitizer. These tests do not validate actual HTTP parsing/transport, Wi-Fi, NVS, FreeRTOS scheduling, LVGL, BLE or OTA installation. Add fixture tests and IDF/component tests as those paths are integrated.

## Board bring-up and later functional smoke test

The [ES3C35P guide](../../boards/lcdwiki-es3c35p/README.md) contains the first hardware-only test. Run it only with a successful board-specific CI artifact and matching checksum. The following extended sequence additionally requires Wi-Fi/device UI integration, which this hardware-only build does not include.

Expected sequence after board implementation:

1. USB boot log identifies project/version, correct MCU, Flash/PSRAM and hardware revision without exposing secrets.
2. Display shows the welcome screen in correct orientation; backlight and color are correct.
3. Touch matches all corners; long labels/scrolling work; inactivity turns the backlight fully off if supported, and first touch only wakes it.
4. Enter Wi-Fi through the implemented setup UI, save settings, reboot and reconnect. A wrong password is reported and recoverable; UI stays responsive.
5. Enter a Shelly IPv4 address on the same network. Probe `/shelly`, then a verified relay/switch channel. Protected devices show an authentication requirement until auth is implemented.
6. Use a known safe test load and issue ON, then OFF. UI waits for observed readback and does not claim an unconfirmed state.
7. Disconnect the Shelly and Wi-Fi independently, confirm offline/error behavior and reconnection without restart, command replay or UI lockup.
8. Reboot, confirm settings survive, and check memory/tasks over an extended run. OTA has a separate acceptance suite in [OTA.md](OTA.md).

Return the board/model and revision, Shelly model/firmware/component, firmware version/hash, exact reproduction steps, expected/actual behavior and redacted serial log. Do not include network passwords, auth headers, keys or tokens. Collect serial logs using the board-specific `idf.py -p <port> monitor` command once its USB setup is confirmed.
