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
| UI-07 | Termostat: odczyt, nastawa i ergonomia −/+ | **Karta zaakceptowana na podglądzie przez właściciela**: odczyt, „Soll”, bateria i jej ramka, wspólna oś pól oraz wyśrodkowane +/−. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-08 | Okno i drzwi: grafika otwartego/zamkniętego skrzydła | **Karty zaakceptowane na podglądzie przez właściciela**: nazwa u góry, mała ikona po lewej, duża po prawej, pośrodku pomieszczenie → ikona z lx → bateria z procentem. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-09 | Roleta: wspólna animacja trzech skórek, skala, status, pozycja i STOP | **Cały punkt zaakceptowany na podglądzie przez właściciela**: przyciski, odstęp od nawigacji, skala, animacja oraz karta Dashboardu z pomieszczeniem zamiast stanu, pozycją i sterowaniem. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-10 | Cyber Blue: turkusowe ramki treści, w tym wynik i zgoda Wi-Fi AP | Wzór dashboardu przeniesiony do wspólnych paneli, akcji i pól; podwójny kontur i grubsza strzałka górnego przycisku powrotu **zaakceptowane na podglądzie**. Końcowy przegląd całej skórki przed DEV |
| UI-11 | Równy zakres funkcjonalny skórek oraz ekrany dimmera/RGB | Oczekuje na ocenę |
| CFG-01 | Dodaj/edytuj/usuń pomieszczenia, dashboardy, trwałe nazwy i zdjęcia | Oczekuje na ocenę |
| NET-01 | LAN, Wi-Fi AP i BLE: skan, wybór, zgody, błędy, retry i powrót do LAN | **Przycisk Dalej, znacznik wybranej metody i górny przycisk powrotu Cyber Blue zaakceptowane na podglądzie**. Naprawiono przycięcie pola kanału, przewijanie opisu/komunikatów i kolizję przy klawiaturze. Pozostały odbiór kreatora i sprawdzenie na sprzęcie pozostają otwarte |
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

Korekta po kolejnym podglądzie: właściciel ocenił ramkę jako zbyt długą. Usunięto stałą szerokość 92 px; aktualna ramka zachowuje wysokość 24 px, a jej szerokość wynika z ikony, odstępu i zmierzonego tekstu procentu, z marginesem 6 px po obu stronach. Położenie ikony oraz procentu pozostaje zaakceptowane. Ta sama regresja przeszła dla trzech skórek i PL/EN/DE, potwierdzając skrócenie rzeczywiście rysowanych krawędzi oraz dopasowanie także dla 0%, 100% i braku odczytu. Zapas pamięci dla 16 termostatów pozostał bez zmian.

