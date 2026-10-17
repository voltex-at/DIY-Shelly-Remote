# Stabilny stan podczas odczytu — 0.6.3-polling-dev (Issue #12)

Oddzielny draft PR oparty na graficznym PR #16. Poprawia potwierdzony przeskok
stanu HOME co pięć sekund i obsługę dotknięcia podczas odczytu. Nie zmienia
transportu HTTP, autoryzacji, sterowników ani formatu NVS.

## Zachowanie

- Automatyczny PROBE zachowuje ostatni potwierdzony stan i pomiary. Nie zastępuje
  Włączone/Wyłączone etykietą Odczyt stanu i nie wygasza przycisku ON/OFF.
- Identyczna odpowiedź GET nie przebudowuje ekranu. Na szczegółach odświeża się
  jedynie mała etykieta czasu od potwierdzonego odczytu; timer nie tworzy obiektów.
- Dotknięcie ON/OFF podczas PROBE zapisuje jedno jawne żądanie docelowego ON lub
  OFF. Pokazuje Wysyłanie polecenia i blokuje kolejne dotknięcia. Po potwierdzonym
  odczycie worker otrzymuje SET przed następnymi zadaniami konfiguracyjnymi.
- Żądanie dotyczy identyfikatora, generacji, hosta i kanału wybranych w momencie
  dotknięcia. Błąd odczytu, brak połączenia, zmiana tożsamości albo odrzucenie
  dispatch kasuje oczekujące żądanie i pokazuje błąd. Nie ma ponowienia SET.
- Stan zmienia się dopiero po SET + obowiązkowym odczycie zwrotnym. Oczekujące
  żądanie nie jest optymistycznym potwierdzeniem ruchu/przełączenia.
- Odśwież stan podczas trwającego GET korzysta z tego odczytu; nie tworzy kolejnej
  komendy. Nadal pozostaje wyłącznie odczytem.

Dotychczasowe Nieprzypisane jest nazwą braku pomieszczenia. Poprawka nie zmienia
przypisania — usuwa zastępowanie stanu urządzenia przez etykietę oczekiwania.

## Regresja automatyczna

`python3 tools/test_ui.py --ubsan-only` testuje prawdziwy kod LVGL i adaptera UI:
dwa kolejne cykle 5/10 s ze stałym stanem, niezmienionymi obiektami karty,
dotknięcie podczas PROBE, dokładnie jeden odroczony SET i wymagany readback,
duplikat dotknięcia, błąd logowania, odrzucenie dispatch, utratę Wi-Fi w kolejce,
brak automatycznego ponowienia SET, ręczne odświeżanie i wiek próbki.

Rendery `70-home-poll-stable.png`, `71-command-queued-behind-read.png` oraz
`72-details-poll-freshness.png` są rzeczywistym framebufferem LVGL 320×480.
Mają oznaczenie TEST / DEMO, gdyż test wstrzykuje zdarzenia transportu.

## Test fizyczny ES3C35P + Shelly 1PM Mini Gen3

1. Zapisz wersję i SHA, wgraj aplikacyjny BIN przez dotychczasowe OTA. Sprawdź,
   że Wi-Fi, zapisany Shelly i dashboard pozostały po aktualizacji.
2. Nagraj HOME przez co najmniej 12 s bez dotykania. Włączone/Wyłączone i kontrast
   przycisku mają pozostać stabilne między kolejnymi odczytami. Nieprzypisane
   może pozostać, jeśli wcześniej nie przypisano urządzenia do pomieszczenia.
3. Wejdź w szczegóły: obserwuj wiek odczytu. Co ok. 5 s powinien wracać do zera,
   bez znikania W/V/A i bez zmiany widoku. Porównaj stan z aplikacją Shelly.
4. Włącz i wyłącz rzeczywiste, znane obciążenie; następnie naciśnij ON/OFF kilka
   razy w kolejnych próbach w pobliżu momentu odczytu. Każde zaakceptowane
   pojedyncze dotknięcie ma dać jeden SET/readback. Nie dotykaj wielokrotnie
   wyłączonego przycisku oczekującej komendy.
5. Odłącz Wi-Fi lub Shelly przy oczekującym żądaniu. Błąd ma być jawny; po powrocie
   połączenia pilot ma tylko odczytywać stan. Nie może sam ponowić SET.
6. Zachowaj oczyszczone logi wyniku SET/readback i GET oraz film bez haseł,
   prywatnych SSID/IP. Potwierdź dotyk, jasność, sen/wybudzanie i restart.

Kompilacja/testy hosta nie zastępują tego testu. Dotychczasowe potwierdzenie
fizycznego ON/OFF od użytkownika pozostaje ważne dla wcześniejszej wersji.
Bez scalenia do main/stable i bez domniemanej akceptacji graficznej.
