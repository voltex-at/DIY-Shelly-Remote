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

At packaging time, no physical test had occurred. The following user evidence was received afterward. The 0.1.0 bring-up UI does not enable Wi-Fi, BLE or OTA installation.

## User hardware report received 2026-10-15

The user wrote “Działa” and supplied two photographs: native portrait Home with readable text and `Touch controller: responding`, and the corner test with `Corner 4 detected` / `Touch count: 4`. This is physical boot/display rendering and touch-response evidence for the supplied 0.1.0-bringup firmware, not an independent verification of every corner, exact colors, brightness/wake behavior or stability. No physical radio/Shelly/battery test was reported. Photos were not republished in this repository.


## Additional user hardware confirmation — 2026-10-15 02:55 Europe/Vienna

Before installing 0.2.0, the user explicitly confirmed working colors, brightness adjustment and wake in the first (`0.1.0-bringup`) firmware. These are user-reported physical results; no quantitative color/brightness measurements or wake timing were supplied. This report does not establish every corner mapping, first-touch action suppression, long-duration stability, battery or radio behavior.

The user also noted that updates were inactive in the first version and planned to install 0.2.0 next. OTA installation remains intentionally unavailable in both versions; 0.2.0 is installed through USB. No physical 0.2.0 Wi-Fi/Shelly result was reported at this checkpoint.

## 0.2.0-local-dev local validation

94 portable checks and 371 platform checks passed normally and with UBSan. The new platform executable compiles actual HTTP/identity/NVS adapters and real cJSON, with fake HTTP client and NVS APIs. Tests cover Gen1/Gen2 identification, MAC normalization, type errors, malformed/oversized responses, 401/403/429/redirect rejection, 401-body reset, explicit output/readback, mismatch-before-command, no replay after timeout, bounded strings, storage errors and unknown schemas. This does not validate IDF Digest cryptography, actual NVS atomicity or a physical device. The completed CI/artifact result follows; physical Wi-Fi/Shelly tests remain pending.

## 0.2.0-local-dev verified CI and download

Source: `425682dadf7992e068afddf395f9527ae62e98e6`.
Tested PR merge: `6fdbd9d604ca98cacdc234fa7ee7424f8007b874`.
PR: https://github.com/voltex-at/DIY-Shelly-Remote/pull/3
CI: https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38662049616
Artifact: https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38662049616/artifacts/11939312624

All four jobs passed: native 94 + 371 checks with ASan/UBSan, ESP32 diagnostic, ESP32-S3 diagnostic and ES3C35P board build/package. No compiler warnings/errors were reported in the board log. The package includes 63 collected SDK/component notices plus an index, the project/upstream notices, the Polish guide and board reference. The source depends on the same pinned IDF/LVGL/panel versions listed above. The package-only notice change did not alter device-control C code after the previous successful CI run.

| File | Bytes | SHA-256 |
| --- | --- | --- |
| Merged USB `DIY-Shelly-Remote-es3c35p-local.bin` at 0x0 | 1245776 | `6ad73a543f6ba795dd95ef846514700be8a2a716681379695553b57d46553b63` |
| App-only `diy_shelly_remote.bin` at 0x20000 | 1114704 | `8e640cb2d27c85fec7114f5aa92252e2a32480ef84270652d699186c2d3a4c44` |

ZIP: 1760233 bytes; SHA-256 `4bde35622cd657ee3ffea322d716d8b7f563d65e95140564777edaaee141111e`.

The downloaded ZIP matched the artifact service digest and passed CRC/path checks. Both BINs matched SHA256SUMS.txt. The app descriptor contains `0.2.0-local-dev`; image chip ID is ESP32-S3 (9), and the merged image contains exactly the same application bytes at 0x20000. Generated metadata confirms DIO / 80 MHz / 16 MB and unchanged partition addresses. cJSON/lwIP notices are present. The app uses 18% of the 6 MiB slot. This verifies the artifact, not the new physical UI/network/authentication workflow.
