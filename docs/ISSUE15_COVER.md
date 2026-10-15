# Ekran rolety — 0.6.2, odbiór wizualny (Issue #15)

Ten etap wdraża nowy widok LVGL w istniejącym firmware. Nie wdraża transportu
Shelly Cover. Działające wykrywanie LAN, autoryzacja, ON/OFF przekaźnika,
sterowniki LCD/dotyku/podświetlenia, zapis konfiguracji i OTA pozostają zachowane.
Gałąź `work/issue-15-cover-details` jest oparta na PR #14; bez merge do main.

## Porównanie z wymaganym ekranem Shelly Smart Control

Porównanie odnosi się do opisu właściciela i recenzji #15. Prywatny zrzut ekranu
rolety nie jest dołączony do tego checkoutu ani publikowany w repozytorium.
Nie udajemy, że wykonano porównanie piksel po pikselu z niedostępnym plikiem.

| Wzorzec / wymaganie | Rzeczywisty widok LVGL 320 × 480 |
| --- | --- |
| Nazwa urządzenia i edycja | Roboto 18 px, powrót 44 × 44, skracanie długiej nazwy, ołówek otwierający zapisany wpis urządzenia w istniejącym edytorze |
| Kompaktowe parametry | Mierzone chipy pozycji, W, V, Hz; przewijanie poziome po przekroczeniu szerokości; W/V/Hz tylko gdy pole jest dostępne i poprawne |
| Duży pionowy niebieski wskaźnik | Zaokrąglona grafitowa rama, niebieskie wypełnienie proporcjonalne do potwierdzonego otwarcia, subtelne linie i uchwyt; bez losowej animacji |
| Opis pod wskaźnikiem | `Zamknięte 0%`, `Pozycja 65%`, `Otwarte 100%`; brak lub błędna/stara próbka nie daje pozornej wartości |
| Dwa duże przyciski | Wspólna grafitowa sekcja, pola 148 × 64 px, jasne koła 44 px i niebieskie Material Symbols Rounded |
| Stop | Osobne pole 64 × 64 px, aktywne wyłącznie z dostępną funkcją Stop lub w jawnym DEMO |
| Lewa kolumna sekcji | Pominięta; zachowano powrót, istniejące zakładki i pięć pozycji dolnej nawigacji |

Ikony są licencjonowane Apache-2.0, typografia Roboto; nie skopiowano zasobów
Shelly. Kolory, promienie i skala tekstu korzystają z dotychczasowego systemu.
Wykresy i ustawienia w zakładkach nadal są nieaktywne, bez atrap funkcjonalności.

Zgodnie z [oficjalnym Cover API](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Cover/)
pozycja 0 oznacza zamknięcie, 100 otwarcie. `GoToPosition` wymaga kalibracji.
W tej wersji wskaźnik jest tylko informacyjny: dotknięcie nie wysyła komend,
nie uruchamia kalibracji i nie ustawia docelowej pozycji.

## Co działa na pilocie

`Ustawienia → Podgląd rolety • DEMO` otwiera tę samą implementację LVGL
na LCD. Otwórz/Zamknij zmieniają wyłącznie lokalną przykładową pozycję; Stop
dotyczy wyłącznie przykładu. Nie ma komend sieciowych ani zapisu fikcyjnego
urządzenia. Ołówek w DEMO jest zablokowany. Każdy ekran podglądu ma złoty napis
`DEMO • dane przykładowe`. Po wyjściu odtwarzany jest bieżący model rzeczywisty,
w tym aktualizacje otrzymane w trakcie oglądania DEMO.

Zapisane urządzenie typu `cover` otwiera ten sam widok bez przykładowych danych.
Bez transportu/odczytu pozycja pozostaje nieznana, a Otwórz/Zamknij/Stop są
zablokowane. `Brak obsługi Cover RPC` jasno opisuje ograniczenie. Prawdziwe
odczyty i ruch wymagają oddzielnej implementacji i fizycznego testu zgodnej rolety.
Nie oznaczamy obsługi rzeczywistych rolet jako ukończonej.

## Dowody automatyczne

`python3 tools/test_ui.py --review --ubsan-only` i
`python3 tools/test_ui.py --ubsan-only` uruchamiają rzeczywisty LVGL 9.3.0.
Testy obejmują raster 0/65/100%, brak pozycji, offline, otwieranie, zamykanie,
błąd i wysyłanie komendy; brak niebieskiego wypełnienia dla niepewnego stanu;
chip overflow i przewijanie; geometrię przycisków i rzeczywiste dotknięcia;
PL/EN/DE; blokadę bez transportu oraz izolację DEMO od callbacks sieci/edycji.
80 cykli podglądu/sterowania/wyjścia sprawdza stabilność pamięci LVGL.

Render `11-cover-details.png` pokazuje 65%, `60`/`61` — 0/100%, `62`/`63` — ruch,
`64` — brak pozycji, `65` — offline, `66` — błąd, `67` — wysyłanie,
`68`/`69` — EN/DE. Wszystkie te pomiary są oznaczonymi danymi testowymi DEMO.
`42-cover-preview-on-device.png` pochodzi z kodu produkcyjnego bez flagi
`DSR_UI_REVIEW_DEMO` i pokazuje funkcję dostępną w menu firmware.

## Test na rzeczywistym ES3C35P — wymagany po wgraniu

1. Zapisz wersję i commit z paczki. Zachowaj konfigurację; aktualizuj przez OTA
   samym `diy_shelly_remote.bin` zgodnie z `INSTALL_AND_TEST.md`.
2. Sprawdź HOME z dotychczasowym Shelly 1PM Mini Gen3 i rzeczywisty ON/OFF.
   Nie przypisuj tego testu do rolety. Nowa wersja wymaga własnego potwierdzenia.
3. Otwórz `Ustawienia → Podgląd rolety • DEMO`. Sprawdź widoczność napisu DEMO,
   pozycję 65%, ostrość i centrowanie ikon, czytelność tekstów i kontrast LCD.
4. Naciśnij Otwórz (100%), Zamknij (0%), Stop. Dotknięcie wskaźnika nie powinno
   zmieniać pozycji. Obserwuj, że rzeczywisty przekaźnik nie reaguje na DEMO.
5. Powtórz wejście/wyjście co najmniej 30 razy. Zachowaj oczyszczone logi
   `Cover DEMO: internal_free=... internal_largest=... psram_free=...` z pierwszego
   i ostatniego wejścia. Nie uznajemy pomiaru pamięci hosta za pomiar PSRAM ESP32.
6. Sprawdź PL/EN/DE, powrót do HOME, aktualność rzeczywistego stanu, jasność,
   usypianie/wybudzanie, restart i zachowanie ustawień. Dołącz zdjęcie LCD 320×480.
7. Sprawdź także odświeżanie HOME przez 12 s. Znana korekta polling/SET z Issue #12
   jest osobnym etapem, niezależnym od tego PR graficznego.

Kompilacja docelowa i testy hosta nie potwierdzają fizycznego działania nowej
wersji. Odbiór wizualny i test prawdziwej rolety pozostają otwarte. Bez stable.
