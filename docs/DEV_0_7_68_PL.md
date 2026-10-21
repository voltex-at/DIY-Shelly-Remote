# DIY Shelly Remote 0.7.68-dev — kontynuacja i odbiór

Prace kontynuują `work/dev-0.7.68`, w tym etapy `73ac2c8` i `5eda47b` oraz
niezacommitowane poprawki interfejsu. Przed zmianami zachowano lokalną kopię
33 plików, patch i manifest sum. Narzędzia komputera, konfiguracja Codex,
kopie robocze i sekrety nie należą do firmware ani paczek instalacyjnych.

## Zakres zmian

- Zachowano trzy skórki: Grafitowy, Jasny i Shelly HUD — Cyber Blue.
  Zapisane identyfikatory pozostają odpowiednio 0, 2 i 3. Stare `midnight`
  i identyfikator 1 przechodzą do Grafitowego bez usuwania urządzeń,
  pulpitów, zdjęć, scen ani ustawień. Menu mapuje pozycję na identyfikator.
- Zachowano powiększony centralny STOP, pilną kolejkę, podgląd klawiszy
  i animację baterii z szybszym timerem działającym w aktywnym stanie.
  STOP z dotyku trafia do pilnej ścieżki przed przebudowaniem widoku.
- Uzupełniono odzyskiwanie po HTTP 429: udany odczyt usuwa tymczasowy błąd
  ograniczenia żądań, umożliwiając kolejne jawne polecenie. Nie powtarza
  niepewnych zapisów. Ponowienia niedostępnej, nieruchomej rolety ustępują
  odczytom innej rolety w ruchu lub kalibracji. Uszkodzona telemetria daje
  błąd odpowiedzi zamiast komunikatu o nieobsługiwanym kanale.
- Zachowano wcześniejsze poprawki automatycznego reconnect Wi-Fi,
  utraty adresu IP i walidacji pola `progress` kalibracji oświetlenia.
- Karty urządzeń odkładają renderowanie podczas wygaszenia i pokazują
  najnowszy model po wybudzeniu. Animacja rolety nie rysuje w uśpieniu.
  Nie oznacza to zatrzymania wszystkich timerów LVGL ani ruchu sieciowego.
- Panel WWW pobiera podgląd LCD na przycisk „Odśwież podgląd”. Status ma
  szeregowy harmonogram, ukryta karta nie rozpoczyna tych odczytów,
  a spóźniony obraz nie zastępuje aktualnego widoku. Zachowano kontrolę
  rewizji, konfliktów, zapisu i zmiany języka bez zmiany danych użytkownika.
- Uzgadnianie i ponowne renderowanie używają zweryfikowanego dokumentu
  zależnego od rewizji workspace. W teście 30 sekund bezczynności liczba
  rzeczywistych parsowań i kopii konfiguracji spadła z 240 do zera;
  15 ponownych renderów również nie parsuje dokumentu. Zmiana rewizji,
  utrata gotowości lub błąd odświeżenia unieważnia poprzedni dokument.
- Usunięto przycięcia napięcia/prądu i niemieckiej zakładki grup w Cyber Blue.
  Polskie podpisy nawigacji pozostają pełne. Trzy modele zadania UI,
  łącznie 21 600 B, przeniesiono do zewnętrznego BSS w PSRAM.

## Dowody z hosta i kompilacji

Wyniki dotyczą rzeczywistych źródeł firmware i LVGL 9.3.0, nie symulatora
innej wersji biblioteki. Dane urządzeń i zdarzenia radiowe w fixture'ach
są modelami testowymi.

| Kontrola lokalna | Wynik |
| --- | --- |
| ESP-IDF 5.5.4, ESP32-S3, LCDWiki ES3C35P | kompilacja i linkowanie obrazu |
| Regresje platformy HTTP i sterowania | 41 159 kontroli |
| Mailbox / worker baterii | 816 / 4 373 kontroli |
| Cyber UI | 214 571 kontroli, 28 klatek |
| Uśpienie i odbiór UX | 233 kontrole |
| Cache układu, kolejka poleceń i błędy pamięci | 57 kontroli |
| UI Review | 2 643 kontrole, 24 klatki |
| Atlas trzech skórek i PL/EN/DE | 450 klatek, 50 widoków/stanów |
| Panel: rzeczywisty Chrome, zapis i podgląd LVGL | 46 kontroli |
| Panel: runtime / akcje / języki w Chrome | 105 / 227 / 1 018 kontroli |
| Panel: opóźnione odpowiedzi i podgląd | 95 kontroli |
| Słownik WWW PL/EN/DE | 2 772 kontrole, 355 kompletnych komunikatów |
| Narzędzie wydawnicze | 135 decyzji, także testy odrzucenia błędnych polityk |

Pełny zestaw hostowego UI przeszedł pomyślnie. Atlas przed zmianami miał
41 przycięć tekstu w Cyber Blue; końcowy atlas ma zero przycięć tekstu
i zero elementów poza ekranem dla każdej skórki. Pozostałe ostrzeżenia
automatu obejmują m.in. nakładanie modalnego okna na podkład, małe elementy
i zamierzone skracanie długich nazw. Przejrzano również zrzuty sterowania
roletą oraz polskiej i niemieckiej nawigacji.

