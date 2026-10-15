# 0.7.3 — kontrolowana diagnostyka krytycznej awarii startu

Osobna gałąź `work/fix-startup-recovery` na PR #30. Odpowiedź na blokującą uwagę
recenzji: przywrócenie zasobów SRAM w 0.7.2 nie usuwało `ESP_ERROR_CHECK` po
starcie UI. Nie zmieniamy istniejącej paczki 0.7.2 ani jej wyniku testu właściciela.

## Świadoma polityka awarii

Wybrano **pozostanie w diagnostyce USB**, nie automatyczny reset/ponawianie ani
nowy mechanizm wyboru obrazu rollback. Dla zwróconych błędów krytycznych NVS,
płytki, workera, zadania UI oraz podświetlenia app_main:

1. Zatrzaskuje stan DIAGNOSTIC, zapisuje w konsoli etap, kod i internal_free /
   largest / psram_free. Nie zapisuje przyczyny do NVS ani licznika do Flash.
2. Wraca normalnie. ESP-IDF usuwa zadanie główne, a scheduler/pozostałe zadania
   pracują dalej. Nie tworzymy nowego zadania recovery wymagającego wolnego RAM.
3. Nie uruchamia ponownie żadnego etapu. Nie wywołuje esp_restart, erase NVS,
   format SPIFFS, wyboru partycji ani kasowania poprzedniej aplikacji.
4. LVGL nie zaczyna kolejnych timerów/obsługi dotyku. API admin, parowanie i
   zezwolenie OTA są zablokowane; istniejący HTTPS może zwrócić lokalnie 503,
   ale nie stanowi alternatywnego, niezabezpieczonego panelu recovery.
5. Nie potwierdza pending firmware jako zdrowego. Po zebraniu logu operator
   wybiera świadomy odzysk USB; ostatni potwierdzony punkt odniesienia: 0.6.3.

| Stan procesu | LVGL owner | Administracja | Potwierdzenie OTA |
| --- | --- | --- | --- |
| STARTING | Oczekuje, oddaje CPU co 20 ms | 503 / brak parowania | Zabronione |
| READY | Normalne timery i dotyk | Dotychczasowa autoryzacja i lokalność | Dopiero po dotychczasowym teście UI/storage/panel/touch |
| DIAGNOSTIC | Nie zaczyna obsługi UI | 503 / brak parowania | Zabronione |

Zadanie UI nadal ma 8192 B statycznego stosu/TCB w wewnętrznym BSS. Callback
gotowości jest przekazany przed jego utworzeniem; nie ma zależności sterownika
płytki od serwera/transportu. Gotowość zmienia się dopiero po sukcesie tworzenia
zadania i podświetlenia. Nie pozostawiamy aktywnego sterowania pod czarnym LCD.

To nie jest ogólny handler panic: błędy CPU/SDK assert, watchdog, awaria zasilania
lub wcześniejszego bootloadera pozostają poza tą ścieżką zwracanych błędów.
Nie deklarujemy fizycznie sprawdzonego rollback. Istniejąca konfiguracja
bootloadera rollback nie jest wyłączana; pending nie zostaje automatycznie
zaakceptowany podczas diagnostyki. Nie zmieniamy slotów OTA ani NVS/SPIFFS.

Oficjalne podstawy ESP-IDF 5.5:
[powrót z app_main](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-guides/startup.html),
[stany OTA i potwierdzenie](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/system/ota.html).

## Regresje automatyczne

`tools/test.py --sanitize` kompiluje **rzeczywiste main/app_main.c i dsr_boot.c**
z kontrolowanymi adapterami SDK. Test wymusza UI ESP_ERR_NO_MEM (również na
kolejnym modelowanym starcie), UI INVALID_STATE, podświetlenie, worker, NVS i
płytkę. W każdym przypadku main wraca, latch pozostaje DIAGNOSTIC, nie można
go otworzyć spóźnionym finish, a odczyt/mark-valid OTA nie jest wykonywany.
Nowe jawne uruchomienie app_main modeluje kolejny boot, nie automatyczną pętlę.

Sprawdzane są też pending/valid/undefined/new/invalid/aborted, brak partycji,
błąd odczytu stanu, brak rekordu USB i błąd mark-valid z późniejszym retry.
Funkcja zwraca false przy nieudanej próbie, zamiast uznać ją za potwierdzenie.
Nie linkujemy implementacji erase/restart lub zastępczego ESP_ERROR_CHECK.
Oddzielny test rzeczywistej pętli UI dowodzi, że przed gotowością i po zamknięciu
bramki nie jest wywoływany lv_timer_handler; nadal sprawdza 10/15/20 ms w READY.

Te testy są hostowe i nie dowodzą resetu/rollback na układzie ani rzeczywistej
odpowiedzi esp_http_server. Fizyczna bramka pozostaje otwarta. Target CI zachowuje
sprawdzenie adresów ELF puli LVGL/stosu, SDK i wszystkie wcześniejsze testy.
Brak zmian wizualnych; aktualne kadry LVGL 320×480 pochodzą z normalnego CI.

## Test na fizycznym pilocie

1. Najpierw dokończ test 0.7.2 właściciela: pełny łańcuch board_ready →
   worker_ready → wifi_ready → storage_panel_ready → discovery_ready →
   ui_constructed → ui_started, bez resetów. Nie zmieniaj obrazu w połowie próby.
2. Następnie osobno test 0.7.3: ten sam łańcuch, dotyk/jasność/wybudzanie,
   Wi-Fi/LAN/ON-OFF, >=10 BLE cykli i HTTPS; log free/largest/PSRAM bez sekretów.
3. Zachowanie istniejących ustawień i zdjęć przez restart i lokalne OTA.
   Instrukcja USB bez kasowania: BOOT_MEMORY_FIX_PL.md. Nigdy erase-flash.
4. Test awarii na układzie wymaga osobnego, kontrolowanego fault/debug build
   albo debuggera wymuszającego zwrot ESP_ERR_NO_MEM z tworzenia zadania UI.
   Ten produkcyjny development BIN nie zawiera wyzwalacza awarii z UI/API.
   Oczekiwane: jeden DIAGNOSTIC stage=ui_task, brak ui_started, brak pętli reboot,
   brak potwierdzenia pending; odczekać co najmniej 60 s. Zebrać log, następnie
   odzyskać jawnie USB bez kasowania NVS/SPIFFS. Nie prowokować awarii zasilania.
5. Analogiczna kontrolowana awaria podświetlenia nie może uruchomić LVGL/input
   ani zdrowego potwierdzenia OTA. Konkretny test OTA-pending/rollback jest
   osobnym testem sprzętowym; wynik kompilacji nie jest jego substytutem.

Do czasu tych dowodów: Draft, bez merge/main/Stable i bez etykiety Physically
tested. Kontekstowe pomieszczenia #28 wracają jako kolejny etap po tej poprawce.
