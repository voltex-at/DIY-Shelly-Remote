# Shelly HUD — Cyber Blue Premium

Opcjonalna czwarta skórka istniejącego systemu wyglądu. Baza implementacji:
`e6e2359` (`work/p0-spiffs-ota-brand`, 0.7.57-dev), ESP-IDF 5.5.4,
LVGL 9.3.0 (`c033a98afddd65aaafeebea625382a94020fe4a7`), ES3C35P 320 × 480.

Najnowszą iterację rolety, ikon okien/drzwi, baterii i modułów sterowania opisano
w [quality pass](cyber-blue/QUALITY_PASS.md). Wcześniejsze dopracowanie czytelności, edytorów i 17 brakujących ikon opisano w
[etapie 3](cyber-blue/stage3/README.md), wraz z rzeczywistymi renderami i
porównaniem przed/po. Poniższe liczby integracji bazowej opisują etap 2.

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
- `dsr_ui_cyber_icons.*`, `cyber_icons/`: wybrany zestaw oryginalnych masek A8 z dostarczonej paczki CC0.
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

## Dostarczone ikony A8

Zintegrowano `Shelly_HUD_Cyber_Blue_Icons_LVGL9.zip` 1.0.0: **41 symboli w 62
używanych wariantach**, **56256 B surowych masek A8** zamiast pełnych 253760 B
wszystkich 195 masek z paczki. Symbole `shud_*` i każdy bajt masek zachowano.
Hash paczki i maski są zapisane w `components/dsr_ui/cyber_icons/manifest.json`.
Import nie przerysowuje SVG, nie przetwarza PNG i nie wprowadza fontów.

| Obszar | Zasoby z paczki |
| --- | --- |
| Nawigacja i nagłówek | back, wifi, dashboard, home, settings, plus, info |
| Karty i sterowanie | plug, lightbulb, shutter, power, thermometer, display, snowflake, sensor |
| Czujniki BLU | window-open, window-closed, battery |
| Roleta | shutter-up, shutter-down, stop |
| Wi-Fi / BLE | wifi, access-point, bluetooth, refresh |
| Diagnostyka | battery-charging jako symbol zasilania, memory, activity |
| OTA | cpu, git-branch, refresh, warning |
| Ustawienia i formularze | palette, display, shield, edit, save, trash, chevron-down, chevron-right |

Zachowano aktualną funkcję trzeciego przycisku dolnej nawigacji: otwiera baterię,
więc jego ikoną pozostaje bateria. Symbol baterii z błyskawicą na karcie napięcia
nie dodaje danych o ładowaniu: dotychczasowy komunikat o braku wiarygodnych danych
pozostaje. W etapie 3 brakujące meble, sprzęty, zawór oraz osobne stany drzwi
otrzymały 17 dedykowanych ikon konturowych i stałe maski A8. Oryginalna paczka
i atlas pozostałych skórek pozostają niezmienione.

Wspólny komponent ikon przełącza źródło wyłącznie dla Cyber Blue. Stosuje natywne
24 px w kompaktowych kontrolkach, 32 px w kafelkach i 48 px w dużych wskaźnikach;
małe strzałki list dopasowuje do istniejącego miejsca. Pola dotyku i callbacki
pozostały bez zmian. Kolor nadają właściwości `image_recolor` / `image_recolor_opa`
i standardowy renderer A8 LVGL. Nie tworzy się dodatkowych widżetów ani kopii
masek w PSRAM. Te same maski służą kolorom białym, cyjanowym, czerwonym i bursztynowym.

Oryginalne tło, fonty i atlas pozostałych skórek pozostają bajtowo niezmienione.
Nowe maski są stałymi w Flash; nie dodano tła, dekodera PNG/SVG, blur, animacji
HUD ani timera. Refleksy i gradienty nadal rysuje zwykła warstwa LVGL.

Powtórny import: `python3 tools/import_cyber_icons.py /ścieżka/do/paczki.zip`.
Kontrola integralności i doboru wariantów: `python3 tools/check_cyber_icons.py`.
Test działa też w CI; notę CC0 dołączono do źródeł i pakietów budowanych przez CI.

