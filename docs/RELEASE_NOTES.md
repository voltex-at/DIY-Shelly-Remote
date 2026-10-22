0.7.70-dev: ekran główny, pomieszczenia, ściemniacz i RGB według zdjęć referencyjnych właściciela.

- Pulpit: jeden wiersz z wyborem pulpitu i trzema kwadratowymi akcjami (+, edycja, informacje), dzięki czemu mieszczą się trzy karty. Karta ma nazwę, kwadratowe pole ikony, wiersz „Pomieszczenie • Stan”, pigułkę pomiarów i jedną akcję po prawej: duży okrągły włącznik, ramkę ze strzałkami góra/dół dla rolety (STOP w całej ramce podczas ruchu), kafel stanu dla czujnika otwarcia oraz pasek nastawy −/+ pod odczytami termostatu. Długi stan (oczekiwanie, wysyłanie) zajmuje cały wiersz zamiast nazwy pomieszczenia.
- Cyber Blue: ciemnogranatowe kafle z fazowaną podwójną ramką zamiast niebieskiego wypełnienia; stan „włączone” pokazuje niebieski włącznik z białą ikoną. Pomieszczenia mają jedną cienką ramkę, zdjęcie bez własnej obwódki, mniejszy znacznik i ołówek 34 px z polem dotyku 44 px; niewybrane zakładki mają cienki kontur.
- Ściemniacz (Cyber Blue): zakładki Sterowanie / Zużycie / Ustawienia, moc, napięcie i prąd, tarcza jasności z przeciąganiem (zapis po puszczeniu), −/+ co 5%, wiersz „Włączony” z przełącznikiem, szybkie ustawienia 10/25/50/100% i wiek potwierdzonego odczytu; problemy (offline, blokada) są w ostatnim wierszu.
- RGB (Cyber Blue): zakładki Sterowanie / Sceny / Ustawienia, koło barw (kolor wysyłany po puszczeniu), suwak jasności, ustawienia Ciepły / Neutralny / Niebieski / Relaks, odczyt RGB i mocy oraz przycisk Wyłącz/Włącz. „Kolor >” otwiera dokładne suwaki R/G/B z „Zastosuj”.
- Poprawka: gdy współdzielona tablica stylów była pełna, lokalne właściwości panelu przesłaniały style Cyber Blue (np. włącznik zostawał granatowy). Powierzchnie skórki przejmują teraz wypełnienie i promień.
- Wszystkie polecenia używają istniejących akcji Light/RGB/Switch/Cover/TRV i potwierdzonych danych. Dane, partycje, klucz OTA i format konfiguracji bez zmian. Testy hosta nie zastępują odbioru na urządzeniu.

0.7.69-dev: dopracowane UI trzech skórek, kompaktowe karty i niezależny restart.

- Dokładnie Cyber Blue, Grafit i Jasny. Procent baterii jest wewnątrz wskaźnika; dolna nawigacja używa domu, siatki i procesora. Listy ustawień mają czytelniejsze odstępy i dostępny ostatni wiersz.
- Nazwa dashboardu zajmuje osobny wiersz, akcje są rozdzielone, nazwy urządzeń i pokojów mogą się zawijać. Pomiary są bliżej opisu, przyciski przekaźników wyżej. Karty termostatu i okna są kompaktowe; pozycję zaworu pozostawiono w szczegółach termostatu. Usunięto pusty komunikat o dodatkowych pomiarach, zachowano rzeczywiste odczyty także równe zero.
- Wszystkie skórki współdzielą animację rolety, podgląd gestu i informacje o stanie/pomiarach. Nagłówek rolety ma wyśrodkowany, większy tytuł i odstęp od powrotu; zbędny ołówek usunięto. Podgląd nadal nie zmienia potwierdzonego stanu, zapis następuje po puszczeniu, a pilny STOP zachowuje priorytet.
- Kreator dodawania przez Wi-Fi AP korzysta z istniejącego backendu, ma czytelne wyszukiwanie, pustą listę, wybór, wynik i ponowienie. Cyber Blue obejmuje treść turkusowymi ramkami. Nadal wymagane są dwie osobne zgody; retry ponawia wyszukiwanie, nie wysyła ponownie hasła. Sukces wymaga weryfikacji tej samej tożsamości w LAN.
- W Cyber Blue poprawiono odstępy w szczegółach lampy, ramki wyniku AP, restartu i niedostępnej konfiguracji. Akcje usuwania mają wyraźne czerwone wypełnienie, czerwony kontur i zachowane potwierdzenia.
- Lokalnie potwierdzony restart nie czeka na zakończenie Wi-Fi, BLE ani innych aktywnych operacji. Dedykowane zadanie wywołuje standardowy restart SDK po 500 ms na odświeżenie komunikatu; sprzętowy RTC watchdog chroni zamykanie SDK (3 s do resetu systemu i dalsze 3 s do resetu RTC). Nowe operacje są blokowane bez zerowania istniejących liczników. WWW i automatyczny restart OTA zachowują dotychczasową kontrolę zajętości. Niezapisane zmiany zostają utracone zgodnie z potwierdzeniem.
- Walidacja etapami: jeden pełny lokalny przegląd UI, później tylko dotknięte przypadki i ekrany, build ESP32-S3 oraz wymagane audyty. Wydanie wymaga pełnego CI dokładnego SHA i podpisu chronionego środowiska `release`. Dane, partycje i klucz OTA pozostają zgodne. Testy hosta i kompilacja nie zastępują odbioru na urządzeniu. Raport: [DEV_0_7_69_PL.md](DEV_0_7_69_PL.md).

0.7.68-dev: trzy skórki, priorytet STOP, odzyskiwanie po 429 i mniej pracy UI w tle.

- Dostępne są dokładnie trzy skórki: Shelly HUD — Cyber Blue, Grafitowy i Jasny. Dawny zapis `midnight` oraz historyczny identyfikator 1 są bezstratnie normalizowane do Grafitowego; pozostałe identyfikatory 0/2/3, dane urządzeń, sceny, pokoje i zdjęcia pozostają zgodne. Wybór w UI używa jawnej mapy pozycji, zamiast traktować pozycję menu jako zapisany identyfikator.
- STOP rolety jest centralnym, powiększonym sterowaniem podczas ruchu. Pilny STOP wyprzedza oczekujące polecenia; odczyty i ponowienia niedostępnych urządzeń ustępują odczytom poruszającej się lub kalibrującej rolety. Po niepewnym zapisie nie ma automatycznego replay. Nie jest to gwarancja fizycznego czasu STOP: trwające połączenie z urządzeniem nadal ma własny timeout.
- Zapisana sieć Wi-Fi odzyskuje połączenie również po przejściowym błędzie handshake i utracie IP. HTTP 429 ma automatyczny backoff; późniejszy potwierdzony odczyt usuwa tymczasową blokadę kolejnego jawnego polecenia. Uszkodzona telemetria jest błędem odpowiedzi, a nie deklaracją nieobsługiwanego kanału. Ochrona tożsamości urządzenia pozostaje przed operacjami.
- Klawiatura pokazuje powiększenie naciskanego klawisza, animacja baterii korzysta z krótszego timera wyłącznie w aktywnym stanie. Przy wygaszonym ekranie karty urządzeń odkładają przebudowę, a animacja rolety nie rysuje; po wybudzeniu widok pokazuje ostatni potwierdzony model.
- Pętla UI i ponowne renderowanie korzystają ze zweryfikowanego układu zależnego od rewizji w PSRAM. Regresja 30 sekund bezczynności zmniejszyła liczbę parsowań i kopii konfiguracji z 240 do zera. Zapis nazwy lub wyglądu zachowuje oczekujące polecenie; błędny lub zmieniony dokument nie daje dostępu do starego mapowania.
- Panel WWW pobiera obraz LCD na jawne „Odśwież podgląd”. Zwykły odczyt statusu nie wymusza zrzutu 300 KiB; żądania nie nakładają się, a ukryta karta wstrzymuje odczyty. Edycja i kontrola konfliktów konfiguracji zachowują dotychczasowe zabezpieczenia.
- Poprawiono przycięcia pomiarów i niemieckiej zakładki grup w Cyber Blue. Atlas obejmuje trzy skórki i PL/EN/DE. Trzy modele używane wyłącznie przez zadanie UI przeniesiono do PSRAM; pula LVGL 128 KiB, stos UI 8192 B i dotychczasowe limity audytorów pozostają bez zmian.
- Proces podpisywania wymaga zgody właściciela, braku obejścia administracyjnego i pełnej listy reguł dopuszczających wyłącznie tagi właściwego kanału. Akcje GitHub przypięto do zweryfikowanych SHA. Publikacja DEV wymaga pełnego CI dla dokładnego commita i klucza w chronionym środowisku; nie korzysta z niechronionego sekretu repozytorium.
- Aktualizacja zachowująca dane: podpisane DEV OTA lub paczka UPDATE-APP-ONLY. Nie zmieniono partycji, formatu danych ani klucza zaufania OTA. FACTORY-CLEAN służy do świadomej instalacji czystej. Testy hosta i CI nie zastępują odbioru fizycznego dotyku, radia, ADC, opóźnienia STOP, instalacji i rollbacku OTA. Szczegóły i wyniki: [DEV_0_7_68_PL.md](DEV_0_7_68_PL.md).

