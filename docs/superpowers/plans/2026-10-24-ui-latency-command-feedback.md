# UI latency and command feedback implementation plan

> **For agentic workers:** Use `superpowers:executing-plans` to implement this continuation task by task, followed by one independent review.

**Goal:** Reduce avoidable UI work and distinguish an uncertain command from an unavailable device after the owner's 0.7.74 hardware report.

**Architecture:** Keep the sole worker, existing guards, confirmed telemetry and approved layouts. Carry a transient command phase through existing event/fleet/model padding. Coalesce network-driven model paints in one owner poll and retain electrical labels when only measurements change.

**Tech stack:** ESP-IDF 5.5.4, LVGL 9.3.0, ES3C35P N16R8 320×480; exactly Cyber Blue, Grafit and Jasny.

**Spec:** Owner's continuation: slow pages/display feedback and transient “No response” after a command; USB log identifies 0.7.74, LVGL passes of 224–354 ms and an HTTP timeout at 24696 ms. Current logs cannot identify the failed HTTP phase or correlate endpoint/job IDs. Preserve genuine failure visibility, STOP, no write replay and all previously accepted layouts.

## Constraints and evidence limits

- No driver sensitivity/filter changes, new worker, dependency/partition/key changes or device flashing.
- No successful action inferred from a timed-out POST or from a requested target.
- Auth, identity, malformed-data and rate-limit failures retain their existing gates.
- One early GET after an uncertain write; a failed recovery returns to ordinary backoff and visible failure.
- Host object-lifetime/render checks do not prove hardware frame times. USB diagnostics use numeric job/layout IDs and phases; no hosts, credentials or payloads.

## Task 1: Command phase and bounded confirmation

- [x] Add focused regressions for preflight failure, lost POST response, valid ACK with lost readback, recovery GET and no replay.
- [x] Carry NONE/WRITE/ACK/READBACK/CONFIRMED through the HTTP observer and existing event/fleet/mobile padding; suspend it across urgent STOP.
- [x] Show an honest command/state-unconfirmed message for the corresponding failure, rather than generic “No response”; keep initial read failures and failed recovery visible.
- [x] Schedule one early GET for an attempted command's transport uncertainty. Preserve actor outcomes and never resend POST.
- [x] Verify affected HTTP/fleet/worker and UI cases, including PL/EN/DE and footprint assertions.

## Task 2: Avoid repeated drawing work

- [x] Add actual-LVGL regressions proving electrical-only telemetry preserves relay/dimmer/RGB title and input objects and updates visible numbers.
- [x] Retain the existing measurement labels; use the full safety renderer for capability, identity, status or structure changes.
- [x] Coalesce model renders within one network poll while keeping direct touch feedback synchronous.
- [x] Avoid resetting unchanged group age text and rebuilding an already selected bottom-navigation page.
- [x] Verify only priority/refresh/touch and affected lighting/relay views, with representative images.

## Task 3: Review and target verification

- [x] Independent review of phase ownership, STOP isolation, bounded recovery, retained references and unchanged safety gates.
- [x] Build the target once after the changes and run existing affected memory/stack audits.
- [x] Record commands, results and remaining physical acceptance. Do not claim the owner's timeout or hardware latency is resolved before a new device observation.

Review found stale phase after urgent STOP and a new intent rejected before
dispatch. Reproduced both failures, reset only the newly recorded result/intent,
then reran the affected platform, native TRV and three LVGL cases. Rebuilt the
changed target objects and repeated boot/actions/fleet limits; all passed.
The same reviewer checked that incremental lifecycle correction without further
findings. See [the evidence and physical limits](../../RESPONSIVENESS_AUDIT_0_7_74_PL.md).
