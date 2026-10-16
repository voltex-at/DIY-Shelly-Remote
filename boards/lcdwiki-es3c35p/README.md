# LCDWiki ES3C35P / pierwszy test sprzętu

Hardware ID: `lcdwiki-es3c35p-n16r8`. Target: `esp32s3`. This page retains the original `0.1.0-bringup` instructions and hardware reference. For the current `0.2.0-local-dev` image and on-screen network workflow use [LOCAL_CONTROL_PL.md](../../docs/LOCAL_CONTROL_PL.md).

Verified compilation: [GitHub Actions 38658803879](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38658803879). [Download the hardware-test artifact](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38658803879/artifacts/11938610486). Source commit: `f592876f532f9de0714c3cd04a074280d57d9278`. The user subsequently supplied photos confirming boot/readable display and touch responses, then explicitly confirmed colors, brightness adjustment and wake on 0.1.0-bringup (2026-10-15, 02:55 Europe/Vienna). Complete corner mapping, first-touch action suppression and long-duration operation remain separate checks. The 0.2.0 network workflow still awaits its physical test; OTA installation is unavailable in both versions.

## Evidence and boundaries

- [Vendor product page and pin allocation](https://www.lcdwiki.com/3.5inch_ESP32-S3_Display)
- [Vendor schematic](https://www.lcdwiki.com/res/ES3C35P/ESP32-S3%E5%8E%9F%E7%90%86%E5%9B%BE.pdf), dated 2026-01-30
- [Vendor specification](https://www.lcdwiki.com/res/ES3C35P/3.5inch_IPS_ESP32-S3_Specification_V1.0.pdf)
- [Vendor manual](https://www.lcdwiki.com/res/ES3C35P/3.5inch_IPS_ESP32-S3_User_Manual.pdf)
- [Exact-board upstream port](https://github.com/78/xiaozhi-esp32/blob/da4bf4feb2f3c343136bd7d19d893ac60e9245b4/main/boards/lcdwiki-es3c35p/lcdwiki-es3c35p.cc), MIT; initialization-table blob `138be5c01b9cb98ba8bec62e899c60ab72af010c`
- [ST77922 driver](https://components.espressif.com/components/espressif/esp_lcd_st77922/versions/2.1.1/readme), Apache-2.0

Documented hardware: 16 MiB QSPI Flash, 8 MiB OPI PSRAM, 320x480 IPS LCD, RGB565, ST77922 over QSPI. The project uses conservative 40 MHz panel QSPI and native portrait orientation. The vendor schematic names a Flash part inconsistently with the product memory specification, so boot checks the detected size. The actual unit/PCB revision has not been inspected yet.

The ST77922 touch block is accessed at I2C address `0x55` using 16-bit register addresses: capability `0x0009`, info `0x0010`, points `0x0014`. That protocol and the module-specific display initialization come from the cited exact-board upstream source; they are not a guessed FT6336 driver. Touch is polled in a separate task; the LVGL read callback only consumes a synchronized sample. If capability probing fails, the screen reports it and automatic screen-off is disabled so diagnostic information stays visible.

## Pin map (original bringup milestone; later battery stage below)

| Function | GPIO | Treatment |
| --- | --- | --- |
| LCD CS / CLK | 10 / 12 | QSPI |
| LCD D0 / D1 / D2 / D3 | 11 / 13 / 14 / 9 | QSPI |
| LCD reset | EN, shared with MCU | No GPIO reset; controller software reset |
| Backlight | 41 | Active high, LEDC PWM; 0 means physically off |
| Touch SDA / SCL | 38 / 39 | Shared I2C, 400 kHz |
| Touch reset / interrupt | 48 / 47 | Reset used; interrupt pin not driven, polling used |
| Addressable RGB LED | 40 | Not initialized in this milestone |
| Battery divider ADC | 8 | Not sampled in this milestone |
| Audio amplifier enable | 1 | Active low; held high/disabled |
| Audio MCLK/BCLK/DOUT/WS/DIN | 17 / 18 / 15 / 21 / 16 | Not initialized |
| SD CLK/CMD/D0/D1/D2/D3 | 5 / 4 / 6 / 7 / 2 / 3 | Not initialized |
| BOOT button | 0 | ROM download behavior retained |
| USB D- / D+ | 19 / 20 | Native USB Serial/JTAG |
| Expansion | 45 / 46 | Untouched; no pogo assignment assumed |

The schematic shows a 100k/100k battery divider (nominal 2:1). No charge-state signal is assumed connected to an MCU pin, so this milestone makes no charge-indicator claim. ESP32-S3 supports Bluetooth LE; the vendor page's BR/EDR wording is not used to claim Classic Bluetooth support.

Battery first stage on the independent 0.7.8 branch: calibrated on-demand GPIO8
voltage, nominal divider 2:1; physical multimeter acceptance pending. No percent
or charging inference. [Scope](../../docs/BATTERY_VOLTAGE_PL.md).

## Build

Use ESP-IDF v5.5.4 and a clean sdkconfig for this profile:

```sh
idf.py -B build-board -D DSR_BOARD=lcdwiki-es3c35p set-target esp32s3
idf.py -B build-board -D DSR_BOARD=lcdwiki-es3c35p build
idf.py -B build-board -D DSR_BOARD=lcdwiki-es3c35p flash monitor
```

Use a separate checkout/configuration when switching from a generic diagnostic build. `set-target` may regenerate sdkconfig. The CI hardware-test job uses a clean checkout, pins LVGL 9.3.0 and ST77922 driver 2.1.1, and uploads the resolved dependency lockfile.

## Pobranie i pierwsza instalacja przez USB

Po pomyślnej kompilacji w GitHub Actions pobierz artefakt `es3c35p-hardware-test-NOT-STABLE` z właściwego commitu. To wersja do sprawdzenia sprzętu, a nie stabilne wydanie. Rozpakuj ZIP i sprawdź `SHA256SUMS.txt`.

Plik `DIY-Shelly-Remote-es3c35p-bringup.bin` jest połączonym obrazem pierwszej instalacji. Wgrywa się go pod adresem **0x0**, wyłącznie do ES3C35P N16R8. Ta instalacja zastępuje dotychczasowy firmware i zawartość objętą obrazem, w tym obszar ustawień. Zachowanie ustawień dotyczy przyszłych aktualizacji OTA, nie tej pierwszej instalacji.

Przykład dla zainstalowanego esptool; zastąp port swoim portem USB:

```sh
python -m esptool --chip esp32s3 --port /dev/ttyACM0 --baud 460800 write_flash 0x0 DIY-Shelly-Remote-es3c35p-bringup.bin
```

Na Windows port ma zwykle nazwę `COM...`. Możesz też użyć oficjalnego Flash Download Tool Espressif: ESP32-S3, plik połączony pod 0x0, Flash 16 MB, DIO, 80 MHz. Użyj przewodu USB-C z transmisją danych. Jeśli automatyczny tryb pobierania nie zadziała: przytrzymaj BOOT, naciśnij i puść RESET, następnie puść BOOT i ponów wgrywanie. Po wgraniu naciśnij RESET.

`diy_shelly_remote.bin` jest obrazem samej aplikacji. Nie wgrywaj go pod 0x0. Oddzielne pliki i `flasher_args.json` służą instalacji z poprawnymi adresami przez narzędzia ESP-IDF. W tym buildzie nie ma jeszcze instalatora OTA.

## Co powinno się pojawić i co sprawdzić

1. Ekran pionowy 320x480 z napisem `DIY SHELLY REMOTE`, `Home`, `Hardware test` i stanem kontrolera dotyku.
2. `Check touchscreen`: naciśnij wszystkie cztery narożniki; komunikat ma wskazywać właściwy numer. Sprawdź także przycisk Back.
3. `Check display colors`: czerwony, zielony, niebieski i biały pasek mają odpowiadać podpisom. Prześlij zdjęcie, jeśli kolory są zamienione lub obraz przesunięty.
4. `Menu -> Settings`: suwak ma płynnie zmieniać podświetlenie. Ustawienie jest na razie tylko w RAM.
5. `Screen off / touch to wake`: podświetlenie ma zgasnąć całkowicie. Pierwsze dotknięcie wyłącznie wybudza; dopiero następne obsługuje przycisk. Automatyczne wygaszenie działa po 60 s, jeśli dotyk został wykryty. To wygaszenie podświetlenia, nie deep sleep MCU.
6. Sprawdź brak restartów przez 10 minut i po kilku cyklach wygaszenia/wybudzenia. Wi-Fi, Bluetooth, urządzenia Shelly i OTA nie są jeszcze aktywne w tym teście.

Log przez USB: monitor ESP-IDF z prędkością 115200 lub terminal USB Serial/JTAG. Oczekiwane wpisy obejmują wersję, `Flash=16777216 PSRAM=8388608`, identyfikator profilu, wynik dotyku przy `0x55`, `UI started` oraz współrzędne przy nowych dotknięciach. Nie ma tu zapisanych haseł Wi-Fi.

Przekaż: zdjęcie ekranu, wynik czterech narożników i kolorów, działanie suwaka/wybudzenia, oznaczenie rewizji PCB oraz pełny log startu. Dopiero wtedy oznaczymy tę płytkę jako sprawdzoną fizycznie.

## Flash layout

NVS: 64 KiB; otadata: 8 KiB; PHY: 4 KiB; `ota_0` at `0x20000`, size 6 MiB; `ota_1` at `0x620000`, size 6 MiB. Both slots fit within 16 MiB; remaining space is deliberately unassigned. Rollback is configured, but installation, signature validation and post-OTA health confirmation remain future work. No stable update can be offered until that path is implemented and tested.