Style Cyber Blue inicjalizują się przy pierwszym wybraniu. Ich ograniczony,
współdzielony zestaw zostaje do ponownego użycia po zmianie skórki. Nie jest
resetowany, gdy istnieją obiekty posiadające referencje. Struktura motywu jest
umieszczona w PSRAM na ESP32. Użyto konkretnej struktury motywu z przypiętego
LVGL 9.3 (`lv_theme_private.h`); aktualizacja LVGL wymaga ponownej kompilacji
i testów tego miejsca.

## Sprawdzone lokalnie po integracji ikon

- Pełny istniejący zestaw testów UI oraz fixture review z prawdziwym LVGL 9.3.0:
  zaliczone. **344 wcześniejsze obrazy PPM identyczne bajt w bajt** względem
  `0f1c2dd`, w tym trzy wcześniejsze skórki. Review: 2643 sprawdzenia / 24 klatki.
- Test Cyber Blue: **14445 sprawdzeń, 24 rzeczywiste klatki 320 × 480**;
  osobno zaliczony także z UBSan. W tej liczbie jest 13440 porównań kanałów pikseli
  z oryginalną maską A8 na czarnym tle: 24/32/48 px, przezroczystość, kolory
  i ponowne kolorowanie; tolerancja obejmuje kwantyzację RGB565.
  Wybór/zapis/anulowanie/odczyt ustawień,
  dialogi, klawiatury, dropdown, stany ON/OFF, PL/EN/DE, postęp OTA.
- Dotyk powiększonej rolety: 0/50/100% trafiają do callbacku raz po puszczeniu,
  bez wysyłania podczas przeciągania i bez zastępowania potwierdzonego 84%.
  Sprawdzono także Otwórz/Zamknij/STOP. Callbacki/radio/magazyn/ADC to modele.
- **80 zapisanych przełączeń:** 111136 → 111136 B wolnej pamięci LVGL po
  rozgrzaniu; największy blok 69176 B. Pomiar na pustym, uspokojonym ekranie.
- Pierwszy wybór na ekranie wyglądu: 100488 → 98224 B wolnych. Obejmuje to
  leniwe cache widżetów i współdzielone style, nie tylko samą strukturę motywu.
- **24 kafelki:** 7720 B wolnych, największy blok 7664 B, przy niezmienionej
  puli LVGL **128 KiB**. Premium przed integracją ikon miało 7824 B wolnych w tym fixture.
  Nowa wersja pozostawia o 104 B mniej. W tym teście wymagane są oba zapasy powyżej 4096 B.
- Pełne odmalowanie widoku pulpitu zawierającego 24 kafelki, średnia z 8 na hoście bez
  sanitizerów: Premium **3,39 ms**, Grafit **3,12 ms**. To orientacyjny koszt
  rysowania na tym komputerze, **nie pomiar opóźnień ESP32 ani dotyku**.
- `git diff --check`: poprawny. Oryginalne bitmapy i fonty firmware bez zmian.

Odtworzenie renderów: `python3 tools/test_ui.py --case test_cyber_ui`.
Niezmienione klatki wyjściowe są w `build-ui/native/screenshots/6*-cyber-*.ppm`.
Klatka 625 to plansza testowa masek A8, a nie dodatkowy ekran firmware.
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
roletę, BLU i aktualizację firmware. [Porównanie przed i po integracji ikon](cyber-blue/comparison-supplied-icons.png)
pokazuje pulpit, BLU i OTA. Dane BLU w nowym fixture mają potwierdzony
czas próbki; ta różnica tekstu nie wynika ze zmiany logiki produkcyjnej.

W katalogu `cyber-blue` są PNG z rzeczywistego renderera LVGL 320 × 480.
Napisy TEST / DEMO oznaczają modele danych/radia; przykładowa wersja firmware
na podglądzie pochodzi z fixture. Nie są to fotografie urządzenia.

Nie przeprowadzono testów na fizycznej płytce. Do odbioru na ES3C35P pozostają:
zmiana i restart dla wszystkich czterech skórek, powrót do poprzedniego akcentu,
okna/klawiatura i czytelność na LCD, rzeczywiste sterowanie i STOP, pomiary
heap/PSRAM oraz opóźnień dotyku. Testy hostowe nie dowodzą tych wyników.
Ta gałąź nie publikuje wydania, nie podpisuje firmware i nie zmienia `main`.
