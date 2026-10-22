# Stan prac i jeden backlog po porządkowaniu — 2026-10-22

Zlecenie właściciela: przejrzeć lokalny projekt i wszystkie gałęzie, zebrać zlecenia oraz wykonane prace, wskazać niespójności, zastąpić stare zgłoszenia jednym issue i usunąć stare wyniki/rendery. Ten etap nie dodaje funkcji firmware.

## Baza dalszej pracy

- Repozytorium: `voltex-at/DIY-Shelly-Remote`, lokalnie `C:\Users\franz\DIY-Shelly-Remote`.
- Aktualna gałąź: `work/dev-0.7.70`, źródło wydania `d43da439f757abaffe7363a8622e90b175882b29`.
- [Opublikowany DEV 0.7.70](https://github.com/voltex-at/DIY-Shelly-Remote/releases/tag/dev-v0.7.70), pięć publicznych zasobów.
- Zachowane dowody bieżącego wydania: [CI 5/5](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/39782898484), [Signed release](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/39782898458).
- `main` nadal jest starszą bazą `51c5840`; nie jest scalany ani usuwany podczas porządków. Starsze opublikowane tagi i zasoby OTA pozostają niezmienne.

## Wynik porządków

- Przejrzano wszystkie 62 zdalne gałęzie; usunięto 60. Zostały `main` i `work/dev-0.7.70`. Usunięto również dwie nieaktywne gałęzie lokalne.
- Utworzono [issue #74](https://github.com/voltex-at/DIY-Shelly-Remote/issues/74), zamknięto 17 starych issues i 7 PR-ów bez scalania. Zamknięcie oznacza konsolidację, nie odbiór wszystkich funkcji.
- Usunięto 379 historycznych przebiegów Actions. Lista artefaktów spadła z 1951 do zera; ich początkowy rozmiar według metadanych wynosił 15 526 146 417 B. Pozostały dwa udane przebiegi bieżącego wydania.
- Lokalnie usunięto 13 723 pliki starych wyników, buildów i zbędnych cache: 2 958 286 173 B. W tym 192 wersjonowane obrazy/media oraz nieaktualne raporty i żądania wydania.
- Zarchiwizowano osiem rozmów projektu ChatGPT „DIY Shelly Remote”. Widok projektu potwierdza brak aktywnych rozmów; bieżący czat Codex pozostaje.
- Zweryfikowano zachowanie 28 publicznych wydań, 130 plików wydania i zgodność 29 lokalnych/zdalnych referencji tagów. Kod firmware, testy źródłowe, generatory, workflowy, aktywne toolchainy i materiały właściciela pozostają.

## O co proszono i co rzeczywiście jest w kodzie

„W kodzie” nie oznacza odbioru na fizycznym ES3C35P. Lista łączy #6–#66, ustalenia z rozmów projektu, konfigurację środowiska, kontynuację Claude 0.7.68 oraz poprawki 0.7.69–70.

| Zlecenie | Wykonanie / ograniczenie | Główne źródło |
| --- | --- | --- |
| Środowisko Windows, ESP-IDF i cztery skills | Zainstalowane i opisane; Espressif MCP oraz lokalny lvgl-project dostępne. Zachować toolchain i tools/dev. | `docs/ESP32_AI_SETUP_PL.md; .agents/skills; tools/dev` |
| Piny sprzętu i bibliotek | ES3C35P N16R8, ESP32-S3, 320×480; ESP-IDF 5.5.4 / LVGL 9.3.0 / ST77922 2.1.1 / mDNS 1.8.2. | `CMakeLists.txt; components/dsr_board/idf_component.yml; components/dsr_platform/idf_component.yml` |
| Trzy skórki | Cyber Blue/Grafit/Jasny; Midnight wycofany z selektora, migracja starych ustawień zachowana. | `components/dsr_ui/dsr_ui_style.c; components/dsr_config/dsr_config.c` |
| Status, bateria, zegar, odstępy i nawigacja | Poprawki 0.7.68–69: procent wewnątrz baterii, dom/grid/CPU, przewijanie ustawień, długie nazwy. Odbiór fizyczny aktualnego UI nadal otwarty. | `components/dsr_ui/dsr_ui_style.c; components/dsr_ui/dsr_ui.c` |
| Pulpit i karty zgodne z ostatnimi zdjęciami | 0.7.70: jeden wiersz wyboru/akcji, zwarte karty, pomieszczenie•stan, pełne pomiary, termostat/okno i duży włącznik. Późniejsze zlecenie zastępuje część układu 0.7.69. | `components/dsr_ui/dsr_ui_mobile.c; docs/RELEASE_NOTES.md` |
| Pomieszczenia i edycja | Dodaj/Edytuj/Usuń, stabilne ID, trwały zapis; 0.7.70 poprawia kafle i liczniki. | `components/dsr_ui/dsr_ui_config.c; components/dsr_ui/dsr_ui_mobile.c` |
| Klawiatura i dotyk | Większe znaki, wspólny keyboard, podgląd klawisza, obsługa przerwanego dotyku i przewijania. | `components/dsr_ui/dsr_ui.c; components/dsr_board` |
| Wiele Shelly i usuwanie kanałów | Rejestr/scheduler, oddzielny stan/błędy, priorytety i backoff; usuwanie właściwego ID/kanału wraz z odwołaniami. | `components/dsr_platform/dsr_fleet.c; components/dsr_ui/dsr_ui_network.c` |
| LAN, nazwa zamiast SKU i profile | Kreator LAN, weryfikacja tożsamości, lokalna nazwa ma pierwszeństwo, komponenty/profile i kanały. Gen1 wycofany z zakresu. | `components/dsr_platform/dsr_discovery_model.c; components/dsr_ui/dsr_ui_lan.c` |
| Wi-Fi AP | Istnieje skan/wybór, dwie zgody, provisioning i przywrócenie LAN; retry nie odtwarza POST hasła. Ramki treści Cyber Blue dodane 0.7.69. | `components/dsr_platform/dsr_ap.c; components/dsr_ui/dsr_ui_ap.c` |
| BLE i BLU | Skan ESP-NimBLE, otwarty setup Wi-Fi, osobne bonding/PIN/GATT/bramka dla wspieranych klasycznych BLU. RF/SMP na aktualnej wersji wymaga odbioru. | `components/dsr_platform/dsr_ble.c; components/dsr_platform/dsr_ble_rpc.c; components/dsr_platform/dsr_blu.c` |
| Rolety i STOP | Cover RPC, pozycja/gest, potwierdzony odczyt, priorytet STOP; wspólna animacja trzech skórek, 0%=zamknięta/100%=otwarta. | `components/dsr_ui/dsr_ui_mobile_cover_cyber.inc; components/dsr_ui/dsr_ui_cyber.c; components/dsr_platform/dsr_fleet.c` |
| Światło/dimmer/RGB/H&T/TRV | Prawdziwe akcje oraz capability; H&T tylko odczyt, TRV przez bramkę. 0.7.70 przebudowuje Cyber dimmer/RGB, pozostałe skórki zachowują swoje widoki. | `components/dsr_ui/dsr_ui_mobile.c; components/dsr_ui/dsr_ui_mobile_lighting_cyber.inc` |
| Ustawienia urządzenia i Ogólne | Trwała edycja karty oddzielona od świadomych zmian konfiguracji Shelly Gen2/3/4; formularze zależne od rzeczywistych możliwości. | `components/dsr_platform/dsr_device_settings.c; components/dsr_ui/dsr_ui_device_settings.c` |
| Grupy i sceny | Ręczne sceny Switch/Cover oraz grupy Switch, wyniki częściowe, edytory LCD/WWW. Automatyczne reguły/warunki/harmonogramy poza dostarczonym zakresem. | `components/dsr_platform/dsr_actions.c; components/dsr_ui/dsr_ui_actions.c` |
| Aktywność | Backend 128 rekordów RAM, paginacja i API log/clear są obecne. Osobny widok LCD dla pierwotnego #21 nie jest potwierdzony i wymaga dokończenia/decyzji zakresu. | `components/dsr_platform/include/dsr_actions.h; components/dsr_ui/dsr_ui_actions.c` |
| Historia energii i wykresy | Były wdrożone, następnie usunięte decyzją właściciela 0.7.51. Nie przywracać automatycznie na podstawie starych #22/#29. Bieżące pomiary i kontrolki pozostają. | `docs/FEATURE_MATRIX.md; docs/RELEASE_NOTES.md` |
| Panel WWW i języki | Wspólna konfiguracja, pairing/zgody i osobne PL/EN/DE przeglądarki; lokalny podgląd LCD. | `components/dsr_platform/dsr_panel.c; components/dsr_platform/web` |
| Bateria | Kalibracja ogniwa/pojemności/progów/multimetru; ~SOC i trend ≥10 s / 20 mV jako przypuszczalne ładowanie. To nie sygnał fizycznej ładowarki. | `components/dsr_platform/dsr_battery.c; components/dsr_ui/dsr_ui_battery_calibration.c` |
| Wydajność i pamięć | Zachowano poprawki Claude: mniej parsowań/kopii, odłożone renderowanie przy wygaszonym LCD, budżety PSRAM/BSS/stosu i cleanup animacji. | `components/dsr_ui/dsr_ui_network.c; components/dsr_ui/dsr_ui_mobile.c` |
| Wi-Fi/BLE cleanup i restart | Reconnect/backoff oraz bounded BLE cleanup. 0.7.69 dodaje potwierdzony lokalny restart niezależny od aktywnych lease, z RTC watchdog; normalne WWW/OTA zachowują wyłączność. | `components/dsr_platform/dsr_restart.c; components/dsr_platform/dsr_activity.c` |
| OTA z LCD i WWW | Natywny updater, manifest Ed25519, TLS/model/wersja/kanał/hash, dwa sloty i rollback. Instalacja 0.7.53 potwierdzona historycznie; pełny aktualny rollback wymaga sprzętu. | `components/dsr_platform/dsr_native_ota.c; components/dsr_ota; components/dsr_crypto` |
| Bezpieczne wydania DEV | 0.7.68/69/70 opublikowane; tag==commit z CI 5/5, klucz w chronionym środowisku, jeden build/publikacja, bez Stable i bez merge main. | `tools/release_tool.py; .github/workflows/release.yml` |
| Eksport/backup i reset fabryczny | Nadal niewdrożone; system_info jawnie to komunikuje. To zaległość zakresu System z #24/#25, nie gotowa funkcja. | `components/dsr_ui/dsr_ui.c` |
| Odbiór urządzenia | Starsze potwierdzenia dotyczą konkretnych wersji. Zielone host/CI nie akceptuje najnowszego UI, RF/AP, ADC, STOP, reboot ani rollback. | `README.md; dotychczasowe #6/#12/#18/#24/#25` |

## Niespójności i rzeczy do dalszej poprawy

- [ ] **P0: odbiór UI 0.7.70 na urządzeniu**, we wszystkich trzech skórkach: długie nazwy, jedna linia narzędzi, ostatni wiersz ustawień, status/bateria, room edit, przyciski, okno/TRV i dimmer/RGB. Nie zamykać jako gotowe tylko na podstawie liczby asercji.
- [ ] **P0: RF/AP/BLE i restart**: rzeczywisty scan/setup, anulowanie/błędy i powrót do LAN; SMP/PIN/bramka; brak callbacków i restart przy zajętym radiu. Czas RTC watchdoga 0.7.69 nadal nie jest pomiarem urządzenia.
- [ ] **P0: trwałość i sterowanie**: wiele Shelly, usunięcie jednego kanału bez odtworzenia po reboot/WWW, potwierdzony readback, STOP/pozycja, Light/RGB/TRV i sceny na rzeczywistych urządzeniach.
- [ ] **P0: OTA i bateria**: aktualizacja do bieżącego DEV bez utraty konfiguracji/zdjęć, przerwany transfer i rzeczywisty rollback; BAT vs multimetr, ~SOC i wygaszenie animacji ładowania.
- [x] **Dokumentacja bieżąca — poprawiona podczas porządków**: matryca funkcji, przewodnik rozwoju i indeks opisują 0.7.70, aktualny restart i wykonany zakres BLU. Wcześniej mieszały 0.7.67/69 oraz nieaktualne „planned”.
- [ ] **Historyczne wymagania UI są sprzeczne**: cztery vs trzy skórki; grid/dom; osobny wiersz akcji 0.7.69 vs wspólny 0.7.70; dwa wiersze metadanych vs bieżące „Pokój•Stan”. Obowiązują ostatnie zdjęcia/zlecenia, starszych żądań nie realizować ponownie.
- [ ] **Instrukcje lokalne**: AGENTS i skills nadal używają nazw „Shelly HUD — Cyber Blue/Grafitowy”, choć UI używa skróconych nazw. Lokalne `.agents/`, `tools/dev/` i raport setup nie są wersjonowane; ustalić świadomie ich dalsze utrzymanie.
- [ ] **Aktywność**: zachować istniejący backend 128 wpisów i API, ustalić/dokończyć widok LCD pierwotnego #21. Nie pisać drugiego silnika.
- [ ] **Eksport/reset**: pierwotne pełne System #24/#25 nie jest w całości dostarczone; zaplanować osobny przepływ z potwierdzeniem i analizą trwałości danych.
- [ ] **Czas i zabezpieczenie danych**: SNTP istnieje, brak potwierdzonego ekranu wyboru strefy; NVS prototypu nie jest szyfrowane. Decyzje/migracje wymagają oddzielnego etapu, bez zmiany klucza OTA i eFuse przy porządkowaniu.
- [ ] **Stare tłumaczenia**: słowniki zawierają historyczne napisy o „niewdrożonym” AP/BLE/OTA/restart. To kandydaci do audytu użycia, nie dowód, że aktualny ekran pokazuje te komunikaty. Usuń dopiero po sprawdzeniu odwołań.
- [ ] **Gałąź domyślna GitHub**: default `main` pokazuje starą bazę; bieżący kod jest na `work/dev-0.7.70`. Zmiana default/merge to odrębna decyzja, porządki jej nie zastępują.

Wycofane lub poza bieżącym zakresem: historia energii/wykresy (#22 i część #29), Midnight, Gen1; chmurowe automatyzacje, nowe BLU/Zigbee/Matter i automatyczne reguły nie są potwierdzoną pełną implementacją. Nie przywracać ich automatycznie.

## Ankieta odbioru — dalszy etap

Na prośbę właściciela z 2026-10-22 lista staje się ankietą i kolejką bieżących poprawek. Najpierw ustalamy wersję faktycznie zainstalowaną na pilocie. Każdy punkt otrzymuje ocenę właściciela: **działa / wymaga poprawy / brakuje / niesprawdzone**. Obecność implementacji w tabeli powyżej nie wypełnia automatycznie tej oceny.

| ID | Co oceniamy | Ocena właściciela / problem |
| --- | --- | --- |
| UI-01 | Trzy skórki i ich nazwy: Cyber Blue, Grafit, Jasny | Oczekuje na ocenę |
| UI-02 | Status bar: bateria z procentem wewnątrz, zegar i Wi-Fi | Oczekuje na ocenę |
| UI-03 | Nawigacja: dom/grid, odstępy i pola dotyku | Oczekuje na ocenę |
| UI-04 | Ustawienia: ostatni element listy, ikona–tekst–strzałka | Oczekuje na ocenę |
| UI-05 | Dashboard: wybór, długie nazwy, +/edycja/informacje | Właściciel: pulpit i karty wymagają poprawek; szczegóły pierwszej rundy poniżej |
| UI-06 | Karty: odstępy, pełne pomiary, brak pustego „Keine weiteren Messwerte”, główna akcja | Poprawiono roboczo odstęp akcji od dolnej ramki; odbiór otwarty |
| UI-07 | Termostat: odczyt, nastawa i ergonomia −/+ | Runda 4: wyśrodkowanie odczytu i położenie baterii zaakceptowane na podglądzie; dodano wspólną ramkę ikony i procentu baterii. „Soll” w ramce i sterowanie 104×56 jak roleta; odbiór na pilocie otwarty |
| UI-08 | Okno i drzwi: grafika otwartego/zamkniętego skrzydła | Przywrócono duże grafiki po prawej i pojedynczy tekst stanu po lewej; odbiór otwarty |
| UI-09 | Roleta: wspólna animacja trzech skórek, skala, status, pozycja i STOP | Poprawiono odstęp akcji od nawigacji; regresja gestu/STOP przeszła; odbiór otwarty |
| UI-10 | Cyber Blue: turkusowe ramki treści, w tym wynik i zgoda Wi-Fi AP | Wzór dashboardu przeniesiony do wspólnych paneli, akcji i pól; końcowy przegląd całej skórki przed DEV |
| UI-11 | Równy zakres funkcjonalny skórek oraz ekrany dimmera/RGB | Oczekuje na ocenę |
| CFG-01 | Dodaj/edytuj/usuń pomieszczenia, dashboardy, trwałe nazwy i zdjęcia | Oczekuje na ocenę |
| NET-01 | LAN, Wi-Fi AP i BLE: skan, wybór, zgody, błędy, retry i powrót do LAN | Kolejne kroki przeniesiono pod tytuł; etykiety mówią o urządzeniach; poprawiono kolizję RSSI i pomiar szerokości czcionki; odbiór sprzętowy otwarty |
| NET-02 | BLU: bonding/PIN, konfiguracja i odczyt przez bramkę | Oczekuje na ocenę |
| CTRL-01 | Wiele Shelly, readback, usunięcie jednego kanału i trwałość po restarcie | Oczekuje na ocenę |
| CTRL-02 | Switch/Cover/Light/RGB/H&T/TRV oraz ręczne sceny i grupy | Oczekuje na ocenę |
| SYS-01 | Bateria, kalibracja, wybudzanie, Wi-Fi reconnect i restart | Oczekuje na ocenę |
| SYS-02 | Brakujące funkcje: widok Aktywność, eksport/backup i reset fabryczny | Zakres do uzgodnienia; braki opisane powyżej |
| WEB-01 | Panel WWW: konfiguracja, zgody, język i podgląd LCD | Oczekuje na ocenę |
| OTA-01 | Aktualizacja bez utraty danych, przerwany transfer i rollback | Oczekuje na ocenę |

Problemy z ankiety dopisujemy tutaj wraz z wersją, ekranem, skórką i oczekiwanym zachowaniem. Naprawiamy je kolejno; po drobnej zmianie sprawdzamy tylko dotknięty moduł i ekrany. Kolejny DEV powstanie po zakończeniu uzgodnionego zakresu, końcowym przeglądzie UI i pełnej walidacji commita wydania. Nie oznacza to automatycznego wdrażania wycofanych funkcji ani scalania do main.

### Runda 1 — uwagi właściciela z pilota 0.7.70-dev

Potwierdzona zainstalowana wersja: **0.7.70-dev**. Poniższe poprawki powstają w kodzie roboczym; nie są jeszcze nowym wydaniem ani odbiorem na urządzeniu.

1. Karty: co najmniej 12 px odstępu głównej akcji od dolnej ramki.
2. Szczegóły rolety: co najmniej 16 px odstępu akcji od nawigacji, z zachowaniem rozdzielenia podglądu od potwierdzonej pozycji, wysyłania po puszczeniu oraz STOP.
3. **Tylko karta termostatu na Dashboardzie**: ramka 104×56 px i dwa przyciski 44 px, jak sterowanie rolety. Temperatura bieżąca i nastawa pozostają widoczne. Niemiecki podpis: **Soll**. Pierwotną uwagę o usunięciu baterii z karty zastępuje późniejsze ustalenie z rundy 2 poniżej.
4. Okno i drzwi: duża grafika prawego panelu dla obu stanów. Nie powielać napisu „Otwarte/Zamknięte” w prawym panelu; tekst pozostaje przy pomieszczeniu.
5. Cyber Blue: sfazowane turkusowe obramowania dashboardu przenieść do wspólnych paneli, akcji i pól; zachować czytelne czerwone/żółte stany oraz własny wygląd pozostałych skórek.
6. Kreator: **pierwszy ekran z „Krok 1 z 3” jest zaakceptowany**. Dopiero kolejne ekrany wyszukiwania i konfiguracji mają mieć numer kroku pod nagłówkiem, zamiast obok zegara. Przesunąć treść AP/BLE/LAN, aby uniknąć nowej kolizji.
7. Etykiety dodawania BLE mają mówić o urządzeniach: **Urządzenie/Gerät/Device**. Zachować odrębne zgody oraz komunikaty o niepotwierdzonej tożsamości i niesparowanym urządzeniu.

Stan tej rundy: powyższe poprawki są w kodzie roboczym. Oceny właściciela w tabeli pozostają otwarte do obejrzenia podglądu i późniejszego odbioru na pilocie.

Walidacja: `test_dev_069_ui`, `test_cover_ui` i `test_cyber_ui` przeszły na przypiętym LVGL 9.3. Sprawdzono wybrane karty/kreator, oba stany okna i drzwi, napisy PL/EN/DE, wysyłanie pozycji po puszczeniu, priorytet STOP, natywne kontrolki i 24 kafelki w niezmienionej puli 128 KiB. Przełączanie skórek nie wykazało wzrostu użycia pamięci. To testy hosta z modelami radia i zapisu. Pełny atlas, build płyty i pełne CI należą do końcowej walidacji przed następnym DEV.

### Runda 2 — karta termostatu po obejrzeniu podglądu

Ostatnie ustalenie właściciela: **„Soll” i temperatura zadana razem w ramce; wolne miejsce po lewej wykorzystać na ikonę baterii oraz jej poziom**. Dotyczy karty na Dashboardzie, zastępuje wcześniejsze usunięcie baterii z tej karty.

- Sterowanie −/+ zachowuje ramkę 104×56 px i przyciski 44 px, jak karta rolety.
- Pod sterowaniem znajduje się osobna ramka 104×24 px z nastawą. Temperatura bieżąca pozostaje w dotychczasowym polu pomiaru.
- Po lewej, na wysokości nastawy: ikona baterii termostatu i procent. To odczyt danego urządzenia, niezależny od baterii pilota w status barze. Brak poprawnego bieżącego odczytu daje „—”; niski poziom ma kolor ostrzegawczy.
- Dolny odstęp ramki nastawy wynosi co najmniej 12 px. Napisy mieszczą się w trzech skórkach i językach PL/EN/DE.

Walidacja tej poprawki: `DSR_UI_FOCUS_TRV=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS na przypiętym LVGL 9.3. Wygenerowano tylko 10 podglądów zmienionej karty: trzy skórki × PL/EN/DE oraz brak baterii. Sprawdzono też 0%, 100%, brak/nieprawidłowy odczyt, stan offline, rzeczywiste obszary rysowania tekstów i odstępy. Dashboard z 16 termostatami pozostawia 12 456 B wolnej pamięci (największy blok 12 400 B) w niezmienionej puli 128 KiB; dolne odczyty korzystają z istniejących obiektów i zasobów ikon.

Wspólne rysowanie ikon sprawdzono osobno: `DSR_UI_FOCUS_ICONS=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_cyber_ui` — PASS, jedna galeria, bez pozostałych ekranów tego zestawu. `python3 tools/check_docs.py` i `git diff --check` również przeszły.

Stan: kod roboczy do oceny na podglądzie, następnie na pilocie. To wynik hosta z modelem urządzenia, bez potwierdzenia na sprzęcie. Nowy DEV pozostaje odłożony do końca uzgodnionych poprawek i pełnej walidacji wydania.

### Runda 3 — wyrównanie odczytu i baterii termostatu

Po obejrzeniu podglądu właściciel zlecił wyśrodkowanie temperatury odczytanej wewnątrz jej ramki oraz przesunięcie ikony baterii wraz z procentem w prawo. Etykieta odczytu korzysta teraz z rzeczywistej wysokości tekstu i jest wyśrodkowana w obu osiach, z uwzględnieniem obramowania. Bateria jest wyrównana do lewej krawędzi kolumny odczytu, z odstępem przed ramką nastawy.

Walidacja: ten sam tryb `DSR_UI_FOCUS_TRV=1` — PASS, tylko 10 podglądów karty w trzech skórkach i PL/EN/DE. Regresja sprawdza środek etykiety względem ramki oraz wyrównanie baterii; przed poprawką wykazała błędny środek etykiety. Dashboard z 16 termostatami pozostawia 12 256 B wolnej pamięci, największy blok 12 200 B, przy puli 128 KiB. Odbiór wyglądu na podglądzie i później na pilocie pozostaje otwarty; nowy DEV po zakończeniu ankiety.

### Runda 4 — ramka baterii termostatu

Właściciel zaakceptował wyśrodkowanie temperatury odczytanej i położenie baterii z procentem. Dodatkowo zlecił wspólne obramowanie ikony i poziomu baterii. Dodano ramkę 92×24 px wokół obu elementów, zachowując ich położenie. Cyber Blue używa wspólnego turkusowego, sfazowanego konturu; Grafit i Jasny mają ramkę dopasowaną do swojej skórki.

Walidacja: `DSR_UI_FOCUS_TRV=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS; tylko 10 podglądów karty, trzy skórki i PL/EN/DE. Regresja sprawdza rzeczywiście narysowane cztery krawędzie ramki; przed poprawką wykazała ich brak. Ramka korzysta z istniejącej warstwy rysowania, bez dodatkowych obiektów LVGL. Dla 16 termostatów zapas pamięci pozostał bez zmian: 12 256 B, największy blok 12 200 B, pula 128 KiB. To walidacja hosta; odbiór na pilocie i nowy DEV pozostają po zakończeniu ankiety.

## Konsolidacja starych zgłoszeń

Zamknięcie oznacza przeniesienie dalszej koordynacji tutaj, a nie potwierdzenie usunięcia wszystkich defektów ani wykonania fizycznych testów.

| Stare issue | Dalszy status |
| --- | --- |
| #6 — UI/UX quality gate: Shelly-style design system validated on ES3C35P | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #7 — Device onboarding: real local Shelly discovery wizard (LAN, Wi-Fi AP, BLE) | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #8 — Wi-Fi keyboard: larger iOS-inspired keys and readable 320×480 touch layout | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #12 — Physical Gen3 test: confirm relay ON/OFF, expose real V/A and command errors | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #15 — Shelly-style cover details: vertical position indicator and large Open/Close controls | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #18 — PILNE: rzeczywiste skanowanie Bluetooth LE na ES3C35P — Shelly Wi-Fi i Shelly BLU | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #19 — Wi-Fi AP: lokalne dodawanie nowych urządzeń Shelly z fabrycznego punktu dostępowego | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #20 — Silnik wielu urządzeń Shelly: niezależne połączenia, kolejki i stan w czasie rzeczywistym | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #21 — Grupy, sceny i aktywność: wykonanie lokalnych akcji zamiast samych edytorów | Grupy/sceny i backend aktywności są; widok LCD oraz odbiór pozostają w #74 |
| #22 — Energia, wykresy i statystyki Shelly: prawdziwe pomiary, zapis historii, spójne UI | Wycofane decyzją właściciela; pomiary bieżące zostają |
| #23 — Rzeczywiste typy urządzeń Shelly: Cover, dimmer, RGB/RGBW, sensory i termostaty | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #24 — Kompletne aktualizacje OTA i ustawienia: GitHub Releases, kontrola wersji i przywracanie | Zakres częściowy: OTA/ustawienia są; eksport/reset i odbiór pozostają w #74 |
| #25 — MASTER QUALITY GATE: aktywować i przetestować wszystkie widoczne funkcje DIY Shelly Remote | Zakres częściowy: OTA/ustawienia są; eksport/reset i odbiór pozostają w #74 |
| #28 — Pomieszczenia na pilocie: bezpośrednie Dodaj/Edytuj/Usuń z widoku pomieszczeń | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #29 — Szczegóły urządzenia: działająca zakładka Ustawienia oraz wejście do historii i Wykresów | Ustawienia są; historia/wykresy wycofane decyzją właściciela |
| #32 — UI/UX: nowoczesny kreator dodawania Shelly inspirowany bottom-sheet Shelly Smart Control | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #66 — Koordynacja: status prac i decyzje właściciela | Koordynacja przeniesiona do #74 |

PR-y zamknięte bez merge: #73, #72, #71, #70, #69, #67, #65. Ich nowszy kod znajduje się w historii 0.7.70; #67 zastąpiony późniejszymi rozwiązaniami.

## Wszystkie gałęzie — stan przed porządkami

Przejrzano 62 zdalne gałęzie. Zachowano `main` i `work/dev-0.7.70`; pozostałe 60 usunięto po konsolidacji PR-ów. Pięć ma łącznie sześć unikalnych historycznych commitów, bez utraty funkcji w aktualnym kodzie; zachowano plik odzyskiwania 64 KB `.local/audit-cleanup-2026-10-22/legacy-unique-commits.bundle`, zweryfikowany przez `git bundle verify`. Pozostałe końcówki są przodkami bieżącej gałęzi.

| Gałąź | SHA | Decyzja / zachowanie historii |
| --- | --- | --- |
| `main` | `51c5840e4098` | Zachowano |
| `reference/0.2.0-local-dev` | `51c5840e4098` | Usunięto; commit w historii 0.7.70 |
| `ui/shelly-smart-control` | `c496479e3e8c` | Usunięto; commit w historii 0.7.70 |
| `work/battery-calibration` | `799861169231` | Usunięto; commit w historii 0.7.70 |
| `work/codex-p0-review` | `4a4d69cb200f` | Usunięto; commit w historii 0.7.70 |
| `work/config-save-photo-fixes` | `fdce36fa47d5` | Usunięto; commit w historii 0.7.70 |
| `work/config-web-panel` | `5f6a653e9a85` | Usunięto; commit w historii 0.7.70 |
| `work/cyber-blue-complete` | `8a8dffdc7961` | Usunięto; commit w historii 0.7.70 |
| `work/cyber-blue-final-polish` | `c138c990ae6e` | Usunięto; commit w historii 0.7.70 |
| `work/cyber-blue-quality-pass` | `d2290f21a0fd` | Usunięto; commit w historii 0.7.70 |
| `work/cyber-blue-reference-refinements` | `5d33c2c9a755` | Usunięto; commit w historii 0.7.70 |
| `work/dev-0.7.68` | `5fbd86dc554d` | Usunięto; commit w historii 0.7.70 |
| `work/dev-0.7.70` | `d43da439f757` | Zachowano |
| `work/device-settings-context` | `693d869f54c1` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/es3c35p-bringup` | `f592876f532f` | Usunięto; commit w historii 0.7.70 |
| `work/fix-071-boot-memory` | `5a21559b7548` | Usunięto; commit w historii 0.7.70 |
| `work/fix-startup-recovery` | `aa1969a32c8c` | Usunięto; commit w historii 0.7.70 |
| `work/foundation` | `4a40e428e722` | Usunięto; commit w historii 0.7.70 |
| `work/issue-5-web-locale` | `863729646280` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-8-unified-ui` | `b9daba3cd937` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-display-settings` | `2641ecda325e` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-global-settings` | `f7fb16d97ac5` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-lcd-restart` | `1765187de0ff` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-visual-system` | `54bbef68095b` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-web-pairing-revoke` | `b1286945ef05` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/issue-7-lan-discovery` | `895480c3436a` | Usunięto; commit w historii 0.7.70 |
| `work/issue-12-stable-polling` | `ed2ef9fdfc73` | Usunięto; commit w historii 0.7.70 |
| `work/issue-12-telemetry-errors` | `9436220300df` | Usunięto; commit w historii 0.7.70 |
| `work/issue-15-cover-details` | `41137b62a7b1` | Usunięto; commit w historii 0.7.70 |
| `work/issue-18-ble-ota-full` | `3b96534ea312` | Usunięto; commit w historii 0.7.70 |
| `work/issue-18-ble-scan` | `c4ca8728ea62` | Usunięto; commit w historii 0.7.70 |
| `work/issue-19-ap-onboarding-full` | `267c32f31d7d` | Usunięto; commit w historii 0.7.70 |
| `work/issue-19-ap-visit` | `633863fd7e8c` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/issue-20-independent-shelly` | `07331b829a22` | Usunięto; commit w historii 0.7.70 |
| `work/issue-21-actions-full` | `b5a36ef927b9` | Usunięto; commit w historii 0.7.70 |
| `work/issue-22-history-integrated` | `f240d15bc402` | Usunięto; commit w historii 0.7.70 |
| `work/issue-23-cover-full` | `a194554a811c` | Usunięto; commit w historii 0.7.70 |
| `work/issue-23-ht-full` | `ab8a728ebb42` | Usunięto; commit w historii 0.7.70 |
| `work/issue-23-light-full` | `18f91a61ea91` | Usunięto; commit w historii 0.7.70 |
| `work/issue-23-rgb-full` | `16481fa226d9` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-app-download` | `1153d7104554` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-image-stream` | `2a3e7dd74a97` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-manifest-fetch` | `a298e6862475` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-manifest-verifier` | `c9dd6f712cd7` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-native-controller` | `e69e24a9be67` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-release-candidate` | `2fb3e452d6c8` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-release-chain` | `0790e6e803c0` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-release-fetch` | `784f99aefe2f` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-safe-restart` | `e72074c86bcd` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-upload-stream` | `3501a67213b1` | Usunięto; commit w historii 0.7.70 |
| `work/issue-25-battery-voltage` | `37dd18763b14` | Usunięto; commit w historii 0.7.70 |
| `work/issue-25-feature-audit` | `d5ce4024b81c` | Usunięto; commit w historii 0.7.70 |
| `work/issue-28-bind-factory` | `51124b4df202` | Usunięto; commit w historii 0.7.70 |
| `work/issue-28-editor-acceptance` | `f9bac663dd1d` | Usunięto; commit w historii 0.7.70 |
| `work/issue-29-settings-current` | `dc5a42ea7e28` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/issue-32-onboarding-full` | `0ee4dd2455f4` | Usunięto; commit w historii 0.7.70 |
| `work/local-switch` | `76683a0cc513` | Usunięto; commit w historii 0.7.70 |
| `work/p0-spiffs-ota-brand` | `e6e23597a966` | Usunięto; commit w historii 0.7.70 |
| `work/rooms-context-editor` | `81e734d38fe8` | Usunięto; commit w historii 0.7.70 |
| `work/shelly-hud-cyber-blue` | `e201332d8d0b` | Usunięto; commit w historii 0.7.70 |
| `work/ux-battery-clock-ble` | `b82a922d5314` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/wifi-onboarding` | `81c0753fe5e8` | Usunięto; commit w historii 0.7.70 |

## Porządkowanie — wykonane

- [x] Audyt lokalnego stanu, wszystkich 62 gałęzi, 17 issues i 7 otwartych PR-ów; lista wymagań i źródeł zachowana.
- [x] Odróżniono implementację od starych planów, wycofanych wymagań i fizycznego odbioru.
- [x] Zamknięto 17 starych issues i 7 PR-ów bez merge; dalszy zakres przeniesiono do #74.
- [x] Usunięto 60 zdalnych i dwie nieaktywne lokalne gałęzie; zachowano main i work/dev-0.7.70 oraz mały plik odzyskiwania unikalnych commitów.
- [x] Usunięto stare wyniki Actions i wszystkie artefakty; zachowano ostatnie CI i Signed release 0.7.70 oraz publiczne pliki wszystkich wydań.
- [x] Usunięto lokalne build-ui/build-ai, stare handoffy/logi i wygenerowane obrazy/raporty z docs. Zachowano źródła, testy, zasoby firmware, referencje właściciela i aktywne środowisko.
- [x] Zarchiwizowano osiem rozmów projektu ChatGPT; zachowano bieżący czat Codex.
- [x] Uporządkowano dokumentację i odwołania do usuniętych obrazów; walidacja ograniczona do dokumentacji, diff i stanu repozytorium. Nie uruchamiano nowych testów firmware ani atlasu.

Etap porządkowania nie publikuje nowej wersji. Dalszy etap, zlecony przez właściciela, obejmuje ankietę, uzgodnione poprawki na bieżąco i nowy DEV po ich zakończeniu oraz walidacji. Flashowanie, reset danych, zmiana klucza OTA i scalenie do main nie są częścią porządkowania.

Bieżąca koordynacja: https://github.com/voltex-at/DIY-Shelly-Remote/issues/74
