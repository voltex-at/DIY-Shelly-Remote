# LCDWiki ES3C35P N16R8 — hardware reference

Hardware ID: `lcdwiki-es3c35p-n16r8`. Target: `esp32s3`. Installation:
[FACTORY (0x0)](../../docs/INSTALL_FACTORY_PL.md) and
[UPDATE / OTA](../../docs/INSTALL_UPDATE_PL.md). Build: [DEVELOPMENT.md](../../docs/DEVELOPMENT.md).

## Evidence and boundaries

- [Vendor product page and pin allocation](https://www.lcdwiki.com/3.5inch_ESP32-S3_Display)
- [Vendor schematic](https://www.lcdwiki.com/res/ES3C35P/ESP32-S3%E5%8E%9F%E7%90%86%E5%9B%BE.pdf), dated 2026-01-30
- [Vendor specification](https://www.lcdwiki.com/res/ES3C35P/3.5inch_IPS_ESP32-S3_Specification_V1.0.pdf)
- [Vendor manual](https://www.lcdwiki.com/res/ES3C35P/3.5inch_IPS_ESP32-S3_User_Manual.pdf)
- [Exact-board upstream port](https://github.com/78/xiaozhi-esp32/blob/da4bf4feb2f3c343136bd7d19d893ac60e9245b4/main/boards/lcdwiki-es3c35p/lcdwiki-es3c35p.cc), MIT; initialization-table blob `138be5c01b9cb98ba8bec62e899c60ab72af010c`
- [ST77922 driver](https://components.espressif.com/components/espressif/esp_lcd_st77922/versions/2.1.1/readme), Apache-2.0

Documented hardware: 16 MiB QSPI Flash, 8 MiB OPI PSRAM, 320x480 IPS LCD, RGB565, ST77922 over QSPI. The project uses conservative 40 MHz panel QSPI and native portrait orientation. The vendor schematic names a Flash part inconsistently with the product memory specification, so boot checks the detected size. The actual unit/PCB revision has not been inspected yet.

The ST77922 touch block is accessed at I2C address `0x55` using 16-bit register addresses: capability `0x0009`, info `0x0010`, points `0x0014`. That protocol and the module-specific display initialization come from the cited exact-board upstream source; they are not a guessed FT6336 driver. Touch is polled in a separate task; the LVGL read callback only consumes a synchronized sample. If capability probing fails, the screen reports it and automatic screen-off is disabled so diagnostic information stays visible.

## Pin map

| Function | GPIO | Treatment |
| --- | --- | --- |
| LCD CS / CLK | 10 / 12 | QSPI |
| LCD D0 / D1 / D2 / D3 | 11 / 13 / 14 / 9 | QSPI |
| LCD reset | EN, shared with MCU | No GPIO reset; controller software reset |
| Backlight | 41 | Active high, LEDC PWM; 0 means physically off |
| Touch SDA / SCL | 38 / 39 | Shared I2C, 400 kHz |
| Touch reset / interrupt | 48 / 47 | Reset used; interrupt pin not driven, polling used |
| Addressable RGB LED | 40 | Not initialized in this milestone |
| Battery divider ADC | 8 | On-demand calibrated voltage (2:1 divider) |
| Audio amplifier enable | 1 | Active low; held high/disabled |
| Audio MCLK/BCLK/DOUT/WS/DIN | 17 / 18 / 15 / 21 / 16 | Not initialized |
| SD CLK/CMD/D0/D1/D2/D3 | 5 / 4 / 6 / 7 / 2 / 3 | Not initialized |
| BOOT button | 0 | ROM download behavior retained |
| USB D- / D+ | 19 / 20 | Native USB Serial/JTAG |
| Expansion | 45 / 46 | Untouched; no pogo assignment assumed |

The schematic shows a 100k/100k battery divider (nominal 2:1). No charge-state signal is assumed connected to an MCU pin, so this milestone makes no charge-indicator claim. ESP32-S3 supports Bluetooth LE; the vendor page's BR/EDR wording is not used to claim Classic Bluetooth support.

## Flash layout (`partitions.csv`)

| Partition | Offset | Size |
| --- | --- | --- |
| nvs | 0x9000 | 64 KiB |
| otadata | 0x19000 | 8 KiB |
| phy_init | 0x1b000 | 4 KiB |
| ota_0 | 0x20000 | 6 MiB |
| ota_1 | 0x620000 | 6 MiB |
| dsrdata (SPIFFS) | 0xc20000 | 0x3e0000 (3968 KiB) |

App rollback is enabled; a new image confirms its boot only after the UI is ready.
The partition table must not change without a separate, explicit owner decision.
