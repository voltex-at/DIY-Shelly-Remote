# Shelly Smart Control — visual reference audit

Reviewed 2026-10-15, Europe/Vienna. This branch is a design review, not a release.

Primary sources inspected:

- [Manufacturer app page](https://www.shelly.com/pages/shelly-app)
- [Manufacturer's detailed app screenshots](https://us.shelly.com/pages/shelly-smart-control)
- [Google Play listing and screenshots](https://play.google.com/store/apps/details?id=cloud.shelly.smartcontrol&hl=en) — listing reports an update on 2026-10-12; public promotional screenshots are not proof of the exact appearance of every installed app version or experimental layout.
- [My Home guide](https://kb.shelly.cloud/knowledge-base/my-home-page)
- [Shelly 1PM control guide](https://kb.shelly.cloud/knowledge-base/shelly-1-pm-device-smart-control)

Observed reference elements: dark personalised background; thin blue active-tab outline; graphite/grey rounded cards; circular room/device images; contextual add/edit/info toolbar; separate round power control; blue ON ring versus grey OFF ring; five-icon bottom bar (dashboard, home, consumption, settings, add). Rooms and all devices are top-level My Home tabs. The device control view separates status/power from additional measurements.

Implementation interpretation for 320×480: keep the reference's hierarchy and two-column density, reduce visible rows rather than shrink controls beyond use, retain 40–44px power targets and a 52px bottom bar. No fake OS clock, battery percentage, weather, cloud status or historical graph is inserted.

The app's photos, logos, icon files, HTML/CSS and proprietary implementation are not distributed here. Circular room/device pictograms are original geometric LVGL drawings; a neutral dark gradient replaces the user's custom wallpaper. This is a deliberate asset difference, visible for approval, not a claim of a pixel-identical copy.

Typography is independently sourced Roboto (Apache 2.0), not extracted from Shelly. Source: `googlefonts/roboto-2`, `src/hinted/Roboto-Regular.ttf` and `Roboto-Medium.ttf`. Regular SHA-256: `56a45233d29f11b4dfb86d248e921939d115778f87325e7ae8cc108383d6664d`; Medium: `2879a5ecb7fbfa13a7fc3e2cdd7fecbf73aa45e91b541dfdfa2c442eed0aac21`. Conversion is pinned to `lv_font_conv@1.5.3`; see `tools/build_ui_fonts.py` and `third_party/Roboto-LICENSE`.

Open acceptance decision: layout, visual density, typography, room imagery/pictograms, bottom navigation and control hierarchy. Do not propagate this design to further screens or merge it until the user approves it.
