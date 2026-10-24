# OTA restart, room refresh and memory indicators

Continue existing firmware and accepted three-skin layouts. Preserve the
uncommitted Cover recovery changes and all unrelated local work.

- [x] Reproduce OTA restart through the real UI poll and actual atomic activity
  gate, with a held operation and overdue background polling. Move only restart
  admission outside its own poll lease; defer ordinary background admissions while
  restart waits, let accepted work drain, and keep the zero countdown instead of
  restarting it at 5 s. Do not use the force-confirmed restart API or bypass
  storage, OTA, boot or maintenance safety gates.
- [x] Reproduce deletion/recreation of room cards/images on telemetry changes.
  Cache only the existing metric label alongside placed cards, update aggregates
  in place on Rooms and Dashboard, preserve scroll/photos/touch targets, and
  rebuild on structural/configuration/theme changes as before.
- [x] Reverse RAM/PSRAM/LVGL/stack bar fill to occupied/peak-used percentage,
  retaining free/minimum-free text. Update PL/EN/DE legend; BLE remains binary
  module state. Check zero/full/unknown/invalid totals without overflow.
- [x] Run focused LVGL regressions and relevant restart checks, inspect only
  changed representative frames, compile ESP32-S3 and audit the resulting memory
  budgets. One independent final review. No device flashing or publication.

Completed locally on 2026-10-25. The original three regressions failed before
their fixes. Affected existing OTA tests caught an ordering regression when the
whole updater tick was moved; only restart admission now runs after the poll
lease. Independent review also identified a pending manual TRV without a lease;
the final regression checks both automatic and manual admission wait for it.

PASS: room refresh and memory cases (three skins, PL/EN/DE), existing native
OTA case, automatic/manual/TRV restart integration, priority/Cover-readback
cases, restart backend module, ESP-IDF 5.5.4 build and three target memory
auditors. Physical reset and frame-time acceptance remain open. Evidence is in
`docs/RESPONSIVENESS_AUDIT_0_7_76_PL.md`; generated logs remain ignored.