0.7.67-dev: kalibracja baterii, szacowany procent i animacja trendu napięcia.

- Ustawienia → Bateria i zasilanie → Kalibracja baterii: typ ogniwa 1S Li-Po/Li-ion, napięcie nominalne, pojemność z etykiety, punkty napięcia 0/50/100% oraz korekta dzielnika względem równoczesnego odczytu multimetru. Profil służy do interpretacji pomiaru i nie programuje ładowarki. Wartości wstępne są przybliżone i wymagają sprawdzenia dla konkretnego ogniwa.
- Procent ma oznaczenie `~` i wynika z odcinkowej interpolacji trzech punktów. Pojemność jest metadanymi; firmware nie udaje pomiaru prądu ani pozostałego czasu pracy. Brak skalibrowanego ADC, wyłączony/uszkodzony profil, szum lub napięcie poza wiarygodnym zakresem nie daje procentu. Brak pomiaru po 30 s wygasza wskaźnik.
- Włączana w profilu animacja trendu: pomiar co 1 s, co najmniej 10 s obserwacji, 20 mV łącznego wzrostu i pięć niezależnych wzrostów o min. 2 mV. Tolerancja szumu 5 mV. Wskaźnik i opis oznaczają **prawdopodobne ładowanie**, bez ustawiania potwierdzonych pól ładowarki. Spadek >5 mV od szczytu, brak wzrostu przez >3 s, luka pomiarów >2,5 s, błąd ADC, wyłączenie funkcji, edycja profilu lub uśpienie zatrzymują animację. Zmiana obciążenia może wywołać taki sam trend; stan USB/ogniwa i rzeczywisty prąd ładowania pozostają nieznane. Po wyłączeniu animacji odczyty są co 20 s. Pomiary korzystają z istniejącego workera i ustępują obsługiwanym operacjom.
- Własny pojedynczy rekord NVS ze schematem i CRC32, walidacja przed zapisem oraz odczyt zwrotny. Błąd zapisu/odczytu nie daje komunikatu sukcesu; można ponownie odczytać profil lub jawnie utworzyć nowy. Zapis przez worker działa także offline. Klawiatura liczbowa, przewijanie, stałe Wstecz/Zapisz i ochrona niezapisanych zmian obejmują PL/EN/DE i cztery skórki.
- Testy obejmują rzeczywisty model/profil/ADC/worker, uszkodzenia rekordu, przerwy i szum trendu, zawinięcie zegara, stany zapisu UI, referencję multimetru, animację pikseli i 40 cykli bez ubytku puli LVGL. Dashboard 24 kafelków nadal mieści się po użyciu wszystkich formularzy. Testy hosta/CI nie zastępują próby na płytce; procedura: [BATTERY_CALIBRATION_PL.md](BATTERY_CALIBRATION_PL.md).
- Wydanie Dev. Aktualizacja: podpisane Dev OTA lub UPDATE-APP-ONLY. Zachowano partycje, istniejące dane urządzeń/Wi-Fi i klucz OTA. Nie scalono do main ani nie opublikowano Stable.

0.7.66-dev: spójne ramki Cyber Blue, przewijanie baterii i trwałe usuwanie kanału.

- Kontury kart, formularzy, okien i przycisków Cyber Blue mają wspólny turkus oraz 2 px. Zachowano znaczenie ostrzeżeń i błędów. Dodano Roboto Medium 12/14 i Bold 18/22/28 z polskimi i niemieckimi znakami; fonty innych skórek pozostają bez zmian.
- Bateria i inne przewijane panele przyjmują rzeczywiste gesty również na pustej przestrzeni. Nagłówek i dolne akcje pozostają nieruchome. Wskaźnik przewijania Cyber Blue jest turkusowy. Nagłówek Wstecz korzysta z tego samego celu co przycisk strony, w tym ochrony niezapisanych zmian.
- Karty urządzeń Cyber Blue rezerwują dwa jednoliniowe wiersze: stan, następnie pomieszczenie. Długie nazwy są skracane. Kompaktowa karta ma 104 px; sterowanie i zaakceptowane szczegóły rolet/termostatu zachowano. Przycisk + ma większy turkusowy symbol i stan naciśnięcia; powtórne otwarcie kreatora jest blokowane.
- Usuwanie urządzenia jest dostępne w edycji karty. Potwierdzenie zawiera nazwę i zakres jednego kanału. Jedna walidowana zmiana CAS usuwa wyłącznie jego ID oraz odwołania w pulpitach, grupach i scenach. Nie usuwa pomieszczeń ani scen. Po potwierdzonym zapisie workspace powiązanie NVS jest usuwane osobno; błąd jest widoczny i ponawiany. Brak karty trwale blokuje stare sterowanie również po restarcie. Nie resetuje fizycznego Shelly.
- W prawym górnym rogu znajduje się kontur baterii i aktualne napięcie BAT lub stan nieznany. ES3C35P dostarcza skalibrowany odczyt ADC1 GPIO8 przez nominalny dzielnik 100k/100k, medianę 9 pomiarów i wiek wyniku. Po 30 s wynik jest nieaktualny. Odczyt pozostaje na żądanie. Brak wiarygodnego pomiaru procentu, obecności ogniwa/USB i ładowania: firmware ich nie zgaduje. Komponent obsługuje opcjonalne potwierdzone dane fuel gauge/ładowarki, animację i niski poziom; na tej płytce te zdolności pozostają niedostępne. Animacja zatrzymuje się po utracie sygnału, zakończeniu ładowania lub uśpieniu ekranu. Stany procentów/ładowania w atlasie są wyłącznie jawnie oznaczonymi danymi testowymi hosta.
- Aktualizacja: podpisane Dev OTA lub UPDATE-APP-ONLY. Tablica partycji, NVS/SPIFFS, klucze OTA, pula LVGL 128 KiB i stos UI 8192 B pozostają bez zmian. FACTORY-CLEAN-16MB jest wariantem instalacji czystej, nie rutynową aktualizacją.
- Testy hostowe i CI nie stanowią odbioru na fizycznym ES3C35P. Gesty na LCD, pomiar BAT multimetrem, restart po usunięciu rzeczywistego kanału i aktualizacja z zachowaniem danych wymagają testu właściciela. Szczegółowy protokół: [CYBER_BLUE_FINAL_PL.md](CYBER_BLUE_FINAL_PL.md).

0.7.65-dev: turkusowe ikony, nowe karty systemowe i czytelniejsze sterowanie Cyber Blue HUD.

- Spójne turkusowe ikony w nagłówkach, nawigacji, akcjach i kafelkach. Dashboard ma symbol czterech kafelków, a zakładka zasobów — procesor. Zachowano odrębne kolory ostrzeżeń i brakujących danych.
- Testy sprzętu: większe napisy, osobne ramki informacji i stanu kontrolera dotyku, ikony oraz wyraźne przyciski testu dotyku i kolorów. Menu System ma spójne ramki z ikonami firmware, diagnostyki, testów, scen i restartu.
- Bateria i zasilanie: zwarta pierwsza karta z napięciem, opisem i wiekiem odczytu; osobna ramka ostrzeżenia o niewiarygodnych danych ładowania/ogniwa/procentu; czytelne wiersze RAM, PSRAM, grafiki, stosu i BLE. Wstecz znajduje się po lewej, Zmierz napięcie po prawej.
- Diagnostyka konfiguracji: osobne ramki SPIFFS, pamięci, HTTPS, USB, adresów danych i ostrzeżenia. Wyświetlane wartości pochodzą z rzeczywistego stanu; ekran pozostaje tylko do odczytu. Długie opisy PL/EN/DE zawijają się, a Wstecz pozostaje dostępne.
- Termostat: usunięto górną ramkę Nastawa, status Online/Offline z ikoną jest bez dodatkowego obramowania. Przyciski sterowania na Dashboardzie są wyśrodkowane pionowo; roleta ma czytelniejsze strzałki i podpisy akcji. Długie nagłówki dopasowują rozmiar czcionki.
- Weryfikacja interfejsu: 2471 kontroli na 75 ekranach PL/EN/DE, pełna regresja natywna z UBSan, kompilacja ES3C35P oraz atlas wszystkich 524 różnych widoków/stanów z 641 klatek (44 strony PDF). Testy obejmują rzeczywiste zakończenia pomiaru baterii, akcje sprzętowe i stany diagnostyki. Cykle otwierania ekranów nie wykazały narastania użycia puli LVGL; pula 128 KiB w PSRAM i stos UI 8192 B pozostają bez zmian. Numer wydania przechodzi osobne CI 5/5 i podpisany build.
- Odbiór fizyczny nowych poprawek na ES3C35P pozostaje do wykonania. Aktualizuj przez kanał Dev OTA lub UPDATE-APP-ONLY, aby zachować dane. FACTORY-CLEAN-16MB usuwa dane. To wydanie Dev; Stable nie jest publikowane.

