# Firmware 0.7.73 responsiveness and functional audit

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Repair confirmed control/refresh failures and review every firmware feature family against the owner's physical 0.7.73-dev reports.

**Architecture:** Retain the existing LVGL owner, serial device worker, persistent workspace and independent STOP lane. Reduce redundant protocol work and distinguish confirmed samples from drafts, acknowledgements and transport failures. Review independent feature domains without running duplicate broad suites.

**Tech Stack:** ESP-IDF 5.5.4; LVGL 9.3.0; ES3C35P ESP32-S3, 320×480; Cyber Blue, Grafit, Jasny; PL/EN/DE.

**Spec:** Owner reports in issue #74 and this chat: temporary No response after Cover GOTO and slow TRV refresh on 0.7.73; review and repair all functions, preserve safety and responsive active-screen control.

## Global constraints

- Preserve pre-existing local changes and branch work/dev-0.7.70.
- Preserve OTA trust, data/partition compatibility, consent and authentication.
- Never replay uncertain physical writes or show requested values as confirmed.
- No live device operations; host tests and compilation are not physical acceptance.
- Focused red/green regressions per fix; one integration validation at the end.
- No full atlas/CI/release after individual fixes.

## Review focus

- Accepted write followed by lost readback: GET-only bounded recovery; sustained failures remain errors.
- Slow BLE gateway/mirrored TRV status: retain identity checks; avoid re-discovery of known sensor objects and false target confirmation.
- Group Cover STOP during active actor: cancel unsent moves, preserve all member STOP requests and the urgent lane.
- Same-value manual thermostat intent and editor header Back: ownership and unsaved changes remain protected.
- OTA/backup/panel preview concurrency: physical consent, exact success state, fresh capture, bounded transport ownership and serviceable STOP.

## Work items

### 1. Device RPC and refresh

Files: dsr_platform/dsr_http.c, dsr_fleet.c, tests/cover_platform.inc, tests/blu_platform.inc, focused platform/UI cases.

- [x] Add failing protocol tests for accepted Cover/TRV writes with one failed readback, lost ACK, repeated failures, auth/429 and STOP/guard interruption.
- [x] Reduce known TRV refresh from discovery/multiple sensor requests to documented typed status without weakening gateway/MAC checks or fabricating omitted fields.
- [x] Bound GET-only recovery; retain serial worker and no write replay.
- [x] Cover delayed mirror convergence with bounded follow-up reads and explicit pending/confirmed behavior where required by evidence.
- [x] Run affected protocol and actual LVGL regressions; compile changed target objects.

### 2. Group STOP and manual TRV ownership

Files: group UI/model, network callbacks/mailboxes, actor/native controller and focused tests.

- [x] Reproduce group STOP while actor reserves moving members and serial urgent-lane saturation.
- [x] Reuse existing STOP mailboxes and actor cancellation; preserve subsequent member STOP until its handoff.
- [x] Reproduce + then − to the reported target; ensure the durable manual override is recorded even for equal value.
- [x] Run directly affected actor/mailbox/UI regressions.

### 3. Maintenance and editing

Files: native OTA LCD, panel preview/OTA transport, editor header callbacks and related tests.

- [x] Reproduce actual successful OTA snapshot with boot selection attempted, then verify countdown/postpone.
- [x] Reproduce repeated explicit preview requests around a screen change; wait for current capture rather than returning stale success.
- [x] Verify native and footer Back share dirty/save guards in all editors.
- [x] Validate local web OTA server starvation scenario and implement bounded asynchronous transport with cancellation/consent lifetime checks, using existing patterns.
- [x] Run only affected native OTA, panel, editor and upload regressions.

### 4. Coverage and final integration

- [x] Document reviewed entry points and results for devices, groups/scenes, automations, Wi-Fi/AP/LAN/BLE, battery, settings/editors/photos, activity/backup/reset, OTA, panel, boot/touch/display and release/security boundaries.
- [x] Independently review changes, address confirmed important findings, and run relevant integration checks once.
- [x] Record commands/results and remaining physical checks in issue #74; never claim all hardware behavior is proved by host fixtures.

## Implementation rulings and evidence

- Known TRV status uses documented typed mirror fields, with gateway and MAC
  verification retained. First discovery and ordinary BLU sensors stay unchanged.
- One GET retry belongs only after an acknowledged Cover/TRV write; no change
  to uncertain POST policy, authentication/429 handling or SDK timeouts.
- Mirror convergence uses an existing slot's target and one byte in alignment
  padding: slot remains 504 B and fleet 8080 B. Actor starts copy the current
  target too. The UI wait pauses across mode changes and ends on recovery.
- STOP bypasses group actor admission through existing endpoint mailboxes;
  taking another mailbox waits until the sole urgent lane is free.
- Header Back was reviewed and is already rebound to the guarded footer by
  dsr_ui_button; the initial reviewer hypothesis was withdrawn. The battery
  shortcut was a separate confirmed bypass and now preserves its destination.
- Web unknown completion releases only the ability to submit a NEW explicit
  intent after a post-failure successful GET. It does not prove the lost POST's
  outcome; uncertainty remains visible. No automatic replay was added.
- Local OTA reuses the backup task owner and 16 KiB stack reservation. Context
  ownership includes stage, copied session hash and a 180 s deadline. Every
  outcome cleans up exactly once; session is rechecked before boot selection.
- Final module tests and selected integration/UI/browser/target checks passed.
  Detailed commands, limitations and reviewed feature families are recorded in
  docs/RESPONSIVENESS_AUDIT_0_7_73_PL.md. Physical timing/Flash/RF remain open.
