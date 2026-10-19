# Shelly HUD — Cyber Blue Premium

Opcjonalna czwarta skórka istniejącego systemu wyglądu. Baza implementacji:
`e6e2359` (`work/p0-spiffs-ota-brand`, 0.7.57-dev), ESP-IDF 5.5.4,
LVGL 9.3.0 (`c033a98afddd65aaafeebea625382a94020fe4a7`), ES3C35P 320 × 480.

## Wybór i zachowanie danych

Ustawienia → Wygląd i język → Motyw → **Shelly HUD — Cyber Blue** → Zapisz.
Ta sama opcja jest dostępna w istniejącym selektorze Web Panelu. Identyfikatory
0 `graphite`, 1 `midnight`, 2 `light` pozostają bez zmian. Nowy: 3 `cyber_blue`.
Domyślna konfiguracja nadal wybiera Grafit. Pole `accent` nie jest nadpisywane:
Cyber Blue używa własnej palety, a powrót przywraca zapisany akcent użytkownika.

Zapis korzysta z istniejącej walidacji, rewizji CAS, dwóch punktów kontrolnych
workspace i potwierdzenia trwałego zapisu. Anulowanie nie zmienia aktywnej skórki.
Przełączenie przebudowuje widok bez restartu. Przy starcie workspace odtwarza
skórkę; istniejąca wskazówka NVS `dsr_ui/style` rozpoznaje również wartość 3,
aby ekran startowy wybrał dotychczasową ciemną grafikę.

## Implementacja

- `dsr_ui_style.*`: czwarty identyfikator i mapowanie nazwy, wspólne kolory,
  karty, przyciski, ikony, nawigacja i akcenty HUD.
- `dsr_ui_cyber.*`: paleta, współdzielone style i nakładka na standardowy motyw
  LVGL. Obejmuje pola, dropdowny i ich listy, checkboxy, klawiatury i macierze
  przycisków, suwaki, wskaźniki, rolki oraz standardowe msgboxy z przyciemnieniem.
- `dsr_ui_config.c`, `dsr_config.c`, `panel.js`, `dsr_style_hint.c`: rozszerzenia
  istniejącego wyboru i zapisu. Bez drugiego selektora i bez kopii ekranów.

Wszystkie ekrany korzystają z tych samych komponentów; własne dialogi błędów,
potwierdzenia usuwania, kreatory i formularze otrzymują tę samą paletę. Główne
akcje są niebieskie; usuwanie zachowuje czerwony kolor i potwierdzenie. Ciemniejsze
wypełnienia przycisków utrzymują czytelność jasnych napisów. Nie zmieniono
komunikacji, sterowania Shelly, sieci, partycji, kluczy, logiki OTA ani wersji.

## Dopracowanie Premium

Iteracja na bazie `2ef69a9`. Paleta odpowiada przesłanej tablicy stylu Premium:
tło `#0A1324`, panel `#0E223A`, cyjan `#00E5FF`, niebieski `#007BFF`,
bursztyn `#FFC107`, czerwony `#FF3B30`, tekst `#FFFFFF` / `#A9B7CC`.

| Cecha referencji | Implementacja w prawdziwym LVGL 320 × 480 |
| --- | --- |
| Metaliczne granatowe panele, ukośne refleksy | Pionowe gradienty, jasna górna krawędź, wewnętrzny kontur i pięć delikatnych ukośnych linii; oryginalne tło przyciemnione granatowym recolorem tylko w Cyber Blue. |
| Rozświetlone akcje i ON/OFF | Kobaltowy rdzeń ON, cyjanowy pierścień i dwa krótkie refleksy; OFF ma stalowy gradient. Aktywna zakładka i główne akcje mają niebieskie wypełnienie, a naciśnięcie jaśniejszy gradient. |
| Większy wskaźnik rolety | Szyna 136 × 182 zamiast 96 × 148; pole wskaźnika 114 × 160 zamiast 78 × 130. Dodano 50%, duży cyjanowy procent i osobne niebieskie Otwórz/Zamknij. STOP pozostaje dostępny. |
| Dominujący status BLU | Font 28 zamiast 22, osobne zagłębienie, cyjan Zamknięte / bursztyn Otwarte. Ikona i dane baterii, sygnału, czasu oraz błędów pozostają widoczne. Sprawdzono PL/EN/DE. |
| Karty urządzeń o większej głębi | Różne materiały ON/OFF, pionowy pasek stanu, zagłębione metryki i pola ikon. Dane oraz stan potwierdzony przez urządzenie nadal sterują wyglądem. |
| Wi-Fi, bateria i firmware | Cztery wiersze sieci z ikonami i RSSI; panel napięcia z dużą wartością; oddzielne panele wersji, kanału i bursztynowego błędu. Render także formularza hasła i postępu OTA. |

Referencje mają większą rozdzielczość i mocne poświaty. Na LCD zachowano
czytelność, kompaktową nawigację i miejsca dotyku; blask zastępują cienkie
kontury i kontrast wypełnień. RGB565 może pokazywać delikatne stopniowanie
gradientów. To dopasowanie stylistyki do realnego interfejsu, nie skalowanie
wizualizacji jako tapety.

