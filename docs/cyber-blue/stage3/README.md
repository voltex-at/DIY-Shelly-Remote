# Cyber Blue Premium — etap 3

Rozwinięcie `ca805ab23d07780dc34e99e7e8b013350aa031ea`, ta sama opcjonalna
skórka `cyber_blue` / ID 3. LVGL 9.3.0, ESP32-S3 ES3C35P, 320 × 480.
Identyfikatory, domyślne ustawienia i zasoby trzech poprzednich skórek pozostały
bez zmian. Nie zmieniono transportu, interpretacji procentów rolety, OTA,
partycji, konfiguracji sieci ani transakcyjnego zapisu/usuwania pomieszczeń.

## Czytelność i ergonomia

| Obszar | Zmiana wyłącznie Cyber Blue |
| --- | --- |
| Pulpit | Nazwa 16 px w osobnym wierszu na szerokość karty; status 12 px i wartości 14 px poniżej. Długie nazwy mają wielokropek, przyciski zachowują wolne miejsce. Aktywny turkus i metaliczne gradienty pozostają. |
| Roleta | Pomocnicze pomiary 14 px, stan ruchu 16 px, potwierdzona pozycja 28 px, wskaźnik 136 × 182. Otwórz/Zamknij mają 48 px wysokości, STOP nadal 64 × 64. Dotyk nie zmienia potwierdzonej pozycji przed odczytem urządzenia. |
| BLU | Główny stan 28 px zajmuje pełny wiersz; bateria/RSSI 16 px, pomiary 14–18 px, czas 12 px. Bramka i jej błędy są w oddzielnym przewijanym panelu. Zachowano stany bez danych, offline i błędnego klucza oraz odświeżanie. |
| Bateria | Napięcie 28 px, status 14 px, wiek i ostrzeżenie 12 px. Pamięć ma pięć osobnych wierszy: RAM, PSRAM, grafika, stos, BLE; nazwy 14 px, wartości 16 px. Min. RAM i największy blok pozostają jako szczegóły 12 px. Zawartość przewijana, Wstecz/Pomiar stale widoczne. Bez wyliczonych procentów pamięci. |
| OTA | Wersja 22 px, kanał osobno, komunikat 14 px i kod 12 px w przewijanym panelu. Postęp 28 px, bajty osobno 12 px. Procent pochodzi z otrzymanych bajtów i rzeczywistego rozmiaru; dla nieznanego rozmiaru jest „— %”. |
| Wi-Fi | SSID 18 px w liście i 16 px w formularzu; RSSI i zabezpieczenie drugorzędne. Błąd hasła zawija się w przewijanym polu. Pole, Połącz/Wstecz i klawiatura pozostają rozdzielone. Maskowanie hasła i walidacja bez zmian. |
| Edytory | Klawiatura edytora zajmuje y=192–415, footer y=420–467. Pięć rzędów klawiszy ma ok. 40 px. Aktywne pole 44 px jest przewijane do widoku. Wstecz/Zapisz/Dalej działają również podczas pisania. |
| Pokój / pulpit | Czytelne dropdowny 16 px, zarezerwowane miejsca na nazwę i opis zdjęcia, czerwona operacja usuwania z potwierdzeniem i nazwą pokoju. Dwuwierszowe nazwy kafelków pulpitu 16 px nie nachodzą na akcje. Dłuższy edytor ma przewijanie nad stałym footerem. |

Pozostałe formularze, dialogi, kreatory, edycja lokalnej nazwy/ikony urządzenia,
grupy/sceny oraz natywne ustawienia Shelly/BLU używają istniejących wspólnych
komponentów i palety HUD. Nie powstały kopie ekranów ani drugi selektor motywów.

## Ikony i pamięć

Dodano 17 dedykowanych źródeł SVG i PNG 24/32/48: sofa, bed, kitchen, bathroom,
fan, boiler, garage, floor-lamp, wall-lamp, ev-charger, kettle, coffee-machine,
washing-machine, speaker, door-closed, door-open, valve. Maska A8 jest kolorowana
przez LVGL. Indeksy zapisanych ikon i dotychczasowa lista 20 ikon urządzeń nie
zmieniły się. Źródła oraz generator: `assets/cyber-icons/README.md`.

- Nowe maski: **66 368 B Flash**, 51 wariantów; dodatkowo stałe deskryptory.
  Dotychczasowe 62 maski / 56 256 B pozostały identyczne.
- **0 nowych fontów**, 0 nowych bitmap tła, 0 buforów alfa w RAM/PSRAM.
  Dodano sześć wskaźników etykiet diagnostyki i jeden postępu (28 B przy
  32-bitowych wskaźnikach ESP32). Obiekty aktywnego ekranu zwalnia LVGL.
