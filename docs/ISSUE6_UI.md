# Issue #6: HOME and icon system review

This is a development proposal, `0.4.1-ui-review`, stacked on PR #5. Do not merge
or mark stable before visual acceptance and the physical tests below.

| Token | Value / behavior |
| --- | --- |
| Canvas / card | #101A22 / #323638 rooms; #41474B–#3C4246 devices |
| Accent / text / muted | #3999F6 / #F5F6F7 / #C3C8CE; persisted user accent still wins |
| Type | independently licensed Roboto; 18px headings, 16px card names, 14px tabs/forms, 12px parameters, 11px badges |
| Layout | 12px outer margin, 8px card gap, 14px card radius |
| Controls | toolbar and tabs 44px; device controls 44–48px; bottom targets 53×47px |
| Active | blue accent and selected-tab outline; powered light has blue-tinted icon medallion |
| Offline | amber status, unavailable measurements and disabled controls; no invented zero |
| Scrolling | vertical list; fixed top shell/bottom bar; no shrinking fonts to add rows |

Icons are **Google Material Symbols Rounded**, Apache-2.0, pinned in
`third_party/material-symbols/sources.json`. The source SVGs and license are
included. `tools/build_ui_icons.py` crops/centers the optical body, renders at
4× coverage, filters to 64×64 A8 and exports a 96 KiB Flash atlas. LVGL software
draw filters the downscale at actual target size. There is no SVG/font/CDN
request at runtime and no Shelly artwork. Pixel tests cover all 24 symbols,
six sizes and four borders (576 cases), including their real hit containers.

## Reference comparison

Reference: the user's Shelly Smart Control 3.77.26 phone screenshot, kept private.
Compare it beside `renders/10-home-room-photos.png` and `renders/01-home.png`.

| Reference composition | Project result |
| --- | --- |
| Two top tabs, blue outline on My Dashboard | Same hierarchy, translated labels, 44px height |
| Graphite dashboard toolbar, plus/pencil/info | Shared toolbar, larger touch areas for 320px |
| Two-column photo room cards, name, dark power pill | Configurable HOME room tiles, 144×130px, real photo crop and honest readings/counts |
| Wide device row, icon left, data center, control right | Separate full-width light/contact/cover variants; dark W/V/battery/position strip |
| Six bottom icons and blue add circle | Same navigation hierarchy with independently licensed rounded symbols |

Deliberate differences: no Shelly logo; native 320×480 resolution uses shorter
headings, vertical scrolling and 16px room names. DEMO telemetry is test-only.
Room photos are real photographs by **Nenad Stojković / Shixart1985**, CC BY 2.0.
Their exact file pages, license and crop/resize/RGB565 changes are recorded in
`tests/ui/photos/SOURCES.json`. These fixture images never enter firmware.

## Reproduce and inspect

```sh
python3 tools/test_ui.py --review
python3 tools/test_ui.py
python3 tools/render_review.py --no-run
```

The runner uses the same pinned LVGL 9.3.0 software draw and project C as firmware.
It tests typed control dispatch, no DEMO network commands, offline controls,
navigation, maximum card count and memory across repeated navigation. The
integrated fixture exercises real Wi-Fi screens, manual wizard and touch editor.
DE/EN renders translate interface labels; user-defined device/room names remain
unchanged. A target build is reported separately by GitHub Actions.

## Physical acceptance on ES3C35P — not yet performed

Use the separate-file USB installation from `CONFIG_WEB_PL.md`; retain NVS.
Record artifact source SHA, board revision, free heap/PSRAM and actual Shelly
model/firmware. This test needs the user's physical remote.

1. Boot and photograph HOME at normal brightness, with both rooms and devices.
   Check rounded glyph edges, readable 12–18px text, blue/gray colors and touch.
2. Pair the HTTPS panel after checking its certificate fingerprint. Upload the
   four attributed sample JPGs from `tests/ui/photos/`, or your own images.
3. Create four rooms and two dashboards. Add half-width room tiles to one and
   full-width device tiles to the other. Save; change dashboards via its toolbar
   title. Verify the same order/appearance on touch and web.
4. Open a room and a real device. Check back navigation, controls, offline state
   and scroll. Cover detail can be visually inspected with DEMO on the host;
   physical cover control is not claimed without its transport implementation.
5. Reboot and disconnect/reconnect power after a completed save. Confirm room
   photos, order and active dashboard return. Repeat with 8 room photo references
   and inspect free PSRAM, scrolling and repeated navigation.
6. Switch PL/EN/DE. Check Wi-Fi password screen, add-device wizard, manual form,
   editor options, diagnostics and error messages for clipping/mixed language.
7. Separately execute the configuration-race, photo-GC and OTA interruption
   procedures from their fix PRs. A successful CI run does not pass these steps.

Report photos and observed failures against the exact SHA. Issue #6 stays open
and the PR stays draft until the user accepts the physical result.
