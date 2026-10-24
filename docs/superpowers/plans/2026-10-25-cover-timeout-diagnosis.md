# Cover timeout recovery and transport evidence

> Execute inline with focused red/green regressions. Continue the approved
> `2026-10-24-cover-ack-readback.md` contract; no new transport or UI design.

**Goal:** remove confirmed avoidable delays after a Cover command, and make
the still-unresolved physical HTTP timeout distinguishable in the next log.

**Evidence:** owner log from published 0.7.76-dev: command jobs 17/78/89 end
with phase WRITE and OFFLINE; job 42 gets ACK. No phase, status or body log
identifies the failed HTTP leg. Job 89 is followed by actual opening data.
Initial view builds also take 907–909 ms; that separate cost is not evidence
that LVGL caused the HTTP timeout.

**Constraints:** same ESP-IDF 5.5.4, serial worker, identity/auth/admission
checks, three accepted skins and task/memory budgets. No replayed POST,
synthetic ACK/position, hidden persistent error, release or hardware command.

- [x] Reproduce delayed recovery with the real fleet/LCD provider: lost ACK
  schedules a GET at once; it takes one recovery opportunity before routine
  unrelated polls, without bypassing manual commands or STOP.
- [x] Reuse the existing PROBE payload bytes for a telemetry-only flag.
  During Cover follow-up, deliver verified status without waiting for optional
  Sys.GetConfig/name persistence. Ordinary polls still refresh metadata.
- [x] Add bounded non-secret Cover HTTP diagnostics: request stage, elapsed
  time, SDK error/status, header/body event counts and received byte count.
  Do not change timeout/auth/transport policy without evidence of its cause.
- [x] Run affected native worker/fleet suites and the existing Cover ACK,
  STOP and refresh LVGL cases; compile the board once. Record what was fixed
  separately from the unresolved physical timeout and initial-view cost.

Completed locally: affected native suites and actor runtime, five focused LVGL
cases, target compilation and own-frame/static-memory audits PASS. Independent
review found a PROBE payload alias issue; a failing regression reproduced it,
the conversion was corrected and the reviewer reported no remaining findings.
Only relevant checks and incremental compilation were repeated after that fix.
Physical missing-response diagnosis and initial Dashboard cost remain open.
