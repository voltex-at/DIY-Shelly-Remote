# Przegląd funkcji i responsywności po 0.7.73-dev

Data: 2026-10-24. Baza: opublikowany `dev-v0.7.73`, commit `633a38c`.
Gałąź: `work/dev-0.7.70`. Niniejsze poprawki są następnym stanem roboczym;
nie zmieniają opublikowanych plików ani tagu 0.7.73.

Zlecenie: przeanalizować wszystkie rodziny funkcji, a nie tylko termostat
i roletę. Zgłoszenie sprzętowe: po komendzie chwilowe „No response”, szczególnie
długi odczyt TRV. Fizyczne urządzenie reaguje wcześniej niż ekran.

## Naprawione niespójności

| Obszar | Przyczyna i zmiana |
| --- | --- |
| Odczyt TRV przez bramkę | Każde odświeżenie znanego TRV ponownie odkrywało obiekty BTHome. Typowany `BluTrv.GetStatus` zmniejsza liczbę żądań z 8 do 3 przy odświeżeniu oraz z 15 do 5 przy zapisie z odczytem. Weryfikacja bramki/MAC pozostaje; brakujące pomiary są nieznane. Nie dotyczy pierwszego wykrywania ani innych czujników BLU. |
| Roleta/TRV po potwierdzonym zapisie | Jedna utrata GET po poprawnym ACK natychmiast kończyła operację jako offline. Dodano dokładnie jedną próbę GET bez ponownego POST. Druga awaria, 401, 429, błędne dane i przerwanie nadal kończą operację błędem. |
| Opóźniony mirror TRV | Po pierwszym odczycie następowała przerwa 15 s mimo starej nastawy. Maksymalnie trzy wczesne odczyty kończą się po zgodności lub błędzie. Żądana temperatura pozostaje oddzielna od raportowanej. Cel jest kopiowany również dla grup, scen i automatyzacji. Pauza trybu i późniejszy poprawny odczyt nie pozostawiają osieroconego oczekiwania. |
| STOP grupy rolet | Rezerwacja grupowego wykonawcy blokowała również STOP. Zatrzymanie wykorzystuje istniejące skrzynki i pilny kanał, anuluje niewysłane ruchy i zachowuje ekran grupy. Kolejne przyjęte STOP-y czekają na zwolnienie kanału zamiast znikać jako BUSY. |
| Ręczna temperatura | Sekwencja +/− wracająca do bieżącej wartości pomijała ręczne nadpisanie automatyzacji. Jest teraz pełną ręczną intencją, nadal po 1,2 s i po trwałym potwierdzeniu nadpisania. |
| Karty WWW | Niepewny/odrzucony POST mógł trwale blokować roletę, dimmer i RGB. Jawne odrzucenie zwalnia blokadę. Przy utracie transportu dopiero nowy poprawny GET bez oczekującej operacji pozwala na nową jawną intencję. Niepewność pozostaje widoczna; nie ma automatycznego POST. Starszy trwający GET nie odblokowuje karty. |
| Nagłówek baterii | Wejście z edytora usuwało niezapisany draft. Wspólny mechanizm pyta o odrzucenie i zachowuje cel „Bateria”; blokuje przejście podczas zapisu. Kalibracja zachowuje własne zabezpieczenie. |
| Restart po natywnym OTA | Rzeczywisty sukces ma STAGED oraz `boot_selection_attempted=true`; ekran oczekiwał przeciwnego znacznika i pomijał odliczanie. Odliczanie 10→0 i „Później” działają dla rzeczywistego sukcesu, bez zmiany stanu niepewnego wyboru startowego. |
| Podgląd LCD WWW | „Odśwież” zwracało poprzednią klatkę i dopiero zamawiało następną. Żądanie `fresh` unieważnia cache; generacja odrzuca spóźnioną publikację. Jedno kliknięcie czeka na nową klatkę, z limitem 8 s i przerwaniem po zmianie widoku. Parowanie/AP/BLE/ekrany prywatne nadal blokują przechwytywanie. |
| OTA przez WWW | Synchroniczny odbiór BIN zajmował jedyne zadanie HTTPS. Wykorzystuje teraz wspólny z backupem transport 16 KiB, maksymalnie jeden transfer. Zgoda LCD jest jednorazowa; sesja i limit 180 s są sprawdzane podczas odbioru i przed wyborem boot. Przerwanie zwalnia rezerwacje; niepewny wybór boot nadal blokuje dalsze OTA do restartu. |