Pełne Linux CI jest wymagane dla dokładnego commita wskazanego przez tag:
`native` (ASan/UBSan), `ui-review`, dwa buildy diagnostyczne i `es3c35p`.
Lokalne próby na Windows/Cygwin nie zastępują sanitizerów ani tych pięciu
zadań. Wyniki źródła i kompilacji zapisuje `BUILD_COMMIT.txt` w artefaktach CI.

## Pamięć

Punkt odniesienia pochodzi z ELF z odziedziczonymi zmianami; wartości puli
LVGL i stosu pochodzą z audytu rzeczywistego obrazu, nie z deklaracji C.

| Obszar | Przed | Końcowy ELF |
| --- | ---: | ---: |
| Wewnętrzny BSS | 81 800 B | 60 208 B |
| Zewnętrzny BSS | 152 720 B | 174 320 B |
| Pula LVGL w PSRAM | 131 072 B | 131 072 B |
| Stos UI | 8 192 B | 8 192 B |

Pułap wewnętrznego BSS pozostaje 81 920 B. Nie powiększono puli LVGL,
stosu ani progów audytorów. Hostowy test 24 kafelków pozostawia około
6 KiB wolnej puli; nie jest to cel 40% zapasu z propozycji audytu.
Fizyczny szczyt stosu, minimum sterty i fragmentacja po 24–72 h nie zostały
zmierzone. Czasów renderowania na hoście nie przedstawia się jako czasu ESP32.

Cache ma osobny koszt dynamiczny: jedną arenę w PSRAM obejmującą węzły
i teksty. Nie korzysta z globalnych hooków cJSON. Fixture z limitami
kolekcji, 14 138 B JSON i 1 284 węzłami potrzebował 91 474 B areny
na 64-bitowym hoście. Kompilator Xtensa potwierdził węzeł 40 B: dla tego
samego fixture arena żąda 60 658 B PSRAM (węzły i 9 298 B tekstów).
To wyliczony rozmiar alokacji, bez narzutu alokatora i pomiaru na płytce.
Przejściowe alokacje parsera występują przy nowej rewizji. Zmniejszenie BSS
nie oznacza takiego samego zmniejszenia całkowitego zużycia RAM.

## Podpisywanie i instalacja

Środowisko DEV `release` wymaga recenzenta `voltex-at`, zabrania obejścia
administracyjnego i dopuszcza wyłącznie tagi `dev-v*`. Bramka odrzuca
brakujące zabezpieczenia, innych recenzentów i niepełną listę reguł.
Akcje GitHub są przypięte do SHA. Klucz `DSR_OTA_SIGNING_KEY` musi znajdować
się w chronionym środowisku, a kopia dostępna dla dowolnej gałęzi jako
sekret repozytorium musi zostać usunięta po bezpiecznym przeniesieniu.

Publikacja jest dozwolona dopiero po sprawdzeniu tych ustawień i pełnym CI
dla tego samego SHA. Workflow podpisuje dotychczasowym kluczem
`voltex-ota-2026-1`, weryfikuje podpis kodem firmware, sprawdza pakiety,
odczytuje opublikowane sumy i kontroluje katalog OTA o limicie 16 KiB.
Brak poprawnego klucza lub zgody środowiska zatrzymuje publikację.

Aktualizację zachowującą dane wykonuje się przez podpisane DEV OTA albo
UPDATE-APP-ONLY. FACTORY-CLEAN-16MB jest świadomą instalacją czystą.
Partycje, format danych i klucz zaufania urządzenia pozostają zgodne.
Stable i automatyczne scalenie do `main` nie należą do tego wydania.

## Pozostałe pomiary i ograniczenia

Pilny STOP nadal może czekać na zakończenie trwającego żądania HTTP;
nie potwierdzono fizycznego czasu poniżej 1 s. Weryfikacja tożsamości przed
operacjami pozostaje aktywna. Cache tożsamości, współdzielenie połączeń HTTP,
automatyczna zmiana hosta po DHCP i przebudowa cyklu życia ADC wymagają
odrębnej regresji oraz pomiaru na sprzęcie. Zaleca się rezerwację DHCP
dla zapisanych urządzeń Shelly; po zmianie adresu należy zweryfikować
tożsamość i ponownie zapisać połączenie przez istniejący kreator.

Nie zmieniono szyfrowania Flash/NVS ani procesu lokalnego uploadu OTA
z parowaniem i zgodą fizyczną. Nie rotowano kluczy. Fizycznego odbioru
wymagają: dotyk i nawigacja, radio BLE/Wi-Fi i powroty AP, Light/RGBW/H&T/
BLU/TRV, napięcie i trend baterii z multimetrem, STOP z urządzeniem offline,
aktualizacja z poprzedniego DEV, zachowanie danych, rollback i próba
wytrzymałościowa 24–72 h. Host i CI nie stanowią dowodu tych prób.

W audycie pozostały też propozycje dalszej optymalizacji: indeks tłumaczeń,
diagnostyka niebilansu liczników aktywności i refaktoryzacja kontekstu HTTP.
Nie znaleziono rzeczywistego niebilansu ani błędnego zdarzenia workera;
sama różnica liczby wywołań `begin`/`end` nie dowodzi błędu. Zachowano
zatrzymanie operacji przy uszkodzeniu licznika. Nie wprowadzono proponowanego
bezwarunkowego kasowania `busy` przy obcym wyniku, które mogłoby dopuścić
nakładające się operacje. API używane przez testy pozostawiono: nie są
potwierdzonym martwym kodem. Te pozycje nie są oznaczone jako naprawione.
