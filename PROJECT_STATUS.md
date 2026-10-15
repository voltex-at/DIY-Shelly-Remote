# Project status

## PR #5 review fixes — durable acknowledgement and photo reclamation

Branch `work/config-save-photo-fixes`, stacked on PR #10. Native and browser
editors retain an immutable submitted snapshot A and preserve edits B made
while A is being saved. Only a matching, published durable document confirms
A; B stays visibly unsaved and can be submitted against the new revision.
Failure keeps the draft. A second Save while pending does not lose tracking.
Native messages, tile size and accent choices use PL/EN/DE; Polish glyphs and
symbol fallback are covered by the Issue #7 font correction.

At boot, the verified current document refreshes the older checkpoint. Then
unused content-addressed photos are removed before the immutable photo cache
or LVGL is created. Both retained checkpoint references are protected; any
remaining corrupt/future checkpoint disables deletion. Live LVGL photo pointers
are never freed. Save removals and restart to reclaim Flash/cache slots.

Local UBSan: 265 configuration checks (real checkpoint files/GC), 378 integrated
LVGL checks, 94 core, 545 HTTP/parser/NVS and 222 Wi-Fi checks pass. Browser
regression edits B during delayed A acknowledgement. Target CI is required for
this commit. [Acceptance procedure and limits](docs/REVIEW_FIXES.md).
No physical ESP32 validation or main merge is claimed.

## Issue #7 A — real LAN discovery and wizard

Dedicated branch `work/issue-7-lan-discovery`, stacked on the visual PR #9.
Portable identity/capability/deduplication/subnet policy and real mDNS + passive
ARP/known-host probing are implemented in a separate bounded network task.
HTTP identity reads carry no credentials; protected component inspection uses
the retained Basic/Digest transport and checks the selected ID/MAC again.
545 HTTP/parser/NVS checks, 94 core and 222 Wi-Fi checks pass with UBSan.
Touch wizard and authenticated panel API now use this service. Users select a
verified device, authenticate, choose an actual switch channel, name and room;
the old binding is retained on a failed probe. DHCP rebind updates the prior
configuration entry for the same saved physical ID. Unsupported covers/sensors
are identified but cannot be falsely bound as a relay. AP/BLE stay disabled.
Native LVGL: 362 checks and 19 frames including no-Wi-Fi, discovery, auth error,
nonconsecutive channels and room binding. The service checkpoint `efe4db2` passed
all five jobs in run 38748844770. Integrated UI/API `8a83628` passed all five
jobs in [run 38752429842](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38752429842).
Browser onboarding tests use explicit radio fixtures plus the actual C parser;
they do not establish physical LAN operation. [Physical procedure](docs/ISSUE7_LAN.md).

## Issue #6 — rounded icon family and configurable photographic HOME

2026-10-15: branch `work/issue-6-visual-system`, based on PR #5's reviewed
`5f6a653`. Source version `0.4.1-ui-review`. Dedicated draft PR; no main merge.
Replaced the diagnostic line/arc icon renderer with 24 Apache-2.0 Material
Symbols Rounded masks, filtered at build time and during LVGL scaling. Added
44px toolbar/tab targets, graphite tokens, room photo headers and a dashboard
picker. Device cards remain horizontal; four photographic room cards fit two
columns on HOME. Added a distinct cover detail view and PL/EN/DE strings for
Wi-Fi, manual onboarding, diagnostics and editor forms.

13 real review frames include both HOME variants, cover/light details, offline
and EN/DE. Licensed CC-BY-2.0 test photos are kept outside firmware. Native
review and integrated navigation tests pass; target CI evidence belongs to the
dedicated PR. Physical appearance/photograph persistence is **pending user test**,
not established by these host renders. [Exact test procedure](docs/ISSUE6_UI.md).
The save race/photo reclamation review items remain separate tracked fixes;
AP/BLE are explicitly disabled, LAN discovery follows in Issue #7.

## Current: shared configuration and local panel review

2026-10-15, `0.4.0-config-dev`, branch `work/config-web-panel`, draft PR #5
(base `ui/shelly-smart-control`). No merge to `main` is authorized or performed.

Implemented: full-width typed device cards; shared schema/revisions; dashboard,
room, device and scene editors on touch and web; drag ordering on web; appearance;
durable two-checkpoint configuration; photo upload/crop; real LVGL preview;
physical pairing, per-device TLS identity, local authenticated administration;
operator-selected OTA upload with inactive-slot validation and boot confirmation.
Original LCD/touch/backlight and Shelly request/transport modules are retained.

All five CI jobs passed for source `5d309013bdfeb64b628b2c1c2c7e6475561fe126`
in [run 38737249365](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38737249365):
ESP32-S3 board package, both diagnostic builds, native sanitizers and real
LVGL/browser tests. See [current validation](docs/CONFIG_VALIDATION.md).

Limits: one actual relay connection; no new cover/sensor transport or scene
executor; partial PL/EN/DE translation; no publisher-signed OTA release updater.
Physical HTTPS, Flash interruption, OTA/rollback and long-run memory tests remain
outstanding. [Operation and installation](docs/CONFIG_WEB_PL.md).

The following sections are historical checkpoints, not the current feature list.

## Active branch: UI review, awaiting user approval

2026-10-15: development priority changed to a closer visual/interaction match to Shelly Smart Control. `reference/0.2.0-local-dev` preserves the working baseline; `work/wifi-onboarding` preserves pending Wi-Fi work; `ui/shelly-smart-control` isolates the UI proposal. Source version here is `0.3.0-ui-review`, not a public release. Do not merge or expand further screens before approval.

Five core views, shared styles/original icons/Roboto fonts, and actual LVGL RGB565 rendering are implemented. Eight 320×480 review states include clearly marked DEMO data and a genuinely empty real-mode view. Transport and board code are unchanged from the frozen Wi-Fi checkpoint. Review source/data separation and no-demo-command behavior are tested; target compilation is tracked separately. Details: [UI review](docs/ui-review/README.md).

2026-10-15 icon review v2: fixed border-aware alignment of circular power,
thumbnail, toolbar and navigation icons, balanced the original glyph geometry,
and added a ninth actual LVGL frame showing all 18 icons. Native raster checks
cover 432 size/border combinations, with visible glyph centres within 1px of
their containers. The user's additional PL/EN/DE language selector and OTA
installer requests are recorded as pending, not working features. No merge,
stable release, firmware-install request or physical UI result is implied.

The historical record below describes the previously delivered 0.2.0 firmware; it is not a claim that the new UI has passed physical testing.

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