## Zakres przeglądu całego firmware

Przegląd obejmował wejścia funkcji, właścicieli stanu, kolejki, odczyty,
potwierdzenia, anulowanie i nawigację. Trzy niezależne przeglądy źródeł objęły
sterowanie, utrzymanie systemu oraz UI/sieć; potwierdzone uwagi do poprawek
zostały uwzględnione.

| Rodzina funkcji | Przejrzane ścieżki i wynik |
| --- | --- |
| Przekaźniki, dimmer, RGB | HTTP/typed status, potwierdzanie wyjścia i celu przejścia, fleet, retained UI, blokady kart WWW. Naprawiono blokady WWW; reguły potwierdzania i brak ponawiania zapisu pozostają. |
| Rolety i termostaty | Polecenia, odczyt, historia pozycji, gest/anulowanie, STOP, mirrory i bezpośrednie odczyty TRV. Naprawy powyżej; bezpośredni dowód TRV używany przez ogrzewanie pozostaje osobną ścieżką. |
| Grupy i sceny | Wykonawca, rezerwacje, preflight, tożsamość, kolejność i wyniki kroków. Naprawiono STOP grup i aktualny cel follow-upów TRV. |
| Automatyzacje ogrzewania | Dziennik własności, ręczne nadpisanie, świeżość czujników, wznowienie, trwałość przed zapisem i guard workera. Naprawiono pomijanie ręcznej intencji tej samej wartości. |
| Wi-Fi, AP, LAN, BLE i bramki | `dsr_wifi_*`, reconnect/rollback, skan i anulowanie, `dsr_ap_execute`, `dsr_lan_*`, `dsr_ble_rpc_*`, parowanie/rejestracja BLU, ustawienia urządzeń. Nie potwierdzono dodatkowego błędu wymagającego zmiany; RF i realne bramki pozostają do próby sprzętowej. |
| Bateria, kalibracja, ekran | ADC, profil, korekta, trend, monitoring w tle, ręczny pomiar, sleep/wake. Naprawiono wejście do baterii z draftu; automatyczny pomiar nadal nie imituje dotknięcia. |
| Ustawienia, edytory, zdjęcia | Draft/CAS, potwierdzenie zapisu, zmiana kolejności, `dsr_photo_save`, cache konfiguracji, cofanie. Stopka i górny Back już współdzielą guard; nie wprowadzano zbędnej drugiej poprawki. |
| Aktywność, backup, odtwarzanie, reset | Rejestr RAM, zgody, sesje, szyfrowanie/odbiór, właściciel config, anulowanie i odzyskiwanie. Uogólniono istniejący transport; nie wykonywano resetu, odtwarzania ani operacji na danych pilota. |
| OTA i panel WWW | Natywny updater, lokalny upload, staging, wybór boot, restart, sesje, Origin, podgląd LCD. Naprawy powyżej. Klucze, podpisy, truststore, podział Flash i zasady wydania pozostają. |
| Start, dotyk i LVGL | Zadania i właściciele UI, touch mailbox/filter, flush, quiesce, opóźnione renderowanie i harmonogram odczytów. Priorytety dotyk/UI/sieć 6/5/4 pozostają. Bieżący ekran wyprzedza zwykłe tło; trwające I/O nadal kończy bieżącą wymianę. |

## Walidacja

Testy uruchamiano według zmienianego modułu. Nie generowano pełnego atlasu
ani nie uruchamiano pełnego CI po każdej poprawce. Logi i rendery są lokalne
w ignorowanym `.local/` oraz `build-ui/`.

