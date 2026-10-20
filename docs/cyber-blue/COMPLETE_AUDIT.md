# Cyber Blue Premium - complete route audit

Base: `e201332d8d0b5a96f4cdb70ceb96b20d26f247ad` (`0.7.62-dev`), branch `work/shelly-hud-cyber-blue`.
The default `main` branch is older and was not used as the implementation base.
The supplied master prompt and visual references govern this change. Earlier
conversation retrieval returned `Personal context is disabled`; no claim of a
complete conversation search is made.

## Implementation

- Existing relay, shutter, BLU and TRV work is retained. Shutter opening remains
  0% closed / 100% open, interpolating between confirmed samples only. BLE
  cleanup/restart changes from 0.7.62-dev are retained.
- Every discovered Shelly settings group now shares the icon / name / current
  value / chevron composition. Groups, fields, edits and confirmations all use
  the same header and spacing. Discovery still gates available settings.
- Light/Dimmer: separated telemetry, brightness dial, dedicated minus/plus,
  release-only slider writes, existing power and refresh actions. RGB has a
  color preview and individually colored channels with explicit Apply.
- H&T: separate temperature/humidity cards, battery, power source and read
  action. Offline data is represented as unknown. Long sleep guidance scrolls
  above the navigation bar.
- Consumption: cyan range gauge, separate measured kWh, V and A cards, explicit
  freshness. No severity thresholds, energy history, scenes or white channel
  capabilities were invented.
- Display preferences receive grouped brightness/timeout/status panels;
  pairing revocation receives the destructive action treatment in Cyber only.
- Graphite/Midnight/Light IDs, preferences and rendering branches are retained.
  No protocols, OTA trust, partitions, radio code or persistent data formats
  were changed. No release or merge to main is part of this change.

## Coverage

`source-inventory.json` lists presentation entry points found in the C sources;
these include shared helpers and are not a count of distinct screens.

| Family | Captured views |
| --- | --- |
| Navigation | Home, rooms, room details, devices, dashboard selection, empty/list states |
| Devices | Relay on/off/pending/no response; shutter closed/half/open/stopped/moving/offline; BLU window/door/unknown/error/help; TRV setpoint/dial/offline/battery; dimmer/RGB/H&T |
| Shelly | Sys, WiFi, BLE, Cloud, MQTT, Switch, Input, Cover, Light, RGB, RGBW, Temperature, Humidity; field lists and scrolls; text/number/bool/enum/secret forms and confirmations |
| Editors | Room create/edit/photo/delete/keyboard, device card/icons, dashboard tiles/fields/reorder, registry, groups/scenes and their name/select/review forms |
| Network | WiFi list/password/validation, add method/manual IP, LAN scan/results/auth/components/offline, factory AP scan/connect/disclosure, BLE scan/results/details/cancel/empty/unavailable, BLE RPC disclosure/cleanup, BLU pairing/PIN/settings/forget |
| System | Boot splash, appearance/dropdowns, comfort/display, battery/read/error/stale/memory, HTTPS/pair/expiry/revoke, diagnostics/configuration errors, restart confirmation, touch/color tools, OTA errors/progress/warnings, first-storage preparation/resume/failure |

Frames are tagged while the actual production theme is Cyber, before any test
badge is drawn. The exporter includes every tagged capture, merges only exact
pixel duplicates and records all aliases and SHA-256 hashes. It creates a full
native-size board, a multipage PDF, a searchable HTML index, PNGs and a ZIP.
This is not an AI image or a browser recreation of firmware UI.

## Reproduction

```
python3 tools/test_ui.py --ubsan-only
python3 tools/render_cyber_atlas.py --screens build-ui/ubsan/screenshots
```

CI runs the full native LVGL suite with ASan/UBSan and exports the same atlas,
plus the existing native, browser, diagnostic and ESP32-S3 checks. Results must
be read on the exact candidate SHA. Host radio/storage/ADC values are fixtures.
No physical LCD, touch timing, power-loss, BLE RF or target runtime RAM/PSRAM
acceptance is claimed. Visual references have more detail than a 320x480 LCD;
production components use lightweight outlines, gradients and A8 assets.
