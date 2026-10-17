# Wspólny interfejs i klawiatura — 0.6.0-ui-dev

Kontynuacja Issue #6 i #8, oparta na istniejącym firmware 0.5.1. Gałąź
`work/issue-6-8-unified-ui`. Osobny draft PR; bez scalenia do main.

## Zmiany i porównanie

| Element | Przed 0.5.1 | Teraz |
| --- | --- | --- |
| Dolna nawigacja | 6 ikon, gwiazdki mylące z asystentem | 5 pozycji po 64 px: pulpit, dom, energia, ustawienia, dodaj |
| Edycja pulpitu | gwiazdki i górny ołówek | górny ołówek; sceny pozostają w modelu i edytorze |
| Tło pomiarów | stała szerokość prawie do brzegu karty | pomiar tekstu LVGL + 14 px paddingu, limit szerokości karty |
| Wi-Fi / Shelly | nagłówek DIY SHELLY REMOTE, dawne kolory i Home/Menu | wspólny nagłówek, Rounded ikony, grafitowe przyciski, ta sama typografia co HOME |
| Klawiatura | małe litery 14 px, domyślny układ | 22 px Roboto, pięć rzędów, cyfry, QWERTY/QWERTZ, strzałki i krótsza spacja |
| Edytowane pole | część pól Shelly zasłonięta klawiaturą | przewijany formularz; aktywne pole pozostaje nad klawiaturą |

Rendery sprzed poprawki są zachowane w `docs/ui-review/before-0.5.1/`.
Aktualne w `docs/ui-review/renders/`; oba zestawy pochodzą z rzeczywistego LVGL,
nie z ilustracji. Wzorzec Shelly Smart Control użytkownika pozostaje prywatny.
Zachowano grafitowe karty, niebieskie akcenty, poziomą kompozycję urządzeń,
dwukolumnowe fotografie pomieszczeń, górne zakładki i dolną nawigację.
Świadome różnice: brak logo/zasobów Shelly, 5 pozycji bez asystenta oraz
krótsze nagłówki i przewijane formularze na 320×480. Material Symbols Rounded
ma Apache-2.0; Roboto i fotografie testowe mają zachowane osobne licencje.

## Klawiatura

Wspólna implementacja obsługuje Wi-Fi, ręczny IP/kanał, nazwę/login/hasło Shelly
i edytor konfiguracji. Ma rozmiar 320×240, litery 22 px, odstępy 3–4 px,
zaokrąglone grafitowe klawisze i oddzielne strzałki. Dolny rząd:
`123 | ← | Spacja | → | Gotowe` (etykiety lokalizowane).

Shift przełącza wielkość liter. `123` otwiera cyfry/symbole, `#+=` drugą
planszę symboli, `ąä` znaki polskie i niemieckie, `ABC` powrót do liter.
Wszystkie drukowalne znaki ASCII są osiągalne; hasło nie jest ograniczone do
liter. Strzałki wywołują API kursora LVGL i nie wpisują symboli do tekstu.
Gotowe zatwierdza Wi-Fi dokładnie raz, a w pozostałych formularzach zamyka
klawiaturę. Nie włącza logowania wpisywanego tekstu ani hasła.

## Testy odtwarzalne

```sh
python3 tools/test_ui.py --ubsan-only
python3 tools/test_ui.py --review --ubsan-only
python3 tools/render_review.py --no-run
```

CI powtarza testy z ASan/UBSan, testuje panel i kompiluje ES3C35P oraz oba
cele diagnostyczne. Testy używają faktycznych zdarzeń wskaźnika LVGL, geometrii
klawiszy i masek RGB565. Pokrywają PL/EN/DE, UTF-8, poprawianie środka hasła,
wszystkie drukowalne ASCII, granice kursora, IP, klawisze skrajne, Shift,
Backspace, Gotowe oraz brak podwójnego połączenia. HOME ma testy pięciu
obszarów dotyku, disabled energii, ołówka, chipów i nawigacji w trzech językach.
Native fixture radia nie jest częścią firmware i nie udaje fizycznego testu.

## Odbiór na fizycznym ES3C35P — do wykonania

Dotychczas użytkownik potwierdził skan Wi-Fi, prawdziwy LAN, dodanie i ON/OFF
Shelly 1PM Mini Gen3 oraz pomiar mocy. Nowy wygląd nadal wymaga akceptacji.
Nie przypisujemy wcześniejszego wyniku do nowej wersji bez ponownego testu.

1. Wgraj pakiet zgodnie z INSTALL_AND_TEST.md, zachowując NVS. Zapisz SHA
   i wersję z ekranu. Sprawdź HOME z pomieszczeniami, trzema typami kart,
   szczegóły i offline. W menu mają być 5 równych pozycji bez gwiazdek.
2. Ołówek musi otwierać edytor. Zmień nazwę, zapisz i sprawdź panel WWW.
3. Otwórz Wi-Fi, wybierz sieć, sprawdź SSID i błąd widoczne nad klawiaturą.
   W testowym haśle `abcdef123!xyz` przesuń kursor cztery razy w lewo,
   wstaw literę i usuń ją Backspace. Sprawdź maskowanie i pokaż/ukryj.
   Wypróbuj Shift, cyfry, znaki specjalne i Gotowe; połączenie ma ruszyć raz.
4. Powtórz DE/QWERTZ i IP/kanał numeryczny. Sprawdź polskie i niemieckie
   znaki w nazwie pomieszczenia, bez rozbijania UTF-8 podczas edycji.
5. Skanuj LAN, uwierzytelnij Shelly, nadaj nazwę i wybierz pomieszczenie.
   Aktywne pole loginu/hasła/nazwy ma pozostać nad klawiaturą. AP/BLE są
   jawnie niedostępne. W trybie realnym nie może być danych DEMO.
6. Na znanym bezpiecznym obciążeniu sprawdź ON/OFF, stan zewnętrzny, restart,
   utratę i powrót Wi-Fi oraz brak regresji jasności/dotyku/wybudzania.
   Nowe V/A i błędy komunikacji są osobnym zakresem Issue #12.
7. Przekaż prywatnie zdjęcia tych samych ekranów. Nie publikuj SSID,
   adresów, haseł ani prywatnych filmów w publicznym repozytorium.

Po testach i ocenie graficznej PR nadal wymaga wyraźnej zgody właściciela na
scalenie. Nie publikować stable na podstawie kompilacji.