| Kontrola | Wynik |
| --- | --- |
| Wybrany suite `platform` z `tools/test.py` | PASS: HTTP/parsers/fleet, granice, tożsamość, utracony ACK, GET recovery, brak replay, 401/429/STOP; fleet 8080 B i slot 504 B. |
| `tools/test_automation.py --case runtime --runtime-module native` | PASS: trwała ręczna intencja, oczekiwanie na mirror, pauza trybu, odzyskanie po błędzie, brak ponownego zapisu. |
| Wybrane `runtime_mailbox`, `actions_worker`, `actions_http_uncertainty`, `native_ota_actor` z `tools/test.py` | PASS: rzeczywiste modele skrzynek/aktora/workera, pilny STOP i stany instalatora. |
| `tools/test_ui.py --cases test_priority_ui test_cover_ui test_cover_refresh_ui test_group_stop_ui test_native_ota_ui test_cyber_final_ui test_battery_calibration_ui` | PASS: rzeczywisty przypięty LVGL 9.3, zmienione przepływy, trzy skórki i PL/EN/DE tam, gdzie dotyczy. Obejrzano reprezentatywne rendery. |
| `tests/test_panel_runtime_model.cjs`, `tests/test_panel_config_reads.cjs` | PASS: korelacja poleceń, niepewność, świeżość i generacje, 202→200, wyjście podczas oczekiwania, timeout. |
| `tests/test_panel_runtime.cjs` w Chromium | PASS: rzeczywisty DOM panelu i niezależne sterowanie; sieć/urządzenia modelowane. |
| `tools/test_device_settings.py --case privacy` | PASS: rzeczywista gałąź HTTP fresh/202/200, spóźniona klatka oraz kombinacje prywatności i parowania. |
| `tools/test_ota_upload.py` | PASS: receiver i endpoint, kontrola hash/header/marker, przerwanie sesji/czasu, końcowy guard boot, jednorazowa zgoda; bramki autoryzacji zachowane przez kontrolę źródła. |
| `tools/test_system_data.py --case api` | PASS: rzeczywisty wspólny transport, konkurencja, status podczas oczekiwania, cleanup po BUSY/błędzie taska/błędzie async begin. |
| `ninja -C build-ai -j4`, ESP-IDF 5.5.4 | PASS: ESP32-S3; 29% zapasu partycji aplikacji. |
| Audyty targetu fleet/actions/system_data/native_ota | PASS: istniejące limity RAM/stosu zachowane; utrzymanie systemu 663/4096 B stałego stanu, własne ramki ≤1024 B. |

## Pozostały odbiór na płytce

### Materiał porównawczy właściciela: nagranie rolety

Właściciel potwierdził, że nagranie `84b3fc87-0e71-4bb7-9799-849f967e063a.mp4`
pochodzi z opublikowanego **0.7.73-dev**, a nie z lokalnego commita `5dfd52d`.
Film ma 11,75 s, 576×1024; poniżej czasy przybliżone z obejrzanych klatek.

| Czas filmu | Widoczny stan |
| --- | --- |
| 0–2,25 s | Potwierdzona pozycja 25%; przeciąganie zmienia osobny podgląd grafiki i żądanej pozycji. |
| 2,50 s | Po puszczeniu wraca grafika 25%, pojawia się „Sending command…”; widoczne pomiary nadal 0,0 W / 232,3 V. |
| 6,50 s | „Offline / No response”, „Last position: 25%”, pomiary W/V stają się niedostępne. |
| 9,25 s | Powrót do „Online / Opening”, pozycja 30%, 132,2 W / 232,3 V. |

Pierwszy widoczny nowy odczyt jest około 6,75 s po pojawieniu się wysyłania.
Komunikat błędu jest obecny przez kilka sekund. To dowód zachowania starego
wydania na ekranie, nie pomiar czasu fizycznego startu silnika. Film nie
rozstrzyga, czy nie powiodła się odpowiedź na POST, czy któryś GET.
Lokalne odzyskanie pojedynczego GET po poprawnym ACK nie jest dowodem naprawy
przypadku utraconego ACK; porównanie na kolejnym firmware musi to rozróżnić.
Oryginał i wybrane klatki pozostają lokalne; filmu nie wysyłano na GitHub.

### Próby do wykonania

1. Zmierzyć dotyk→lokalny feedback, kolejkę, odpowiedź RPC i zmianę odczytu
   osobno dla rolety GOTO/STOP i termostatu. Sprawdzić także przekaźnik,
   dimmer, RGB oraz grupy podczas zwykłego odświeżania tła.
2. Sprawdzić zachowanie przy rzeczywistym timeout, utracie Wi-Fi, opóźnionym
   mirrorze TRV i przy powrocie z ustawień. Dwie awarie odczytu nadal mają
   pokazać prawdziwy błąd; ekran nie może udawać potwierdzenia.
3. Sprawdzić ADC bez animowania przycisku oraz świeży podgląd WWW.
4. Przed kolejnym DEV wykonać dokładne CI commita wydania i obowiązkowy
   odbiór pobierania/aktualizacji z `RELEASE_CHECKLIST_PL.md`, w tym lokalny
   upload, przerwanie i restart/dane. Host nie potwierdza pracy Flash ani RF.

W tej rundzie nie flashowano urządzenia, nie wykonywano resetu ani nie
publikowano kolejnego wydania.
