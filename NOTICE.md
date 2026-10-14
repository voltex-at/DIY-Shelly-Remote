# Third-party notices

The source in this archive is original project code under MIT. Third-party sources and firmware binaries are not vendored or distributed in it.

ESP-IDF is fetched separately by the build environment. Its main license is Apache-2.0, with separately licensed bundled components. cJSON and FreeRTOS are MIT; lwIP uses a BSD-style license. Final distributions must preserve the relevant upstream notices. Review the exact resolved dependency tree and licenses before publishing binaries.

Planned LVGL is MIT; planned NimBLE derives from Apache Mynewt (Apache-2.0). Neither has been integrated yet. Board vendor drivers must also be reviewed before inclusion.

Upstream license sources:

- https://github.com/espressif/esp-idf/blob/v5.5.4/LICENSE
- https://github.com/espressif/esp-idf/tree/v5.5.4/components
- https://github.com/DaveGamble/cJSON/blob/master/LICENSE
- https://github.com/lvgl/lvgl/blob/master/LICENCE.txt
- https://github.com/apache/mynewt-nimble/blob/master/LICENSE

Shelly is a third-party trademark. No official logo, app graphics or proprietary application code is included. Network API documentation is used as a protocol reference.
