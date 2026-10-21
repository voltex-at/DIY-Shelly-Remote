# Bateria — 0.7.67-dev

Wejście: **Ustawienia → Bateria i zasilanie → Kalibracja baterii**.
Profil jest domyślnie wyłączony. Samo otwarcie formularza nie zapisuje danych.
Zapis wymaga poprawnych liczb i potwierdzonego odczytu zwrotnego z NVS.

## Dane ogniwa

| Pole | Znaczenie |
| --- | --- |
| Typ | Wyłączone, 1S Li-Po albo 1S Li-ion przeznaczone do ładowania 4,2 V. Profil nie zmienia ładowarki płytki i nie obsługuje 2S, LiFePO4, NiMH ani ogniw HV. |
| Napięcie nominalne | 3600 albo 3700 mV, zgodnie z etykietą. To nie bieżący odczyt. |
| Pojemność | 1–20000 mAh z etykiety; informacja o ogniwie. Nie daje pomiaru prądu, faktycznej kondycji ani czasu pracy. |
| Punkty 0/50/100% | Napięcia ogniwa w normalnych warunkach pracy, najlepiej z dokumentacji/charakterystyki konkretnego ogniwa. Każdy kolejny punkt musi być wyższy o co najmniej 100 mV. |
| Korekta pomiaru | Równoczesny odczyt multimetru względem świeżego bazowego BAT z ADC. Współczynnik 900–1100 / 1000, czyli ±10%. „Korekta 1:1” przywraca nominalny dzielnik. |
| Animacja z trendu | Włącza częstsze odczyty i przypuszczenie ładowania z przebiegu napięcia. Nie jest sygnałem ładowarki. |

Wstępne 3300/3800/4200 mV i 1000 mAh są edytowalnym punktem startowym,
nie charakterystyką rozpoznanego ogniwa. Nie należy celowo rozładowywać baterii
poniżej granicy producenta, żeby ustawić punkt 0%.
Progi są wyłącznie punktami wskaźnika — nie ustawieniami zabezpieczeń.

## Pomiar i szacunek

ADC1 GPIO8, 12 bitów, tłumienie 12 dB, kalibracja ESP-IDF i nominalny
dzielnik 100k/100k. Po odrzuceniu pierwszej konwersji pobierana jest mediana
dziewięciu odczytów. Brak kalibracji ADC nie jest zastępowany przelicznikiem
z surowych kodów. Referencja multimetru musi mieć 2500–4350 mV, a odczyt
bazowy mniej niż 30 s. Korekta jest zawsze liczona z niekorygowanego ADC,
więc jej ponowne zastosowanie nie mnoży poprzedniej korekty.

Po zapisaniu profilu worker nakłada korektę i interpoluje liniowo odcinki
0→50 oraz 50→100%. Wynik jest ograniczony do 0–100% i oznaczony `~`.
Poza 2500–4350 mV lub przy rozrzucie ADC >120 kodów dostępne jest jedynie
napięcie. Brak poprawnego profilu daje napięcie albo stan nieznany.
Napięcie pod obciążeniem, podczas ładowania i w innej temperaturze może
odpowiadać innemu stanowi naładowania. Pojemność i typ nie usuwają tego błędu.

Nie ma pomiaru obecności ogniwa. Nawet po zapisaniu profilu napięcie BAT przy
USB bez baterii może dawać pozorny procent. Profil powinien opisywać rzeczywiście
podłączone ogniwo; po jego usunięciu należy wyłączyć szacowanie.

## Animacja „prawdopodobne ładowanie”

Na prośbę właściciela zastosowano heurystykę trendu. Przy włączonej opcji
odczyt jest zamawiany co 1 s przez istniejący worker. Nie powstaje nowy task,
timer ani kolejka próbek. Gdy worker/radio jest zajęte, pomiar czeka na wolny slot.
W czasie edycji profilu i uśpienia nie ma próbkowania tła.

Do uruchomienia animacji potrzebne są równocześnie:

- Co najmniej 10 s ciągłych obserwacji, z lukami nie większymi niż 2,5 s.
- Łączny wzrost przynajmniej 20 mV i co najmniej pięć nowych szczytów o ≥2 mV.
- Brak spadku większego niż 5 mV od ostatniego szczytu i brak zastoju >3 s.
- Poprawny skalibrowany pomiar i włączony profil; brak sprzecznego, potwierdzonego stanu ładowarki.

Pojawia się animowana bateria ze znakiem `~` i opis **„Prawdopodobne ładowanie”**.
Pojedynczy skok po podłączeniu USB nie spełnia warunku czasu.
Spadek, zastój, przerwa, błąd, uśpienie albo zmiana profilu zerują wykrywanie.
Po obudzeniu lub ponownym uruchomieniu wymagane jest nowe okno obserwacji.
Bez nowych pomiarów animacja wygasa po maksymalnie 2,5 s.