0.7.64-dev: realistyczna roleta, czytelne ikony i zwarty Cyber Blue HUD.

- Roleta ma pełną ramę, kasetę, prowadnice i nieruchomy widok ogrodu odsłaniany przez ruchome lamele. 0% oznacza zamknięcie, 100% otwarcie. Podczas przeciągania pancerz natychmiast podąża za palcem; bursztynowy Podgląd i biały znacznik odróżniają cel od potwierdzonej pozycji. Puszczenie wysyła jedno polecenie. Utrata połączenia, zmiana urządzenia lub wyjście poza pole anuluje gest. Potwierdzone odczyty ruchu są wygładzane przez 300 ms.
- Okno otwarte pokazuje oba skrzydła rozwarte poza ramę, zamknięte — dwa frontowe skrzydła. Drzwi mają wyraźne skrzydło, ościeżnicę i klamkę. Kontury kontaktów są cyjanowe; otwarcie nadal wyróżnia bursztynowy status, a brak danych ma oddzielną neutralną ikonę.
- Bateria i zasilanie: pierwsza karta ma 130 zamiast 182 px; napięcie, ikona, opis, stan i wiek odczytu są bliżej siebie. Niepewność ładowania, ogniwa i procentu pozostaje jawna. Pamięć i radio mają pięć osobnych wierszy z ikonami.
- Sterowanie przekaźnikiem, oświetleniem/RGB, roletą, H&T i TRV rozdziela połączenie i stan urządzenia na ramki statusów. Online/Offline mają dedykowane ikony, a blokada oświetlenia czytelny komunikat PL/EN/DE. Usunięto nagłówek Główne sterowanie z Cyber Blue. Akcje, odświeżanie oraz W/V/A są osobnymi modułami.
- Dodano spójne ikony napięcia, prądu, mocy, PSRAM, grafiki, firmware, testów sprzętu, diagnostyki, restartu i połączenia oraz uzupełniono podstrony i listy. Zachowano identyfikatory pozostałych skórek i zapisanych ikon.
- Pełny atlas produkcyjnego LVGL obejmuje 468 różnych widoków/stanów z 565 klatek, w tym formularze, błędy, przewinięcia i warianty językowe. CI publikuje planszę PNG, 39-stronicowy PDF, wyszukiwalny HTML i komplet PNG w ZIP w artefakcie real-lvgl-ui-review-NOT-FIRMWARE. Opis, źródła grafiki i odtwarzanie: [cyber-blue/QUALITY_PASS.md](cyber-blue/QUALITY_PASS.md).
- Testy obejmują rzeczywiste zdarzenia pointera, stałość odsłoniętego ogrodu, kierunek pancerza, brak zapisów podczas gestu, anulowanie i odczyt w trakcie przeciągania oraz 100 cykli usuwania animowanego ekranu bez ubytku pamięci hosta. Wydanie przechodzi CI 5/5, audyty ELF, kontrolę pakietów i podpis istniejącym kluczem. Renderowane dane są fixture'ami; fizyczny LCD, dotyk, podłączona roleta i OTA pozostają do odbioru na ES3C35P.
- Aktualizacja przez kanał Dev OTA lub UPDATE-APP-ONLY zachowuje dane. FACTORY-CLEAN-16MB służy do czystej instalacji i usuwa dane. Format konfiguracji, transport, partycje i zaufanie OTA pozostają zgodne z poprzednią wersją.

0.7.63-dev: pełny Cyber Blue i atlas 434 ekranów i stanów.

- Cyber Blue: wspólny układ wszystkich 13 wykrytych grup ustawień Shelly, list pól, edytorów i potwierdzeń. Dostępne pola nadal wynikają z odpowiedzi urządzenia; wartości poufne pozostają ukryte. RGB i RGBW mają osobne nazwy.
- Dimmer/RGB: osobne pomiary W/V/A, wskaźnik jasności, przyciski -/+ po 5 punktów oraz suwak wysyłający zmianę dopiero przy puszczeniu. RGB ma podgląd koloru, trzy kolorowe kanały i jawne Zastosuj; wyświetlana wartość potwierdzona czeka na odczyt urządzenia. Nie dodano nieobsługiwanego kanału White ani scen.
- H&T: osobne karty temperatury i wilgotności, bateria, zasilanie zewnętrzne, odświeżanie i przewijana informacja o uśpieniu. Offline i błąd nie pokazują poprzednich pomiarów jako bieżących.
- Zużycie: turkusowy wskaźnik zakresu, oddzielne karty kWh/V/A i informacja o aktualności danych. Bez arbitralnych progów alarmowych i historii energii. Ustawienia wyświetlacza mają grupowane sekcje, a unieważnienie parowania otrzymało wyróżnienie akcji destrukcyjnej.
- Zachowano zaakceptowane sterowanie przekaźnikiem, termostatem, BLU i animację rolety oraz poprawki zakończenia BLE/restartu z 0.7.62-dev. Format danych, partycje, protokoły i zaufanie OTA pozostają zgodne z bazą.
- Pełny atlas z produkcyjnego LVGL 9.3.0: 434 odrębne klatki 320×480, także ekran startowy, formularze, przewinięcia, błędy i warianty PL/EN/DE. Eksporter scala tylko identyczne piksele, zachowuje aliasy i SHA-256; tworzy PNG, 37-stronicowy PDF, galerię HTML i ZIP. CI publikuje komplet w artefakcie real-lvgl-ui-review-NOT-FIRMWARE. Inwentarz, zakres i odtwarzanie: [cyber-blue/COMPLETE_AUDIT.md](cyber-blue/COMPLETE_AUDIT.md).
- Implementacja przeszła testy hosta ASan/UBSan, 1510 sprawdzeń atlasu, 44 testy panelu WWW i 2763 kontrole słowników. 60 cykli jasność/kolor/powrót nie zwiększa zużycia puli LVGL hosta. Publikacja wymaga ponownego CI 5/5 dla dokładnego commita wydania, audytów rzeczywistego ELF i podpisu w chronionym środowisku release.
- Dane renderów, zegar i numery wersji na ekranach testowych pochodzą z fixture'ów. Testy hosta i statyczny układ pamięci ELF nie zastępują odbioru fizycznego LCD, dotyku, BLE RF, runtime RAM/PSRAM i OTA na ES3C35P.
- Aktualizacja zachowująca dane: kanał Dev OTA lub UPDATE-APP-ONLY. FACTORY-CLEAN-16MB jest wariantem czystej instalacji usuwającym dane. Wydanie pozostaje Dev; bez publikacji Stable i scalania do main.

0.7.62-dev: dodawanie BLE kończy operację po błędzie; restart nie pozostaje zablokowany przez brak potwierdzenia rozłączenia.

