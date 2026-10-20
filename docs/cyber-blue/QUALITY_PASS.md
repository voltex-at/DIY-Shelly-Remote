# Cyber Blue HUD Premium — kolejny quality pass

Baza: `8a8dffdc7961cd60456a892eb90085ee52a0cfdb`, `0.7.63-dev`.
Gałąź: `work/cyber-blue-quality-pass`. Docelowy ekran: ES3C35P, 320 × 480,
LVGL 9.3.0. Ta iteracja rozwija opcjonalną skórkę `cyber_blue` / ID 3.

## Roleta

Pełna rama 176 × 162 obejmuje metaliczną kasetę, dwie prowadnice i parapet.
Ogród za oknem jest nieruchomą teksturą RGB565. Pancerz składa się z osobnych
poziomych lameli, a jego dolna listwa przesuwa się względem kasety.
0% zasłania całe okno; 100% odsłania ogród. Przykłady obejmują też 25%, 50%
i 84%. Stany bez pozycji pokazują neutralne pole bez odsłoniętego ogrodu.

Gest na rolecie aktualizuje pancerz natychmiast na każdym zdarzeniu pointera.
Bursztynowa listwa i podpis „Podgląd” odróżniają cel od potwierdzonego stanu;
biały znacznik na prowadnicy pokazuje potwierdzoną pozycję. Główna wartość
pochodzi nadal z odczytu. Ruch palca nie wysyła żądań; puszczenie wewnątrz
sterowania wysyła jedno. Wyjście poza pole, utrata połączenia lub zmiana
tożsamości urządzenia anuluje gest. Po puszczeniu przywracana jest pozycja
potwierdzona; odczyty poruszającego się urządzenia są interpolowane przez 300 ms.

## Ikony, bateria i sterowanie

- Okno zamknięte ma dwa frontowe skrzydła. Otwarte pokazuje oba skrzydła
  rozwarte poza ramę. Drzwi mają czytelną ościeżnicę, skrzydło i klamkę.
  Duże ikony kontaktów są cyjanowe; bursztyn pozostaje w statusie otwarcia.
  Zachowano oddzielne neutralne ikony dla braku danych.
- Pierwsza karta baterii ma **130 px zamiast 182 px**. Tytuł, ikona, napięcie,
  opis i wiek odczytu znajdują się razem. Niepewność ładowania/ogniwa/procentu
  ma osobną zwartą ramkę. Pamięć i radio mają ikony i pięć czytelnych wierszy.
- Przekaźnik, roleta, jasność/RGB, H&T i TRV rozdzielają połączenie od stanu
  urządzenia. Online/Offline mają dedykowane symbole. Nagłówek „Główne
  sterowanie” usunięto z widoku Cyber. Przyciski, odświeżanie i metryki są
  oddzielnymi modułami. Dane offline pozostają nieznane.
- Dodano semantyczne ikony napięcia, prądu, mocy, PSRAM, grafiki, firmware,
  sprzętu, diagnostyki, restartu i połączenia. Sekcje Systemu, sceny/wpisy,
  Shelly settings, pamięć i odświeżanie korzystają ze wspólnego zestawu.

## Zasoby i odtworzenie

Nowa tekstura: **38 912 B Flash**. Jedenaście dodatkowych masek A8 po 24 × 24:
**6336 B Flash**, poza deskryptorami. Ikony nie dodają fontów ani dekodera.
Stan rysowania rolety jest zwalniany wraz z ekranem; podgląd nie tworzy
obiektów ani timerów na każdą klatkę. Pula LVGL i stos UI pozostają odpowiednio
128 KiB i 8192 B. Transport, trwałe dane, partycje i zaufanie OTA są bez zmian.

```bash
python3 tools/build_cyber_scene.py
python3 tools/check_cyber_icons.py
python3 tools/test_ui.py --ubsan-only
python3 tools/render_cyber_atlas.py --screens build-ui/ubsan/screenshots --out build-ui/cyber-atlas
```

Pełny atlas jest generowany ze wszystkich oznaczonych klatek produkcyjnego
LVGL, także podstron, formularzy, przewinięć i błędów. Łączone są wyłącznie
identyczne piksele; manifest zapisuje aliasy i SHA-256. Plansza PNG, PDF,
wyszukiwalny HTML i komplet PNG w ZIP są również artefaktem CI
`real-lvgl-ui-review-NOT-FIRMWARE`.

`test_cyber_quality_ui.c` sprawdza niezależnie stałość odsłoniętego ogrodu,
kierunek zasłaniania, podgląd w 21 pozycjach, jeden zapis po puszczeniu,
odczyt podczas gestu, anulowanie i 100 cykli usuwania animowanego ekranu.
Pozostałe testy obejmują PL/EN/DE, aktory sterowania i pełne trasy UI.
Sceny, czasy, ADC, radio i magazyn testów hosta są modelami. Kompilacja
ESP32-S3 i kontrola rzeczywistego ELF nie zastępują testu fizycznego LCD,
płynności dotyku i ruchu podłączonej rolety.

Źródło i prompt grafiki: [assets/cyber-scenes](../../assets/cyber-scenes/README.md).
