# Cyber Blue Premium - finalne dopracowanie 0.7.66-dev

## Zlecenie i baza

Zadania: dostarczony ZIP `DIY_Shelly_Remote_Cyber_Blue_Premium_Work_Prompt(1).zip`, SHA-256 `7b4ee4435649511ba96756c72fec46d7a9f2440a9763424b2c9e1ca617961f4f`, oraz trzy zdjęcia bieżącej rozmowy. ZIP zawiera opis prac, nie źródła firmware. Baza implementacji: `5d33c2c9a755ba5b5c3692b97d766f6a6e21a04e` (PR #71, wydanie 0.7.65-dev). `main` pozostaje `51c5840e4098829fe8bd4cd0dfc13a552ce2133b`; bez merge do main/Stable. Kolejne etapy Cyber #69 -> #70 -> #71 są przodkami bazy. Starszy #67 pozostaje odłożony; nie nadpisano jego decyzji ani historii.

Właściciel w tej rozmowie zezwolił na zmiany GitHub i publikację. Jednorazowa prośba 0.7.66-dev wskazuje wyłącznie tę gałąź i wersję; normalny gate nadal wymaga CI 5/5 na dokładnym SHA. Klucze i środowisko podpisu zachowano.

## Implementacja

- Centralne kontury Cyber: 2 px i dotychczasowy CYAN `0x00e5ff`; wyjątki semantycznych ostrzeżeń/błędów. Obramowania nie zależą od stanu aktywnego. Shared styles oraz wspólne settery obsługują natywne widżety i ręcznie budowane karty. Trzy wcześniejsze skórki zachowują swoje fonty i parametry. Ikony danych niedostępnych i ostrzeżeń zachowują znaczenie.
- Medium 12/14 dla opisów, Bold 18/22/28 dla nagłówków i wartości; 11 Regular pozostaje pomocniczy, 16 Medium dla nazw i przycisków. Istniejące Roboto z licencją Apache-2.0. Generowanie: `tools/build_ui_fonts.py --cyber-only`; podstawowy alfabet ASCII i jawny zestaw polskich/niemieckich znaków. Rozmiar końcowego obrazu i RAM podaje raport buildu; rozmiar plików C nie jest kosztem Flash.
- Powrót w lewym nagłówku dostaje dokładnie ten sam callback co lokalne Wstecz, także z ochroną zmian edytora. Podstrony urządzeń zachowują swój stos nawigacji. Samo położenie strzałki nie zastępuje poprawnego celu.
- Przewijane panele są hit-testowalne po usunięciu domyślnych flag przez kreator panelu. Pasywny fragment karty i pusta przestrzeń przyjmują pionowy gest. Nagłówki/stopki pozostają poza viewportem. Pasek Cyber jest turkusowy.
- Dashboard: dwa wiersze o stałej geometrii, stan i pokój. Długi pokój jest skracany na granicy UTF-8. Kompaktowa karta 104 px, powiększona 120 px. Pokój rysuje rzeczywisty renderer LVGL w istniejącej warstwie karty, aby nie tworzyć dodatkowego widżetu dla każdego z 24 kafelków. Model sterowania, pomiary i zaakceptowane szczegóły rolety/TRV pozostają wspólne.
- Plus: większy kontur/symbol i reakcja na naciśnięcie; wejście do istniejącego kreatora ma blokadę wielokrotnego otwarcia. Istniejące LAN/AP/BLE zachowują anulowanie, skończone timeouty i sprzątanie połączeń.

## Usuwanie i trwałość

Przyczyną blokady były wymaganie ręcznego wyczyszczenia wszystkich referencji oraz brak Usunięcia w kontekstowej edycji karty. Nowa operacja ma potwierdzenie nazwy i zakres jednego stabilnego ID/kanału.

1. Waliduj i duplikuj dokument. Usuń tylko wskazane ID oraz jego device tiles, członkostwa grup i działania scen. Pozostaw pokoje, same sceny/grupy oraz pozostałe kanały (także o tym samym IP).
2. Opublikuj przez dotychczasowe CAS i dwa checksummed checkpointy SPIFFS. Błąd przygotowania nie modyfikuje draftu, a błąd zapisu nie ogłasza sukcesu. Ponowienie zachowuje ten sam zakres.
3. Po dokładnym trwałym ACK brak karty wyłącza sterowanie, odrzuca stare kolejki i uruchamia istniejące usunięcie jednego powiązania NVS. Operacja czeka na zakończenie trwającego transportu/STOP. Dodawanie w fazie publikacji blokuje nową operację usuwania.
4. UI rozróżnia zapisywanie, niepotwierdzony zapis, kartę usuniętą z trwającym cleanupem, błąd NVS z ponowieniem oraz sukces. Sukces wymaga także zniknięcia powiązania z aktualnego fleet po trwałym ACK NVS. Stara prośba NVS nie może skasować nowszego powiązania o innym layout ID.
5. Po przerwaniu między magazynami brak karty jest trwałą intencją usunięcia. Rozruch wczytuje workspace, blokuje stary endpoint i ponawia cleanup. Nie jest to atomowa transakcja pomiędzy SPIFFS i NVS. Jeśli zapis checkpointu doszedł do Flash, lecz readback nie został potwierdzony, restart może odczytać nowy poprawny checkpoint; komunikat celowo mówi o braku potwierdzenia.

Usunięcie dotyczy pilota, nie resetu fabrycznego Shelly. Brak osobnego magazynu ulubionych w tym schemacie; ich odpowiednikiem są karty pulpitów. Ponowne dodanie korzysta ze zwolnionego endpointu; nie usuwa i nie zastępuje innych kanałów.

## Bateria - dostępne dane

Źródła producenta: [schemat ES3C35P](https://www.lcdwiki.com/res/ES3C35P/ESP32-S3%E5%8E%9F%E7%90%86%E5%9B%BE.pdf) oraz [instrukcja, rozdziały 8 i 11](https://www.lcdwiki.com/res/ES3C35P/3.5inch_IPS_ESP32-S3_User_Manual.pdf). Dzielnik R14/R15 100k/100k udostępnia BAT_ADC. Sterownik projektu mierzy GPIO8 ADC1, odrzuca pierwszą próbkę, bierze medianę dziewięciu odczytów i kalibrację curve fitting ESP-IDF. Bez kalibracji pokazuje brak napięcia; po 30 s wynik jest nieaktualny. Pomiar jest na żądanie i przechodzi przez istniejący worker; nie blokuje LVGL.

Sam BAT nie dowodzi obecności ogniwa, USB ani ładowania. Schemat ładowarki TP4054 nie daje projektowi obsługiwanego sygnału CHRG do MCU. Wskazanie procentu i krytycznego poziomu wymaga dodatkowej wiarygodnej informacji o SOC/ogniwie. Obecny sterownik nigdy nie ustawia capability flags procentu/ładowania. Nagłówek pokazuje napięcie lub nieznane dane, bez zmyślonego wypełnienia.

Komponent obsługuje opcjonalne potwierdzone capabilities przyszłego dostawcy: wypełnienie SOC, alarm <=10%, cykl ładowania 1.6 s + 0.4 s zatrzymania. Nie tworzy timera ani animacji/obiektów na każdą klatkę. Cykl zatrzymuje się przy wygaszeniu sygnału po 30 s, odłączeniu ładowania lub uśpieniu ekranu. Procenty/ładowanie w atlasie mają jawne nazwy `provider-fixture`; nie dowodzą wsparcia sprzętowego ES3C35P.

## Walidacja i ograniczenia

`tools/test_config.py`: usunięcie jednego z 16 kanałów, wiele referencji, nienaruszony dokument wejściowy, brakujący ID, zachowanie scen/grup, prawdziwy zapis/odczyt checkpointu po usunięciu oraz ponowne dodanie. `tests/test_platform.c`: rzeczywisty rejestr NVS z modelem SDK, inne urządzenia i kanały, błąd commit/readback, idempotentne ponowienie oraz odrzucenie starego layout ID. `test_cyber_final_ui`: prawdziwe gesty LVGL, dolny i górny koniec baterii, oba cele Wstecz, lokalizowane potwierdzenie/anulowanie, odłożony ACK i nieudany zapis/ponowienie, status-pokój, capability fixtures i brak wzrostu pamięci podczas cyklu.

Nowy test przewijania jest czerwony na niezmienionej bazie 0.7.65: brak `LV_OBJ_FLAG_CLICKABLE` w kontenerze. Po poprawce rzeczywiste gesty do dołu i do góry przechodzą. Pełna regresja obejmuje wszystkie wcześniejsze funkcje, cztery skórki, PL/EN/DE i 24 kafelki. Test 24 kafelków pozostawia 6192 B wolne i największy blok 6136 B przy puli 128 KiB; 80 zapisanych przełączeń skórek: 111128 -> 111128 B, 221 -> 221 bloków. Pomiar dotyczy hostowego LVGL, nie SDK/ESP32. `test_delete_cleanup_ui` dodatkowo pokrywa opóźniony i nieudany ACK NVS, ponowienie po 5 s, zachowanie kanału 1 i brak poleceń sieciowych. Lokalne UBSan oraz pełne ASan/UBSan w GitHub Actions są rozdzielone: lokalny runtime nie udostępnia /proc dla LeakSanitizer. Żadnego testu CI nie wyłączono.

Rendery powstają z produkcyjnych źródeł LVGL 9.3.0 w RGB565, 320 x 480. Atlas zawiera wszystkie istotne trasy i stany oraz źródłowy manifest klatek. Porównanie przed/po wykorzystuje rzeczywiste klatki bazowego SHA i aktualnego drzewa. Podpisy atlasu leżą poza pikselami ekranu. To nie są makiety HTML ani obrazy AI.

## Krótki odbiór na pilocie

1. Zainstaluj 0.7.66-dev przez dotychczasowy podpisany kanał Dev albo UPDATE-APP-ONLY zgodnie z dołączoną instrukcją. Zachowaj istniejące dane. Nie używaj Erase All ani wariantu FACTORY do zwykłej aktualizacji.
2. Otwórz Bateria i zasilanie. Przesuń do końca, sprawdź BLE/stos/ostatnie wiersze, wróć na początek. Nagłówek i akcje powinny pozostać nieruchome. Wstecz w nagłówku i stopce powinno wrócić do tego samego rodzica.
3. Zmierz BAT i porównaj z multimetrem dla swojej rewizji PCB/ogniwa. Nie oczekuj procentu ani animacji ładowania bez dodatkowego źródła stanu.
4. Na kopii konfiguracji wybierz jeden kanał testowy z kartą oraz referencją w grupie/scenie. Anuluj pierwsze potwierdzenie. Następnie świadomie usuń wyłącznie ten kanał i poczekaj na sukces. Sprawdź drugi kanał, pokoje i sceny. Zrestartuj pilot i sprawdź trwałość. Ponownie dodaj testowy kanał.
5. Sprawdź długie nazwy i PL/EN/DE, roletę (w tym STOP), termostat, kolory/dotyk oraz szybkie podwójne naciśnięcie +. LAN/AP/BLE powinny kończyć się, dawać anulować i zwalniać zasoby.

Nie wykonano operacji na fizycznym pilocie, Shelly ani instalacji. Długotrwała pamięć SDK, wartości ADC, zakłócenia radiowe, fizyczne gesty i OTA/rollback wymagają odbioru właściciela. Istniejący problem inicjalizacji pustego SPIFFS nie jest diagnozowany jako nowy błąd tej zmiany i nie upoważnia do formatowania niepustych danych.