- BLU oraz konfiguracja Wi-Fi przez BLE RPC: usunięte nieskończone pętle rozłączenia. Sprzątanie czeka do 4 s. Brak zdarzenia GAP kończy operację błędem, zwalnia globalną blokadę i izoluje stary host Bluetooth. Asynchroniczny reset hosta nie usuwa bondów. Nowe BLE jest blokowane do pełnego stop/deinit; restart oraz inne operacje nie dziedziczą wiszącej blokady. Późne callbacki starej sesji są ignorowane.
- BLU ma dodatkowo 120 s limitu całej operacji od przyjęcia zlecenia. Anulowanie przed uruchomieniem nie inicjuje połączenia. Wynik nieudanej sesji nie udostępnia klucza do rejestracji bramki. Nie ma ponawiania zapisów Wi-Fi ani ustawień.
- LCD: jedno potwierdzenie restartu anuluje pozostałe BLE i automatycznie ponawia żądanie restartu po zwolnieniu admisji. Oczekiwanie jest ograniczone do 10 s i można je anulować. Aktywny zapis lub OTA nadal chroni globalna bramka; nie dodano wymuszonego restartu.
- Komunikaty PL/EN/DE rozróżniają skan, zamykanie do 4 s i zakończony błąd BLE. Usunięto nakaz nierestartowania przy samym skanie/odczycie.
- OTA: obsługa `-ESP_ERR_HTTP_EAGAIN` również przy odbieraniu nagłówków, z zachowaniem częściowego parsera, jednego połączenia, 30 s budżetu, anulowania i walidacji podpisu/SHA/rozmiaru. Zdjęcie `5/4/-1/1` oznacza brak zakończonej odpowiedzi katalogu; samo zdjęcie nie rozstrzyga błędu DNS/TLS ani wpływu współdzielenia radia.
- Testy: brak callbacku rozłączenia i anulowania, nieistniejące już połączenie, późne zdarzenia, izolacja/reinicjalizacja hosta, globalny limit, brak zapisu po anulowaniu; rzeczywisty LVGL dla restartu w 4 motywach i 3 językach. Transport, radio i fizyczny przycisk czujnika pozostają do odbioru na urządzeniu. Partycje i zapisane dane bez zmian; użyć OTA lub UPDATE-APP-ONLY.

0.7.61-dev: zintegrowane ekrany Cyber Blue — termostat, przekaźnik, ustawienia i rzeczywista wizualizacja rolety.

- Integracja zaakceptowanych referencji oraz załączonego edytora `dsr_ui_device_settings.c`. Termostat: segmentowy pierścień, większe wartości, natywny dotyk po okręgu, symetryczne −/+ i karty zaworu/baterii. Zmiana nastawy aktualizuje istniejące obiekty; zapis nadal następuje raz po 1200 ms. Szkic jest oznaczony jako oczekujący; potwierdzone dane pozostają własnością bramki. Offline/błąd nie pokazuje starych temperatur jako bieżących.
- Przekaźnik: panel mocy, wyśrodkowany przycisk, odświeżanie i osobne karty napięcia/prądu. Brak potwierdzonego odczytu oznacza `— W/V/A` i blokadę sterowania; liczby nie są zastępowane pozornym zerem.
- Ustawienia urządzenia: sześć wierszy rzeczywistych metadanych oraz widoczna akcja otwierająca istniejący edytor Shelly. „Ogólne” ma karty nazwy, ECO, strefy czasowej, współrzędnych i serwera czasu, tylko gdy odpowiedź urządzenia udostępnia te pola. Formularze, potwierdzenie, walidacja i odczyt kontrolny korzystają z dotychczasowego aktora.
- Dwie zakładki „Sterowanie” / „Ustawienia” w czterech motywach, zgodnie z późniejszą instrukcją z przekazanego opisu. Wewnętrzny miernik pozostaje testowany osobno. Starsze referencje przekaźnika zawierają jeszcze trzecią zakładkę; jest to zamierzona różnica.
- Roleta Cyber Blue: pancerz wychodzi z kasety u góry; 0% oznacza zamknięcie, 100% odsłonięcie okna. Wygładzanie LVGL trwa 300 ms pomiędzy potwierdzonymi odczytami ruchu. STOP natychmiast pokazuje potwierdzoną pozycję, błąd zatrzymuje animację. Cel pod palcem pozostaje osobnym podglądem. Nie ma ekstrapolacji osiągnięcia celu ani automatycznego powtarzania komendy ruchu.
- Zintegrowane poprawki dotyku i pobierania OTA z 0.7.60 pozostają w tej wersji. Partycje i magazyny danych nie zmieniają się. Aktualizacja przez Dev OTA lub UPDATE-APP-ONLY zachowuje konfigurację.
- Porównanie z załączonymi referencjami i rzeczywiste klatki LVGL: [cyber-blue/recovered/README.md](cyber-blue/recovered/README.md). Dane renderów są testowe; odbiór czytelności, dotyku, komunikacji z rzeczywistą bramką i OTA na fizycznym ES3C35P pozostaje do wykonania.

0.7.60-dev: stabilniejszy dotyk, przewijanie bez przypadkowych kliknięć i poprawki pobierania OTA.

- Wszystkie cztery motywy: natychmiastowe naciśnięcie, potwierdzone puszczenie po 30 ms poprawnych odczytów bez kontaktu. Pojedynczy zanik sygnału nie tworzy drugiego kliknięcia. Nieudany odczyt I2C nie oznacza puszczenia; po 100 ms awarii gest jest anulowany i wymaga czystego puszczenia. Wiele kontaktów również anuluje gest.
- Bufor zachowuje naciśnięcie, scalony ruch i puszczenie w kolejności. Zapamiętuje przekroczenie 10 px również po powrocie palca. Kliknięcie jest blokowane po przeciągnięciu w dowolnym kierunku; natywne suwaki, rolety i pokrętło nadal otrzymują koniec przeciągania. Przepełnienie i stare gesty są anulowane. Brak globalnej blokady kolejnych świadomych dotknięć.
- OTA: `esp_http_client_read()` w IDF 5.5.4 może zwrócić `-ESP_ERR_HTTP_EAGAIN` po krótkim oczekiwaniu bez danych. Pobieranie kontynuuje wtedy ten sam strumień. Zachowano limit 30 s bez postępu, całkowity limit transferu, anulowanie, kontrolę podpisu/SHA/rozmiaru i jednorazowy zapis otrzymanych bajtów. Nie dodano automatycznego wznawiania po zerwaniu połączenia ani powtarzania operacji Flash.
- Zdjęcie właściciela z 0.7.58-dev pokazuje `12/4/0/0`: błąd instalacji/I/O po poprawnym manifeście. Ten stary kod nie odróżniał przerwy pobierania od błędu zapisu SDK i zawsze gubił HTTP instalatora. Nowa diagnostyka zachowuje HTTP, wynik instalatora, surowy błąd I/O i SDK. Komunikat rozróżnia pobieranie i zapis. Nie przypisujemy konkretnej przyczyny fizycznej próbie bez jej logu.
- Regresje odtwarzają błąd przed poprawką: krótką utratę kontaktu, przesunięcie między odczytami LVGL i timeout SDK. Testy obejmują cztery motywy, slider/arc, przerwanie i ponowne uzbrojenie, kolejkę i wygaśnięcie, trzy języki oraz rzeczywisty podpisany strumień instalatora z sumą SHA i błędami SDK. Źródła SDK/radia/Flash w testach hosta są modelami; odbiór na fizycznym pilocie pozostaje do wykonania.
- Aktualizacja zachowująca dane: kanał Dev OTA lub UPDATE-APP-ONLY. Poprawka downloadera zaczyna działać po uruchomieniu nowej wersji; jeśli stary firmware nadal przerywa pobieranie, użyj paczki UPDATE-APP-ONLY zgodnie z jej instrukcją. Zachowano partycje, NVS/SPIFFS, magazyn kluczy, limity pamięci i procedurę podpisu w chronionym środowisku wydania.

0.7.59-dev: nowy ekran BLU Cyber Blue, czytelne stany okien i drzwi oraz osobna karta bramki.

- Wyłącznie Cyber Blue: przebudowana podstrona Sterowanie dla czujników BLU, duża ikona 80 px i status 28 px, czytelne oświetlenie, bateria, RSSI i wiek sygnału. Osobna karta komunikacji przez bramkę i szeroki przycisk odświeżania z normalnym, wciśniętym, oczekującym i nieaktywnym stanem. Nagłówek i dolna nawigacja pozostają nieruchome podczas przewijania.
- Dedykowane, lekkie maski A8: okno i drzwi zamknięte (cyan), otwarte (pomarańczowo-złote) i brak danych (szaroniebieski). Symbol drzwi wynika z istniejącej konfiguracji ikony, nie z nazwy urządzenia. Brak danych, błąd odczytu lub szyfrowania nie potwierdza zamknięcia. Ikona okna uchylonego jest dostępna jako zasób, ale nie jest pokazywana bez rzeczywiście obsługiwanego stanu telemetrycznego.
- Zachowano istniejące odczyty i callback odświeżania przez bramkę, pozostałe zakładki, TRV oraz ogólne pomiary czujników. Nie zmieniono protokołów, formatu konfiguracji, partycji ani mechanizmu OTA. Grafitowy, Nocny i Jasny pozostają bez zmian; wybór motywu użytkownika nie jest nadpisywany.
- Kod BLU w commicie `719edb0` przeszedł CI 5/5: 894 sprawdzenia UI i 169 rzeczywistych klatek LVGL; 443 porównania wcześniejszych skórek są identyczne bajtowo. Przetestowano PL/EN/DE, długie nazwy, stany bez danych i błędów, dotyk, przewijanie, zakładki, modal pomocy i usunięcie wybranego urządzenia przy otwartym modalu. Rzeczywisty fixture bramki zachowuje odczyt zamknięte → otwarte → błąd szyfrowania bez nowego polecenia SET. Rendery i porównanie: [cyber-blue/blu-control/README.md](cyber-blue/blu-control/README.md).
- Koszt względem 0.7.58: zmierzony obraz kodu BLU 4 068 976 B (+66 224 B); DIRAM 212 967 B, wewnętrzny BSS 81 792 B i zewnętrzny BSS 152 376 B bez zmian. Istniejąca pula LVGL 131 072 B w PSRAM i stos UI 8192 B bez zmian. Porównawczy test hostowy otwartego ekranu: +3072 B w tej puli; 80 otwarć/zamknięć i 80 przełączeń skórek bez narastania. Nie jest to pomiar szczytu pamięci SDK na ESP32-S3. Commit z numerem wydania przechodzi osobne pełne CI i build.
- Odbiór fizyczny nowego układu, dotyku LCD, świeżych odczytów prawdziwej bramki, pamięci SDK i OTA/rollback pozostaje do wykonania przez właściciela. Aktualizuj przez kanał Dev OTA lub UPDATE-APP-ONLY, aby zachować dane. FACTORY-CLEAN-16MB usuwa dane i nie jest rutynową aktualizacją. Wydanie Dev nie jest wydaniem Stable.

