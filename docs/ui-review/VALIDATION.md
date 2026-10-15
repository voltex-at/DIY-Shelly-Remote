# UI review validation — 2026-10-15

Local native results:

- 49 review assertions with actual LVGL pointer hit-testing: tap room/device thumbnails, tap power, return through the previous room, and use bottom navigation. Static artwork must not intercept the card's touch target.
- Eight actual 320×480 RGB565 frames, 80 repeated review navigation cycles at a 128 KiB LVGL heap, no sustained free-memory loss.
- DEMO commands never invoke the network callback. Real mode invokes an explicit desired-state callback. Offline controls block the callback and remove unconfirmed measurements.
- The dedicated Wi-Fi/UI integration fixture passed 311 assertions and 60 full navigation cycles; comparable Home-screen heap samples were both 104600 bytes free.
- Existing native/UBSan suites: 94 portable assertions, 371 HTTP/NVS checks, 222 Wi-Fi lifecycle checks. These use fixture transport/storage/events, not physical devices.

CI evidence for the initial proposal source `dced732b0a6216bf4a48d7b9b2349a8932cbe1be`: [run 38722612787](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38722612787), native checks and real-LVGL UI review passed including ASan/UBSan. The final pointer-target correction and updated OFF-current frame have their own local verification and subsequent branch CI. Full target-build results are not implied by a successful native render.

Every committed PNG matches `renders/render-manifest.json`. The PNGs are lossless exports of the renderer framebuffer, not postprocessed screen illustrations. The contact sheet only places six untouched frames side-by-side. The empty real-mode frame has no DEMO data or fabricated sensors.

No physical ES3C35P validation has been reported for this design proposal. The known display, touch and backlight implementation is preserved. The design remains a draft awaiting the user's approval; no stable release or main-branch merge was made.
