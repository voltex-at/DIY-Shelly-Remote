DIY SHELLY REMOTE — 0.6.0-ui-dev — rzeczywisty LVGL 320 × 480

Gałąź: work/issue-6-8-unified-ui, osobny draft PR. Bez scalenia do main.
To wynik kodu C/LVGL 9.3.0 i bufora RGB565, nie makieta HTML ani ilustracja.
Manifest opisuje gałąź, renderer i SHA-256 każdego PNG.

01-home / 10-home-room-photos: poziome karty i fotografie pomieszczeń.
20–28: prawdziwe formularze z testowym dostawcą radia/konfiguracji.
30–35: klawiatura PL/EN/DE i IP, z literami 22 px i ruchem kursora.
36: aktywne hasło Shelly nad klawiaturą. 40–41: menu i ustawienia.
before-*: zachowane rendery 0.5.1 do porównania przed/po.

Poprawiono 5-pozycyjną nawigację bez asystenta, dynamiczne chipy pomiarów,
wspólne nagłówki/przyciski/typografię formularzy, większą klawiaturę i fokus.
Dolny rząd: 123 | ← | Spacja | → | Gotowe. PL/EN: QWERTY, DE: QWERTZ.
Shift, symbole, polskie/niemieckie znaki i numery/IP są funkcjonalne.

Kompozycja odnosi się do prywatnego zrzutu Shelly Smart Control użytkownika:
grafitowe karty, niebieski akcent, poziome urządzenia, 2 kolumny pomieszczeń,
górne zakładki i dolne ikony. Nie publikujemy zdjęć/filmów użytkownika.
Ikony: Material Symbols Rounded, Apache-2.0; font: niezależnie licencjonowany
Roboto. Zdjęcia: Nenad Stojković / Shixart1985, CC BY 2.0; dokładne źródła,
licencje i zmiany: tests/ui/photos/SOURCES.json w repozytorium.

Dane DEMO oraz testowy dostawca radia nie wchodzą do firmware. Rzeczywisty
transport nadal obsługuje jedno połączenie relay/switch; AP/BLE są wyłączone.
Dotychczas użytkownik potwierdził prawdziwy LAN, dodanie i ON/OFF 1PM Mini Gen3.
Nowe UI nadal wymaga fizycznej regresji i akceptacji. Instrukcja: UNIFIED_UI.md.