Walidacja: `DSR_UI_FOCUS_TRV=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS; tylko 10 podglądów karty, trzy skórki i PL/EN/DE. Regresja sprawdza rzeczywiście narysowane cztery krawędzie ramki; przed poprawką wykazała ich brak. Ramka korzysta z istniejącej warstwy rysowania, bez dodatkowych obiektów LVGL. Dla 16 termostatów zapas pamięci pozostał bez zmian: 12 256 B, największy blok 12 200 B, pula 128 KiB. To walidacja hosta; odbiór na pilocie i nowy DEV pozostają po zakończeniu ankiety.

### Runda 5 — wspólna oś baterii i temperatury oraz znaki +/−

Właściciel zaakceptował krótszą szerokość ramki baterii. Zlecił wyśrodkowanie całego pola baterii względem pola temperatury odczytanej „21.5 °C” oraz wyśrodkowanie znaków + i − wewnątrz przycisków. Ramka baterii wraz z zawartością ma teraz środek zgodny ze środkiem rzeczywistego pola odczytu, także dla 0%, 100% i braku danych. Przyciski pozycjonują widoczny kształt znaku z uwzględnieniem jego wymiarów i linii bazowej fontu, zamiast stałego przesunięcia etykiety.

Walidacja: ten sam tryb `DSR_UI_FOCUS_TRV=1` — PASS; 10 podglądów zmienionej karty, trzy skórki i PL/EN/DE. Sprawdzono środek rzeczywistych pikseli obu znaków oraz wspólną oś pól. Ponieważ ten sam helper obsługuje również przyciski 48 px w szczegółach termostatu, sprawdzono ich wyśrodkowanie w trzech skórkach, bez generowania dodatkowego atlasu. Regresja przed poprawką wykazała przesunięte znaki. Dla 16 termostatów pozostaje 12 632 B wolnej pamięci, największy blok 12 576 B, w puli 128 KiB. Podgląd kodu roboczego do oceny; odbiór na pilocie i nowy DEV po zakończeniu ankiety.

**Odbiór ankiety:** właściciel potwierdził „Tak, karta zaakceptowana” po wyśrodkowaniu ramki baterii względem temperatury i znaków +/−. Punkt UI-07 zamknięty na poziomie wyglądu podglądu; nie jest to potwierdzenie wdrożenia ani odbioru na fizycznym pilocie. Dalsza ankieta dotyczy kolejnych ekranów.

### UI-08 — zaakceptowany podgląd kart okna i drzwi

Właściciel wybrał ten punkt jako następny po zaakceptowaniu karty termostatu. Przygotowano świeży podgląd okna i drzwi w obu stanach dla Cyber Blue, Grafit i Jasny, z dodatkowym przykładem dłuższej nazwy oraz rzeczywistych odczytów 0 lx i baterii 92% z modelu testowego.

Wykryta niespójność: wiersz „pomieszczenie • stan” skraca pomieszczenie „Schlafzimmer” przy „Offen”, a dłuższe „Geschlossen” usuwa pomieszczenie z tego wiersza. Nazwa urządzenia i przypisanie do pomieszczenia są odrębnymi polami. Grafika po prawej poprawnie rozróżnia okno/drzwi i stan otwarty/zamknięty, bez powielania tekstu stanu w swoim panelu.

Właściciel zaakceptował wielkość i wygląd grafik. Ostateczne doprecyzowanie układu zastępuje wcześniejszą propozycję pełnego wiersza pomieszczenia: **nazwa urządzenia u góry; mała ikona po lewej; zaakceptowana duża grafika po prawej; w środkowej kolumnie od góry pomieszczenie, ikona i wartość lx, następnie ikona baterii i procent**.

Ten wariant jest w kodzie roboczym. Pomieszczenie zawija się w środkowej kolumnie bez skracania i bez rywalizacji z tekstem stanu; otwarcie/zamknięcie wskazują zaakceptowane grafiki. Odczyty lx i baterii mają osobne, krótkie ramki oraz ikony. Rzeczywiste 0 lx pozostaje zerem, brak albo nieprawidłowy odczyt jest oznaczony „—”. Zachowano dotychczasowe komunikaty offline/braku danych.

**Odbiór ankiety:** właściciel potwierdził „Tak, układ zaakceptowany” po obejrzeniu tego wariantu. Punkt UI-08 zamknięty na poziomie wyglądu podglądu, obejmując grafiki oraz układ kart okna i drzwi. Odbiór na fizycznym pilocie pozostaje otwarty po nowym DEV.

Walidacja: `DSR_UI_FOCUS_CONTACT=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS na przypiętym LVGL 9.3. Tylko 15 podglądów wybranych kart: 12 kombinacji skórki/typu/stanu i trzy dłuższe nazwy z odczytami. Sprawdzono PL/EN/DE, długie nazwy pomieszczeń do granicy pola modelu, odstępy i wyśrodkowanie wierszy pomiarowych, brak odczytów oraz offline. Grafika 52 px zachowała rozmiar. Pomieszczenie i odczyty są rysowane w istniejącej karcie; dla 16 urządzeń pozostaje 61 008 B wolnej pamięci, największy blok 60 952 B, w puli 128 KiB. To host z modelem danych, bez odbioru na pilocie; nowy DEV po zakończeniu ankiety.

### UI-09 — zaakceptowany podgląd rolety, przycisków i animacji

Po zaakceptowaniu termostatu oraz kart okna i drzwi właściciel wybrał roletę. Przygotowano aktualne piksele LVGL 9.3 dla Grafit, Jasny i Cyber Blue: zatrzymana roleta, przeciąganie z oddzielnym podglądem celu, ruch z czerwonym STOP, nieznana pozycja oraz karta Dashboardu w spoczynku i w ruchu. Krótki GIF pokazuje klatki istniejącej animacji otwierania 50→80% i zamykania 80→20%, z własną kolorystyką każdej skórki. Przyciski szczegółów mają wysokość 48 px i 16 px odstępu przed nawigacją.

**Wykryta niespójność pierwszego podglądu:** na karcie Dashboardu przypisane pomieszczenie „Wohnzimmer” znikało przy stanie „Gestoppt” oraz „Wird geöffnet”. Współdzielony wiersz pomieszczenia i stanu oddawał całe miejsce dłuższemu stanowi.