Wykorzystano oryginalne ciemne tło i istniejący atlas ikon, bez zmiany bajtów
19 istniejących plików tła/fontów. Bluetooth, odświeżanie i konturowa bateria
są lekkimi prymitywami rysowania. Nie dodano bitmap do firmware ani dekodera.
Nie ma blur, animacji HUD, dodatkowego timera ani kopiowania tła do RAM.
Refleksy są rysowane bezpośrednio na zwykłej warstwie LVGL. Wskaźnik rolety
rysuje wypełnienie i listwy bez tworzenia osobnych obiektów dla każdej listwy.

Style Cyber Blue inicjalizują się przy pierwszym wybraniu. Ich ograniczony,
współdzielony zestaw zostaje do ponownego użycia po zmianie skórki. Nie jest
resetowany, gdy istnieją obiekty posiadające referencje. Struktura motywu jest
umieszczona w PSRAM na ESP32. Użyto konkretnej struktury motywu z przypiętego
LVGL 9.3 (`lv_theme_private.h`); aktualizacja LVGL wymaga ponownej kompilacji
i testów tego miejsca.

## Sprawdzone lokalnie w iteracji Premium

- Pełny istniejący zestaw testów UI oraz fixture review z prawdziwym LVGL 9.3.0:
  zaliczone. **344 wcześniejsze obrazy PPM identyczne bajt w bajt** względem
  `2ef69a9`, w tym trzy wcześniejsze skórki. Review: 2643 sprawdzenia / 24 klatki.
- Test Cyber Blue: **994 sprawdzenia, 23 rzeczywiste klatki 320 × 480**;
  osobno zaliczony także z UBSan. Wybór/zapis/anulowanie/odczyt ustawień,
  dialogi, klawiatury, dropdown, stany ON/OFF, PL/EN/DE, postęp OTA.
- Dotyk powiększonej rolety: 0/50/100% trafiają do callbacku raz po puszczeniu,
  bez wysyłania podczas przeciągania i bez zastępowania potwierdzonego 84%.
  Sprawdzono także Otwórz/Zamknij/STOP. Callbacki/radio/magazyn/ADC to modele.
- **80 zapisanych przełączeń:** 111184 → 111184 B wolnej pamięci LVGL po
  rozgrzaniu; największy blok 69136 B. Pomiar na pustym, uspokojonym ekranie.
- Pierwszy wybór na ekranie wyglądu: 100488 → 98904 B wolnych. Obejmuje to
  leniwe cache widżetów i współdzielone style, nie tylko samą strukturę motywu.
- **24 kafelki:** 7824 B wolnych, największy blok 7776 B, przy niezmienionej
  puli LVGL **128 KiB**. V1 w swoim fixture miało 10256 B wolnych; Premium
  ma mniejszy zapas. W tym teście wymagane są oba zapasy powyżej 4096 B.
- Pełne odmalowanie tych samych 24 kafelków, średnia z 8 na hoście bez
  sanitizerów: Premium **3,53 ms**, Grafit **3,13 ms**. To orientacyjny koszt
  rysowania na tym komputerze, **nie pomiar opóźnień ESP32 ani dotyku**.
- `git diff --check`: poprawny. Oryginalne bitmapy i fonty firmware bez zmian.

Odtworzenie renderów: `python3 tools/test_ui.py --case test_cyber_ui`.
Niezmienione klatki wyjściowe są w `build-ui/native/screenshots/6*-cyber-*.ppm`.
PNG w dokumentacji to bezstratna konwersja tych klatek. Manifest zawiera ich
sumy SHA256. Plansze jedynie zestawiają pełne obrazy obok siebie.

Docelowa kompilacja ES3C35P / ESP-IDF 5.5.4, kompilacje diagnostyczne ESP32
i ESP32-S3, pełne ASan/LSan/UBSan oraz testy konfiguracji/protokołów działają
w istniejącym workflow **Firmware checks** dla commitu tej gałęzi. Wynik należy
wiązać z konkretnym SHA i przebiegiem Actions. Lokalny LSan w tym kontenerze
jest ograniczony dostępem do `/proc`; lokalny UBSan działa.

## Podglądy i odbiór sprzętowy

![Przegląd ekranów Premium](cyber-blue/overview.png)

[Porównanie v1 i Premium](cyber-blue/comparison-v1-premium.png) obejmuje pulpit,
roletę, BLU i aktualizację firmware. Dane BLU w nowym fixture mają potwierdzony
czas próbki; ta różnica tekstu nie wynika ze zmiany logiki produkcyjnej.

W katalogu `cyber-blue` są PNG z rzeczywistego renderera LVGL 320 × 480.
Napisy TEST / DEMO oznaczają modele danych/radia; przykładowa wersja firmware
na podglądzie pochodzi z fixture. Nie są to fotografie urządzenia.

Nie przeprowadzono testów na fizycznej płytce. Do odbioru na ES3C35P pozostają:
zmiana i restart dla wszystkich czterech skórek, powrót do poprzedniego akcentu,
okna/klawiatura i czytelność na LCD, rzeczywiste sterowanie i STOP, pomiary
heap/PSRAM oraz opóźnień dotyku. Testy hostowe nie dowodzą tych wyników.
Ta gałąź nie publikuje wydania, nie podpisuje firmware i nie zmienia `main`.