0.7.58-dev: opcjonalny Cyber Blue Premium, czytelniejsze dane, formularze i klawiatura oraz nowe ikony.

- Nowa skórka Shelly HUD — Cyber Blue jest dodatkową opcją w istniejących ustawieniach wyglądu. Grafitowy, Nocny i Jasny zachowują wygląd, identyfikatory i zapisane ustawienia; wybrany motyw nie zmienia się automatycznie.
- Cyber Blue Premium: ciemne metaliczne panele, lekkie gradienty, turkusowe stany aktywne, dedykowane ikony i spójne formularze, dialogi oraz klawiatura. Etap 3 poprawia hierarchię pulpitu, rolety, BLU, baterii/diagnostyki, OTA, Wi-Fi i edytorów. Nazwy, główne wartości i statusy są większe; długie teksty mają ograniczenia lub przewijanie. Stopka pozostaje dostępna przy otwartej klawiaturze.
- Edycja pomieszczeń i pulpitów: czytelne pola, podgląd zdjęcia, wybór symbolu, osobne Zapisz/Wstecz i czerwone usuwanie z potwierdzeniem nazwy. Zintegrowano 17 dodatkowych ikon konturowych w trzech rozmiarach A8. Nie zmieniono protokołów Shelly, logiki sterowania, formatu konfiguracji ani mechanizmu OTA i partycji.
- Weryfikacja etapu 3: rzeczywiste rendery LVGL 9.3.0 320×480, PL/EN/DE, długie nazwy, formularze, klawiatura, zapis/anulowanie/usuwanie, wielokrotne otwieranie i przełączanie skórek. 344 obrazy trzech wcześniejszych skórek pozostają identyczne bajtowo. Wyniki i porównania przed/po: [docs/cyber-blue/stage3/README.md](cyber-blue/stage3/README.md).
- Koszt etapu 3 względem etapu 2: obraz aplikacji +70 324 B, wewnętrzny BSS +32 B. Pula LVGL 131 072 B w PSRAM i stos UI 8192 B bez zmian. W porównawczym teście hostowym ekran baterii zajmuje dodatkowe 4824 B w istniejącej puli; cykle otwierania i przełączania nie wykazały narastania. To wyniki kompilacji i symulatora, nie pomiar szczytu pamięci ESP32-S3.
- Odbiór na fizycznym ES3C35P pozostaje do wykonania przez właściciela: dotyk, czytelność LCD, komunikacja z urządzeniami, pamięć SDK i aktualizacja/rollback OTA. Do aktualizacji z zachowaniem danych służy OTA lub UPDATE-APP-ONLY zgodnie z instrukcją. Wydanie Dev nie jest wydaniem Stable.

0.7.57-dev: płynniejszy dotyk, wspólne zakładki grup/scen, grafiki BLU/TRV i poprawiony układ OTA.

Zakres 0.7.57-dev. Odbiór fizyczny nowych poprawek pozostaje do wykonania.

- OTA: przyciski Sprawdź aktualizacje i Zmień kanał podniesione o 36 px, a kod błędu kontroli wydania o 32 px. Oba pełne pola dotyku mieszczą się w początkowym widoku nad Wstecz; regresja PL/EN/DE obejmuje również naciśnięcie dolnej krawędzi Zmień kanał.

- Główne zakładki Pomieszczenia / Grupy / Sceny / Urządzenia mają wspólny nagłówek i dolną nawigację. Edycja i powrót pozostają w kontekście danej listy. Nie zmieniono formatu zapisanych danych ani wykonania poleceń.
- Zegar po lewej, za przyciskiem powrotu na szczegółach. Nazwa urządzenia nie uruchamia ciągłej animacji przewijania.
- Modal informacji nie jest usuwany przez odczyty w tle. Podczas dotykania lub przewijania stabilnej listy odrysowanie czeka do puszczenia palca; zmiana tożsamości nadal unieważnia stary widok. Dane innego urządzenia nie przebudowują otwartych szczegółów.
- Aktualizacja mocy/napięcia odświeża istniejącą etykietę zamiast całej karty z cieniem. Test 24 kafelków: widoczna zmiana około 0,7 ms / 2784 px, niewidoczna 0 px; pełna przebudowa około 119 ms / 153600 px. To pomiary hostowego LVGL, nie ES3C35P.
- Dotyk ES3C35P: zachowanie obu zboczy krótkiego dotknięcia podczas długiego rysowania, scalanie ruchu, bezpieczne odrzucenie przepełnionego gestu oraz zdarzeń starszych niż 500 ms (bez odtwarzania dotknięć z ekranu startowego); odczyt LVGL co 10 ms. Log USB zgłasza przebiegi LVGL powyżej 200 ms (najwyżej raz na 5 s).
- BLU TRV: ikony zaworu i baterii. Ustawienia rozróżniają temperaturę zadaną i natężenie światła — współdzielone pole modelu nie jest już formatowane jako lx. Prawdziwe lx czujnika kontaktowego pozostają widoczne.
- Czujnik kontaktowy: grafika otwartego/zamkniętego okna; po wybraniu ikony Drzwi na lokalnej karcie — otwarte/zamknięte drzwi. Przy braku połączenia widok nie potwierdza zamknięcia.
- Nagrania IMG_9761 i IMG_9762 oraz zdjęcia IMG_9755/9759 przeanalizowane. Testy sprzętowe płynności, opóźnienia i stanów czujników pozostają do wykonania na kandydacie.


Zakres 0.7.56-dev. Testy fizyczne poprawek pozostają do wykonania; Stable: brak.

- LCD: usunięty ogólny nagłówek domu we wszystkich językach i dodatkowa zakładka pulpitu; większy zegar, uporządkowane odstępy, bateria bez wystającego prostokąta. Powrót ma większe pole dotyku, a Bateria działa również z dolnego paska ustawień.
- Karty: sterowanie wyśrodkowane pionowo, wspólne obramowania przycisków; czerwone przyciski usuwania. Wskaźnik przewijania ma 2 × 36 px i leży w prawym marginesie. Numer kroku dodawania przeniesiony niżej, poprawione niemieckie teksty parowania BLU.
- Jawne pomiary baterii, zapis ekranu i OTA oczekują za trwającym zwykłym odczytem zamiast odmawiać z powodu zajętości. Kolejna automatyczna próba ich nie wyprzedza; blokady zapisu, STOP i jednorazowa zgoda OTA pozostają wymagane. Zadanie dotyku ma pierwszeństwo przed UI i transportem.
- Roleta: po poleceniu, podczas ruchu i po pierwszych dwóch błędach odczytu ponowienie GET po 1 s. Potwierdzony odczyt przywraca rzeczywistą pozycję po błędzie polecenia. Żadne polecenie ruchu nie jest automatycznie powtarzane; utracony ACK nie oznacza osiągnięcia zadanej pozycji.
- Termostat BLU TRV: uproszczony ekran i natywne pokrętło LVGL z przeciąganiem po okręgu. Jedna nastawa po 1,2 s od puszczenia palca; opuszczenie strony, utrata dotyku, zmiana tożsamości lub zewnętrznej nastawy odrzuca niewysłany szkic. Ukryty timer nie może odtworzyć ekranu nad ustawieniami.
- OTA: pasek postępu oparty na rzeczywistej liczbie bajtów od początku instalacji; pełny przycisk Anuluj nad stopką. Zdjęcia i oba filmy właściciela przeanalizowane; testy fizyczne pozostają do wykonania na nowym kandydacie.

