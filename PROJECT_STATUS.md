# Project status

## 0.7.2 — boot SRAM regression fix; physical acceptance pending

Branch `work/fix-071-boot-memory`, stacked on draft PR #27. The user confirmed
that 0.6.3 boots on the same ES3C35P and 0.7.1 does not (2026-10-16,
Europe/Vienna). Their USB log establishes the fatal failure, not an offset guess:
`dsr_board_start_ui_task()` returns `ESP_ERR_NO_MEM`, then `ESP_ERROR_CHECK`
aborts before the backlight is enabled. Flash=16 MiB, PSRAM=8 MiB/test OK,
ST77922 and five-point touch initialization all precede the failure.
**0.7.1 failed physical boot and must not be treated as accepted firmware.**

The 128 KiB built-in LVGL pool is now linked into PSRAM with an exact-object
linker fragment. Its size and allocator stay unchanged. The 8192-byte UI stack
and TCB are statically reserved in internal BSS before optional services can
consume their space; no external task stack or reduced UI stack is used.
Target CI must check actual ELF symbols/section bounds, not just sdkconfig.
Startup logs report internal free/largest and PSRAM at each service boundary.
LCD/touch/QSPI/PWM pins, BLE scanner, relay/LAN transport and OTA partitions are
retained. No extra UI features or visual redesign are included in this repair.

The same log also reports SPIFFS mount failure. That is a separate observation,
not proof of corruption or a reason to erase. Mounting now retains the real
provisioning/read error, never formats on allocation/state errors, and provisions
only a fully read, entirely erased data partition. Regression fixtures exercise
static UI creation and storage failure paths (15938 local UBSan checks pass).
Core 94, platform 847, Wi-Fi 222, BLE 2138 and config 265 checks also pass locally.
Full target/sanitizer/browser CI
and the new ELF check follow on the exact commit; **no physical pass is claimed**.
See [BOOT_MEMORY_FIX_PL.md](docs/BOOT_MEMORY_FIX_PL.md) for recovery and acceptance.

## 0.7.1 — feature audit and working dashboard information (Issue #25)

Branch `work/issue-25-feature-audit`, stacked on BLE PR #26. The versioned
[FEATURE_MATRIX.md](docs/FEATURE_MATRIX.md) traces visible LCD/Web interactions
through workers/API/state, lists supported types, implementation/test status,
issues and required hardware evidence. It explicitly separates configuration
entries, one active relay, BLE discovery, pairing, sensor data and true Cover.
The audit does not mark the unfinished global quality gate complete.

The previously inert toolbar Info now opens actual dashboard/device/room/online
counts with a close action and explicit DEMO label in test models. Current tabs
without callbacks are static selection indicators, no longer inert touch targets.
Other unavailable features retain disabled affordances and their tracked issues.
Local UBSan passes 2635 review checks/24 frames and 1176 integrated UI checks/47
frames. Pointer/language/raster checks cover open/close and live update of the
Info action. Drivers, transports,
OTA, NVS and BLE scanner are unchanged by this isolated audit patch. Target CI
and physical acceptance remain separate; no main merge or stable publication.

BLE PR #26 (`c4ca872`) completed 5/5 CI jobs in
[run 38807134240](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38807134240):
actual ES3C35P firmware, effective NimBLE observer/PSRAM/coexistence configuration,
native ASan/UBSan, LVGL and 28 browser checks. Physical RF remains pending.

## 0.7.0 — real NimBLE scanner, first BLE milestone (Issue #18 / #7B)

Branch `work/issue-18-ble-scan`, stacked on verified PR #17. Actual ESP-NimBLE
legacy active discovery runs for <=20 s in a separate manager/host task, with
software coexistence, cancel, retry, generation fencing and a 16-row cap. LCD
and authenticated local HTTPS use the same bounded snapshot. RF/GATT callbacks
never touch LVGL. NimBLE starts lazily and retains a fixed host baseline after
completion; there is no perpetual scan or repeated init/deinit churn.

Manufacturer/UUID evidence separates Shelly-advertised RPC, known Shelly BLU
and unverified BTHome/other advertisements. Optional manufacturer model IDs use
the documented table. Names never establish authenticated identity or generation.
Generic BTHome is not mislabelled Shelly. Scan response merges by address/type;
malformed/truncated AD, stale generations and overflow are bounded. MACs and
secrets are not logged or returned by the new authenticated scan API.

