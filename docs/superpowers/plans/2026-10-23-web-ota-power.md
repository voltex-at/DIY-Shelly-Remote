# Web OTA battery admission

Goal: apply the agreed native policy to local BIN upload. Known fresh SOC below
20% refuses, exactly20% accepts. Unknown SOC requires explicit stable USB consent;
lack of a worker response never grants access. No continuous USB/charge sensing,
signing/key/partition or release change.

Reserve the existing worker available slot and activity lease. A private bounded
request/completion asks the sole worker for fresh ADC/profile immediately before
esp_ota_begin, after image-header validation. Hold the slot through cleanup; no
new task or enlarged job/event. The mutex protects only short phase changes.

Phases IDLE/QUEUED/RUNNING/DONE/HELD/ABANDONED. Wait at most1000ms. Cancel queued
work on timeout. Running work owns cleanup and discards late results. Discard a
DONE result racing timeout too. No subsequent caller can reuse stale completion.
Urgent STOP remains unchanged. Never access LVGL from HTTP/worker.

LCD adds explicit USB confirmation before the single-use120s grant. Backend
rejects false attestation. A browser parameter or cached LCD SOC cannot admit
Flash. Retain accepted granted/expired/denied layouts. Translate low/busy golden
messages; retry needs a fresh LCD grant.

1. Shared fresh policy dsr_ota_power_model.c/header; native actor reuses it.
   Focus native actor0/19/20/unknown/cancellation, keep diagnostic enum values.
2. Private handoff dsr_ota_power.c/internal header +worker lifecycle. Test
   busy/restart, queue/running/DONE timeout, late result, OOM and balanced slots.
   Extend actual battery/worker fixture for real worker dispatch.
3. Receiver/endpoint gate before any Flash. Test production receiver/extracted
   endpoint, real SHA, prior faults and low/busy/USB refusal. Preserve auth hashes.
4. LCD/browser focused LVGL/Chromium states; inspect genuine pixels.
5. One target build/relevant budgets, independent final review, docs and issue74.
   No full atlas/CI, upload, physical flash/restart, push or publication.

Models/build do not prove battery accuracy, physical USB, SDK stack/heap peaks,
installation, data retention or rollback; those remain hardware acceptance.

## Execution outcome

Steps1–5 implemented locally. Focused broker, receiver, permission, real worker,
native actor, LVGL and browser checks pass. ESP-IDF5.5.4 board build and final
incremental build pass; image0x427150,31%app partition free. Existing ELF budget
audits pass. Independent review has no remaining findings after moving worker
release behind receiver heap cleanup; the new cleanup-order regression passed.
Details and explicit model/hardware limits are in PROJECT_STATUS_PL.md.

Owner accepted the shown USB confirmation and LOW/BUSY previews from a415e17.
Hardware acceptance remains open. No live upload, flash/restart, push,
CI dispatch or release was performed.
