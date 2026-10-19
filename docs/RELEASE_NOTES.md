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
