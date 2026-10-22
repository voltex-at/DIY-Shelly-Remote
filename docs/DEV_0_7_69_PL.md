# DIY Shelly Remote 0.7.69-dev — UI/UX i odbiór

Kontynuacja `work/dev-0.7.68`, baza `ec56dc4`. Zakres wynika ze zdjęć LCD,
uwag na renderach oraz doprecyzowań właściciela z 2026-10-22. Zachowano
wcześniejsze poprawki; lokalne narzędzia, konfiguracja agentów i sekrety nie
wchodzą do firmware ani tego commitu.

## Zmiany widoczne na ekranie

- Dokładnie Cyber Blue, Grafit, Jasny. Zapisane identyfikatory 3/0/2 i migracja
  dawnego `midnight` pozostają zgodne. Nowa nazwa działa też w panelu WWW.
- Bateria i procent w jednym polu statusu; `~` nadal oznacza szacunek. Dom,
  siatka i procesor w dolnej nawigacji. Listy ustawień mają poprawne przewijanie
  i odstęp przed dolnym paskiem.
- Pełny wiersz nazwy dashboardu, osobny rząd akcji, odsunięte zakładki. Pomiary
  przekaźnika bliżej opisu, przełącznik wyżej, pełne W/V bez agresywnego ucinania.
- Kompaktowe karty TRV i okna po dodatkowej korekcie właściciela: duże ±48 px
  po prawej, bez powtórzonej ikony i pozycji zaworu na karcie TRV; zawór nadal
  w szczegółach. Brak pustego „Keine weiteren Messwerte”; rzeczywiste 0 lx,
  temperatury i baterie pozostają widoczne. Długie nazwy zwiększają wysokość.
- Ikony pokoju, jego etykieta i edycja mają rozdzielone obszary. Nazwy pokojów
  są zawijane i pozostają tekstem użytkownika, bez tłumaczenia np. „Salon”.
- Wspólna animacja rolety i obsługa podglądu w trzech skórkach. Nagłówek bez
  ołówka, wyśrodkowany większy tytuł, odstęp od powrotu, czytelna skala.
  Potwierdzony stan nie jest zastępowany podglądem; ruch/STOP zachowuje aktora.
- Cyber Blue: ramki komunikatów AP, restartu i niedostępnej konfiguracji,
  poprawiony odstęp pod „Ausschalten”. Usuwanie ma czerwone wypełnienie,
  semantyczny czerwony kontur i zachowane potwierdzenia.

Rzeczywiste piksele LVGL 9.3.0 z przykładowymi danymi, kolumny Grafit/Jasny/Cyber Blue:

- [Karty](ui-0.7.69/cards.png)
- [Roleta](ui-0.7.69/cover.png)
- [AP: wynik i zgoda](ui-0.7.69/ap.png)
- [Restart, niedostępna konfiguracja, czerwone usuwanie](ui-0.7.69/system.png)
- [Szczegóły lampy i pomieszczenia](ui-0.7.69/lamp-rooms.png)

## Dodawanie przez AP i restart

AP używa istniejącego `dsr_ap`: wyszukiwanie, pusta lista, znalezione AP,
wybór, dwie osobne zgody, wynik/błędy i ponowienie. Retry ponawia wyłącznie
skanowanie, nie POST z hasłem. Anulowanie i błąd odtwarzają sieć domową.
Dodanie nadal wymaga potwierdzenia tej samej tożsamości w LAN i istniejącego
zapisania powiązania. Otwarty AP nie zapewnia kryptograficznego uwierzytelnienia;
pozostawiono ostrzeżenie i potwierdzenie właściciela urządzenia.