This is discovery only: **BLE bonding, Wi-Fi provisioning, persistent BLU sensors
and BLE RPC control are not implemented**. Production results come only from the
NimBLE radio; host radio providers are test-only and renders have TEST / DEMO.
Local UBSan passes: 2138 BLE parser/adapter checks (including 2000 deterministic
AD inputs), 1176 integrated LVGL checks/47 frames, 2615 review checks/23 frames.
BLE UI heap is 100352 -> 100352 B after ten scan/cancel/exit cycles; host figures
are not ESP32/PSRAM measurements. Core 94, platform 847, Wi-Fi 222 and config 265
regressions remain retained. Real 320x480 frames accompany the PR.
Target, sanitizer and browser CI follows on the exact source commit. Physical
RF, coexistence, touch/FPS, heap/PSRAM and >=10 cycles remain unverified; the user
must supply an actual board result before acceptance. [Full scope and test](docs/BLE_PLAN.md).
Existing LCD/touch/backlight, Wi-Fi, relay HTTP/auth/NVS and OTA partitions are
preserved. No merge/main/stable or physical-test claim.

## 0.6.3 — stable background reads and one deferred user command (Issue #12)

Branch `work/issue-12-stable-polling`, stacked on cover PR #16. A periodic PROBE
retains confirmed state/metrics and enabled ON/OFF. Unchanged samples update
only freshness text, preserving card objects and scroll. An accepted tap during
a read queues one explicit target ON/OFF, disables duplicates, verifies the same
device/host/generation/channel after the read and dispatches before config jobs.
Read/auth/Wi-Fi/dispatch failures cancel the intent; SET is never auto-replayed.
The confirmed output changes only after mandatory SET/readback. Manual Refresh
can join an existing read. Physical drivers, relay transport and NVS are retained.

Local UBSan: 1101 integrated LVGL checks/38 frames and 2615 review checks/23
frames pass, including two stable 5/10 s polls, pointer tap during a read, one
deferred SET, double tap, auth failure, Wi-Fi loss, dispatch rejection and sample
age. Repeated UI and cover preview cycles retain the same host heap. Tests and
renders inject clearly marked TEST/DEMO transport events, not physical readings.
[Exact regression and physical test](docs/STABLE_POLLING.md). Target/browser/ASan
CI runs on this draft; physical regression remains pending. No merge/stable.

PR #16 (`41137b6`, cover view 0.6.2) completed all five CI jobs successfully:
[run 38800101997](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38800101997).
This includes the actual ES3C35P firmware and host ASan/browser checks. It does
not establish a physical Cover transport test or visual acceptance.

## 0.6.2 — Shelly-inspired vertical cover details (Issue #15)

Branch `work/issue-15-cover-details`, stacked on PR #14. The existing cover view
now uses a name/edit header, compact scrolling position/W/V/Hz chips, a dominant
vertical blue opening indicator, explicit percentage, 148x64 Open/Close targets
and a separate 64x64 Stop. Unknown/offline/failed samples hide stale position and
measurements. Opening, closing, stopped and pending command are separate model
states; background reads do not replace the cover status. PL/EN/DE are covered.

The production menu includes an explicit isolated Cover DEMO preview for visual
review on ES3C35P. Preview controls never invoke network/edit callbacks or persist
fake data; current live updates survive exiting the preview. Ordinary production
models still reject injected demo flags. Real cover commands require capabilities
and callbacks, which the current relay adapter does not provide. Cover transport,
calibration and physical cover control are NOT implemented by this visual PR.
The informational indicator never sends GoToPosition.

Native regression/raster tests cover 0/65/100%, unknown/offline/motion/error,
metrics overflow, PL/EN/DE, actual pointer geometry, transport gating, preview
isolation and 80 repeated preview/control/exit cycles. Local UBSan passes: 2615
review checks/23 frames (cover cycle heap 89368 -> 89368), 1039 integrated UI
checks/35 frames, 94 core, 847 HTTP/NVS, 222 Wi-Fi and 265 configuration checks.
Target/ASan/browser checks
run in the new draft PR; physical ES3C35P visual, heap/PSRAM and regression tests
remain pending. [Comparison, exact renders and physical test](docs/ISSUE15_COVER.md).
Drivers, relay HTTP/NVS schema, discovery, OTA partitions and main are unchanged.
The separate HOME polling/queued SET correction requested in Issue #12 remains
the next functional stage. No merge/stable or visual acceptance is claimed.

