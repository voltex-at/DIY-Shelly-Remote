# 0.7.2 — naprawa regresji startu 0.7.1

Gałąź `work/fix-071-boot-memory`, osobny draft PR oparty na PR #27. Bez scalenia
main i bez publikacji Stable. Kompilacja/test hosta nie jest testem płytki.

## Przyczyna potwierdzona logiem użytkownika

Użytkownik potwierdził, że na tej samej płytce 0.6.3 działa, a 0.7.1 daje czarny
ekran. Log wersji `0.7.1-feature-audit-dev` zawiera:

```text
es3c35p: Flash=16777216 PSRAM=8388608
es3c35p: ST77922 touch responding, max points=5
SPIFFS: mount failed, -10025
ESP_ERROR_CHECK failed: esp_err_t 0x101 (ESP_ERR_NO_MEM)
expression: dsr_board_start_ui_task()
Rebooting...
```

Brakuje wewnętrznego RAM do dynamicznego utworzenia stosu zadania UI po starcie
usług. Następuje abort przed włączeniem podświetlenia. Nie jest to błąd adresu
wgrania, brak PSRAM ani nieudana inicjalizacja LCD/dotyku. Dodanie BLE zwiększyło
budżet pamięci ponad działający punkt odniesienia; samo radio nadal startuje
dopiero na żądanie skanowania. Film i pełny prywatny log/MAC nie są publikowane.

## Zmiany

- Pula LVGL 128 KiB: z wewnętrznego BSS do PSRAM przez fragment linkera dla
  `lv_mem_core_builtin` z przypiętego LVGL 9.3.0. Rozmiar puli/allocator bez zmian.
- Stos UI 8192 B i TCB: statyczne wewnętrzne BSS, nigdy stos w PSRAM. Wymagany
  obszar nie może zostać przejęty przez Wi-Fi, mDNS lub BLE przed startem UI.
- DMA LCD pozostaje w wewnętrznym RAM. Bez zmian pinów, sterowników, dotyku,
  podświetlenia, transportów, funkcji skanowania BLE ani tabeli partycji.
- `dsr_boot`: pamięć internal_free/largest/psram_free po inicjalizacji płytki,
  workera, Wi-Fi, magazynu/panelu, discovery, konstrukcji i starcie UI.
- Montowanie SPIFFS nie formatuje przy NO_MEM/INVALID_STATE ani niepustej lub
  nieczytelnej partycji. Tylko potwierdzone całkowite 0xff pozwala na pierwsze
  utworzenie systemu plików. Log pokazuje konkretny błąd format/read/remount,
  zamiast zastępować go wcześniejszym ESP_FAIL. Błąd SPIFFS z 0.7.1 nie dowodzi
  uszkodzenia danych; nie kasujemy ich automatycznie.

Źródła zasad pamięci:
[ESP-IDF 5.5 / PSRAM i extram_bss](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-guides/external-ram.html),
[FreeRTOS IDF / statyczne zadania i rozmiar stosu w bajtach](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/system/freertos_idf.html).

## Dowody automatyczne

`python3 tools/test.py --sanitize` wykonuje rzeczywisty moduł zadania UI oraz
montowania magazynu z kontrolowanymi błędami: statyczny stos/TCB, retry po błędzie,
blokada drugiego startu, clamp 10/15/20 ms; istniejący FS, NO_MEM, INVALID_STATE,
brak partycji, błąd odczytu, niezerowy ostatni bajt, pełny blank check, błąd
formatowania/remount i niepełny ostatni blok. Nie są to pomiary radiowe/płytki.

CI ES3C35P sprawdza prawdziwy ELF: pula 131072 B mieści się w granicach
`_ext_ram_bss_start/end`, stos 8192 B i TCB w `_bss_start/end`. Błędny fragment
linkera lub przypadkowe przeniesienie stosu do PSRAM kończy job błędem.
`BOOT_MEMORY.json`, ELF, map i efektywny sdkconfig są zachowane jako osobny artefakt
debugowania. ELF/map nie są plikami do flashowania. Zachowano dotychczasowe testy
LVGL/BLE/konfiguracji/przeglądarki. Brak zmian wizualnych wymagających nowej makiety.

## Instalacja bez kasowania ustawień

Nie używaj `erase-flash` ani zaznaczenia ESPConnect
**Gesamten Flash vor dem Schreiben löschen**. Jeżeli poprzednie wgranie użyło
tej opcji, wcześniejsze ustawienia mogły już zostać usunięte — firmware ich nie
odtworzy. Nie formatuj SPIFFS na podstawie samego komunikatu mount failed.

Dla zachowania NVS użyj osobnych plików z jednej paczki, bez kasowania Flash:

| Plik | Adres USB |
| --- | --- |
| `bootloader/bootloader.bin` | `0x0` |
| `partition_table/partition-table.bin` | `0x8000` |
| `ota_data_initial.bin` | `0x19000` |
| `diy_shelly_remote.bin` | `0x20000` |

Przy obecnej instalacji z obrazu scalonego, **jeśli log potwierdza aktywny
slot `0x20000` i identyczną tabelę z 0.6.3/0.7.1**, wystarcza sama aplikacja
`diy_shelly_remote.bin` pod `0x20000`. Nie stosuj tej skróconej procedury po OTA
do innego slotu bez sprawdzenia aktywnej partycji.

Obraz scalony `DIY-Shelly-Remote-es3c35p-local.bin` jest wyłącznie pod **0x0**;
nawet bez pełnego erase nadpisuje obszar NVS zawarty w przerwach obrazu. To opcja
świeżej instalacji, nie procedura zachowania ustawień. Nigdy nie wysyłaj go przez
OTA; OTA przyjmuje wyłącznie aplikację po fizycznym zezwoleniu na pilocie.

## Test wymagany na ES3C35P

1. Start USB: log nowej wersji, brak resetów, `dsr_boot: ui_started` i widoczny
   ekran. Zachowaj liczby pamięci wszystkich etapów; nie publikuj SSID/MAC/haseł.
2. Kontroler dotyku, kolory, jasność i dotykowe wybudzanie. Pierwszy dotyk po
   wygaszeniu nie może wysłać komendy urządzenia.
3. Wi-Fi, realny LAN discovery, odczyt i ON/OFF znanego Shelly, polling bez
   migania. Następnie >=10 BLE start/cancel/timeout/retry z Wi-Fi i bez Wi-Fi.
   Obserwuj brak rebootów i stabilność internal_free/largest/psram_free po
   pierwszym lazy starcie NimBLE. Nie uznawaj wykrycia za parowanie/provisioning.
4. SPIFFS: przy istniejących danych brak autoformatowania; konfiguracja/zdjęcia
   oraz sparowanie panelu zachowane przez reset. Przy potwierdzonej pustej
   partycji pierwsze utworzenie FS, zapis konfiguracji i odczyt po restarcie.
   Nie kasuj posiadanych danych, żeby sztucznie wykonać przypadek pustej partycji.
5. HTTPS/podgląd i lokalne OTA po zezwoleniu na pilocie; zachowane dwie partycje
   6 MiB, ustawienia i rollback. Awaria magazynu ma pozostać widoczna w logu,
   nigdy powodować automatycznego usunięcia danych.

Do akceptacji potrzebny jest wynik z fizycznej płytki. Udany CI nie zamyka tej
regresji sprzętowej. Do czasu potwierdzenia punktem powrotu jest działająca 0.6.3.