- Tworzenie na stronie Grupy lub Sceny w LCD i Web Panelu: **nazwa → urządzenia/polecenia → podsumowanie i zapis**. Pomieszczenie i zdjęcie są opcjonalne, w osobnym formularzu; nowy wpis nie dostaje przypadkowego zdjęcia.
- Grupa: widoczna lista urządzeń z zaznaczeniami i licznikiem. Scena: „Dodaj polecenie” → urządzenie → jawnie wybrane Włącz/Wyłącz lub Otwórz/Zamknij/Stop. Wstecz z wyboru nie dodaje domyślnego polecenia. Kolejność można zmieniać w obie strony, kroki usuwać, a podsumowanie pokazuje nazwy i polecenia.
- Wyszukiwanie LCD uwzględnia wielkość liter PL/EN/DE. Karty bez wybranego zdjęcia są niższe. Stopka znika podczas wpisywania na klawiaturze i wraca po zakończeniu.
- Anulowanie w WWW odrzuca tylko bieżący wpis po potwierdzeniu, zachowując niezależne zmiany. Dokładny trwały ACK, rewizje, zachowanie późniejszych zmian i ponowienie po błędzie pozostają wymagane. Tworzenie/edycja nie uruchamia urządzeń.
- Zakres wykonania i schemat bez zmian: lokalne grupy Switch ON/OFF, ręczne sceny do 8 poleceń Switch/Cover. Nie dodano chmurowych warunków ani harmonogramów. Dotychczasowe pokoje i zdjęcia pozostają obsługiwane.
- Procedura obsługi i odbioru fizycznego: [SCENES_GROUPS_PL.md](SCENES_GROUPS_PL.md). Testy hostowe, rendery LVGL i Chromium nie oznaczają fizycznego odbioru.

Zakres opublikowanego 0.7.55-dev: parowanie BLU i ustawienia Shelly Gen2/3/4 na LCD.

- Strona urządzenia → Ustawienia → Ustawienia Shelly: rzeczywisty odczyt `ListMethods`, `GetComponents` i `GetConfig`. Edytor pokazuje obecne, obsługiwane pola ogólne, wejść, przekaźników, rolet, świateł/RGB/RGBW, Wi-Fi, Bluetooth, Cloud, MQTT oraz temperatury i wilgotności. Nazwa lokalnej karty pozostaje oddzielna od nazwy w Shelly.
- Zapis wymaga zatwierdzenia na LCD. Pilot odczytuje aktualną konfigurację, odrzuca konflikt i zapisuje tylko wybrane pole. Odczyt potwierdza wynik również po utracie ACK; brak odczytu lub hasło tylko do zapisu daje wynik niepotwierdzony. Zmiana sieci używa osobnego formularza SSID i hasła, z jawnym wyborem sieci otwartej.
- BLU: wybór wykrytego urządzenia, fizyczne okno parowania, SMP/bonding w istniejącym managerze NimBLE, maskowany PIN i trwały magazyn do 16 bondów SDK. Pełny magazyn odmawia; pilot nie usuwa innych powiązań. Zapomnienie dotyczy tylko wybranego peera. Istniejący PIN i szyfrowanie BLU nie są zmieniane.
- Ustawienia GATT klasycznych BLU: beacon; Motion — czułość, impulsy, podtrzymanie, LED; Door Window — próg kąta; Button1 — dźwięk, wieloklik, znajdowanie (wymaga beacon). H&T nie dostaje nieistniejącego przełącznika beacon. TRV — regulacja, flagi z zachowaniem pozostałych bitów, blokada, wyświetlacz, korekta i limity temperatur, minimalne otwarcie zaworu i domyślne czasy boost/override. Pole jest dostępne tylko przy właściwej charakterystyce GATT.
- Wybór zgodnej bramki LAN: lokalny BTHome, ponowne użycie istniejącego adresu i uzupełnianie czujników. Każde AddDevice/AddSensor jest wysyłane raz; po możliwym zapisie pilot sprawdza stan. Klucz szyfrowanego czujnika trafia bezpośrednio z uwierzytelnionego połączenia do wybranej bramki, bez eksportu czy wyświetlania. Zapis karty pozostaje osobnym trwałym krokiem. Odbiór danych jest potwierdzany wyłącznie świeżą próbką przy prawidłowym czasie pilota.
- TRV: osobny przepływ `BLE.StartAssociations(target=8)` na bramce, bez wildcard. Bramka nie udostępnia udokumentowanego filtra adresu tej operacji, więc instrukcja wymaga trybu parowania tylko wybranej głowicy. Wynik musi zawierać jej dokładny adres i powiązane BluTrv/BTHome. Krótki Reset służy do parowania TRV; długie przytrzymanie nie jest zalecane.
- Formularze PL/EN/DE, stała stopka nad klawiaturą. Zdalny podgląd LCD jest blokowany przez cały czas konfiguracji/PIN-u/haseł. Nowa konfiguracja jest dostępna z fizycznego LCD, bez nowego Web API do sekretów lub arbitralnych RPC.
- Zgodność: dotychczasowe partycje, rejestr i limity: UI 8192 B, worker 10240 B, LVGL 131072 B, wewnętrzny BSS ≤81920 B. Bondy używają NVS NimBLE; prototyp nie ma szyfrowanego NVS. Starszy firmware nie korzysta z nowych bondów; nie używać czyszczenia NVS jako rutynowej aktualizacji.
- Zakres: lokalne Wi-Fi RPC Gen2/3/4 oraz klasyczne BLU o rozpoznanym modelu. Przypisanie do konta Shelly Cloud, Zigbee/Matter commissioning, nowe modele BLU/ZB, OTA/reset urządzeń zewnętrznych, harmonogramy TRV i mapowania BTHomeControl nie są dodane. Nie każda bramka Plus ma dynamiczne BTHome; wymagane metody są sprawdzane na żywo.
- Testy hostowe/LVGL oraz build CI nie zastępują odbioru fizycznego: pairing/PIN, trwałość po wyłączeniu, odczyt po zapisie, odbiór na bramce, współpraca z aplikacją i obciążenie BLE+HTTPS+dotyk pozostają do wykonania na urządzeniach właściciela. Nie przeprowadzono żadnych zapisów na fizycznym Shelly.

0.7.54-dev: sceny i grupy edytowane na swoich stronach. Stable: brak.

- LCD: dodawanie i edycja bez przechodzenia do konfiguratora; karty ze zdjęciem, nazwa i pokój, oddzielne sterowanie i ołówek. Ołówek na pulpicie otwiera konkretny wpis. Wyszukiwanie oraz filtr pomieszczeń na liście.
- Cztery kroki: typ/uruchomienie, urządzenia lub działania, pomieszczenie, nazwa i zdjęcie. Urządzenia grupowane według pokoju, licznik zaznaczeń, wybór zdjęć wbudowanych i własnych zapisanych w pilocie. Scena pozwala wybrać polecenie, zmienić kolejność i usunąć krok. Oddzielna stopka nie przesłania przewijanej zawartości; podczas wpisywania widoczna jest klawiatura.
- Web Panel: jedna lista kart z edycją i sterowaniem na stronach Grupy/Sceny, wyszukiwanie, filtr pokoju i formularze czterech kroków. Własne zdjęcie można przesłać bezpośrednio z formularza.
- Zapis nadal wymaga dokładnego trwałego ACK i zgodnej rewizji. Błąd zachowuje formularz, późniejsze zmiany nie są zastępowane wcześniejszym zapisem, porzucenie zmian na LCD wymaga jawnego potwierdzenia. Edycja nie uruchamia urządzeń.
- Zgodność: opcjonalne pola `room` (0 lub istniejący pokój) i `photo` (lokalny identyfikator) dla scen/grup w schemacie 2. Dotychczasowe wpisy działają bez migracji. Zdjęcia wskazane w dowolnym z dwóch checkpointów przez pokoje, grupy lub sceny są zachowywane. Usunięcie pokoju używanego przez scenę/grupę jest blokowane. Po zapisaniu nowych pól powrót do 0.7.53 lub starszej wymaga usunięcia tych pól z kopii konfiguracji; starszy walidator ich nie zna.
- Zakres wykonania bez zmian: grupy przekaźników/gniazdek Switch ON/OFF; sceny ręczne, do 8 działań Switch/Cover. Nowy interfejs nie dodaje automatycznych warunków, harmonogramów, grup Light/Dimmer/RGB ani TRV. Ich referencje graficzne nie oznaczają zaimplementowanego wykonawcy.
- Właściciel potwierdził udaną instalację 0.7.53-dev. Nowy kreator wymaga odbioru na ES3C35P; testy hosta i CI nie zastępują tego sprawdzenia.