Lokalny potwierdzony restart używa `dsr_restart_request_confirmed`. Blokuje nowe
operacje, zachowuje liczniki istniejących lease i nie czeka na ich zakończenie.
Dedykowany dispatcher po 500 ms wywołuje standardowy `esp_restart()`. Przed nim
uzbraja RWDT: 3 s do RESET_SYSTEM i dalsze 3 s do RESET_RTC, ponieważ SDK najpierw
synchronicznie zamyka m.in. Wi-Fi. Ochrona działa bez timera UI, kolejki workera
ani odpowiedzi radia. Błąd notify/boot cofa tylko własną rezerwację, zachowując
liczniki. WWW i automatyczny restart OTA zachowują dotychczasową wyłączność.
Niezapisane zmiany zostają utracone zgodnie z komunikatem potwierdzenia.

## Walidacja i jej granice

- Wykonano jeden pełny lokalny etap UI/atlasu, 60 tras ×3 skórki ×PL/EN/DE.
  Po uwagach właściciela powtarzano tylko dotknięte przypadki i dokładnie wybrane
  trasy. Obejrzano renderowane widoki; późniejsze poprawki mają nowe zrzuty.
- Przechodzą skupione przypadki kart/statusu/AP/gestów, główny UI, BLU/H&T,
  oświetlenie/RGB, priorytet STOP i reprezentatywne zestawy Cyber. Zmienione
  asercje współrzędnych odpowiadają nowym kontrolkom; asercje zgód i poleceń
  pozostają. Test ustawień rozgrzewa oba mierzone ekrany przed porównaniem puli,
  zamiast mierzyć jednorazowe utworzenie cache jako wyciek.
- Zmienione trasy atlasu PL/EN/DE: brak text-cut/offscreen i niezamierzonych
  nakładających się targetów. Ostrzeżenia pełnego audytu o overlay dotyczą
  zamierzonych modalnych zasłon nad nieaktywną treścią; obejrzano je oddzielnie.
- Pula LVGL nadal 128 KiB, stos UI 8192 B. Cyber, 24 kafelki: 9856 B wolne,
  największy blok 9800 B. Po rozgrzaniu 60 zmian skórek: brak ubytku puli.
  Porównania czasu hosta nie stanowią pomiaru ESP32.
- Restart: rzeczywiste atomiki/backend, równoległe żądania, notify/boot-loss,
  aktywne lease i kolejność uzbrojenia watchdoga przed SDK; FreeRTOS/RTC/reset
  są providerami hosta. Cały moduł restartu przechodzi lokalnie.
- Rzeczywisty panel WWW: PL/EN/DE i trzy nazwy skórek przechodzą w Chromium
  z przypiętym Playwright 1.51.1; transport jest modelem.
- Build ES3C35P ESP-IDF 5.5.4 przeszedł; aplikacja lokalna 4240800 B, 33% slotu
  aplikacji pozostaje wolne. Wszystkie 15 wymaganych audytów targetu przechodzi.
  BSS wewnętrzny 60200 B; watchdog nie dodał taska/stosu ani trwałego bufora.
  Lokalny pin źródła klucza normalizuje tylko CRLF→LF; Linux CI sprawdza surowe
  bajty. Klucz `voltex-ota-2026-1` i partycje pozostają bez zmian.

Pełne CI dla dokładnego commitu wydania jest obowiązkowe przed utworzeniem
`dev-v0.7.69`. Podpis i publikacja wymagają chronionego środowiska `release`
z dotychczasowym kluczem; brak obejścia administracyjnego. Publikacja wyłącznie
DEV, bez scalenia do main. Wynik końcowego CI i podpisane zasoby są powiązane
z tagiem na GitHub; wcześniejsze wydania pozostają niezmienne.

Nowy firmware nie był flashowany na fizyczne urządzenie przez agenta. Fizyczny
czas resetu/wygaśnięcia RWDT, płynność dotyku, RF/AP, pomiar ADC i instalacja/rollback
OTA wymagają odbioru na ES3C35P. Host i audyt linkowania ich nie potwierdzają.

Odtwarzanie: `tools/test_ui.py --case test_dev_069_ui`,
`tools/test_restart.py`; końcowe pełne CI z `.github/workflows/ci.yml`.
Pełny atlas finalnego SHA jest artefaktem CI, lokalne delty służą bieżącej pracy.
