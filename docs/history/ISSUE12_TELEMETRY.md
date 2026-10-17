# Rzeczywiste V/A i wyniki sterowania — 0.6.1-telemetry-dev

Osobny draft oparty na PR #13. Nie zastępuje sterowników ani transportu, nie
zmienia partycji i nie wymaga ponownego dodawania zapisanego Shelly.

## Dane

Oficjalny Switch.GetStatus opisuje opcjonalne `apower` (W), `voltage` (V)
i `current` (A): https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Switch/ .
Wartości są odczytywane z rzeczywistej odpowiedzi, nigdy obliczane jako W/V
ani domyślnie przypisywane zeru. Zero jest pomiarem wyłącznie po jawnym polu
liczbowym. Brak oznacza niedostępność. Niepoprawny typ, nieskończoność,
wartość poza szerokim limitem prezentacji 1e9 lub ujemne V/A odrzucają próbkę.
Ujemna moc pozostaje dozwolona. Obsługa Gen1 nie otrzymuje zgadywanych metryk.

Pomiary są ulotne. Jawny typ rekordu NVS zachowuje dokładny układ schema-1
sprzed dodania telemetrii: stary device + credentials, bez nowych pól runtime.
Test wczytuje rzeczywisty stary blob i sprawdza niezmieniony rozmiar po zapisie.
Nie zmieniono schematu ani nie kasuje się konfiguracji.

## Komenda i odświeżanie

Na karcie i szczegółach widoczne są m.in. błąd logowania, brak odpowiedzi,
zmieniona tożsamość i niepotwierdzony stan. Nieudane SET/readback nie tworzy
optymistycznego stanu. Automatyczny polling nie usuwa komunikatu nieudanej
komendy; użytkownik potwierdza stan przyciskiem Odśwież stan. Ta akcja wykonuje
wyłącznie weryfikację tożsamości i GET, bez automatycznego ponawiania SET.
Wysyłanie polecenia i okresowy odczyt mają oddzielne etykiety.

Log `dsr_network` pokazuje wynik transakcji SET/readback oraz odczytu stanu,
flagę potwierdzenia i dostępność V/A. Nie zapisuje IP, ID/MAC ani haseł.
Wynik timeout nie rozstrzyga, czy fizyczna komenda dotarła; należy odczytać stan.

## Regresje automatyczne

`python3 tools/test.py --ubsan-only` uruchamia prawdziwy adapter HTTP/cJSON/NVS
z kontrolowanym transportem. Obejmuje Gen3, brak V/A, jawne zera, złe typy,
ujemne/nieskończone wartości, odrzucenie komendy, brak odpowiedzi, readback
mismatch oraz zgodność starego zapisu NVS. `tools/test_ui.py --ubsan-only`
sprawdza mapowanie V/A, widoczne błędy na karcie/szczegółach, utrzymanie błędu
po pollingu i jawne odświeżanie bez SET. CI powtarza ASan/UBSan, przeglądarkę
i pełną kompilację płytki. Rendery `50-*`–`54-*` mają TEST / DEMO, ponieważ
zdarzenia transportu są wstrzyknięte przez test. Nie dowodzą pomiarów fizycznych.

## Dokładny test na Shelly 1PM Mini Gen3

Użytkownik potwierdził wcześniej prawdziwe wykrywanie, dodanie, pomiar około
4 W i ON/OFF na ES3C35P. Nie wycofujemy tego potwierdzenia; nowa wersja nadal
wymaga regresji i fizycznego potwierdzenia V/A. Nie publikować prywatnych filmów.

1. Zainstaluj aktualny pakiet zgodnie z INSTALL_AND_TEST.md, bez kasowania NVS.
   Zapisz dokładny SHA, wersję i model/firmware Shelly. Powiązanie ma przetrwać.
2. Przy znanym, bezpiecznie podłączonym obciążeniu porównaj W/V/A na pilocie
   z aktualnym Switch.GetStatus lub aplikacją Shelly. Brak pola ma pozostać —.
3. Naciśnij Wyłącz. Zapisz stan pilota i obserwację wyjścia lub aplikacji Shelly,
   następnie Włącz i ponownie porównaj. Zachowaj oczyszczony log
   `Switch.Set/readback result=... confirmed=...` oraz `Switch.GetStatus ...`.
4. Zmień stan w aplikacji Shelly; sprawdź automatyczny odczyt. Odłącz Shelly
   od sieci i spróbuj sterowania: brak odpowiedzi ma być jawny, bez fałszywego
   potwierdzenia i bez ponawiania komendy. Przywróć sieć i Odśwież stan.
5. Sprawdź niepoprawne hasło i powrót po poprawnym dodaniu. Poprzednie
   powiązanie ma pozostać po nieudanej próbie zastąpienia. Nie loguj haseł.
6. Uruchom pilot ponownie, sprawdź binding, hasło, V/A oraz działanie klawiatury,
   skanowania i dotyku. Zatwierdzenie UI nadal należy do właściciela projektu.

Bez automatycznego scalenia do main i bez stabilnego wydania na podstawie CI.