0.7.53-dev: porządek w P0, nazwy Shelly i kanały. Stable: brak.

- Przyczyna dawnych przerwań SPIFFS jest potwierdzona przez właściciela: zewnętrzny reset podczas formatowania, gdy ekran nie informował o wykonywanej czynności. Zachowane: ekran przygotowania pamięci z postępem odczytywanym z Flash, bezpieczne wznowienie wyłącznie rozpoznanej pustej resztki formatu i odmowa kasowania obcych danych.
- Zintegrowane zaległości: Settings bez odsyłacza do usuniętej historii, kontrola gotowości magazynu przy zatwierdzaniu OTA, model/wersja/generacja z rzeczywistego odczytu urządzenia. Zegar, diagnostyka baterii i zwalnianie BLE pozostają z gałęzi P0.
- Nazwa lokalna ma pierwszeństwo. „Użyj nazwy z Shelly” w edytorze LCD/WWW usuwa lokalny wpis i włącza odczyt nazwy przez `Sys.GetConfig` po zweryfikowaniu tożsamości oraz statusu. Pusta nazwa Shelly daje nazwę modelu lub ID; duplikaty mają dopisek ID/kanału. Nie zapisujemy konfiguracji do Shelly. Błędna/niedostępna nazwa nie usuwa ostatniej poprawnej nazwy ani telemetrii. Odczyt nazwy ma limit 650 ms, zapis jej kopii do NVS najwyżej raz na minutę na endpoint (po błędzie ponowienie w następnym oknie).
- Nazwy UTF-8 są skracane na granicy znaków. Nazwa/SKU nie jest kluczem sterowania. `profile` ogranicza typ aktuatora, status komponentów dostarcza kanały; Cover 0 i 1 pozostają oddzielnymi endpointami. Nieznany/zmieniony profil nie uruchamia zastępczego przekaźnika.
- Publikacja DEV ma bramkę CI 5/5 dla dokładnego commitu, podpisany manifest i paczki UPDATE-APP-ONLY oraz FACTORY-CLEAN-16MB. FACTORY kasuje dane i służy osobno zatwierdzonemu czystemu testowi; do zachowania danych wybierz UPDATE zgodnie z instrukcją w paczce.
- Zgodność: bez zmiany partycji, formatu rejestru NVS i limitów pamięci. Pusta lokalna nazwa w schemacie 2 oznacza synchronizację; powrót do firmware 0.7.52 lub starszego wymaga wcześniejszego wpisania niepustych nazw lokalnych. Dotychczasowe nazwy nie są automatycznie czyszczone.
- Fizyczne testy urządzeń, dalszy UX i grafika są kolejnym etapem. Wyniki hosta i CI nie oznaczają odbioru sprzętowego.

0.7.52-dev: płynniejszy dotyk i diagnostyka pamięci. Stable: brak.

Poprawki gałęzi roboczej (bez nowego wydania):
- Settings nie odsyła już do usuniętej historii 1 h / 24 h ani zakładki Wykresy. Test rzeczywistych obiektów LVGL sprawdza PL/EN/DE.
- Sama funkcja zatwierdzająca nowy obraz OTA wymaga poprawnego startu SPIFFS i workspace. Dotychczasowy jedyny caller UI już sprawdzał workspace/panel/dotyk; to dodatkowa ochrona kontraktu API, nie dowód zaobserwowanej awarii rollbacku. UI odzyskiwania nadal działa bez magazynu; nie dodano automatycznego resetu ani kasowania. Test startu odrzuca niegotowe fazy i błędy obu warstw.
- Sprostowanie właściciela 2026-10-18: historyczne formatowania przerwał zewnętrzny reset; brak informacji na ekranie skłaniał do resetowania. Przyczyna została potwierdzona. Testy hostowe/CI nadal nie zastępują odbioru nowej wersji na sprzęcie.

Zmiany względem 0.7.51-dev:
- BLE nie zostaje już włączone do restartu: stos NimBLE i kontroler Bluetooth są zwalniane 30 s po ostatnim skanie lub dodawaniu przez BLE i uruchamiane ponownie przy następnym użyciu. Wcześniej po pierwszym użyciu BLE Wi-Fi do restartu dzieliło radio z Bluetooth, a kontroler trzymał RAM wewnętrzny.
- Wątek ekranu ma priorytet 5, wyżej niż wątek sieciowy (4): przetwarzanie dużych odpowiedzi Shelly/BLU nie dzieli już procesora po równo z obsługą dotyku.
- Dotyk: panel odczytywany co 10 ms (było 20 ms), LVGL odbiera dotyk co 16 ms (było 33 ms).
- Strona baterii (dolny pasek) pokazuje "Pamięć i radio": wolny RAM wewnętrzny, minimum od startu, największy blok, PSRAM, wolną pamięć grafiki, zapas stosu ekranu i stan BLE - do porównania przed i po użyciu BLE.
- Bez zmian: tryb oszczędzania energii Wi-Fi (wyłączenie go zmniejszyłoby opóźnienia odpowiedzi kosztem poboru prądu - decyzja właściciela).

0.7.51-dev: uproszczenie aplikacji i dokończenie oprawy graficznej. Stable: brak.

Zmiany względem 0.7.50-dev (decyzje właściciela 2026-10-18):
- Usunięte: historia energii 24 h (zapis w tle do SPIFFS, zadania workera, API `/api/v1/history`, wykres w Web Panelu), strona Energia i ekran Aktywność na LCD. Zapis historii współdzielił jedyny wątek z odpytywaniem Shelly i zatrzymywał pamięć Flash przy zapisie. Istniejące pliki historii w SPIFFS nie są kasowane (wymaga osobnej zgody). Numery zadań 17/18 zostają zarezerwowane.
- Zostaje: zegar mocy "Zużycie" na stronie urządzenia (bieżący odczyt, licznik kWh z Shelly).
- Dolny pasek: zamiast Energia - diagnostyka baterii. Grupy i sceny tylko w zakładce Pomieszczenia. Dziennik aktywności pozostaje w Web Panelu.
- Pasek stanu: godzina obok Wi-Fi i baterii (czas środkowoeuropejski CET/CEST, SNTP pool.ntp.org uruchamiany automatycznie po połączeniu Wi-Fi; bez czasu pole jest puste).
- Pasek przewijania po prawej na wszystkich długich listach i formularzach.
- Ikony urządzeń do wyboru jak w aplikacji Shelly (Auto + 20, m.in. gniazdko, wentylator, termostat, TV, bojler, brama, lampy). Pole `icon` zapisywane w konfiguracji tylko przy wyborze innym niż Auto - po wyborze ikony powrót do 0.7.50 lub starszej odrzuci konfigurację, dopóki ikona nie wróci na Auto.
- Dodawanie przez Wi-Fi: karta urządzenia pokazuje nazwę z Shelly lub linię produktu, SKU, generację, IP, MAC lub znalezione kanały i wymóg hasła; lista wyników pokazuje nazwę z Shelly.
- Czytelność: najmniejsza czcionka 11 px zastąpiona 12 px, wartości w kapsułach i podpisy pól 14 px.
- Ikony Material Symbols renderowane przez Chromium (cairosvg niedostępny w środowisku budowania).

0.7.50-dev: szybsze działanie interfejsu i stabilność dużych pulpitów. Stable: brak.

Zmiany względem 0.7.49-dev (bez zmian wyglądu, bootloadera, partycji i trybu Flash):
- Odczyt (poll) bez zmian wartości nie przebudowuje już pulpitu. Wcześniej pulpit z własnym układem był przebudowywany przy każdym odczycie (błąd: pola kafelków nadpisywały model).
- Zmiana wartości jednego urządzenia przebudowuje tylko jego karty (i karty pokoi, które sumują moc), zamiast całego ekranu; przewinięcie zostaje.
- Wspólne style LVGL zamiast kilkunastu lokalnych właściwości na każdym obiekcie: 24 kafelki mieszanych urządzeń zostawiały < 2 KB z puli 128 KB i pierwsze rysowanie zawieszało się w alokacji cienia; teraz zostaje ok. 16 KB (pomiar hosta). Pula i limity bez zmian.
- Wyświetlacz: dwa bufory DMA po 20 linii (ta sama pamięć co jeden 40-liniowy) - rysowanie następnego pasa w trakcie wysyłania poprzedniego.
- Kompilacja firmware płytki z -O2 (było -Os) i memcpy/memset z ROM/newlib dla LVGL. BIN 3,29 MB -> 3,67 MB, 42% slotu wolne.