- Pula LVGL **131 072 B**, stos UI **8192 B** bez zmiany konfiguracji.
- Próbka 24 przewijanych kafelków: **7720 B wolne**, największy blok **7664 B**.
- Po 60 cyklach pokój/klawiatura + diagnostyka: **97 208 → 97 208 B** wolne,
  największy blok 89 496 B (natywny host, model transportu/ADC/magazynu).
- 80 zapisanych przełączeń skórek po rozgrzaniu: brak narastającego zużycia;
  drobna zmienność luzu alokatora i cache stylów nie jest pomiarem sprzętu.
- Bez blur, dodatkowych animacji HUD i timerów; odświeżanie diagnostyki nadal
  raz na sekundę. Natywny czas pełnego przemalowania jest miarą hosta,
  nie opóźnieniem ani FPS ESP32-S3.

## Rzeczywiste rendery i weryfikacja

`overview.png` pokazuje 12 widoków. `before-after-1.png` do `before-after-3.png`
zawierają dziewięć porównań bez skalowania klatek 320 × 480. Pliki `before/`
pochodzą z etapu 2; edytor pulpitu wyrenderowano dodatkowo przed zmianą jego
układu. `render-manifest.json` identyfikuje 116 klatek z produkcyjnego C i
renderera LVGL/RGB565. Dane urządzeń i napięcia w podglądach są jawnie modelowe.
Widoki diagnostyczne uwzględniają przewinięcie, dlatego widać także dolne wiersze.

Lokalnie wykonano:

- Pełny zestaw natywnych testów UI, w tym dodatkowe przebiegi ustawień Shelly
  i kreatora BLU w Cyber Blue: **757 sprawdzeń / 66 klatek łącznie** z przebiegami
  wcześniejszej skórki. Zapis wymaga potwierdzenia; hasła pozostają maskowane.
- Cyber UI: **213 920 sprawdzeń / 28 klatek**. Większość to porównania kanałów
  pikseli 51 nowych i pierwotnych masek A8 z niezależnym wzorcem alfa, z tolerancją
  kwantyzacji RGB565. Także dotyk rolety 0/50/100%, Otwórz/Zamknij/STOP,
  odświeżenie BLU, zapis/odczyt motywu, brak restartów i cykle czterech skórek.
- Edytory Cyber: **2286 sprawdzeń / 37 klatek**. PL/EN/DE, długie nazwy,
  znaki Ł/ó/ź/ü/ä, fokus i klawiatura, dotyk footerów, zapis/anulowanie,
  zdjęcie i symbol pokoju, ochrona powiązanego pokoju, potwierdzenie usunięcia,
  nazwa urządzenia, zapis sceny i grupy oraz 60 cykli otwierania/zamykania.
- Cyber Wi-Fi/OTA: **162 sprawdzenia / 18 klatek**. PL/EN/DE, przewijana lista,
  długi SSID, maskowane hasło, błąd bez rozpoczęcia połączenia, formularz przy
  klawiaturze, długie ostrzeżenia i techniczny kod, postęp znany/nieznany/ograniczony.
- Natywny review: **2643 sprawdzenia / 24 klatki**. Konfiguracja: **7338
  sprawdzeń**; dostępny zestaw przenośnej logiki `tools/test.py` zakończył się OK.
- Pełny zestaw natywnych testów UI zaliczono także z **UBSan**.
- **344 wcześniejsze klatki PPM identyczne bajtowo** z bazą `ca805ab`;
  ich SHA256 zapisano w `legacy-frames.json`. Oryginalne fonty, brand, atlas
  i komponenty logiki urządzeń/OTA nie są zmienione w tym etapie.

CI projektu kompiluje docelowy ES3C35P / ESP32-S3 z przypiętym ESP-IDF 5.5.4,
uruchamia ASan/LSan/UBSan i pozostałe kontrole repozytorium. Wynik konkretnego
commitu należy sprawdzić w GitHub Actions; logi lokalne i natywne podglądy same
w sobie nie oznaczają testu firmware na fizycznym urządzeniu.

**Nie wykonano testów na fizycznym ESP32-S3**, rzeczywistym dotyku LCD,
ADC, radiu, Flash/NVS ani instalacji OTA. Natywne testy korzystają z rzeczywistego
LVGL 9.3.0 i kodu aplikacji oraz modeli tych usług.

Odtwarzanie: `python3 tools/test_ui.py --jobs 1`, następnie
`python3 tools/render_cyber_stage3.py`. Eksporter korzysta z dołączonych klatek
`before/` i zapisanych hashy wcześniejszych renderów.
