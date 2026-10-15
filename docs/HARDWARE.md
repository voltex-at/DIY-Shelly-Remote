# Hardware identification

The user selected the [LCDWiki 3.5-inch ESP32-S3 display](https://www.lcdwiki.com/3.5inch_ESP32-S3_Display), SKU **ES3C35P**, on 2026-10-15. Its no-speaker variant is ES3C35P-NS. This is distinct from the earlier 2.8-inch remote.

The board profile and complete pin map are in [boards/lcdwiki-es3c35p/README.md](../boards/lcdwiki-es3c35p/README.md). Vendor documentation confirms ESP32-S3, N16R8 memory, ST77922 QSPI display, capacitive I2C touch and USB Serial/JTAG connectivity. Native portrait orientation is used first; no hardware X/Y swap is assumed.

The source-level port is prepared. Physical PCB revision, observed Flash/PSRAM sizes, display colors and touch orientation still require validation on the user's unit. The boot code checks memory sizes before proceeding with the display. The controller protocol is supported by a cited upstream implementation for this exact board; its success on the user's physical unit remains untested.

Pins for the display, touch, backlight, battery ADC, LED, SD, audio and expansion are documented rather than inferred from enclosure geometry. Pogo accessories have no confirmed electrical assignment in this project and are not driven.

The production Flash layout is no longer guessed: the ES3C35P profile reserves NVS, otadata, PHY initialization and two 6 MiB application slots within the documented 16 MiB. It leaves the rest of Flash unused for now. The layout must be physically validated before stable release. No automatic eFuse programming or flash encryption is enabled.

Additional board profiles must provide their own vendor evidence, pin map, memory configuration, initialization sequence and physical test record.
