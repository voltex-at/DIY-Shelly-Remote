# Cyber Blue — dopracowanie według referencji

Baza: `d2290f21a0fd9af52d72028a705022a526812c4b` (`0.7.64-dev`).
Gałąź: `work/cyber-blue-reference-refinements`. LVGL 9.3.0, ES3C35P 320 × 480.

## Zmiany

- Wspólne ikony akcji i dekoracji korzystają z turkusowego akcentu Cyber.
  Jawne kolory ostrzeżeń, niedostępności i danych zachowują swoje znaczenie.
  API masek A8 nadal przyjmuje dowolny kolor, również biały; inne motywy
  zachowują dotychczasowy wygląd. Wyłączone przyciski zasilania mają ciemne
  tło, aby turkusowy symbol pozostał czytelny.
- System ma osobne pola ikon, większe podpisy i ramki. Testy sprzętu mają
  kartę opisu i rzeczywistego stanu kontrolera dotyku, dwie duże akcje
  z ikonami i strzałkami oraz kartę identyfikacji wyświetlacza.
  Długie nagłówki, np. niemiecka nazwa baterii, dobierają czcionkę 22/18 px
  do dostępnej szerokości, aby cały tekst pozostał widoczny.
- Pierwsza karta baterii ma 114 px: duża wartość z ikoną, opis i oddzielony
  wiek odczytu. Ostrzeżenie ma żółtą ramkę. Pamięć ma większe nazwy i wartości
  oraz osobne wiersze RAM, PSRAM, grafiki, stosu i BLE. Dalsze wiersze można
  przewinąć. **Wstecz jest po lewej**, a pomiar napięcia po prawej. Opóźnione,
  błędne, niekalibrowane i nieaktualne wyniki nadal mają odrębne opisy.
- Diagnostyka konfiguracji ma karty SPIFFS, kodów etapów, HTTPS, USB, adresów
  danych i ostrzeżenia. Wysokość opisów zależy od tekstu w PL/EN/DE. Karty są
  przewijane ponad stałym przyciskiem Wstecz. Wartości pochodzą ze wspólnego
  stanu zdrowia konfiguracji; widok nie formatuje ani nie zapisuje pamięci.
- Termostat ma swobodną ikonę i tekst Online/Offline. Usunięto górną plakietkę
  nastawy. Centralna temperatura, podgląd, potwierdzenie odczytu, blokady
  sterowania i dolne karty zaworu/baterii pozostają dostępne.
- Nawigacja pokazuje cztery kafelki pulpitu oraz procesor prowadzący do
  baterii i zasobów. Ikony baterii w samych pomiarach nadal oznaczają baterię.
- Przyciski zasilania i sterowanie roletą/TRV na pulpicie są wyrównane przez
  LVGL do pionowego środka karty, z uwzględnieniem jej obramowania. Roleta
  ma dwa niebieskie, okrągłe przyciski z dużymi, prostymi strzałkami w ramce.
  Widok szczegółów używa tych samych strzałek i większych podpisów.

Pięć nowych źródeł SVG (dotyk, USB, dokument, góra/dół) jest przeliczanych
na A8 przed budowaniem. Dodatkowe 12 masek zajmuje **12 608 B Flash**, bez
nowego dekodera, fontu lub bufora obrazu. Dotychczasowe maski są zachowane.

## Weryfikacja i odtworzenie

```bash
python3 tools/build_cyber_extra_icons.py
python3 tools/check_cyber_icons.py
python3 tools/test_ui.py --ubsan-only --case test_cyber_reference_ui
python3 tools/test_ui.py --ubsan-only
python3 tools/render_cyber_atlas.py --screens build-ui/ubsan/screenshots --out build-ui/cyber-atlas --base-ref d2290f21a0fd9af52d72028a705022a526812c4b
```

`test_cyber_reference_ui.c` sprawdza PL/EN/DE, kolory rzeczywistych obiektów,
zawieranie opisów w kartach, położenie Wstecz, trafienia pointera w testy,
pomiar i sterowanie, gotowy/niedostępny dotyk, pięć stanów SPIFFS, brak zapisu
z diagnostyki, blokady sterowania oraz nieobramowane statusy TRV. Wspólny
test edytorów sprawdza teraz powiększone podpisy pamięci. Dotychczasowe testy
gestu i animacji rolety oraz aktorów sterowania są wykonywane nadal.

Pełny atlas zbiera wszystkie oznaczone natywne klatki tej wersji, włącznie
z przewinięciami i stanami błędów. Identyczne piksele są łączone z aliasami
w manifeście. PDF, pełna plansza PNG, wyszukiwalny HTML i osobne PNG w ZIP
trafiają do artefaktu CI `real-lvgl-ui-review-NOT-FIRMWARE`.

Wyniki hosta i kompilacja ESP32-S3 nie stanowią testu fizycznego LCD,
kontrolera dotyku, ADC ani silnika rolety. Ta gałąź nie zmienia wersji,
kanału OTA, partycji ani opublikowanego wydania `0.7.64-dev`.
