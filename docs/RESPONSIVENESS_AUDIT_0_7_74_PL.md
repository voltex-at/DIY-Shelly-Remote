# Responsywność i potwierdzanie poleceń po 0.7.74-dev

Data: 2026-10-24. Baza: `dev-v0.7.74`, commit `a47e677`.
Gałąź: `work/dev-0.7.70`. Poniższe zmiany są lokalną kontynuacją;
nie należą do opublikowanego firmware 0.7.74-dev.

## Zgłoszenie i dowody

Właściciel potwierdził powolne strony i chwilowe „No response” po poleceniu
rolety/termostatu. Świeży log identyfikuje 0.7.74-dev: przejścia LVGL trwały
224–354 ms, a przy 24696 ms wystąpił timeout HTTP i nieudany odczyt rolety.
Dotychczasowy log nie wskazuje fazy żądania ani identyfikatora urządzenia.
Nie wolno traktować różnych kolejnych pozycji jako odczytów tej samej rolety.

Pliki sterownika, filtra i skrzynki dotyku oraz zadania UI są identyczne
w tagach 0.7.72, 0.7.73 i 0.7.74. Nie dodano progu nacisku. Priorytety
dotyk/UI/sieć pozostają 6/5/4; pomiary czasu na urządzeniu nadal są wymagane.

## Potwierdzone przyczyny i poprawki

| Obszar | Zmiana |
| --- | --- |
| Przekaźnik, dimmer, RGB | Zmiana samych W/V/A przebudowywała szczegóły i kontrolki. Teraz aktualizuje istniejące etykiety. Tożsamość, błędy, możliwości i zmiana struktury nadal uruchamiają pełną ścieżkę bezpieczeństwa. |
| Poll sieci | Zakończenie operacji i uruchomienie następnego odczytu odmalowywały model osobno. Jeden przebieg publikuje teraz jedną końcową aktualizację. Bezpośrednia reakcja dotykowa pozostaje synchroniczna. |
| Szczegóły grup | Telemetria urządzenia spoza grupy przebudowywała ekran. Nie zmienia już widocznych obiektów. Stały opis liczby członków nie jest ponownie ustawiany co 30 ms. |
| Nawigacja | Ponowne dotknięcie aktywnej strony Dashboard/Pomieszczenia nie usuwa i nie tworzy jej ponownie. |
| Wynik polecenia | Przekaźnik, roleta, dimmer, RGB i TRV przekazują fazę wymiany z workera. Utrata odpowiedzi na zapis oznacza „Polecenie niepotwierdzone”; błąd odczytu po ACK oznacza „Odczyt niepotwierdzony”. Zwykłe awarie odczytu zachowują dotychczasowy komunikat. |
| Odzyskanie po timeout | Po próbie zapisu zakończonej błędem transportu planowany jest jeden wczesny GET po 1 s, także dla TRV. Nie ma ponownego wysłania polecenia. Nieudany GET przywraca zwykły backoff i widoczny błąd. |
| Własność fazy | STOP i polecenie odrzucone/anulowane przed wysłaniem nie dziedziczą fazy poprzedniej komendy. Zwykła utrata Wi-Fi nie usuwa informacji o rzeczywiście rozpoczętym zapisie. |

Nie zmieniono zatwierdzonych układów ani trzech skórek. Żądana pozycja lub
temperatura nie zastępuje raportowanej. Utrata transportu pozostaje widoczna;
autoryzacja, tożsamość, 429, pilny STOP i zakaz replay zachowują swoje bramki.

## Diagnostyka następnej próby sprzętowej

Wyniki sterowania mają numeryczne `job`, `layout`, `kind`, `phase` i `result`.
Fazy: 0 = bez próby zapisu, 1 = rozpoczęcie zapisu, 2 = poprawny ACK,
3 = odczyt po ACK, 4 = uzyskano poprawny odczyt.
Faza 4 nie dowodzi osiągnięcia żądanej pozycji/temperatury; mirror TRV może
nadal pokazywać wcześniejszą nastawę. Osobny pilny wynik STOP nie niesie fazy.

Raporty `Slow view build` i `Slow display refresh` rozdzielają tworzenie
obiektów od rysowania/oczekiwania na wyświetlacz. Są ograniczone do operacji
powyżej 50 ms i najwyżej jednego wpisu danego rodzaju na 5 s. Nie wypisują
adresów urządzeń, haseł ani zawartości ekranów.

## Walidacja

Uruchamiano zestawy związane ze zmianą, bez pełnego atlasu i pełnego CI.
Liczby w logach oznaczają asercje zestawów, nie tysiące oddzielnych uruchomień.
Każda główna poprawka miała wcześniej odtworzony nieudany przypadek.

| Polecenie / zakres | Wynik |
| --- | --- |
| Suite `platform` z `tools/test.py`, lokalny selektor | PASS: fazy preflight/zapis/ACK/odczyt, utrata transportu, jeden GET, brak replay, auth/429 i anulowanie niewysłanej intencji. |
| Suite `actions_worker` z `tools/test.py` | PASS: rzeczywisty worker/aktor, przenoszenie faz i zawieszenie obserwatora przy zagnieżdżonym STOP. |
| `tools/test_ui.py --cases test_priority_ui test_cover_refresh_ui test_touch_ui test_light_ui test_rgb_ui test_group_stop_ui` przez istniejący adapter hosta | PASS: przypięty LVGL 9.3, zachowanie obiektów, współdzielenie odmalowania, grupy i odpowiednie przypadki PL/EN/DE trzech skórek. Obejrzano sześć reprezentatywnych renderów błędu rolety. |
| Dodatkowe `test_cover_ui`, `test_group_stop_ui`, `test_priority_ui` po przeglądzie | PASS: poprzedni poprawny ruch → nieudany STOP oraz nowa intencja odrzucona przed wysłaniem nie dziedziczą poprzedniej fazy. |
| Suite `battery` z `tools/test.py`; `tools/test_automation.py --case runtime --runtime-module native` | PASS: integracja rzeczywistego workera i ręczne intencje TRV. |
| `ninja -C build-ai -j4`, ESP-IDF 5.5.4 | PASS: ESP32-S3, 29% zapasu najmniejszej partycji aplikacji. |
| `check_boot_memory.py`, `check_actions_target.py`, `check_fleet_target.py`, `check_automation_target.py`, `check_system_data_target.py` | PASS: istniejące limity zachowane. Fleet 8080 B, slot 504 B, event 1192 B; stos UI 8192 B i pula LVGL 131072 B bez zwiększenia. |

Jeden niezależny przegląd znalazł dziedziczenie fazy przez STOP. Rozszerzenie
tej samej kontroli wykazało również odrzucenie przed wysłaniem; oba przypadki
odtworzono i poprawiono. Nie wykonywano resetu, flashowania ani publikacji.

## Otwarty odbiór

Host i kompilacja nie potwierdzają czasu reakcji na płytce. W kolejnym DEV
trzeba zmierzyć dotyk → lokalna reakcja → wynik zapisu → świeży odczyt dla
rolety i TRV oraz sprawdzić przekaźnik, dimmer, RGB, grupy i baterię podczas
odświeżania tła. Nowe fazy pozwolą wskazać, która wymiana traci odpowiedź.
Nie stwierdzono jeszcze przyczyny rzeczywistego timeoutu HTTP z logu właściciela.

Pełne CI dokładnego commita oraz kontrola pobierania i aktualizacji z
`RELEASE_CHECKLIST_PL.md` pozostają obowiązkowe przed kolejną publikacją DEV.
