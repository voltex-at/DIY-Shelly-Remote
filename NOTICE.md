# Third-party notices

The UI uses Google Material Symbols Rounded under Apache License 2.0.
Pinned SVG sources, hashes and license: `third_party/material-symbols/`.
The generated alpha atlas is a mechanically rasterized/centered derivative.
Review-only room photographs: Nenad Stojković / Shixart1985, CC BY 2.0;
individual source URLs and crop/resize details: `tests/ui/photos/SOURCES.json`.

The UI review adds independently sourced Roboto, licensed under Apache 2.0. Its full license is in `third_party/Roboto-LICENSE`; generated LVGL subsets are in `components/dsr_ui/fonts/`. No font, image, logo, icon file or code was extracted from the Shelly application. New geometric icons are original LVGL drawings. The source audit and reproducible font conversion are documented in `docs/ui-review/SOURCES.md`.

Original project code is under MIT. The module-specific ST77922 initialization table in `components/dsr_board/st77922_init.h` is adapted from the MIT-licensed `78/xiaozhi-esp32` exact-board port (upstream blob `138be5c01b9cb98ba8bec62e899c60ab72af010c`). The full upstream copyright and license are preserved in `third_party/xiaozhi-esp32-LICENSE`. The touch protocol implementation was informed by that source; its register mapping is attributed in the board guide.

Pinned managed dependencies are LVGL 9.3.0 (MIT) and Espressif ST77922 driver 2.1.1 (Apache-2.0). They are downloaded by the build, not vendored. The resolved lockfile is retained with hardware-test artifacts.

ESP-IDF is fetched separately by the build environment. Its main license is Apache-2.0, with separately licensed bundled components. cJSON and FreeRTOS are MIT; lwIP uses a BSD-style license. Final distributions must preserve the relevant upstream notices. Review the exact resolved dependency tree and licenses before publishing binaries.

LVGL is now integrated. Planned NimBLE derives from Apache Mynewt (Apache-2.0) and is not integrated yet. Final board/driver dependency notices must accompany binary distributions.

Upstream license sources:

- https://github.com/espressif/esp-idf/blob/v5.5.4/LICENSE
- https://github.com/espressif/esp-idf/tree/v5.5.4/components
- https://github.com/DaveGamble/cJSON/blob/master/LICENSE
- https://github.com/lvgl/lvgl/blob/master/LICENCE.txt
- https://github.com/apache/mynewt-nimble/blob/master/LICENSE

Shelly is a third-party trademark. No official logo, app graphics or proprietary application code is included. Network API documentation is used as a protocol reference.

Binary development packages also retain license/notice files from the pinned ESP-IDF components (including cJSON, lwIP and radio libraries) and managed components in `licenses/vendor/`. This is an intentionally inclusive SDK notice collection; it does not imply every component is linked or that its license is MIT.