0.7.49-dev: odświeżenie wyglądu w stylu Shelly Smart Control (etap 1). Stable: brak.

Zmiany względem 0.7.48-dev (wzór: zrzuty aplikacji Shelly od właściciela):
- Wiersze urządzeń: pasek nagłówka, czarne kółko z ikoną, wartości w jednej kapsule z separatorami, jasny okrągły przycisk zasilania w obwódce; włączone urządzenie ma niebieski pierścień i niebieskie obramowanie karty z poświatą. Roleta: kapsuła góra/dół. Jasny motyw ma własne odpowiedniki kolorów.
- Strona urządzenia: zakładka "Zużycie" zamiast "Wykresy" - zegar mocy jak w aplikacji (łuk zielony-żółty-czerwony, podziałka, wskazówka, W lub kW, "Ostatnia aktualizacja", licznik kWh). Odczyt starszy niż 15 s jest pokazywany jako brak odczytu, nigdy jako stara wskazówka.
- Ustawienia > System: usunięty "Podgląd rolety • DEMO".
- Synchronizacja czasu UTC dla historii zostaje na stronie "Energia".

0.7.48-dev: termostat BLU TRV w stylu Shelly Smart Control. Stable: brak.

Zmiany względem 0.7.47-dev (wzór: zrzuty aplikacji Shelly od właściciela):
- Ekran termostatu: rząd wartości (temperatura, cel, zawór, bateria), łuk 4-30 °C od niebieskiego do czerwonego z gałką na celu, aktualna temperatura i duży cel w środku, okrągłe -/+.
- -/+ wysyłają cel 1,2 s po ostatnim dotknięciu (bez przycisku "Ustaw"); odczyt co 15 s jak w aplikacji (bez przycisku odświeżania).
- Lista urządzeń: wiersz TRV z pastylką "- cel +" po prawej.
- Poprawka: przy TRV offline lista mogła pokazać cel jako "lx".

0.7.47-dev: termostat BLU TRV, stabilny odczyt czujników BLU, wbudowane zdjęcia pomieszczeń. Stable: brak.

Zmiany względem 0.7.46-dev:
- Czujniki BLU: wartości nie znikają co 15 s. Odczyt uznawany za aktualny przez 45 s (bramka jest odpytywana co 15 s, jeden odczyt to kilka zapytań); odczyt w tle nie zasłania wartości.
- Termostat BLU TRV (przez bramkę): temperatura przy głowicy, cel, zawór, bateria; ustawianie celu 4-30 °C co 0,5 °C przyciskami -/+ i "Ustaw" (BluTrv.Call TRV.SetTarget). Pilot pokazuje to, co raportuje bramka; niepewny zapis nie jest powtarzany. Web Panel pokazuje termostat (bez ustawiania).
- Pomieszczenia: 8 wbudowanych zdjęć (Salon, Kuchnia, Sypialnia, Łazienka, Przedpokój, Piwnica, Strych, Rozdzielnia) do wyboru w edytorze; własne zdjęcia z Web Panelu działają jak dotąd.
- Poprawka: zapis z treścią spoza pola żądania nie wyłączał już ponowienia logowania digest (dotyczyło tylko nowej ścieżki TRV; provisioning AP bez zmian).

0.7.46-dev: czujniki BLU w kreatorze pod nazwą z bramki albo modelem. Stable: brak.

Zmiany względem 0.7.45-dev:
- Lista kanałów bramki: nazwa czujnika ustawiona na bramce (jeśli jest), inaczej model i końcówka MAC, np. "BLU Door/Window 0b11", "BLU TRV b567". Ta sama etykieta staje się domyślną nazwą urządzenia.
- Znane modele: BLU Door/Window (model 2), BLU TRV (model 8), potwierdzone na prawdziwej bramce; pozostałe jako "Czujnik BLU" + MAC, dopóki nie potwierdzimy numerów.
- Nazwy z aplikacji Shelly są w chmurze, nie na bramce; nazwę widoczną dla pilota ustawia się w panelu WWW bramki.

0.7.45-dev: bramka BLU Gateway Gen3 rozpoznaje czujniki, automatyczny restart po aktualizacji. Stable: brak.

Zmiany względem 0.7.44-dev (dev-v0.7.44 = kod 0.7.43):
- Czujniki BLU na bramce: pilot czyta listę z Shelly.GetComponents (stronami), bo Shelly.GetStatus bramki BLU Gateway Gen3 ich nie podaje. Dotyczy tylko urządzeń Shelly z Bluetooth.
- BLU TRV z dwiema temperaturami: pokazywana pierwsza (idx 0).
- Po zainstalowaniu aktualizacji z pilota restart następuje sam po 10 s; "Później" go wstrzymuje, "Uruchom ponownie" od razu.

0.7.43-dev: naprawa instalacji OTA z pilota (kod 12/11: limit czasu pobierania obrazu). Stable: brak.

Zmiany względem 0.7.42-dev:
- Pobieranie obrazu aplikacji miało ten sam łączny limit 30 s co manifest; 3,3 MB z zapisem Flash nie mieściło się. Teraz: przerwanie po 30 s bez postępu albo po 10 min łącznie.
- Partycja docelowa jest kasowana sektor po sektorze w trakcie zapisu (zamiast całej z góry w środku pobierania).
- Podpis, SHA-256 i rozmiar weryfikowane jak dotąd.
- Uwaga: wersje do 0.7.42 mają ten błąd, więc na 0.7.43 trzeba przejść raz przez USB (0x20000) lub Web Panel.

0.7.42-dev: czujniki Shelly BLU (Motion, Door, Window) odczytywane przez bramkę Shelly w sieci LAN. Stable: brak.

Zmiany względem 0.7.40-dev (dev-v0.7.41 zawiera kod 0.7.40, bez BLU):
- Shelly BLU: Dodaj urządzenie > Skanuj sieć Wi-Fi > wybierz Shelly, który już odbiera czujnik (np. Plus/Pro/Gen3 lub BLU Gateway Gen3) > kanał "Czujnik BLU N". Pilot odpytuje bramkę co 15 s (BTHomeDevice/BTHomeSensor), tylko odczyt: ruch, otwarcie, jasność, bateria, temperatura, wilgotność, RSSI.
- Tożsamość czujnika to identyfikator bramki + MAC czujnika; inny czujnik na tym samym slocie bramki jest odrzucany.
- Rejestr połączeń: bez czujnika BLU zostaje schemat 6 (zgodny z 0.7.40, także przy automatycznym rollbacku). Po dodaniu czujnika BLU zapisywany jest schemat 7, którego 0.7.40 nie odczyta (odmowa bez kasowania danych); przed ręcznym powrotem do 0.7.40 usuń czujniki BLU.
- Ekran skanu BLE wskazuje, że czujnik BLU dodaje się przez bramkę.
- Web Panel: karta BLU w stanie rzeczywistym.

0.7.40-dev: naprawa OTA z pilota (rekordy TLS 16 KiB, przekierowanie CDN GitHub), katalog Dev stronami, kody błędów OTA. Stable: brak.

Zmiany względem 0.7.37/0.7.38:
- TLS: bufor odbioru 16 KiB w PSRAM; github.com wysyła ~6 KB nagłówków w jednym rekordzie, 4 KiB powodowało błąd I/O (kod 16/4 w 0.7.39).
- OTA z pilota: akceptuje przekierowanie na objects.githubusercontent.com (przyczyna FAIL w 0.7.37); Dev czyta katalog po jednym wydaniu na stronę; LCD pokazuje kod błędu, log USB linię "native_ota".
- BLE: rozpoznaje Shelly Gen2+ bez UUID RPC w reklamie; więcej informacji na liście skanu.
- Pierwsze formatowanie i wznowienie przerwanego formatowania SPIFFS z postępem na LCD.
- Ekran startowy z logo, tło aplikacji, nowy styl "Jasny".
- Nowe instalacje startują po angielsku; nazwy pomieszczeń i urządzeń nie są tłumaczone.
- Edytor pomieszczenia mieści się bez ucinania tekstu (PL/EN/DE).

Instalacja:
- Pierwsza instalacja USB: DIY-Shelly-Remote-es3c35p-FACTORY-CLEAN-16MB.zip od 0x0 (kasuje dane).
- 0.7.39 lub starsza: raz wgraj diy_shelly_remote.bin przez Web Panel (Zezwól na OTA) lub USB app-only; potem Ustawienia > Firmware Update > kanał Dev.

Znane ograniczenia: brak wydania Stable; test fizyczny OTA/rollback, BLE i wielu Shelly w toku.