**Odbiór ankiety:** właściciel potwierdził „Tak, te elementy zaakceptowane” dla przycisków, odstępu od nawigacji, skali oraz animacji otwierania/zamykania. Dla karty Dashboardu zlecił: **„Stan zastąp nazwą pomieszczenia”**. To zastępuje propozycję dodania osobnego wiersza pomieszczenia nad stanem.

W kodzie roboczym karta rolety pokazuje teraz nazwę pomieszczenia w miejscu zwykłego stanu ruchu. Pod nią pozostaje pozycja, a po prawej dotychczasowe sterowanie lub czerwony STOP w ruchu. Długie nazwy zawijają się z automatycznym przesunięciem pozycji i dopasowaniem wysokości karty. Użyto istniejącej etykiety, bez dodawania obiektów LVGL. Brak przypisania ma lokalizowany komunikat; błędy i nieaktualny odczyt zachowują ostrzeżenie w tym miejscu. Przy okazji usunięto przycinanie dłuższego niemieckiego komunikatu błędu logowania na karcie rolety. Szczegóły i zaakceptowana animacja nie zostały zmienione.

**Końcowa akceptacja podglądu:** właściciel potwierdził „Tak, karta rolety zaakceptowana” dla wariantu z pomieszczeniem zamiast stanu. W połączeniu z wcześniejszą akceptacją przycisków, skali i animacji zamyka to punkt UI-09 na poziomie wyglądu podglądu. Nie jest to potwierdzenie wdrożenia ani odbioru na fizycznym pilocie; nowy DEV pozostaje po zakończeniu ankiety i walidacji wydania.

Walidacja pierwszego podglądu: `DSR_UI_FOCUS_COVER=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS. Tryb uruchamia wyłącznie roletę; 60 klatek obejmuje 42 klatki dwóch animacji w trzech skórkach. Sprawdzono rzeczywistą zmianę pikseli rolety, odstęp akcji i brak zapisu pozycji podczas przeciągania, z pojedynczym wywołaniem po puszczeniu. Podgląd używa aktywnych atrap callbacków otwierania, zamykania i STOP; nie są to polecenia wysyłane do sprzętu. Sam pierwszy przegląd nie zmieniał kodu firmware i nie generował pełnego atlasu. Późniejsza zmiana karty została opisana powyżej; odbiór na pilocie pozostaje otwarty.

Istniejąca regresja `python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_cover_ui` — PASS, osiem klatek. Potwierdza pojedyncze żądanie po puszczeniu, pierwszeństwo STOP i końcowy potwierdzony odczyt, odzyskanie odczytu bez powtarzania ruchu po błędzie oraz blokady przy kalibracji i błędzie autoryzacji. Napisy sprawdza w PL/EN/DE. Radio, worker i zapis pozostają modelem hosta.

Walidacja zmienionej karty: `DSR_UI_FOCUS_COVER_CARD=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, tylko dziewięć klatek kart, bez powtarzania animacji ani szczegółów. Sprawdzono trzy skórki i PL/EN/DE, spoczynek/otwieranie/zamykanie, pełne nazwy do granicy pola modelu, brak przypisania oraz zachowanie ostrzeżeń offline i błędu logowania. Regresja przed poprawką wykazała brak pomieszczenia; podczas korekty wykazała też przycięty niemiecki komunikat błędu. Rzeczywiste piksele krótkiego i długiego pomieszczenia sprawdzono na renderach LVGL 9.3. To wynik hosta, bez wdrożenia na pilocie i bez nowego DEV.

### NET-01 / UI-10 — podgląd dodawania urządzeń

Po akceptacji rolety właściciel wybrał dodawanie urządzeń: kroki, napisy i ramki. Przygotowano świeży podgląd wyboru metody, ręcznego IP, AP oraz LAN w trzech skórkach, a także etykiet BLE w PL/EN/DE w Cyber Blue. Pierwszy ekran zachowuje zaakceptowane położenie „Krok 1 z 3”. Na kolejnych ekranach numer kroku znajduje się pod nagłówkiem, a nie obok zegara. Zgody połączenia z AP i wysłania Wi-Fi oraz ekran błędu mają turkusowe, sfazowane ramki Cyber Blue. Etykiety BLE mówią „urządzenie / device / Gerät”; RSSI i opis typu mają oddzielne, czytelne ramki.