## 0.6.1 — optional V/A and visible command outcomes (Issue #12)

Branch `work/issue-12-telemetry-errors`, a small isolated change on UI PR #13.
Real Gen2+/Gen3 Switch.GetStatus voltage/current are parsed only when present,
numeric, finite and within a generous display bound (absolute value <= 1e9;
V/A nonnegative). Missing fields remain unavailable, not zero. Invalid optional
types reject the sample. Volatile telemetry is excluded from an explicit exact
schema-1 NVS record, preserving existing stored bindings and passwords.

Authenticated read/write identity checks, request transport and no-replay policy
are retained. Cards/details show auth, unreachable, identity, response/state and
storage errors; failed-command notices remain until explicit Refresh or another
successful command. Refresh only reads state, never replays SET. Pending reads
and pending commands have different labels. Worker logs transaction outcomes
and availability flags without IP, identity or credentials.

Local UBSan: 847 HTTP/parser/NVS checks, plus the retained 94 core and 222 Wi-Fi
checks. 1026 native UI checks/34 frames and 2496 review checks pass. Tests drive
V/A, auth failure, timeout, failed-command latching,
explicit read-only refresh and old-format NVS persistence. Renders of these
injected event samples carry TEST / DEMO; they are not physical measurements.
[Exact Gen3 hardware regression](docs/ISSUE12_TELEMETRY.md). Target/ASan/browser
CI belongs to the new draft PR. Physical V/A/error tests remain pending.
Previous user-reported ON/OFF success remains recorded; no main merge.

## 0.6.0 — shared UI shell and readable keyboard (Issues #6 / #8)

Branch `work/issue-6-8-unified-ui`, stacked on the retained 0.5.1 PR #11.
Five equally spaced navigation targets replace the misleading sparkle shortcut;
scene icons/editor functionality remain available. Device parameter backgrounds
are measured from the actual translated text, with padding and a width cap.
HOME and native Wi-Fi/Shelly/settings/editor screens share header, graphite
button/card tokens, fonts and active colors. Forms scroll the focused field
above the keyboard; native password inputs remain masked with a show toggle.

The shared 240px keyboard uses 22px Roboto glyphs, QWERTY (PL/EN), QWERTZ (DE),
all printable ASCII, separate PL/DE diacritics and numeric/IP mode. The bottom
row is 123 / cursor left / compact space / cursor right / localized Done.
Actual LVGL pointer tests cover insertion/deletion inside hidden UTF-8 text,
cursor limits, all ASCII keys, numeric mode, dimensions and single submission.
Local UBSan: 986 integrated UI checks/29 frames and 2496 design-review checks
(including 576 icon raster cases) pass. Target/browser/ASan CI follows in the PR.
Real screenshots cover the complete Wi-Fi/Shelly flow and settings/editor.
[Visual comparison and hardware regression procedure](docs/UNIFIED_UI.md).

User-reported physical results received 2026-10-15 via the independent reviewer:
Wi-Fi scan, three real Gen2/Gen3 LAN results, binding a Shelly 1PM Mini Gen3
(`S3SW-001P8EU`), roughly 4.0–4.1 W and explicit ON/OFF control work. This is
evidence for the previously delivered build, not a controlled test of 0.6.0.
Private footage, SSIDs and addresses are not published. Visual acceptance is
still open; new hardware regression is pending. Communication/drivers/OTA are
unchanged. Optional real V/A and visible command errors follow separately in
Issue #12. No main merge or stable release is authorized.

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

Follow-up: native labels/forms/dropdowns use the licensed Roboto font explicitly,
including Polish/German glyphs and LVGL keyboard-symbol fallback. 2400 review
checks (13 glyph checks added), 330 integrated checks and real native frames
pass with UBSan. This correction is also present in the stacked LAN branch.

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
