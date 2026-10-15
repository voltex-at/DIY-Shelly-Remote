# ES3C35P bring-up validation record

Date: 2026-10-15, Europe/Vienna. Firmware version: `0.1.0-bringup`.

Source commit: `f592876f532f9de0714c3cd04a074280d57d9278`.
Tested PR merge commit: `8e857169a8c4cb509751b9619e10f800c29c9c99`.
Merged PR: https://github.com/voltex-at/DIY-Shelly-Remote/pull/2
CI run: https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38658803879
Artifact: https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38658803879/artifacts/11938610486

All four CI jobs passed: native 94-assertion ASan/UBSan tests, ESP32 diagnostic compilation, ESP32-S3 diagnostic compilation, and the ES3C35P hardware image/package build. Local native and UBSan-only runs had also passed. Initialization table payload lengths and Flash partition bounds were checked separately.

## Download verification

| File | Bytes | SHA-256 |
| --- | --- | --- |
| Merged first-install `DIY-Shelly-Remote-es3c35p-bringup.bin` | 618592 | `64a06ac3d8b13f40074908856f93077243c619f8fdd5f983ca66e2232be82fb9` |
| App-only `diy_shelly_remote.bin` | 487520 | `25cacb57494f7975b2301f8cecef9ddc0b6a32171d1ec46b2854d12d40b5873e` |

Artifact ZIP SHA-256: `24c0e80fec6b27ae5f3bb450d309828a5a6fbdeaee461e2a28b03a3eef97ded3`.

The downloaded ZIP passed CRC checks and matched the artifact service digest. Both BIN files matched the included SHA256SUMS.txt. The app has an ESP image magic byte, ESP32-S3 chip ID 9 and the expected version string. Generated flasher_args.json confirms DIO, 80 MHz, 16 MB, bootloader at 0x0, partition table at 0x8000, otadata at 0x19000 and application at 0x20000. The merged image is flashed at 0x0, not the app-only image.

The package includes the real images, Flash metadata, checksums, source/tested commit record, Polish installation/test guide, dependency lockfile and license notices. It is a hardware-test artifact, not a stable Release.

## Resolved dependencies

| Dependency | Version |
| --- | --- |
| ESP-IDF | 5.5.4 |
| LVGL | 9.3.0 |
| Espressif ST77922 | 2.1.1 |
| Espressif cmake_utilities (transitive) | 0.5.3 |

The exact component hashes and source registry are preserved in the artifact's dependencies.lock. Build timestamps can change binary hashes when rebuilding; the hashes above identify the provided artifact.

## Not yet validated

No physical power-up, display orientation/color, touch-corner, brightness, wake, battery, radio or Shelly-device test has been performed. Compilation cannot establish those outcomes. The first user test follows [the board guide](../boards/lcdwiki-es3c35p/README.md). The bring-up UI does not enable Wi-Fi control, BLE or OTA installation.