**Niespójność pierwszego podglądu:** w kroku LAN „Nazwa i pomieszczenie” dolna część pola kanału była ucięta przez przewijany kontener formularza. Kontener miał wysokość 184 px, a pole kanału zaczynało się na 156 px i miało wysokość 44 px, więc jego dolne 16 px wykraczało poza początkowy obszar widoczny. Opis pomocniczy pod formularzem pozostawiał zbyt mało miejsca na pełny widok wszystkich pól. Problem występował we wszystkich trzech skórkach. Właściciel zaakceptował układ z pełnymi polami i opisem w przewijanej treści; wdrożenie oraz nowy podgląd opisano poniżej.

Walidacja podglądu: `DSR_UI_FOCUS_ONBOARDING=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden przypadek ograniczony do dodawania urządzeń, 48 wybranych klatek, bez pełnego atlasu ani ponawiania zaakceptowanych kart. Obejmuje pustą listę AP, błąd i ponowienie, dwie odrębne zgody, anulowanie z powrotem do domowego Wi-Fi w modelu hosta, wyniki LAN i konfigurację oraz rozpoznane/nierozpoznane BLE i języki PL/EN/DE. Obejrzenie rzeczywistych pikseli ujawniło opisane przycięcie pola kanału mimo przejścia dotychczasowych sprawdzeń. To walidacja hosta z atrapami radia i zapisu; akceptacja wyglądu oraz rzeczywiste dodawanie urządzeń na pilocie pozostają otwarte. Żaden nowy DEV nie został opublikowany.

### NET-01 — przycisk „Dalej / Continue / Weiter”

Właściciel oznaczył podgląd kreatora jako wymagający korekty i zlecił: **„Przycisk Weiter dodajmy ikonę oraz kolor zmieńmy na ten niebieski z przycisku Speichern”**. Dodano strzałkę w prawo z istniejącego zestawu ikon, z miejscem oddzielonym od etykiety. Zmiana dotyczy przycisku Dalej w stopce kreatora i działa w trzech skórkach oraz PL/EN/DE.

Aktywny Dalej już korzystał z tego samego stylu podstawowej akcji co Zapisz; poprzedni podgląd pokazywał stan przed wyborem metody. Nowy podgląd obejmuje oba stany oraz aktywny niebieski przycisk po wyborze. Porównanie ze Zapisz potwierdza identyczny kolor tła, kolor gradientu i kierunek gradientu. Wymóg wyboru metody pozostaje aktywny; samo zaznaczenie metody nie uruchamia skanowania, a przejście następuje po naciśnięciu Dalej.

Walidacja: `DSR_UI_FOCUS_ONBOARDING_NEXT=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, tylko 12 klatek pierwszego ekranu, trzy skórki i PL/EN/DE. Sprawdzono ikonę, czytelność pełnej etykiety i odstępy, zgodność aktywnego stylu ze Zapisz oraz brak uruchomienia skanowania przed wyborem i naciśnięciem Dalej. Przed poprawką regresja wykazała brak ikony. Te same sprawdzenia dla DE w trzech skórkach są także w standardowym przebiegu tego przypadku UI, przewidzianym na końcowe CI. Pozostałych ekranów i atlasu nie uruchamiano ponownie w tej rundzie. Podgląd oczekuje na ocenę właściciela. Przycięte pole kanału oraz pozostałe uwagi do kreatora nadal są otwarte; odpowiedź dotycząca przycisku nie zamyka tych punktów.

Doprecyzowanie właściciela po podglądzie: strzałkę przenieść na prawą stronę przycisku, a po wybraniu Bluetooth, Wi-Fi AP lub sieci lokalnej dodać białą kropkę wewnątrz niebieskiego okręgu. Obie zmiany są w kodzie roboczym. Znacznik jest wyśrodkowany i pojawia się tylko przy jednej wybranej metodzie; przełączenie usuwa go z poprzedniej opcji. Rysowanie korzysta z istniejącej kontrolki, bez dodatkowych obiektów ani zasobów graficznych. Etykieta Dalej/Continue/Weiter ma wydzielone miejsce po lewej od strzałki. Aktywny niebieski styl Zapisz pozostaje wspólny.

Walidacja tej korekty: ten sam tryb `DSR_UI_FOCUS_ONBOARDING_NEXT=1` — PASS, 14 wybranych klatek pierwszego ekranu, trzy skórki i PL/EN/DE. Sprawdzono rzeczywiste białe piksele w środku zaznaczonego okręgu oraz ich brak przy pozostałych opcjach, zmianę zaznaczenia między trzema metodami, położenie strzałki po prawej i pełne etykiety bez kolizji. Wybór opcji nie uruchamia transportu; skan startuje dopiero po Dalej. Nie powtarzano pozostałych ekranów, animacji ani pełnego atlasu.

