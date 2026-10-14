# Project status

Checkpoint: 2026-10-15, Europe/Vienna. Version: `0.0.1-dev`.

## Evidence

| Area | Written | Compiled | Automated validation | Physical test |
| --- | --- | --- | --- | --- |
| Portable C requests, registry, version and OTA metadata policy | Yes | Host GCC, C11 | 94 assertions passed | No |
| HTTP probe/status/ON/OFF/readback adapter | Yes | Not yet with ESP-IDF | Not yet | No |
| NVS settings and Wi-Fi reconnect adapter | Yes | Not yet with ESP-IDF | Not yet | No |
| Single-outstanding-job network worker | Yes | Not yet with ESP-IDF | Not yet | No |
| Unconfigured diagnostic application | Yes | Not yet with ESP-IDF | CI authored, not run | No |
| Board/display/touch driver | No; explicit unsupported result | No | No | No |
| LVGL UI | Design documented only | No | No | No |
| BLE scan, BTHome, encryption and BLE RPC | Requirements documented only | No | No | No |
| OTA installer, manifest signature validation and rollback | Design documented only | No | No | No |

## Completed foundation

- Independent ESP-IDF project, no modifications to other remote/sensor projects.
- Gen1 and Gen2+ request paths, explicit commands (no retry of ambiguous toggles), input limits and portable tests.
- Transport/platform source separated from a board boundary that does not guess GPIO.
- Build, hardware, API, UI, OTA, testing and release documentation; MIT license and dependency notice.
- Diagnostic CI for ESP32 and ESP32-S3, explicitly not a hardware compatibility declaration.

## Verification limits and known issues

- ESP-IDF, CMake and target toolchains are unavailable in the current runtime. No ESP32 compilation, flash or release has occurred.
- AddressSanitizer/LeakSanitizer execution fails because this restricted runtime cannot read process metadata. Native tests and a separate UndefinedBehaviorSanitizer run both passed all 94 checks. CI requests ASan+UBSan on a normal GitHub runner.
- No board specification in the attachment. Previous-context lookup is disabled, so it supplied no missing board data.
- GitHub account `voltex-at` was verified. Repository lookup returned 404; the integration has no create-repository operation. No remote repository, commit, CI run or Release is claimed.
- The diagnostic application deliberately stops at the unconfigured board gate; the adapters are not a complete runnable remote.
- Manual IPv4 only; no DNS/mDNS discovery, DHCP identity tracking, WebSocket subscriptions or polling scheduler yet.
- Plain local HTTP only. No Basic/Digest credential handling yet; 401/403 are reported as `DSR_AUTH_REQUIRED`. Do not disable device authentication for this foundation.
- Only relay/switch components are implemented, with explicit channel selection. No generalized capability enumeration or all-model support.
- Wi-Fi start is a once-per-boot API. A failed partial initialization requires restart; runtime reconfiguration and cleanup are future work.
- NVS schema 1 stores a settings blob with field validation. It is not encrypted by this foundation and must receive a migration/secure-storage review before a production release. No UI currently collects credentials.
- OTA metadata checks do not verify an image, hash or signature and cannot install anything. The development version is intentionally rejected by the stable-only version policy.
- No physical battery measurement, LED, backlight, sleep or wake behavior is claimed.

## Next step

Obtain exact board model and vendor schematic/example, display resolution/controller/interface, touch controller, GPIO map, Flash/PSRAM sizes, power/backlight controls and hardware revision. Create/access the public `DIY-Shelly-Remote` repository, import this checkpoint and run diagnostic CI. Then implement the board port and physical smoke test, pin LVGL and proceed to the first relay milestone.

The intended final project includes all requested screens, rooms/scenes, energy, BLE and OTA. Those remain tracked work, not completed features. See [docs/ROADMAP.md](docs/ROADMAP.md).