Zmniejszenie obciążenia także może podnieść napięcie i wywołać fałszywy sygnał.
Pod koniec ładowania napięcie może się ustabilizować, a animacja wtedy zgaśnie
mimo trwającego ładowania. Ten model nie potwierdza podłączenia USB, przepływu
prądu ani zakończenia ładowania. Nigdy nie ustawia `has_charge_state`/`charging`.
Przyszły rzeczywisty dostawca tych danych ma pierwszeństwo nad heurystyką.

Po wyłączeniu opcji trendu nadal działa procent, z odczytami co 20 s.
Po 30 s bez poprawnego pomiaru nagłówek pokazuje stan nieznany.

## Zapis i błędy

Osobny namespace `dsr_battery`, klucz `profile`, jeden 24-bajtowy rekord:
wersja, długość, typ, opcja trendu, liczby little-endian i CRC32. Nie zmieniono
starych rekordów Wi-Fi/urządzeń, konfiguracji workspace ani partycji.
Stan formularza i bufor wskaźnika należą do UI i znajdują się w PSRAM obok
puli LVGL; nie są używane przez ISR ani przy wyłączonym cache. Zachowano
budżet wewnętrznego BSS 80 KiB.
Zapisy są serializowane przez worker i objęte blokadą aktywności/restartu.
Wymagany jest commit oraz poprawny, identyczny odczyt zwrotny.

Brak rekordu oznacza wyłączony profil. Nieznany schemat, uszkodzenie lub błąd
odczytu wyłączają oszacowanie. Formularz zachowuje szkic po błędzie zapisu;
ponowny odczyt ustala zapisany stan przed kolejną próbą. „Nowy profil” wymaga
potwierdzenia i zastępuje tylko profil baterii dopiero po naciśnięciu Zapisz.
Nie formatuje ani nie kasuje całej pamięci. Wstecz i nagłówek chronią zmiany;
w trakcie przyjętego zapisu nie można wysłać kolejnego.

## Sprawdzenie na płytce

1. Zaktualizować przez podpisane Dev OTA lub UPDATE-APP-ONLY. Sprawdzić zachowanie konfiguracji urządzeń i Wi-Fi.
2. Z etykiety/dokumentacji zgodnego ogniwa wpisać typ, napięcie i pojemność. Zweryfikować punkty napięcia zamiast zakładać poprawność wartości wstępnych.
3. W Korekcie pomiaru wykonać odczyt BAT i równoczesny odczyt multimetru. Obliczyć korektę, zapisać, porównać świeży wynik. Nie używać błędnego/szumiącego pomiaru.
4. Sprawdzić profil po restarcie, wyłączenie szacowania, przewijanie, klawiaturę i anulowanie zmian.
5. Włączyć trend i obserwować rzeczywiste próbki po podłączeniu oraz odłączeniu USB. Zweryfikować animację przy narastaniu, jej wygaśnięcie po spadku/zastoju i brak animacji po samym skoku.
6. Sprawdzić nagłówek przy uśpieniu/wybudzeniu i zajętym workerze. Porównać wskazania przy różnych obciążeniach. To odbiór fizyczny, którego testy hosta nie zastępują.

## Źródła i odtwarzanie

- [LCDWiki ES3C35P — parametry i dokumentacja producenta](https://www.lcdwiki.com/3.5inch_ESP32-S3_Display).
- [TI SLUAAA1 — State of Charge Estimation Using Smart Battery Charger](https://www.ti.com/lit/an/sluaaa1/sluaaa1.pdf), charakterystyka konkretnego ogniwa i ograniczenia napięciowego szacowania w rozdziałach 2 i 5. Z tej publikacji nie skopiowano krzywej procentów; trzy punkty należą do profilu użytkownika.
- `python3 tools/test.py --sanitize` — rzeczywiste model, NVS i worker, jawne modele SDK/Flash.
- `python3 tools/test_ui.py --sanitize --case test_battery_calibration_ui` — rzeczywiste LVGL 9.3.0, wszystkie języki i motywy, błędy, klawiatura, animacja pikseli, 40 cykli i 24 kafelki po formularzach. W ograniczonym środowisku lokalnym użyto `--ubsan-only`; pełne ASan/UBSan pozostaje w CI.

Rendery `atlas-battery-calibration-*.ppm` są niezmienionymi pikselami produkcyjnego
LVGL z jawnymi danymi testowymi. CI dołącza je do pełnego atlasu.
