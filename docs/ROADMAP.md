# Staged delivery

| Stage | Exit condition | Status |
| --- | --- | --- |
| 0 - Foundation and GitHub | Repo, source, status, docs, successful native/diagnostic CI | Completed: public repo, merged foundation, native and both diagnostic CI jobs passed |
| 1 - Identify hardware | Schematic/model, LCD/touch, pin map, Flash/PSRAM, reviewed profile | ES3C35P vendor model, memory and GPIO documented; physical PCB revision pending |
| 2 - Board bring-up | Boot, display, touch, Wi-Fi, stability on physical board | User boot/display/touch response confirmed; colors/brightness/wake/stability pending; Wi-Fi UI implemented, physical test pending |
| 3 - UI | LVGL navigation, dashboard, rooms/devices/details/settings and marked demo | One-switch Home, on-screen Wi-Fi/device forms and hardware tests implemented; full UI pending |
| 4 - First actual Shelly | Manual add, auth as required, state, explicit ON/OFF, offline recovery | One-switch integration, auth configuration and protocol fixtures implemented; actual Shelly test pending |
| 5 - Broader local support | Discovery, live status, capabilities, dimmers/covers/lights, energy, persistence, rooms/scenes | Not started |
| 6 - Bluetooth | NimBLE coexistence, scan, verified BLU/BTHome events, encrypted payloads with keys, applicable BLE RPC | Not started |
| 7 - OTA | Signed compatible update, progress/approval, rollback, preserved settings, local upload | Metadata policy and per-board dual-slot layout; installer pending |
| 8 - Public releases | Model test matrix, optimized memory/power, recovery docs, signed release assets | Not started |

OTA partition planning belongs to board identification, not the last stage. Optional Cloud analysis/import must not delay local control. The first installable build is a prerelease until the actual physical acceptance tests have passed. Version `1.0.0` requires a separately reviewed stability milestone.
