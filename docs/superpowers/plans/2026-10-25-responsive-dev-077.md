# Responsive DEV 0.7.77 implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task by task. Preserve all existing local changes and the current branch.

**Goal:** Remove confirmed unnecessary redraws outside Rooms, then publish a signed DEV candidate for the owner's hardware test.

**Architecture:** Keep the sole LVGL owner, asynchronous workers, safety/identity gates, pinned dependencies and accepted three-skin design. Update retained values when presentation structure is unchanged; rebuild when state, capability or binding requires it. Never replace reported device state with requested values.

**Tech stack:** ESP32-S3 ES3C35P, ESP-IDF 5.5.4, LVGL 9.3.0.

- [x] Reproduce BLU signal-age detail rebuild, group electrical detail rebuild and action-result periodic rebuild using focused LVGL regressions; trace usage/OTA label churn. Minimal retained-widget fixes only; verify changed safety/state paths still repaint.
- [x] Review remaining screen ticks, draw paths and blocking ownership. Preserve task priorities, display/touch configuration, photo/frame acceptance and all Cover/OTA fixes already present. Document confirmed fixes and hardware limits.
- [x] Run only affected UI cases while editing and inspect representative pixels. One independent source review of final changes; address concrete findings. Focused cases, ESP-IDF 5.5.4 target build and boot/fleet/actions target audits passed; final radio-transition regression and three-skin BLU/group detail renders checked.
- [ ] Prepare 0.7.77-dev version/notes and exact release candidate commit, excluding generated files, local setup and unrelated schema fixtures. Run final required CI once for that commit, repairing/rerunning only failed jobs if needed.
- [ ] Publish tag dev-v0.7.77 through the protected signed workflow. Owner explicitly requested publication before physical testing; mark the prerelease as a candidate, not hardware-verified. No Stable/main/flash or trust-key changes.
- [ ] Check anonymous HTTP/1.1 catalogue, signed manifest, complete BIN, SHA256SUMS and app-only package integrity with firmware-compatible limits. Report URLs, exact commit and remaining owner tests: OTA/restart/data retention, all-screen responsiveness and Cover timeout diagnostics.