**Odbiór ankiety:** właściciel potwierdził „Tak, te poprawki zaakceptowane” dla strzałki po prawej oraz białej kropki wybranej metody. Te elementy pierwszego ekranu są zaakceptowane na podglądzie. Przycięte pole kanału, pozostały zakres NET-01 / UI-10 i odbiór na fizycznym pilocie pozostają otwarte; akceptacja nie oznacza nowego wydania DEV.

### NET-01 — formularz „Nazwa i pomieszczenie”

Właściciel odpowiedział „Tak, taki układ odpowiada” na propozycję zachowania szerokości i wysokości pól, powiększenia formularza oraz przeniesienia opisu pod pola do przewijanej treści. Jest to akceptacja układu do wdrożenia; końcowa ocena poniższego podglądu pozostaje otwarta.

Obszar formularza ma teraz 208 px wysokości zamiast 184 px. Pole kanału 288×44 px mieści się w całości, a pola nazwy i pomieszczenia zachowują swoje rozmiary. Opis pomocniczy znajduje się za polami. Walidacja pustej nazwy, rozpoczęcie zapisu, błędy i niepotwierdzony zapis automatycznie przewijają komunikat do widocznej części, nad stopką. Nieobsługiwany kanał nadal blokuje zapis i od razu pokazuje wyjaśnienie.

Przy sprawdzaniu klawiatury wykryto także kolizję pola nazwy z kartą tożsamości urządzenia. Na czas wpisywania karta jest ukrywana, a aktywny wiersz ma własny obszar nad klawiaturą. Po zamknięciu klawiatury wracają karta, pełny formularz i początkowe przewinięcie. Formularz danych logowania zachowuje osobną wysokość 184 px. Radio, reguły dodawania i etapy zapisu nie zostały zmienione.

Walidacja: `DSR_UI_FOCUS_LAN_FORM=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden przypadek ograniczony do tego formularza, dziewięć wybranych klatek, trzy skórki i PL/EN/DE. Sprawdzono pełne pole kanału przed i po klawiaturze, dostępność opisu, widoczność komunikatów walidacji/niepotwierdzonego zapisu/limitu urządzeń oraz powrót do logowania po błędzie autoryzacji. Pusta nazwa nie uruchamia żądania BIND. Regresja uruchomiona na oryginalnym formularzu potwierdziła przycięcie pola. Podgląd używa przypiętego LVGL 9.3 i właściwych motywów natywnych pól; sprawdzono rzeczywiste piksele. Nie generowano pełnego atlasu ani nie ponawiano zaakceptowanych kart. To wynik hosta z modelem radia i zapisu, bez nowego DEV ani odbioru na fizycznym pilocie.

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

### UI-10 / NET-01 — górny przycisk „Zurück” w Cyber Blue

Po podglądzie formularza właściciel wskazał jako jedyne zastrzeżenie brak podwójnego turkusowego obramowania górnego przycisku powrotu i zbyt cienką strzałkę. Wspólny renderer dodawał wewnętrzny kontur dopiero od szerokości 80 px, podczas gdy ten przycisk ma 44×44 px.

Górny przycisk otrzymał wewnętrzny sfazowany kontur przy zachowaniu dotychczasowej ramki zewnętrznej. Strzałka w istniejącym obszarze ikony ma teraz zaokrąglony obrys 3 px. Zmiana dotyczy wspólnego nagłówka Cyber Blue; pozostałe skórki i zaakceptowane karty zachowują wygląd. Nie dodano obiektów LVGL, zasobów bitmapowych ani timerów. Obszar dotyku i przypisywanie funkcji powrotu pozostają bez zmian.

Walidacja: `DSR_UI_FOCUS_HEADER_BACK=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden przypadek ograniczony do przycisku nagłówka, trzy skórki i PL/EN/DE, cztery klatki. Sprawdzono stan naciśnięcia i rzeczywisty powrót z formularza do listy przez zdarzenia wskaźnika. Obejrzano rzeczywiste piksele LVGL 9.3, w tym porównanie przed/po i stan naciśnięty. Właściciel potwierdził „Tak, przycisk zaakceptowany”. Bez pełnego atlasu, nowego DEV i odbioru na fizycznym pilocie.

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
