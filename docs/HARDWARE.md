# Hardware confirmation / Dane naszej płytki

Known: a custom remote uses an ESP32-family board with a nominal 3.5-inch touchscreen. No working firmware for this board was provided. The screen diagonal and project name do not identify the PCB.

| Required item | Current value | Evidence needed |
| --- | --- | --- |
| Manufacturer, exact board/PCB model and revision | Unknown | Product link, board markings and vendor schematic |
| SoC and module variant | Unknown | Module marking or vendor BOM |
| LCD resolution and usable orientation | Unknown | Panel specification and enclosure orientation |
| LCD controller and interface | Unknown | Vendor schematic and working example |
| Touch controller, bus, reset/interrupt | Unknown | Schematic and example |
| GPIO map and shared buses | Unknown | Schematic including strapping/Flash/PSRAM pins |
| Flash capacity/mode | Unknown | Module spec and boot log/esptool output |
| PSRAM capacity/mode | Unknown | Module spec and vendor sdkconfig |
| Backlight enable/PWM polarity and range | Unknown | Schematic |
| Power management, battery ADC and charge detection | Unknown | Schematic and electrical values |
| Buttons, LED type/polarity, USB pins | Unknown | Schematic |
| Pogo accessory pin assignments | Unknown | Electrical specification, not enclosure geometry |

**Prośba do właściciela:** prześlij link do dokładnej płytki lub dokumentację producenta. Jeśli oznaczenie jest niepewne, zdjęcia obu stron PCB i napisów na module pomogą je ustalić. Nie wnioskujemy parametrów z samego rozmiaru ekranu.

## Board port acceptance

Store a reviewed profile in `boards/<vendor-model-revision>/` with source links, MCU target, sdkconfig defaults, schematic-derived pin table and initialization sequence. Implement the `dsr_board` boundary, then confirm display/backlight/touch, power and reset behavior on the actual board. Validate any vendor pin conflict or boot strap before driving pins.

Do not implement OTA partition sizes until Flash capacity is confirmed. Reserve NVS, otadata, two suitably sized app slots and any required assets; document USB recovery and whether the first installation needs a layout migration. A chip target alone is not a board profile.
