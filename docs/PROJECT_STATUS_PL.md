# Stan prac i jeden backlog po porządkowaniu — 2026-10-22

Zlecenie właściciela: przejrzeć lokalny projekt i wszystkie gałęzie, zebrać zlecenia oraz wykonane prace, wskazać niespójności, zastąpić stare zgłoszenia jednym issue i usunąć stare wyniki/rendery. Zakres porządkowania nie dodawał funkcji firmware; późniejsze wdrożenia opisano poniżej.


## Przygotowanie 0.7.76-dev do próby na płytce — 2026-10-24

Właściciel zlecił publikację nowego DEV z poprawkami `83f9fb7` i układem rolety
`f5c185e`. Docelowy tag to `dev-v0.7.76`. Wymagane są pełne CI 5/5 dokładnego
commita, podpis chronionego środowiska oraz publiczne pobrania katalogu,
manifestu i całego BIN z weryfikacją. Właściciel wykona odbiór aktualizacji
0.7.75→0.7.76 i sterowania na płytce po udostępnieniu kandydata; ten odbiór
pozostaje otwarty. Wyłącznie DEV, bez main i Stable.

## Roleta: ACK i odczyt w tle po 0.7.75 — 2026-10-24

Właściciel zatwierdził rozdzielenie przyjęcia komendy i późniejszego odczytu.
Komenda Cover, również grupy/sceny i STOP, kończy się na prawidłowym ACK.
Odczyt jest osobnym zleceniem; jego błąd nie zmienia wyniku przyjętej komendy.
Zachowano rzeczywiste pozycje, wiek danych, pilny STOP i zakaz replay.
[Raport i walidacja](RESPONSIVENESS_AUDIT_0_7_75_PL.md) opisują także poprawki
trzech przypadków STOP/grup znalezionych w niezależnym przeglądzie.
Zmiana jest lokalna po opublikowanym 0.7.75-dev; odbiór na płytce pozostaje otwarty.

## Przygotowanie 0.7.75-dev do odbioru — 2026-10-24

Właściciel zlecił publikację kolejnego DEV z poprawkami commita `a747fbe`.
Docelowy tag to `dev-v0.7.75`. Sprawdzane są pełne CI dokładnego commita,
podpis oraz publiczne pobrania katalogu, manifestu i BIN. Wydanie pozostaje
kandydatem do próby na płytce; odbiór dotyku/płynności, rzeczywistego timeoutu
i aktualizacji 0.7.74→0.7.75 jest otwarty. Bez Stable i scalenia do main.

## Poprawki po sprzętowym zgłoszeniu 0.7.74 — 2026-10-24

[Raport responsywności po 0.7.74](RESPONSIVENESS_AUDIT_0_7_74_PL.md) opisuje
odtworzone przyczyny zbędnych przebudów ekranów, rozdzielenie wyniku zapisu
od odczytu i ograniczone odzyskanie przez GET. Dotyk nie otrzymał nowego progu
nacisku. Są to poprawki po opublikowanym tagu, przeznaczone do 0.7.75-dev;
potwierdzenie płynności i diagnoza rzeczywistego timeoutu nadal wymagają płytki.

## Opublikowany 0.7.74-dev do odbioru — 2026-10-24

Właściciel zlecił publikację kolejnego DEV z lokalnymi naprawami. Opublikowany
tag `dev-v0.7.74` wskazuje `a47e677`. Dokładne CI (5/5), podpis i publiczne
pobrania przeszły. Wydanie obejmuje poprawki po 0.7.73 opisane poniżej.
Właściciel dostarczył log uruchomionego 0.7.74 i zgłosił dalsze opóźnienia
oraz „No response”; nie uznajemy responsywności za odebraną na sprzęcie.

## Przegląd wszystkich funkcji po 0.7.73 — 2026-10-24

Po dalszych zgłoszeniach właściciela rozszerzono analizę na wszystkie rodziny
funkcji. [Raport przeglądu i napraw](RESPONSIVENESS_AUDIT_0_7_73_PL.md)
zawiera tabelę niespójności, pokryte moduły, walidację oraz odbiór sprzętowy.
Poprawiono odczyty Cover/TRV, STOP grup, ręczne nadpisanie tej samej nastawy,
blokady kart WWW, ochronę edycji przy wejściu do baterii, odliczanie OTA,
świeżość podglądu LCD oraz zajmowanie serwera przez lokalny upload OTA.
Są to zmiany robocze po opublikowanym tagu; wymagają próby na płytce i nie
zmieniają plików wydania 0.7.73. Wybrane regresje, LVGL, Chromium, pełny build
ESP32-S3 i związane audyty pamięci przeszły.

## Opublikowany etap: responsywność 0.7.73-dev — 2026-10-24

Właściciel zlecił publikację DEV do sprawdzenia na płytce. Wersja 0.7.73-dev
zachowuje dotychczasowe dane i poprawkę pobierania OTA z 0.7.72. Szczegóły
rolety i pojedynczego termostatu aktualizują istniejące widżety przy poprawnych
odczytach; oczekująca nastawa pozostaje odróżniona od potwierdzonego stanu.
Pomiar baterii w tle nie imituje dotknięcia. Ręczne polecenia oraz odczyty
bieżącego ekranu wyprzedzają zwykłe odświeżanie innych urządzeń, z zachowaniem
obsługi tła, backoff i pilnej ścieżki STOP.

Trwające żądanie sieciowe nadal kończy się przed następnym zwykłym zleceniem.
Wydanie jest kandydatem do odbioru, a nie potwierdzeniem płynności na sprzęcie.
Pełne CI dokładnego commita, podpis i publiczne pobrania są bramkami publikacji;
natywne OTA 0.7.72 → 0.7.73, restart, dane i czasy reakcji wymagają próby
właściciela zgodnie z `docs/RELEASE_CHECKLIST_PL.md` oraz issue #74.

## Poprawka OTA 0.7.72-dev — 2026-10-24

Zdjęcie pilota 0.7.70-dev pokazało `16/6/200/1`: pobranie manifestu odmówione
z powodu nagłówków przed weryfikacją podpisu. Publiczny redirect GitHub dla
manifestu i BIN zawierał CSP 9217 B wobec dotychczasowych 8192 B na wartość.
Poprawka dopuszcza go w niezmienionym łącznym budżecie 16 KiB, zachowując
politykę TLS/podpisu/framingu. Wersja źródeł: `0.7.72-dev`, ta sama gałąź.
Starszy pilot potrzebuje jednorazowej aktualizacji przez WWW lub USB app-only;
instrukcja jest w `docs/INSTALL_UPDATE_PL.md`. Potwierdzenie sprzętowe pozostaje
otwarte; publiczna integralność 0.7.71 nie była dowodem jego pobrania przez 0.7.70.

## Etap SYS-02 w DEV 0.7.71 — 2026-10-24

Właściciel polecił zakończyć bieżące wdrożenie i opublikować DEV, a następnie
kontynuować ankietę graficzną i ustalenie priorytetów. Gałąź pozostaje
`work/dev-0.7.70`; wersja źródeł to `0.7.71-dev`. Starsze wpisy poniżej opisują
wyniki poprzednich etapów, nie cofają aktualnego stanu.

Aktywność LCD jest wdrożona i zaakceptowana na podglądzie. Pełny szyfrowany
backup, przywracanie na tym samym pilocie i reset po fizycznej zgodzie LCD są
wdrożone. Jeden niezależny przegląd SYS-02 wskazał pięć istotnych poprawek:
STOP na wejściu kolejki WWW, nieblokujący transfer HTTPS, anulowanie także po
utworzeniu kandydata importu, reset bez eksportowania starych bonds oraz
datę/wersję kopii w potwierdzeniu. Wszystkie poprawiono z regresjami.
Dodatkowo wykryto i poprawiono rozbieżny numer stanu zgody panelu JS i firmware.

Zbiorczy zestaw SYS-02 przeszedł przed końcowym przeglądem; po poprawkach
uruchamiano tylko dotknięte przypadki. API: PASS370, kontroler: PASS91,
kolejka runtime/STOP: PASS850, właściciel BLE: PASS52, Chromium: PASS60,
LCD LVGL 9.3: PASS488. Staging i scenariusz niepewnego commita również PASS.
To liczby asercji w pojedynczych zestawach, nie tysiące oddzielnych uruchomień.
Build ESP32-S3 przeszedł; audyt 19 nowych modułów: ramki własne ≤1024 B,
663 B stałego stanu; stos UI 8192 B i pula LVGL 131072 B bez zwiększenia.

Pełne CI dokładnego commita i podpisane wydanie są wymaganymi końcowymi bramkami.
Odbiór nowych widoków backupu/resetu w ankiecie nastąpi po DEV. Sprzętowe
przerwania zasilania, trwałość Flash, ponowne połączenie Bluetooth, ADC i dotyk
pozostają niepotwierdzone; pilota nie resetowano ani nie flashowano.

## Aktualizacja: okno/drzwi → ogrzewanie — 2026-10-23

[Wynik etapu w ankiecie #74](https://github.com/voltex-at/DIY-Shelly-Remote/issues/74#issuecomment-6370925230).

Właściciel zatwierdził projekt oraz ośmioetapowy plan do wykonania tutaj. Model,
dziennik NVS, źródłowa świeżość kontaktów, bezpośredni odczyt TRV, schemat 3,
wykonawca i edytory LCD/WWW są wdrożone w lokalnych commitach. Nowa reguła jest
wyłączona, z domyślną nastawą 8°C. Ręczne zmiany są zachowywane; wznowienie po
przerwie wymaga nowych raportów wszystkich uczestników. Wykorzystano istniejącego
aktora/workera i zadanie konfiguracji, bez nowego zadania RTOS.

LCD ma wejście ze Scen, trzy skórki i PL/EN/DE, osobny stan reguły, złote ważne
komunikaty i sfazowane podwójne ramki Cyber Blue. WWW edytuje ten sam dokument
przez CAS; odczyt statusu pozostaje za istniejącym parowaniem i kontrolą lokalnego
pochodzenia. Nie eksportuje prywatnych tożsamości ani dziennika.

Końcowa integracja hostowa przeszła: model i dziennik, rzeczywisty config-task,
aktor/worker, odczyty i wznowienie, API, wybrane nowe ekrany LVGL 9.3 oraz panel
Chromium. Sprawdzono też dotknięte wcześniejsze przypadki aktora/workspace i
spójność tłumaczeń. Pełnego atlasu ani całego historycznego zestawu UI nie generowano.

Niezależny przegląd ośmiu etapów zakończono. Pięć istotnych uwag zostało
poprawionych w lokalnym commicie `c1dd336`, z regresjami odtwarzającymi błędy:

| Niespójność z przeglądu | Wynik poprawki |
|---|---|
| Bariera świeżości była przypisana do indeksu reguły | Wstawienie/usunięcie wcześniejszej reguły zachowuje właściwą barierę i cykl; stary raport nie zezwala na przywrócenie temperatury |
| Niedostępny TRV zatrzymywał kolejkę pozostałych reguł | Niewysłany odczyt ustępuje innym celom; usunięcie/wyłączenie reguły może wycofać taki odczyt |
| Pusta pozycja tabeli 429 zasłaniała dalszy wpis | Wyszukiwanie obejmuje całą tabelę; drugi limit zachowuje dłuższe oczekiwanie, a stare ID nie zajmują jej bez końca |
| Nieudane usunięcie na LCD modyfikowało szkic | Usuwany jest osobny kandydat do zapisu; Wstecz i Zapisz po błędzie nie usuwają reguły |
| Błąd bezpośredniego odczytu był pokazywany jako zwykłe oczekiwanie | LCD/WWW/API podają przyczynę dla celu; udany odczyt ją kasuje, a ważniejszy błąd dziennika pozostaje widoczny |

Po tej serii poprawek wykonano jeden wspólny przebieg
`python3 tools/test_automation.py --case all` — PASS, w tym 18 scenariuszy
wykonawcy z rzeczywistym config-task/aktorem/workerem i modelami radia/Flash.
Nowy przypadek `test_automation_ui` — PASS na LVGL 9.3 w trzech skórkach i
PL/EN/DE; `node tests/test_panel_automation.cjs` — PASS w Chromium, w tym
mobilny formularz bez kolizji stopki. Spójność słownika PL/EN/DE również przeszła.
Po drobnych poprawkach odtwarzano tylko odpowiadające im przypadki; bez pełnego
atlasu i ponawiania całego historycznego zestawu testów.

Build ES3C35P na ESP-IDF 5.5.4 dla implementacji `c1dd336` przeszedł. Plik aplikacji
ma 0x4242e0 bajtów i 31% wolnej partycji aplikacji. Audyt ELF potwierdził pulę
LVGL w PSRAM i 8 KB stosu UI w pamięci wewnętrznej. Ramki funkcji: konfiguracja
352 B, cykl wykonawcy 336 B, kopia statusu/API 528 B, renderer formularza 832 B;
wszystkie mieszczą się w zarejestrowanych budżetach. To statyczne ramki kompilatora,
bez pomiaru rzeczywistego high-water mark. Wersje zależności i ABI job/event są
zachowane.

Rzeczywiste podglądy nowych LCD i WWW przedstawiono do ankiety. **Złote ramki
Cyber Blue po korekcie są zaakceptowane na podglądzie; pełny odbiór nowych
formularzy/statusów pozostaje otwarty**. Wcześniejsze zaakceptowane karty
urządzeń/grup zachowują swój odbiór. Host używa modeli radia/Flash/RTOS i nie ma
lokalnego UBSan; nowe obowiązkowe testy ASan/UBSan są zarejestrowane w CI,
którego tutaj nie uruchamiano zdalnie. Bezpośredni odczyt TRV ogranicza się do
udokumentowanej bramki Gateway Gen3. Potwierdzenie rzeczywistego RPC, przerw
zasilania i high-water mark stosu wymaga sprzętu. Przed kolejnym DEV pozostają
końcowy odbiór UI, walidacja sprzętowa, CI dokładnego commita oraz kontrola OTA
i integralności wydania. Bieżący etap zachowuje `0.7.70-dev` na gałęzi roboczej,
bez push, flashowania, merge i publikacji.


### Korekta po ocenie właściciela: złote ramki Cyber Blue

Właściciel odrzucił podgląd nowych ekranów: złote ramki z tekstem nachodziły na
siebie. Odtworzono to na rzeczywistym LVGL — komunikat zapisu był osobnym
stałym panelem, który zmniejszał obszar treści i przecinał wcześniejszą ramkę.
Komunikaty zapisu/błędów są teraz częścią tej samej przewijanej treści, po
pozostałych elementach, z odstępem co najmniej 8 px. Nowy komunikat jest
przewijany do widoku, a przyciski dolne pozostają na swoim miejscu.

Regresja przed poprawką wykazała kolizję pełnych granic ramek. Po poprawce
`python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_automation_ui`
przeszedł: w Cyber Blue obie ramki są w całości widoczne ponad akcjami dla
PL/EN/DE, a tekst mieści się wewnątrz. Ten sam przypadek sprawdził pozostałe
formularze, zapis i powrót w trzech skórkach. Obejrzano rzeczywiste piksele.
Bez pełnego atlasu, powtarzania backendu i nowego builda docelowego po tej
niewielkiej zmianie UI; pełny build nadal należy do bramek przed wydaniem.
**Odbiór właściciela:** „Tak, ramki zaakceptowane”. Akceptacja obejmuje pokazane
złote ramki automatyzacji w Cyber Blue po poprawce `1852e97` (PL/EN/DE), ich
rozdzielenie i czytelność. Pozostałe kroki edytora oraz stany automatyzacji
pozostają w ankiecie; nie jest to odbiór wykonawcy na sprzęcie ani całej funkcji.


### NET-02 — konfiguracja sieci Shelly i BLU: pierwsza runda

Właściciel wybrał „Wi-Fi/BLE — konfiguracja przez bramkę”. Ta runda obejmuje
zmianę sieci docelowego Shelly oraz fizyczne parowanie/PIN, ustawienia BLU,
potwierdzenie zapisu i usunięcia powiązania pilota. Wybranie bramki, jej
uwierzytelnienie, rejestracja i zapis karty będą oceniane osobno; nie uznano
całego NET-02 za wykonany na podstawie tych podglądów.

Rzeczywisty render ujawnił brak złotych ramek przy ważnych komunikatach,
nieprecyzyjny tytuł „Hasło Wi-Fi” na podsumowaniu zmiany sieci i brak tłumaczenia
przycisku „Zapomnij” w EN/DE. Dodano wspólny, dopasowany do tłumaczenia panel
ostrzeżenia z zaakceptowanym stylem Cyber Blue, zachowując kolory Grafit/Jasny.
Podsumowanie pokazuje teraz „Sieć Wi-Fi”, a przycisk ma „Forget”/„Vergessen”.
Instrukcja parowania, potwierdzenie BLU i informacja o zachowaniu karty/bramki
mają złote ramki. Hasło i PIN pozostają zamaskowane; edycja wymaga osobnego
potwierdzenia przed zapisem. Protokół RPC i zabezpieczenia nie były zmieniane.

Walidacja: `DSR_UI_FOCUS_GATEWAY_CONFIG=1 python3 tools/dev/run_host.py test_ui.py
--jobs 4 --case test_shelly_settings_ui` — PASS po regresjach wykazujących
powyższe niespójności. Tylko 35 reprezentatywnych klatek: trzy skórki DE oraz
Cyber Blue PL/EN, bez atlasu. Sprawdzono granice tekstu wewnątrz złotych ramek,
podpis podsumowania, tłumaczenia, układ klawiatury, zamaskowanie danych i brak
zapisu bez potwierdzenia. Hostowe `tools/test_blu.py` i
`tools/test_device_settings.py` przeszły raz: powiązanie/PIN/GATT, aktor ustawień,
rejestracja/brak odbioru, uzgadnianie niepewnego wyniku i prywatność podglądu.
Radio, HTTP i Flash są modelowane. Nie powtarzano backendu po korekcie samych
ramek, nie wykonywano nowego builda docelowego ani żadnej operacji na urządzeniu.
Pierwszy podgląd wymagał korekty turkusowych ramek opisanej poniżej.

**Korekta po ocenie — turkusowe ramki Cyber Blue:** właściciel odrzucił oba
podglądy, wskazując brak turkusowych obramowań tekstu. Opis i nazwa sieci na
podsumowaniu Shelly oraz informacje parowania, wartość ustawienia i pytanie o
usunięcie powiązania BLU mają teraz osobne, dopasowane panele z podwójnym
sfazowanym konturem Cyber Blue. Złote komunikaty zachowują swój kolor i styl.
Układ podsumowania wylicza kolejne pozycje z rzeczywistej wysokości paneli,
także przy maksymalnej nazwie sieci. Instrukcja BLU przelicza wysokość ramki
przed ułożeniem kolejnego bloku, ze spójnym odstępem 8 px.

Regresje wykazały brak osobnych ramek oraz kolizje przed przeliczeniem wysokości.
Odbiór pikseli wykrył dodatkowo przycięty dolny panel w PL; zmniejszenie odstępu
i kontrola całej ramki usunęły problem. Końcowo
`DSR_UI_FOCUS_GATEWAY_CONFIG=cyber python3 tools/dev/run_host.py test_ui.py
--jobs 4 --case test_shelly_settings_ui` — PASS: 13 dotkniętych stanów, Cyber Blue
PL/EN/DE, w tym maksymalna nazwa sieci. Sprawdzono granice tekstu, pełną widoczność
ramek ponad akcjami i odstępy co najmniej 8 px. Regresje obramowań są również
włączone w zwykły wariant tego testu. Grafit/Jasny zachowują poprzedni układ;
nie powtarzano ich renderów, backendu, atlasu ani builda docelowego po tej korekcie.
**Właściciel zaakceptował pokazane turkusowe obramowania i odstępy:**
„Tak, obramowania zaakceptowane” (kod `638b56c`). Akceptacja dotyczy czterech
pokazanych ekranów Wi-Fi/BLU Cyber Blue. Odbiór kolejnych etapów opisano poniżej;
rzeczywiste działanie radiowe nadal wymaga weryfikacji.

### NET-02 — wybór bramki i wynik rejestracji BLU

Następna runda obejmuje wyszukiwanie i pustą listę bramek, wybór bramki,
logowanie, instrukcję skojarzenia TRV, rejestrację, jej wyniki oraz przejście
do istniejącego formularza nazwy/pomieszczenia/kanału. Nie zmieniono protokołu
rejestracji, zapisu kluczy ani działania aktora ustawień.

Rzeczywisty render wykazał przycinanie długiej nazwy bramki razem z IP i brak
ramek informacji/ostrzeżeń. Nazwa ma teraz osobny przewijany wiersz, a pełny IP
pozostaje poniżej. Informacje Cyber Blue korzystają z zaakceptowanych podwójnych
sfazowanych ramek; ostrzeżenia mają złote teksty i ramki we wszystkich skórkach.
Kolejne bloki są układane z rzeczywistej wysokości tłumaczeń. Uzupełniono
tłumaczenie „Rozpocznij”. Błąd autoryzacji rejestracji wskazuje sprawdzenie
powiązania BLU oraz danych logowania bramki, ponieważ kod wyniku nie rozróżnia
tych dwóch źródeł. Informacja jest związana z aktualnym wynikiem rejestracji.

Przegląd pikseli ujawnił też przyciętą górną ramkę loginu po przewinięciu do
hasła. Zagęszczono wyłącznie ten formularz: oba pełne pola mają wysokość 44 px,
widoczne podpisy i mieszczą się nad akcjami oraz klawiaturą. Hasło jest maskowane.

Walidacja: `python3 tools/dev/run_host.py test_ui.py --jobs 4 --case
test_blu_gateway_ui` — PASS na przypiętym LVGL 9.3. Jeden scenariusz, 58 klatek:
wspólne stany Grafit/Jasny DE oraz Cyber Blue PL/EN/DE, w tym instrukcja TRV.
Sprawdzono ramki, odstępy, pełne pola logowania, tłumaczenia, brak akcji zapisu
karty po odrzuconej rejestracji i przejście do formularza bez ponownej rejestracji
ani zapisu konfiguracji. Scenariusz używa rzeczywistego UI, kolejki i aktora;
HTTP/GATT/pamięć trwała są modelowane. Nie wykonywano atlasu, pełnego builda,
powtórzenia niezmienionego backendu ani operacji na sprzęcie.
**Właściciel zaakceptował na podglądzie sześć widoków kodu `c2627bc`:**
wybór bramki z przewijaną nazwą i osobnym IP, pełne pola loginu/hasła,
instrukcję parowania TRV oraz komunikaty braku potwierdzonego odbioru,
częściowego błędu konfiguracji i potwierdzonego odbioru pomiarów.
Odpowiedzi: „Tak, te ekrany zaakceptowane na podglądzie” i
„Tak, te komunikaty zaakceptowane na podglądzie”. **W kolejnej odpowiedzi
właściciel zaakceptował także pozostałe sześć pokazanych stanów:** wyszukiwanie
bramki, pustą listę, trwającą rejestrację, złoty komunikat błędu
parowania/logowania, złoty komunikat braku obsługi oraz formularz
nazwy/pomieszczenia/kanału po „Zapisz kartę”. Dla obu rzędów odpowiedział:
„Tak, te widoki zaakceptowane na podglądzie”. Odbiór wyglądu pokazanego przepływu
bramki jest zakończony. Rzeczywiste RF i końcowy zapis karty na urządzeniu nadal
wymagają odbioru przed nowym DEV. Do zapisania akceptacji wykorzystano istniejące
rendery `c2627bc`, bez ponawiania testów firmware.

### SYS-01 — bateria i zasilanie: pierwsza runda podglądów

Właściciel wybrał ekran baterii, kalibrację i ustawienia. Przegląd obejmuje
napięcie i szacunkowy procent, przewijaną diagnostykę pamięci/radia, typ ogniwa,
progi, opcję animacji z trendu, korektę względem multimetru, klawiaturę oraz
błędy i ponowny odczyt profilu. Pomiar ADC, profil, estymator, trend i zapis
z odczytem kontrolnym zachowują dotychczasowe działanie.

Wykryto niespójności UI: komunikaty kalibracji nie miały złotych ramek,
panel błędu używał turkusowej ramki, a błąd zapisu był powtarzany pod panelem.
Brakowało też EN/DE dla „Wczytaj ponownie”. Komunikaty mają teraz osobny panel
z wewnętrznymi marginesami, pionowym wyśrodkowaniem tekstu i złotym tekstem/
konturem dla ostrzeżeń. Odstęp od treści i dolnych akcji wynosi co najmniej 8 px.
Panel jest ukrywany podczas edycji z klawiaturą; po błędzie pozostaje tylko
jeden widoczny komunikat, razem z opcjami odzyskania profilu. Dodano Reload /
Neu laden. Nie zmieniano reguł korekty, progów, pojemności ani ładowarki.

Istniejący test przełączał paletę bez zastosowania natywnego motywu LVGL.
Poprawiono go, aby przełączał motyw tak jak firmware. Wcześniejszy ciemny
selektor w podglądzie Jasny był skutkiem tego błędu testu; zbędną próbę zmiany
kolorów selektora w produkcji wycofano. Końcowe podglądy używają właściwych
motywów, w tym natywnej listy typu ogniwa.

Walidacja: `DSR_UI_FOCUS_BATTERY_REVIEW=1 python3 tools/dev/run_host.py
test_ui.py --jobs 4 --case test_battery_calibration_ui` — PASS, LVGL 9.3:
57 reprezentatywnych klatek (Grafit/Jasny DE, Cyber Blue PL/EN/DE).
Regresja najpierw wykazała brak złotych ramek; końcowa kontrola obejmuje ich
granice i marginesy, brak duplikatu błędu, odstęp od akcji, tłumaczenie,
klawiaturę i ponowne wczytanie po niepewnym zapisie. Po 40 otwarciach i
zamknięciach formularza wolna pamięć pozostaje 90 832 B; pełny Dashboard
24 kafli po powrocie ma 4 888 B wolne, największy blok 4 664 B w puli 128 KiB.
To pomiary hosta; ADC, worker i NVS mają dostawców modelowych. Nie wykonywano
pełnego atlasu, całego zestawu testów, nowego builda docelowego ani operacji
na pilocie. **Właściciel zaakceptował pokazany wygląd kodu `6a47dcc`:**
główny ekran baterii w trzech skórkach; pokazane elementy kalibracji (typ ogniwa,
progi, opcja trendu i korekta z multimetrem); złote komunikaty błędnych progów
i niepotwierdzonego zapisu, ponowne wczytanie oraz układ z klawiaturą.
Odpowiedzi: „Tak, ten ekran zaakceptowany na podglądzie”, „Tak, pokazane
elementy zaakceptowane na podglądzie” i „Tak, te stany zaakceptowane na
podglądzie”. Akceptacja dotyczy pokazanych widoków. Rzeczywisty pomiar vs
multimetr, trwałość profilu, wybudzanie/reconnect/restart i końcowa walidacja
nowego DEV pozostają osobnymi punktami. Zapis odbioru wymagał tylko kontroli
dokumentacji, bez ponawiania testów firmware ani renderów.


### SYS-01 — paski pamięci i stan BLE

Po akceptacji powyższych widoków właściciel zgłosił, że samo „136 KB”
nie wyjaśnia, czy pamięć jest wolna, czy zajęta. Sekcja „Pamięć i radio”
ma teraz w każdej skórce pięć poziomych pasków. RAM, PSRAM i grafika pokazują
**wolne / całość** w KiB; wypełnienie oznacza wolny zapas. Mianowniki
pochodzą z odpowiednich stert SDK i monitora puli LVGL. Stos pokazuje
**minimum wolnego od uruchomienia / rozmiar stosu** w bajtach; rozmiar jest
eksportowany z rzeczywistej rezerwacji zadania UI, nadal 8192 B. BLE pokazuje
wyłącznie stan modułu włączone/wyłączone, z paskiem binarnym. Legenda pod
wierszami wyjaśnia te znaczenia; minimum RAM i największy blok pozostają.
Nieznana lub niespójna pojemność ma kreskę i nieaktywny pusty pasek,
odróżniony od zmierzonego zera. Odświeżanie korzysta z istniejących obiektów.

Walidacja hosta: `test_system_info` i `test_boot_memory` — PASS; odczyty
pojemności mają te same maski możliwości co wolna pamięć, a mianownik stosu
odpowiada rzeczywistej rezerwacji przekazanej do FreeRTOS. `test_memory_ui`
— PASS, rzeczywisty LVGL 9.3, trzy skórki i PL/EN/DE; siedem podglądów
sekcji oraz stanów 0/100%, BLE i nieznanej pojemności. Sprawdzono granice,
brak zawijania wartości, dostępność całej karty nad dolnymi akcjami,
aktualizacje bez odtwarzania widżetów i zakres arytmetyki 32-bitowej.
Po rozgrzaniu ośmiokrotne otwarcie/zamknięcie zachowuje 92 176 B wolne
i tę samą liczbę alokacji. Dashboard 24 kart po powrocie: 6 208 B wolne,
największy blok 6 152 B w puli 128 KiB. To wyniki hosta z modelowymi
dostawcami SDK; nowego builda docelowego ani testów na pilocie nie wykonano.
Nie uruchamiano całego zestawu regresji ani pełnego atlasu.

Semantyka SDK jest sprawdzona w dokumentacji ESP-IDF 5.5.4 dla ESP32-S3:
[pojemność i wolna sterta](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/mem_alloc.html)
oraz [minimum wolnego stosu w bajtach](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/freertos_idf.html).
Walidacja dotyczy tej sekcji; wcześniejsza akceptacja baterii i kalibracji
pozostaje aktualna. **Właściciel zaakceptował sekcję na podglądzie `77c65d4`**
z jedną korektą: niemiecki stan Bluetooth ma brzmieć **Ein / Aus**, z wielkiej
litery. Napis poprawiono we wspólnym tłumaczeniu używanym wyłącznie przez
ten wiersz; polskie i angielskie brzmienie oraz działanie BLE pozostają.
Odbiór rzeczywistych pomiarów i działania na pilocie jest nadal otwarty.

### OTA-01 — natywne OTA: ekrany i komunikaty

Właściciel wybrał kolejny punkt ankiety: ekrany OTA. Pierwszy pakiet obejmuje
natywny aktualizator: kanał, dostępne wydanie, zgodę USB, sprawdzanie,
instalowanie, zakończenie/restart oraz błędy. Osobny ekran lokalnego OTA przez
panel WWW i fizyczne testy instalacji/rollback pozostają do przeglądu.

Rzeczywiste rendery potwierdziły sprzeczność opisu USB z istniejącym restartem
po 10 s, przycięty niemiecki przycisk potwierdzenia, brak ramek potwierdzeń
Cyber Blue, kolizję odliczania z ramką stanu i obcinanie szczegółów błędu.
Ważne komunikaty były białe zamiast złotych. Poprawiono wspólne karty trzech
skórek i ich pozycjonowanie po przeliczeniu rzeczywistych rozmiarów LVGL.
Komunikaty i szczegóły przewija teraz cała strona, bez drugiej przewijanej
ramki błędu. Tekst i obramowanie ostrzeżeń są złote, z ciemniejszym złotem
w Jasny. Potwierdzenie i informacje Cyber Blue mają podwójne sfazowane ramki.

Właściciel ocenił pakiet podglądów i zlecił poniższe korekty. Przycisk brzmi
teraz „Instaluj / Install / Installieren”. Złoty komunikat obok przycisku
wyjaśnia, że jego naciśnięcie potwierdza stabilne zasilanie USB. „Zmień kanał”
w Cyber Blue ma ikonę rozgałęzienia, również na ekranach błędów. Odliczanie
pokazuje 10…0, następnie przyjęcie żądania restartu. Zachowano opcję Później
i ponowienie po zajętym systemie; nie dodano automatycznej zgody na instalację.
Wyszukiwanie ma okrągły wskaźnik; instalacja pokazuje pasek, procent i bajty
w każdej skórce. Wstecz i Anuluj mieszczą się w dolnym wierszu; odliczanie ma
własne miejsce w karcie stanu. Zachowano jawny wybór kanału, jednorazową zgodę,
jej powiązanie z operacją i TTL, odmowę po błędnym podpisie oraz informację
o niepotwierdzonym wyborze startowym. Opis wydania pozostaje ograniczonym,
niezaufanym tekstem bez interpretacji znaczników.

**Uzgodniona blokada baterii:** aktualny, znany poziom poniżej 20% blokuje
natywną instalację i prosi o doładowanie; dokładnie 20% jest dopuszczone.
UI korzysta z potwierdzonej próbki młodszej niż 30 s, potrafi ponowić pomiar
istniejącą asynchroniczną kolejką i przygasza niedostępne „Instaluj”. Worker
przed wywołaniem instalatora wykonuje własny nowy odczyt ADC i profilu,
więc sprawdzenie nie opiera się wyłącznie na UI. Wyszukiwanie wydania nie
jest blokowane. To kontrola przed instalacją, bez ciągłego pomiaru podczas
zapisu. Procent nadal jest szacunkiem istniejącego profilu, nie nowym miernikiem
ładunku ani wykrywaniem obecności USB.

Właściciel wybrał dla nieznanego lub nieaktualnego procentu możliwość
instalacji po potwierdzeniu stabilnego USB. Błąd ADC, brak poprawnego profilu
lub brak wiarygodnego procentu korzysta z tej ścieżki. Nadal konieczna jest
fizyczna jednorazowa zgoda, powiązanie z operacją i kanałem oraz TTL 120 s.
Nowy błąd niskiej baterii dodano na końcu enum, zachowując dotychczasowe kody.
Nie zmieniono kluczy, podpisów, partycji ani reguł weryfikacji instalatora.

Walidacja tej korekty:

- `test_native_ota_actor` — PASS, powiązany zestaw trybów 0…7. Rzeczywisty aktor
  i estimator z modelami ADC/NVS/instalatora: 0% i 19% odmawiają bez wywołania
  instalatora; 20% przechodzi dalej. Brak odczytu/profilu korzysta z jawnej
  zgody USB. Sprawdzono brak instalacji bez zgody oraz anulowanie podczas
  nowego odczytu ADC. Po rozszerzeniu tej ostatniej asercji powtórzono tylko
  tryb baterii.
- `python3 tools/test_ui.py --case test_native_ota_ui` przez adapter Cygwin
  (ponowienia z zachowaniem niezmienionych obiektów) — PASS, 59 wybranych
  klatek LVGL 9.3: DE w trzech skórkach, PL/EN w Cyber Blue. Kontrola odstępów,
  ramek, pełnych podpisów i przejścia 19% → 19% → 20% → nieaktualny odczyt,
  zakończenia komunikatu oczekiwania także po niezmienionym wyniku, a także
  powiązana regresja zgody, TTL, anulowania, spóźnionego wyniku i nawigacji.
  Osobny świeży proces `DSR_UI_OTA_COUNTDOWN_REVIEW=1` — PASS, 11 klatek
  rzeczywistego odliczania 10…0 i dokładnie jedno modelowe żądanie restartu.
- `tools/dev/Build-Firmware.ps1 -Jobs 4` — PASS, ESP-IDF 5.5.4, ESP32-S3;
  po końcowych korektach tekstu i wyniku pomiaru przyrostowe `ninja -C build-ai
  -j 4` — PASS, obraz 0x424e10 B, 31% wolnej partycji aplikacji.
- `check_boot_memory.py` i `check_native_ota_target.py` na tym ELF — PASS,
  zachowane limity statycznej pamięci i własnych ramek stosu OTA. Audyt nie
  mierzy przechodniego szczytu stosu ani sterty SDK.

Podstawą integracji są istniejące odczyty oraz wersjonowane dokumenty
[ADC oneshot ESP-IDF 5.5.4](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/adc_oneshot.html)
i [OTA ESP-IDF 5.5.4](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/ota.html).
Obejrzano rzeczywiste piksele. Modele hosta i build nie potwierdzają fizycznej
instalacji, zachowania danych, działania progu na ogniwie ani rollback.
Bez pełnego atlasu, całej regresji, CI, flashowania i publikacji. **Właściciel
zaakceptował na podglądzie** krótkie „Instaluj / Install / Installieren”, złoty
komunikat blokady poniżej 20%, informację przy nieznanym procencie, ikonę
„Kanal ändern” w Cyber Blue oraz odliczanie 10…0 na GIF-ie (`b9870ae`).
To akceptacja wyglądu; działanie progu na ogniwie/USB, rzeczywista instalacja,
zachowanie danych i rollback pozostają do odbioru na sprzęcie. Lokalne OTA
przez WWW pozostaje osobnym punktem. Numer 0.7.71 w renderach jest przykładowy.

### OTA-01 — lokalny upload BIN przez WWW: wygląd zaakceptowany

Właściciel wybrał przegląd lokalnego OTA przez WWW. Ten istniejący tryb
deweloperski przyjmuje BIN aplikacji po fizycznej zgodzie na LCD (jednorazowej,
maksymalnie 120 s), parowaniu przeglądarki i dotychczasowych kontrolach HTTP.
Sprawdza format/znacznik płytki i SHA-256; nie weryfikuje podpisu wydawcy.
Backend uploadu, partycje, klucze i reguły dostępu nie zostały zmienione.

Na LCD trzy informacje mają własne ramki wyliczane z wysokości tekstu.
Cyber Blue używa podwójnych sfazowanych obramowań. Odmowa, wygaśnięcie oraz
informacja o trybie deweloperskim są złote, z ciemniejszym złotem w Jasny.
Instrukcja prowadzi do System i OTA w panelu oraz przypomina o stabilnym USB.
Pełne ramki mieszczą się nad Wstecz w PL/EN/DE. Zmiana ekranu usuwa referencje
etykiety i ikony; późniejszy tick wygaśnięcia nie dotyka usuniętych obiektów.
LCD pokazuje przyznanie zgody i lokalne wygaśnięcie, nie postęp zapisu Flash.

Panel blokuje Instaluj bez pliku, odrzuca przed wysłaniem plik inny niż BIN
lub poza zakresem 512 B…6 MiB i blokuje powtórne wysłanie podczas operacji.
Pokazuje obliczanie sumy, następnie nieokreślony wskaźnik wysyłania/weryfikacji
— bez wymyślonego procentu Flash. Nazwa i rozmiar wysłanego pliku oraz wynik
przetrwają nawigację. Spóźniony wynik nie nadpisuje innego ekranu. HTTP 400/403
pozwala na ręczne ponowienie po usunięciu przyczyny i odnowieniu zgody;
niepotwierdzony wybór startowy pokazuje złote ostrzeżenie i jawny restart.
Utrata odpowiedzi lub nieokreślony błąd 5xx nie oznacza sukcesu ani pozostawienia slotu bez zmian:
panel prosi o sprawdzenie pilota i blokuje ponowienie w tej sesji widoku.
Restart wysyłany jest wyłącznie po kliknięciu, bez automatycznych ponowień.

Na stronie OTA stopka zapisu konfiguracji jest ukryta, gdy nie ma niezapisanych
zmian. Jeśli istnieje draft konfiguracji, stopka pozostaje dostępna; znika po
potwierdzonym zapisie. Nie zapisujemy go automatycznie z powodu operacji OTA.
Ważne komunikaty panelu mają złote ramki, również na szerokości 320 px.

Walidacja: `test_web_ota_ui` w rzeczywistym przypiętym LVGL 9.3 — PASS,
PL/EN/DE w trzech skórkach, 15 wybranych klatek zgody, odmowy i wygaśnięcia.
Kontrola koloru, wewnętrznych marginesów i pełnych ramek nad dolną akcją.
`node tests/test_panel_ota.cjs` — PASS: rzeczywiste zasoby w Chromium,
próg rozmiaru i wyliczony SHA w żądaniu, brak pliku/duplikatu, odmowa,
odrzucony obraz, wysyłanie, potwierdzony wynik, niepewny wybór startowy,
utrata odpowiedzi, powrót na stronę, mobilny układ i draft/zapis konfiguracji.
`test_web_locale_dictionary.cjs` i istniejący `test_panel_locale.cjs` — PASS.
Nowe przypadki dodano do normalnych runnerów/CI; CI nie było uruchamiane.
Obejrzano piksele, w tym poprawkę stopki znalezioną podczas oględzin mimo
wcześniejszego zielonego testu. Modele obejmują HTTP/uprawnienia/Flash/restart;
nie wykonano uploadu na pilocie, nowego builda firmware ani pełnego atlasu.

**Niespójność stwierdzona w tej rundzie:** próg 20% i nowy odczyt ADC przed
instalacją dotyczyły tylko natywnego OTA. Integrację uploadu WWW opisano
w kolejnym etapie poniżej; tekst o USB nie zastępuje kontroli backendu. **Właściciel
zaakceptował na podglądzie** oba pakiety (`2fd1427`): ekrany LCD z ramkami
trzech skórek, instrukcją BIN/USB i złotymi komunikatami odmowy/wygaśnięcia
oraz panel WWW z wyborem pliku, wskaźnikiem wysyłania/weryfikacji, nazwą pliku,
wynikiem, złotymi błędami i ręcznym restartem. Ta akceptacja dotyczy wyglądu.
Akceptację nowego kroku potwierdzenia USB i komunikatów progu baterii opisano
poniżej. Rzeczywista instalacja, dane i rollback pozostają otwarte.

### OTA-01 — próg baterii 20% dla uploadu WWW

Właściciel wybrał wdrożenie kontroli baterii dla lokalnego OTA. Zastosowano
uzgodnioną politykę natywnego updatera: świeży wynik poniżej 20% odmawia,
dokładnie 20% pozwala kontynuować, a nieznany procent wymaga jawnego
potwierdzenia stabilnego USB na LCD. Wejście na stronę nie udziela już zgody;
przycisk „Zezwól na OTA” potwierdza USB i otwiera dotychczasowe jednorazowe
okno 120 s. Backend odrzuca brak potwierdzenia. Nie dodano wykrywania USB
ani ładowania z trendu napięcia.

Po sprawdzeniu nagłówka obrazu, bezpośrednio przed `esp_ota_begin`, odbiornik
rezerwuje istniejące miejsce w kolejce pracownika i prosi go o nowy odczyt
ADC oraz profilu kalibracji. Natywny OTA korzysta z tej samej funkcji polityki.
Odczyt nie używa cache LCD i nie odbywa się w zadaniu HTTP. Zajęty pracownik
lub brak wyniku w 1000 ms blokuje upload; nie jest traktowany jako nieznany
procent. Spóźniony wynik jest odrzucany, a następna próba wymaga nowego
odczytu. Miejsce pracownika pozostaje zajęte do zakończenia zapisu/abort
i zwolnienia pamięci odbiornika. Nie dodano zadania RTOS ani pól do job/event;
priorytet STOP pozostaje bez zmian.

Za niski poziom zwracany jest HTTP 422, a niedostępna kontrola HTTP 409.
Oba wyniki występują przed pierwszym wywołaniem Flash i zużywają jednorazową
zgodę. Panel pokazuje złoty komunikat PL/EN/DE oraz wymaga odnowienia zgody
przed ręczną próbą. Parowanie, kontrola pochodzenia, weryfikacja obrazu,
wyłączność obu trybów OTA, podpisy i partycje zachowują dotychczasowe zasady.
Lokalny upload nadal jest deweloperskim trybem BIN z kontrolą SHA-256,
bez weryfikacji podpisu wydawcy.

Walidacja ograniczona do zmienionych modułów:

- `python3 tools/test_ota_power.py` — PASS: rzeczywisty kod przekazania
  do pracownika i lease, z modelami RTOS/ADC; zajętość, restart, brak pamięci,
  timeout przed/w trakcie/po odczycie, odrzucenie późnego sygnału i świeża
  kolejna próba. Bariery wątków odtwarzają wyścigi bez losowych opóźnień.
- `python3 tools/test_ota_upload.py` — PASS dla obu wyników wyboru startowego:
  rzeczywisty odbiornik, SHA i wyodrębniony endpoint; LOW/BUSY/brak USB
  nie uruchamiają Flash. Zachowano wcześniejsze testy błędów odczytu,
  zapisu, kończenia/abort i alokacji. Regresja kolejności sprzątania najpierw
  odtworzyła przedwczesne zwolnienie miejsca, a po poprawce przeszła.
  Dotychczasowe hashe kontroli dostępu pozostają bez zmian; normalizacja
  CRLF i istniejącej autoryzowanej trasy statusu automatyzacji dotyczy
  porównania źródeł, nie zastępuje testu autoryzacji w działającym serwerze.
- Selektywnie uruchomione `battery`, `actions_worker`, `native_ota_actor`
  (tryby 0…7) i `panel_permission` z `tools/test.py` — PASS. Rzeczywisty
  pracownik wykonuje świeży odczyt; sprawdzono granicę 0/19/20%, brak
  profilu/ADC, anulowanie, pilny STOP i zgodę USB. Radio/SDK są modelowane.
- `python3 tools/dev/run_host.py test_ui.py --cases test_web_ota_ui
  test_native_ota_ui --jobs 4` — PASS na przypiętym LVGL 9.3; nowe
  potwierdzenie USB nie wywołuje backendu przed kliknięciem. Pozostałe stany
  zgody/odmowy/wygaśnięcia w trzech skórkach i PL/EN/DE oraz powiązana
  regresja natywnego OTA przeszły. Obejrzano rzeczywiste piksele.
- `node tests/test_panel_ota.cjs` i `node tests/test_web_locale_dictionary.cjs`
  — PASS: rzeczywiste zasoby w Chromium, złote wyniki LOW/BUSY we wszystkich
  językach, dotychczasowe stany uploadu i kompletność tłumaczeń. HTTP,
  Flash i restart w tych testach są modelowane.
- `tools/dev/Build-Firmware.ps1 -Jobs 4`, następnie przyrostowe
  `ninja -C build-ai -j4` po korekcie kolejności sprzątania — PASS,
  ESP-IDF 5.5.4 / ESP32-S3. Końcowy obraz 0x427150 B; 31% wolnej partycji.
  `check_boot_memory.py` i `check_native_ota_target.py` — PASS na tym ELF:
  wewnętrzne BSS 60 912 B, stos UI 8192 B, pula LVGL w PSRAM 131 072 B.
  Nowy stan przekazania zajmuje 20 B; własne statyczne ramki funkcji mają
  32 B, a wspólny odczyt/polityka 80 B. Nie mierzono szczytu stosu/sterty SDK.

Runner odbiornika ujawnił lokalny problem Cygwin: linkerowe `--wrap` alokacji
przy OpenSSL kończyło proces przed `main`, czasem z kodem 0. Potwierdzono
to minimalnym programem i śladem startu. Tylko dla Cygwin te same punkty
alokacji obu jednostek produkcyjnych są teraz podmieniane podczas kompilacji
źródeł testu. Wstrzykiwanie błędów pozostaje; Linux zachowuje `--wrap`
i dotychczasowe sanitizery. Runner wymaga końcowego raportu wykonanych
asercji, nie tylko kodu wyjścia.

Jeden niezależny przegląd nie znalazł istotnych błędów; drobną uwagę dotyczącą
kolejności sprzątania poprawiono i ponownie oceniono. **Właściciel zaakceptował
na podglądzie** nowy krok potwierdzenia stabilnego USB na LCD oraz złote
komunikaty niskiej baterii i niedostępnego pomiaru: „Tak, te widoki zaakceptowane
na podglądzie” (kod `a415e17`). Akceptacja dotyczy pokazanych widoków;
do jej zapisania wykorzystano istniejące rendery, bez ponawiania testów firmware.
Build i host nie potwierdzają dokładności SOC, fizycznego USB, instalacji,
zachowania danych ani rollback. Kontrola odbywa się przed startem Flash,
bez ciągłego monitorowania baterii podczas zapisu. Odbiór sprzętowy pozostaje
otwarty. Bez pełnego atlasu, pełnej regresji/CI, uploadu, flashowania,
push ani publikacji; wersja nadal `0.7.70-dev`.

### WEB-01 — konfiguracja, zgody i podgląd LCD — 2026-10-24

Właściciel wybrał przegląd panelu WWW po zaakceptowaniu progu baterii OTA.
Ta runda obejmuje parowanie przeglądarki, ustawienia wyglądu/języka, konflikt
zapisu konfiguracji, podgląd LCD na żądanie oraz ekrany bezpieczeństwa,
kodu i unieważnienia parowania na pilocie. Nie jest odbiorem wszystkich
podstron WWW ani rzeczywistego HTTPS/NVS na urządzeniu.

Odtworzono błąd podglądu: HTTP 202 po wcześniejszym sukcesie pozostawiał stary
obraz i podpis „Obraz pobrany”. Rozpoczęcie nowej próby czyści teraz poprzednią
klatkę. Panel rozróżnia pobieranie, oczekiwanie na nową klatkę, nieprawidłowy
rozmiar obrazu i błąd połączenia; odmowy mają złote ramki PL/EN/DE.
Odświeżanie pozostaje ręczne i pojedyncze. Spóźniony wynik po zmianie widoku,
konfiguracji albo ukryciu karty nie maluje obrazu. Ważne komunikaty konfiguracji
i parowania, w tym konflikt zapisu, również otrzymały złote obramowanie.
Nie zmieniono reguł zapisu ani istniejącego jawnego wczytania konfiguracji.

Na LCD informacje bezpieczeństwa, kod i ostrzeżenia korzystają z istniejących
paneli, z podwójnymi sfazowanymi ramkami Cyber Blue. Bloki mają odstępy
wyliczane z wysokości przetłumaczonego tekstu; przegląd pikseli wykrył i usunął
kolizję niemieckiego podpisu certyfikatu z odciskiem. Stan dostępu jest w karcie
HTTPS/adresów. Potwierdzenie i anulowanie unieważnienia pozostają w stałym
wierszu, 16 px poniżej przewijanej treści i ponad Wstecz. Kod po 120 s znika,
a komunikat wygaśnięcia jest złoty. Zachowano blokadę powtórnego unieważnienia,
obsługę spóźnionego wyniku i powrót po zakończeniu operacji.

Walidacja ograniczona do tego zakresu:

- `node tests/test_panel_settings.cjs` — PASS w Chromium dla PL/EN/DE:
  parowanie/utrata dostępu, trzy motywy, draft przed zapisem i dokładne
  potwierdzenie, osobny język panelu/pilota, zachowany draft i nieaktywne
  Zapisz przy konflikcie, jawne wczytanie oraz stany podglądu 200/202,
  niepełny obraz i błąd transportu. Sprawdzono złote ramki i szerokość 390 px.
- `node tests/test_panel_config_reads.cjs` — PASS: dotychczasowe wyścigi
  odczyt/zapis/BIND/nawigacja i pojedyncze odświeżanie. Model testowy
  uzupełniono o istniejący kontekst automatyzacji i metody canvas/klas CSS;
  wcześniejszych wymagań ochrony draftu nie osłabiono.
- `python3 tools/dev/run_host.py test_ui.py --case test_web_panel_ui --jobs 4`
  — nowa regresja najpierw wykazała brak ramek. Po poprawkach powtarzano tylko
  ten przypadek oraz powiązany `test_web_ota_ui`, z zachowaniem obiektów
  niezmienionych od zakończonej kompilacji źródeł i kontrolą nagłówków.
  Końcowo oba PASS na przypiętym LVGL 9.3. Trzy skórki i PL/EN/DE,
  granice etykiet, odstępy paneli, stałe akcje, wygaśnięcie, zajętość i błąd;
  również istniejąca regresja unieważnienia i 40 cykli bez ubytku wolnej pamięci.
- `node tests/test_web_locale_dictionary.cjs` — PASS. Nowe przypadki dodano
  do istniejących runnerów/CI. Jeden niezależny przegląd nie znalazł istotnych
  błędów w zmianach. Obejrzano rzeczywiste piksele Chromium i LVGL.

HTTP, sesja, zapis i worker w tych testach są modelowane. Klatka w przeglądarce
pochodzi z zaakceptowanego renderu hosta Dashboardu, a nie z fizycznego pilota.
Nie zmieniono backendu autoryzacji, protokołu parowania, ochrony poufnych
ekranów ani API sterowania. Nie ponawiano pełnego atlasu, regresji, builda
docelowego ani CI; nie wykonano operacji na sprzęcie, push ani publikacji.
**Właściciel zaakceptował wszystkie trzy pokazane zestawy widoków** z kodu
`3cdd2f7`:

- konfigurację WWW na komputerze i telefonie, osobny język panelu i pilota
  oraz złoty konflikt zapisu z nieaktywnym Zapisz;
- parowanie przeglądarki oraz komunikaty oczekiwania i błędu podglądu LCD,
  z ręcznym odświeżaniem;
- ekrany LCD bezpieczeństwa/certyfikatu, kodu oraz unieważnienia parowania,
  ich ramki i stałe przyciski.

Odpowiedzi: „Tak, pokazana konfiguracja WWW zaakceptowana”, „Tak, te widoki WWW
zaakceptowane” i „Tak, pokazane ekrany LCD zaakceptowane”. Akceptacja dotyczy
wyglądu pokazanych widoków. Do jej zapisania wykorzystano istniejące rendery,
bez ponawiania testów firmware. Odbiór fizycznego HTTPS, trwałego unieważnienia,
synchronizacji i podglądu pozostaje otwarty.

### SYS-02 — uzgodniony zakres System — 2026-10-24

Właściciel wybrał ustalenie zakresu Aktywności, backupu i resetu oraz
potwierdził zakres i docelowy egzemplarz odtwarzania:

1. **Aktywność:** zachować istniejące 128 wpisów w RAM od uruchomienia pilota.
   Dodać brakujący widok LCD korzystający z tego samego backendu co WWW;
   historia nie ma być utrwalana przez restart. Bez drugiego silnika logów.
2. **Backup/przywracanie przez WWW:** pełna kopia szyfrowana hasłem,
   obejmująca konfigurację, zdjęcia i zapisane połączenia. Eksport samego
   publicznego JSON nie spełnia tej decyzji. Hasła i powiązania nie mogą
   trafić do jawnego pliku kopii, logów ani raportów.
3. **Reset danych pilota:** pełny reset po potwierdzeniu na LCD, obejmujący
   również Wi-Fi, połączenia Shelly, powiązania BLE oraz dostęp WWW.
   Firmware i weryfikacja OTA pozostają. To decyzja o działaniu przyszłej
   funkcji; nie jest poleceniem wykonania resetu na urządzeniu podczas prac.
4. **Odtwarzanie:** wyłącznie na tym samym fizycznym pilocie, również po
   pełnym resecie. Bez migracji na inny egzemplarz.

Rozpoznanie potwierdziło osobne magazyny publicznej konfiguracji/zdjęć,
ustawień Wi-Fi, prywatnych połączeń oraz powiązań NimBLE. Dla pełnej kopii
trzeba uwzględnić obie strony rekordów zabezpieczeń BLE i lokalną tożsamość,
a nie tylko adres/nazwę urządzenia. Wi-Fi projektu działa z pamięcią RAM
sterownika i zapisuje ustawienia przez `dsr_settings`, co należy zachować.

Projekt spójnego zapisu/odtwarzania, formatu szyfrowania, resetu i LCD
Aktywności znajduje się w
[projekcie SYS-02](superpowers/specs/2026-10-24-system-data-design.md).
Właściciel zatwierdził projekt w rozmowie („Tak, projekt zatwierdzony”).
Właściciel zatwierdził wdrożenie obu planów tutaj etapami: [Aktywność LCD](superpowers/plans/2026-10-24-system-activity.md)
oraz [backup, odtwarzanie i reset](superpowers/plans/2026-10-24-system-backup-reset.md).
Niezależny przegląd projektu zakończono po doprecyzowaniu przywracania zgód
automatyzacji bez historycznych nastaw. Sprawdzono tylko dokumentację;
Aktywność LCD wdrożono lokalnie w `f44c5b8`: System → Aktywność, strony po 6
z istniejących 128 wpisów RAM, źródło/czas/ID/wynik i potwierdzane czyszczenie.
Atomowa kontrola epoki chroni nowe wpisy przed starym dialogiem LCD po czyszczeniu
z WWW. Właściciel zaakceptował wygląd w trzech skórkach w ankiecie.
Walidacja: `python3 tools/dev/run_host.py test_ui.py --case test_activity_ui --jobs 4`
— PASS, 650 sprawdzeń produkcyjnego LVGL i rzeczywistego logu RAM; modele radia,
storage i czasu. Obejrzano wybrane kadry; poprawiono brakujące tłumaczenia wyników
PL/EN/DE po wykryciu ich w renderze. Bez pełnego atlasu i builda targetu.
Backup/odtwarzanie/reset są wdrożone; końcowe bramki wydania i odbiór sprzętowy opisuje bieżący etap na początku dokumentu.
Na etapie ustalania zakresu nie uruchamiano testów firmware, nie zmieniano
kodu, nie wykonywano resetu ani operacji na urządzeniu.

## Baza dalszej pracy

- Repozytorium: `voltex-at/DIY-Shelly-Remote`, lokalnie `C:\Users\franz\DIY-Shelly-Remote`.
- Aktualna gałąź: `work/dev-0.7.70`, źródło wydania `d43da439f757abaffe7363a8622e90b175882b29`.
- [Opublikowany DEV 0.7.70](https://github.com/voltex-at/DIY-Shelly-Remote/releases/tag/dev-v0.7.70), pięć publicznych zasobów.
- Zachowane dowody bieżącego wydania: [CI 5/5](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/39782898484), [Signed release](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/39782898458).
- `main` nadal jest starszą bazą `51c5840`; nie jest scalany ani usuwany podczas porządków. Starsze opublikowane tagi i zasoby OTA pozostają niezmienne.

## Wynik porządków

- Przejrzano wszystkie 62 zdalne gałęzie; usunięto 60. Zostały `main` i `work/dev-0.7.70`. Usunięto również dwie nieaktywne gałęzie lokalne.
- Utworzono [issue #74](https://github.com/voltex-at/DIY-Shelly-Remote/issues/74), zamknięto 17 starych issues i 7 PR-ów bez scalania. Zamknięcie oznacza konsolidację, nie odbiór wszystkich funkcji.
- Usunięto 379 historycznych przebiegów Actions. Lista artefaktów spadła z 1951 do zera; ich początkowy rozmiar według metadanych wynosił 15 526 146 417 B. Pozostały dwa udane przebiegi bieżącego wydania.
- Lokalnie usunięto 13 723 pliki starych wyników, buildów i zbędnych cache: 2 958 286 173 B. W tym 192 wersjonowane obrazy/media oraz nieaktualne raporty i żądania wydania.
- Zarchiwizowano osiem rozmów projektu ChatGPT „DIY Shelly Remote”. Widok projektu potwierdza brak aktywnych rozmów; bieżący czat Codex pozostaje.
- Zweryfikowano zachowanie 28 publicznych wydań, 130 plików wydania i zgodność 29 lokalnych/zdalnych referencji tagów. Kod firmware, testy źródłowe, generatory, workflowy, aktywne toolchainy i materiały właściciela pozostają.

## O co proszono i co rzeczywiście jest w kodzie

„W kodzie” nie oznacza odbioru na fizycznym ES3C35P. Lista łączy #6–#66, ustalenia z rozmów projektu, konfigurację środowiska, kontynuację Claude 0.7.68 oraz poprawki 0.7.69–70.

| Zlecenie | Wykonanie / ograniczenie | Główne źródło |
| --- | --- | --- |
| Środowisko Windows, ESP-IDF i cztery skills | Zainstalowane i opisane; Espressif MCP oraz lokalny lvgl-project dostępne. Zachować toolchain i tools/dev. | `docs/ESP32_AI_SETUP_PL.md; .agents/skills; tools/dev` |
| Piny sprzętu i bibliotek | ES3C35P N16R8, ESP32-S3, 320×480; ESP-IDF 5.5.4 / LVGL 9.3.0 / ST77922 2.1.1 / mDNS 1.8.2. | `CMakeLists.txt; components/dsr_board/idf_component.yml; components/dsr_platform/idf_component.yml` |
| Trzy skórki | Cyber Blue/Grafit/Jasny; Midnight wycofany z selektora, migracja starych ustawień zachowana. | `components/dsr_ui/dsr_ui_style.c; components/dsr_config/dsr_config.c` |
| Status, bateria, zegar, odstępy i nawigacja | Poprawki 0.7.68–69: procent wewnątrz baterii, dom/grid/CPU, przewijanie ustawień, długie nazwy. Odbiór fizyczny aktualnego UI nadal otwarty. | `components/dsr_ui/dsr_ui_style.c; components/dsr_ui/dsr_ui.c` |
| Pulpit i karty zgodne z ostatnimi zdjęciami | 0.7.70: jeden wiersz wyboru/akcji, zwarte karty, pomieszczenie•stan, pełne pomiary, termostat/okno i duży włącznik. Późniejsze zlecenie zastępuje część układu 0.7.69. | `components/dsr_ui/dsr_ui_mobile.c; docs/RELEASE_NOTES.md` |
| Pomieszczenia i edycja | Dodaj/Edytuj/Usuń, stabilne ID, trwały zapis; 0.7.70 poprawia kafle i liczniki. | `components/dsr_ui/dsr_ui_config.c; components/dsr_ui/dsr_ui_mobile.c` |
| Klawiatura i dotyk | Większe znaki, wspólny keyboard, podgląd klawisza, obsługa przerwanego dotyku i przewijania. | `components/dsr_ui/dsr_ui.c; components/dsr_board` |
| Wiele Shelly i usuwanie kanałów | Rejestr/scheduler, oddzielny stan/błędy, priorytety i backoff; usuwanie właściwego ID/kanału wraz z odwołaniami. | `components/dsr_platform/dsr_fleet.c; components/dsr_ui/dsr_ui_network.c` |
| LAN, nazwa zamiast SKU i profile | Kreator LAN, weryfikacja tożsamości, lokalna nazwa ma pierwszeństwo, komponenty/profile i kanały. Gen1 wycofany z zakresu. | `components/dsr_platform/dsr_discovery_model.c; components/dsr_ui/dsr_ui_lan.c` |
| Wi-Fi AP | Istnieje skan/wybór, dwie zgody, provisioning i przywrócenie LAN; retry nie odtwarza POST hasła. Ramki treści Cyber Blue dodane 0.7.69. | `components/dsr_platform/dsr_ap.c; components/dsr_ui/dsr_ui_ap.c` |
| BLE i BLU | Skan ESP-NimBLE, otwarty setup Wi-Fi, osobne bonding/PIN/GATT/bramka dla wspieranych klasycznych BLU. RF/SMP na aktualnej wersji wymaga odbioru. | `components/dsr_platform/dsr_ble.c; components/dsr_platform/dsr_ble_rpc.c; components/dsr_platform/dsr_blu.c` |
| Rolety i STOP | Cover RPC, pozycja/gest, potwierdzony odczyt, priorytet STOP; wspólna animacja trzech skórek, 0%=zamknięta/100%=otwarta. | `components/dsr_ui/dsr_ui_mobile_cover_cyber.inc; components/dsr_ui/dsr_ui_cyber.c; components/dsr_platform/dsr_fleet.c` |
| Światło/dimmer/RGB/H&T/TRV | Prawdziwe akcje oraz capability; H&T tylko odczyt, TRV przez bramkę. 0.7.70 przebudowuje Cyber dimmer/RGB, pozostałe skórki zachowują swoje widoki. | `components/dsr_ui/dsr_ui_mobile.c; components/dsr_ui/dsr_ui_mobile_lighting_cyber.inc` |
| Ustawienia urządzenia i Ogólne | Trwała edycja karty oddzielona od świadomych zmian konfiguracji Shelly Gen2/3/4; formularze zależne od rzeczywistych możliwości. | `components/dsr_platform/dsr_device_settings.c; components/dsr_ui/dsr_ui_device_settings.c` |
| Grupy i sceny | Ręczne sceny Switch/Cover oraz grupy Switch, wyniki częściowe, edytory LCD/WWW. Lokalna reguła kontakt BLU → ogrzewanie jest wdrożona w bieżącej gałęzi; ruch, harmonogramy i chmura pozostają poza tym zakresem. | `components/dsr_platform/dsr_actions.c; components/dsr_ui/dsr_ui_actions.c` |
| Aktywność | Backend 128 rekordów RAM, API i widok LCD są wdrożone. Pokazany widok LCD zaakceptowany; historia obejmuje bieżące uruchomienie. | `components/dsr_platform/include/dsr_actions.h; components/dsr_ui/dsr_ui_activity.c` |
| Historia energii i wykresy | Były wdrożone, następnie usunięte decyzją właściciela 0.7.51. Nie przywracać automatycznie na podstawie starych #22/#29. Bieżące pomiary i kontrolki pozostają. | `docs/FEATURE_MATRIX.md; docs/RELEASE_NOTES.md` |
| Panel WWW i języki | Wspólna konfiguracja, pairing/zgody i osobne PL/EN/DE przeglądarki; lokalny podgląd LCD. | `components/dsr_platform/dsr_panel.c; components/dsr_platform/web` |
| Bateria | Kalibracja ogniwa/pojemności/progów/multimetru; ~SOC i trend ≥10 s / 20 mV jako przypuszczalne ładowanie. To nie sygnał fizycznej ładowarki. | `components/dsr_platform/dsr_battery.c; components/dsr_ui/dsr_ui_battery_calibration.c` |
| Wydajność i pamięć | Zachowano poprawki Claude: mniej parsowań/kopii, odłożone renderowanie przy wygaszonym LCD, budżety PSRAM/BSS/stosu i cleanup animacji. | `components/dsr_ui/dsr_ui_network.c; components/dsr_ui/dsr_ui_mobile.c` |
| Wi-Fi/BLE cleanup i restart | Reconnect/backoff oraz bounded BLE cleanup. 0.7.69 dodaje potwierdzony lokalny restart niezależny od aktywnych lease, z RTC watchdog; normalne WWW/OTA zachowują wyłączność. | `components/dsr_platform/dsr_restart.c; components/dsr_platform/dsr_activity.c` |
| OTA z LCD i WWW | Natywny updater, manifest Ed25519, TLS/model/wersja/kanał/hash, dwa sloty i rollback. Instalacja 0.7.53 potwierdzona historycznie; pełny aktualny rollback wymaga sprzętu. | `components/dsr_platform/dsr_native_ota.c; components/dsr_ota; components/dsr_crypto` |
| Bezpieczne wydania DEV | 0.7.68/69/70 opublikowane; tag==commit z CI 5/5, klucz w chronionym środowisku, jeden build/publikacja, bez Stable i bez merge main. | `tools/release_tool.py; .github/workflows/release.yml` |
| Eksport/backup i reset fabryczny | SYS-02 wdrożone: szyfrowana kopia WWW, odtwarzanie na tym samym pilocie i reset po zgodzie LCD; odbiór sprzętowy osobno. | `components/dsr_platform/dsr_maintenance_recovery.c; components/dsr_ui/dsr_ui_maintenance.c` |
| Odbiór urządzenia | Starsze potwierdzenia dotyczą konkretnych wersji. Zielone host/CI nie akceptuje najnowszego UI, RF/AP, ADC, STOP, reboot ani rollback. | `README.md; dotychczasowe #6/#12/#18/#24/#25` |

## Niespójności i rzeczy do dalszej poprawy

- [ ] **P0: odbiór UI 0.7.70 na urządzeniu**, we wszystkich trzech skórkach: długie nazwy, jedna linia narzędzi, ostatni wiersz ustawień, status/bateria, room edit, przyciski, okno/TRV i dimmer/RGB. Nie zamykać jako gotowe tylko na podstawie liczby asercji.
- [ ] **P0: RF/AP/BLE i restart**: rzeczywisty scan/setup, anulowanie/błędy i powrót do LAN; SMP/PIN/bramka; brak callbacków i restart przy zajętym radiu. Czas RTC watchdoga 0.7.69 nadal nie jest pomiarem urządzenia.
- [ ] **P0: trwałość i sterowanie**: wiele Shelly, usunięcie jednego kanału bez odtworzenia po reboot/WWW, potwierdzony readback, STOP/pozycja, Light/RGB/TRV i sceny na rzeczywistych urządzeniach.
- [ ] **P0: OTA i bateria**: aktualizacja do bieżącego DEV bez utraty konfiguracji/zdjęć, przerwany transfer i rzeczywisty rollback; BAT vs multimetr, ~SOC i wygaszenie animacji ładowania.
- [x] **Dokumentacja bieżąca — poprawiona podczas porządków**: matryca funkcji, przewodnik rozwoju i indeks opisują 0.7.70, aktualny restart i wykonany zakres BLU. Wcześniej mieszały 0.7.67/69 oraz nieaktualne „planned”.
- [ ] **Historyczne wymagania UI są sprzeczne**: cztery vs trzy skórki; grid/dom; osobny wiersz akcji 0.7.69 vs wspólny 0.7.70; dwa wiersze metadanych vs bieżące „Pokój•Stan”. Obowiązują ostatnie zdjęcia/zlecenia, starszych żądań nie realizować ponownie.
- [ ] **Instrukcje lokalne**: AGENTS i skills nadal używają nazw „Shelly HUD — Cyber Blue/Grafitowy”, choć UI używa skróconych nazw. Lokalne `.agents/`, `tools/dev/` i raport setup nie są wersjonowane; ustalić świadomie ich dalsze utrzymanie.
- [x] **Aktywność**: istniejący backend 128 wpisów i API wykorzystany w zaakceptowanym widoku LCD.
- [ ] **Eksport/reset**: pierwotne pełne System #24/#25 nie jest w całości dostarczone; zaplanować osobny przepływ z potwierdzeniem i analizą trwałości danych.
- [ ] **Czas i zabezpieczenie danych**: SNTP istnieje, brak potwierdzonego ekranu wyboru strefy; NVS prototypu nie jest szyfrowane. Decyzje/migracje wymagają oddzielnego etapu, bez zmiany klucza OTA i eFuse przy porządkowaniu.
- [ ] **Stare tłumaczenia**: słowniki zawierają historyczne napisy o „niewdrożonym” AP/BLE/OTA/restart. To kandydaci do audytu użycia, nie dowód, że aktualny ekran pokazuje te komunikaty. Usuń dopiero po sprawdzeniu odwołań.
- [ ] **Gałąź domyślna GitHub**: default `main` pokazuje starą bazę; bieżący kod jest na `work/dev-0.7.70`. Zmiana default/merge to odrębna decyzja, porządki jej nie zastępują.

Wycofane lub poza bieżącym zakresem: historia energii/wykresy (#22 i część #29), Midnight, Gen1; chmurowe automatyzacje, nowe BLU/Zigbee/Matter i automatyczne reguły nie są potwierdzoną pełną implementacją. Nie przywracać ich automatycznie.

## Ankieta odbioru — dalszy etap

Na prośbę właściciela z 2026-10-22 lista staje się ankietą i kolejką bieżących poprawek. Najpierw ustalamy wersję faktycznie zainstalowaną na pilocie. Każdy punkt otrzymuje ocenę właściciela: **działa / wymaga poprawy / brakuje / niesprawdzone**. Obecność implementacji w tabeli powyżej nie wypełnia automatycznie tej oceny.

| ID | Co oceniamy | Ocena właściciela / problem |
| --- | --- | --- |
| UI-01 | Trzy skórki i ich nazwy: Cyber Blue, Grafit, Jasny | Oczekuje na ocenę |
| UI-02 | Status bar: bateria z procentem wewnątrz, zegar i Wi-Fi | **Status bar zaakceptowany na podglądzie w trzech skórkach:** wyrównanie i pokazane stany baterii/Wi-Fi. Zgodnie z decyzją właściciela usunięto nazwę urządzenia obok zegara w szczegółach dimmera/RGB. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-03 | Nawigacja: dom/grid, odstępy i pola dotyku | **Dolna nawigacja oraz układ szczegółów przekaźnika Cyber Blue zaakceptowane na podglądzie:** ikony, odstępy, aktywna zakładka, naciśnięcie, niedostępne akcje i odstęp V/A od paska 16 px. Pola dotyku 64×47 px; ikona procesora pozostaje. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-04 | Ustawienia: ostatni element listy, ikona–tekst–strzałka | **Poprawki zaakceptowane na podglądzie:** sześć pozycji na jednym ekranie, mniejsze odstępy, Netzwerk und Verbindungen w jednej linii; ikona dodawania Shelly w podmenu Cyber Blue. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-05 | Dashboard: wybór, długie nazwy, +/edycja/informacje | **Układ zaakceptowany na podglądzie:** jeden wiersz z akcjami i przewijaną nazwą bez wielokropka. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-06 | Karty: odstępy, pełne pomiary, brak pustego „Keine weiteren Messwerte”, główna akcja | **Pokazane karty przekaźników/gniazdek zaakceptowane na podglądzie:** osobne wiersze pełnego pomieszczenia, stanu i pomiarów, przyciski oraz pionowe wyśrodkowanie W/V także po aktualizacji. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-07 | Termostat: odczyt, nastawa i ergonomia −/+ | **Karta zaakceptowana na podglądzie przez właściciela**: odczyt, „Soll”, bateria i jej ramka, wspólna oś pól oraz wyśrodkowane +/−. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-08 | Okno i drzwi: grafika otwartego/zamkniętego skrzydła | **Karty zaakceptowane na podglądzie przez właściciela**: nazwa u góry, mała ikona po lewej, duża po prawej, pośrodku pomieszczenie → ikona z lx → bateria z procentem. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-09 | Roleta: wspólna animacja trzech skórek, skala, status, pozycja i STOP | **Cały punkt zaakceptowany na podglądzie przez właściciela**: przyciski, odstęp od nawigacji, skala, animacja oraz karta Dashboardu z pomieszczeniem zamiast stanu, pozycją i sterowaniem. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| UI-10 | Cyber Blue: turkusowe ramki treści, w tym wynik i zgoda Wi-Fi AP | Wzór dashboardu przeniesiony do wspólnych paneli, akcji i pól; podwójny kontur i grubsza strzałka górnego przycisku powrotu oraz pokazane panele dimmera/RGB **zaakceptowane na podglądzie**. Końcowy przegląd całej skórki pozostaje otwarty |
| UI-11 | Równy zakres funkcjonalny skórek oraz ekrany dimmera/RGB | **Dimmer i RGB zaakceptowane na podglądzie we wszystkich trzech skórkach:** wspólny układ, pomiary, szybkie akcje, ramki, kontrast, edytor R/G/B, offline i odstęp 16 px od nawigacji. Odbiór na pilocie po nowym DEV pozostaje otwarty |
| CFG-01 | Dodaj/edytuj/usuń pomieszczenia, dashboardy, trwałe nazwy i zdjęcia | **Zaakceptowane na podglądzie:** edycja i usuwanie pomieszczeń, Góra/Dół dla Dashboardu i Urządzeń, opisy wartości i przygaszenie niedostępnych kontrolek oraz złote ważne komunikaty konfiguracji. Tworzenie pomieszczeń, pozostałe niespójności dashboardów i odbiór na sprzęcie pozostają otwarte |
| NET-01 | LAN, Wi-Fi AP i BLE: skan, wybór, zgody, błędy, retry i powrót do LAN | **Zaakceptowane na podglądzie:** Dalej, znacznik metody, górny powrót Cyber Blue; pokazane ekrany wyszukiwania, pustej listy i błędu logowania LAN z pełnym IP/MAC; wyszukiwanie, komunikat bez Wi-Fi, pusta lista i niedostępność Bluetooth; pokazane ekrany AP ze wskaźnikiem jak LAN, większymi ikonami i złotym komunikatem w ramce; **wspólny formularz nazwy/pomieszczenia/kanału i przebieg zapisu, złote błędy, nieaktywne „Zapisz”, widoczne pole hasła, zachowanie wpisów i powrót do Dashboardu**. Pozostałe kroki konfiguracji AP/BLE i odbiór na sprzęcie pozostają otwarte |
| NET-02 | BLU: bonding/PIN, konfiguracja i odczyt przez bramkę | **W trakcie ankiety:** pokazano konfigurację sieci Shelly, PIN, ustawienia BLU, potwierdzenia i zapominanie parowania. Poprawiono ramki ważnych komunikatów, tytuł podsumowania sieci i tłumaczenie Zapomnij. **Turkusowe obramowania i odstępy czterech pokazanych ekranów Wi-Fi/BLU Cyber Blue zaakceptowane na podglądzie** (`638b56c`); **Wybór bramki, logowanie, instrukcja TRV i trzy pokazane wyniki rejestracji zaakceptowane na podglądzie** (`c2627bc`). **Pozostałe pokazane stany wyszukiwania/rejestracji, oba błędy i formularz karty również zaakceptowane na podglądzie**; odbiór wyglądu pokazanego przepływu bramki zakończony. Końcowy zapis karty oraz rzeczywiste RF nadal do odbioru |
| CTRL-01 | Wiele Shelly, readback, usunięcie jednego kanału i trwałość po restarcie | Oczekuje na ocenę |
| CTRL-02 | Switch/Cover/Light/RGB/H&T/TRV oraz ręczne sceny i grupy | **Zaakceptowane na podglądzie:** kreator grup, edytor scen, karty grup zgodne z Dashboardem/Urządzeniami, wskazane poprawki widoku termostatu/grupy oraz widoki grup dimmerów i RGB w trzech skórkach. **Szczegóły grup przekaźników i rolet również zaakceptowane na podglądzie**; nastawa TRV w ręcznych scenach również zaakceptowana na pokazanym podglądzie, a zachowanie pierwszej automatyzacji kontakt → ogrzewanie jest uzgodnione; projekt i plan wdrożenia są zatwierdzone, implementacja i niezależny przegląd zakończone lokalnie; odbiór nowych formularzy/statusów pozostaje otwarty. Rzeczywiste sterowanie, readback i trwałość na sprzęcie pozostają otwarte |
| SYS-01 | Bateria, kalibracja, wybudzanie, Wi-Fi reconnect i restart | **Zaakceptowane na podglądzie** (`6a47dcc`, `77c65d4`): główny ekran baterii, pokazane elementy kalibracji, złote błędy, ponowny odczyt, klawiatura oraz paski pamięci i stan BLE; korekta DE: Ein / Aus. Rzeczywisty ADC/profil oraz wybudzanie/reconnect/restart pozostają do odbioru |
| SYS-02 | Aktywność, eksport/backup i reset fabryczny | **Wdrożone; końcowa integracja DEV 0.7.71.** Aktywność LCD zaakceptowana. Pięć uwag końcowego przeglądu poprawionych z regresjami; host/LVGL/Chromium i build ESP32-S3 PASS. Nowe widoki backup/reset do dalszej ankiety po DEV; resetu ani odtwarzania na sprzęcie nie wykonywano |
| WEB-01 | Panel WWW: konfiguracja, zgody, język i podgląd LCD | **Pokazane widoki konfiguracji WWW, parowania/podglądu oraz LCD bezpieczeństwa/kodu/unieważnienia zaakceptowane na podglądzie** (`3cdd2f7`), w tym układ komputer/telefon, osobne języki, złoty konflikt z nieaktywnym Zapisz, komunikaty podglądu i stałe akcje LCD. Powiązane Chromium/LVGL i słownik PASS. Pozostałe podstrony i rzeczywiste HTTPS/NVS/synchronizacja/podgląd na sprzęcie pozostają otwarte |
| OTA-01 | Aktualizacja bez utraty danych, przerwany transfer i rollback | **Zaakceptowane na podglądzie** (`b9870ae`): natywne Instaluj, złoty komunikat progu 20%, informacja przy nieznanym procencie, ikona kanału i odliczanie 10…0. **Lokalne OTA przez WWW również zaakceptowane na podglądzie** (`2fd1427`): dotychczasowe LCD i panel. Próg 20% uploadu WWW jest wdrożony, z nowym odczytem pracownika przed Flash i jawną zgodą USB; powiązane testy i build ESP32-S3 PASS. **Nowy krok USB oraz złote komunikaty niskiej baterii i niedostępnego pomiaru zaakceptowane na podglądzie** (`a415e17`). Odbiór progu na sprzęcie, instalacja, dane i rollback otwarte |

Problemy z ankiety dopisujemy tutaj wraz z wersją, ekranem, skórką i oczekiwanym zachowaniem. Naprawiamy je kolejno; po drobnej zmianie sprawdzamy tylko dotknięty moduł i ekrany. Kolejny DEV powstanie po zakończeniu uzgodnionego zakresu, końcowym przeglądzie UI i pełnej walidacji commita wydania. Nie oznacza to automatycznego wdrażania wycofanych funkcji ani scalania do main.

### Runda 1 — uwagi właściciela z pilota 0.7.70-dev

Potwierdzona zainstalowana wersja: **0.7.70-dev**. Poniższe poprawki powstają w kodzie roboczym; nie są jeszcze nowym wydaniem ani odbiorem na urządzeniu.

1. Karty: co najmniej 12 px odstępu głównej akcji od dolnej ramki.
2. Szczegóły rolety: co najmniej 16 px odstępu akcji od nawigacji, z zachowaniem rozdzielenia podglądu od potwierdzonej pozycji, wysyłania po puszczeniu oraz STOP.
3. **Tylko karta termostatu na Dashboardzie**: ramka 104×56 px i dwa przyciski 44 px, jak sterowanie rolety. Temperatura bieżąca i nastawa pozostają widoczne. Niemiecki podpis: **Soll**. Pierwotną uwagę o usunięciu baterii z karty zastępuje późniejsze ustalenie z rundy 2 poniżej.
4. Okno i drzwi: duża grafika prawego panelu dla obu stanów. Nie powielać napisu „Otwarte/Zamknięte” w prawym panelu; tekst pozostaje przy pomieszczeniu.
5. Cyber Blue: sfazowane turkusowe obramowania dashboardu przenieść do wspólnych paneli, akcji i pól; zachować czytelne czerwone/żółte stany oraz własny wygląd pozostałych skórek.
6. Kreator: **pierwszy ekran z „Krok 1 z 3” jest zaakceptowany**. Dopiero kolejne ekrany wyszukiwania i konfiguracji mają mieć numer kroku pod nagłówkiem, zamiast obok zegara. Przesunąć treść AP/BLE/LAN, aby uniknąć nowej kolizji.
7. Etykiety dodawania BLE mają mówić o urządzeniach: **Urządzenie/Gerät/Device**. Zachować odrębne zgody oraz komunikaty o niepotwierdzonej tożsamości i niesparowanym urządzeniu.

Stan tej rundy: powyższe poprawki są w kodzie roboczym. Oceny właściciela w tabeli pozostają otwarte do obejrzenia podglądu i późniejszego odbioru na pilocie.

Walidacja: `test_dev_069_ui`, `test_cover_ui` i `test_cyber_ui` przeszły na przypiętym LVGL 9.3. Sprawdzono wybrane karty/kreator, oba stany okna i drzwi, napisy PL/EN/DE, wysyłanie pozycji po puszczeniu, priorytet STOP, natywne kontrolki i 24 kafelki w niezmienionej puli 128 KiB. Przełączanie skórek nie wykazało wzrostu użycia pamięci. To testy hosta z modelami radia i zapisu. Pełny atlas, build płyty i pełne CI należą do końcowej walidacji przed następnym DEV.

### Runda 2 — karta termostatu po obejrzeniu podglądu

Ostatnie ustalenie właściciela: **„Soll” i temperatura zadana razem w ramce; wolne miejsce po lewej wykorzystać na ikonę baterii oraz jej poziom**. Dotyczy karty na Dashboardzie, zastępuje wcześniejsze usunięcie baterii z tej karty.

- Sterowanie −/+ zachowuje ramkę 104×56 px i przyciski 44 px, jak karta rolety.
- Pod sterowaniem znajduje się osobna ramka 104×24 px z nastawą. Temperatura bieżąca pozostaje w dotychczasowym polu pomiaru.
- Po lewej, na wysokości nastawy: ikona baterii termostatu i procent. To odczyt danego urządzenia, niezależny od baterii pilota w status barze. Brak poprawnego bieżącego odczytu daje „—”; niski poziom ma kolor ostrzegawczy.
- Dolny odstęp ramki nastawy wynosi co najmniej 12 px. Napisy mieszczą się w trzech skórkach i językach PL/EN/DE.

Walidacja tej poprawki: `DSR_UI_FOCUS_TRV=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS na przypiętym LVGL 9.3. Wygenerowano tylko 10 podglądów zmienionej karty: trzy skórki × PL/EN/DE oraz brak baterii. Sprawdzono też 0%, 100%, brak/nieprawidłowy odczyt, stan offline, rzeczywiste obszary rysowania tekstów i odstępy. Dashboard z 16 termostatami pozostawia 12 456 B wolnej pamięci (największy blok 12 400 B) w niezmienionej puli 128 KiB; dolne odczyty korzystają z istniejących obiektów i zasobów ikon.

Wspólne rysowanie ikon sprawdzono osobno: `DSR_UI_FOCUS_ICONS=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_cyber_ui` — PASS, jedna galeria, bez pozostałych ekranów tego zestawu. `python3 tools/check_docs.py` i `git diff --check` również przeszły.

Stan: kod roboczy do oceny na podglądzie, następnie na pilocie. To wynik hosta z modelem urządzenia, bez potwierdzenia na sprzęcie. Nowy DEV pozostaje odłożony do końca uzgodnionych poprawek i pełnej walidacji wydania.

### Runda 3 — wyrównanie odczytu i baterii termostatu

Po obejrzeniu podglądu właściciel zlecił wyśrodkowanie temperatury odczytanej wewnątrz jej ramki oraz przesunięcie ikony baterii wraz z procentem w prawo. Etykieta odczytu korzysta teraz z rzeczywistej wysokości tekstu i jest wyśrodkowana w obu osiach, z uwzględnieniem obramowania. Bateria jest wyrównana do lewej krawędzi kolumny odczytu, z odstępem przed ramką nastawy.

Walidacja: ten sam tryb `DSR_UI_FOCUS_TRV=1` — PASS, tylko 10 podglądów karty w trzech skórkach i PL/EN/DE. Regresja sprawdza środek etykiety względem ramki oraz wyrównanie baterii; przed poprawką wykazała błędny środek etykiety. Dashboard z 16 termostatami pozostawia 12 256 B wolnej pamięci, największy blok 12 200 B, przy puli 128 KiB. Odbiór wyglądu na podglądzie i później na pilocie pozostaje otwarty; nowy DEV po zakończeniu ankiety.

### Runda 4 — ramka baterii termostatu

Właściciel zaakceptował wyśrodkowanie temperatury odczytanej i położenie baterii z procentem. Dodatkowo zlecił wspólne obramowanie ikony i poziomu baterii. Dodano ramkę 92×24 px wokół obu elementów, zachowując ich położenie. Cyber Blue używa wspólnego turkusowego, sfazowanego konturu; Grafit i Jasny mają ramkę dopasowaną do swojej skórki.

Korekta po kolejnym podglądzie: właściciel ocenił ramkę jako zbyt długą. Usunięto stałą szerokość 92 px; aktualna ramka zachowuje wysokość 24 px, a jej szerokość wynika z ikony, odstępu i zmierzonego tekstu procentu, z marginesem 6 px po obu stronach. Położenie ikony oraz procentu pozostaje zaakceptowane. Ta sama regresja przeszła dla trzech skórek i PL/EN/DE, potwierdzając skrócenie rzeczywiście rysowanych krawędzi oraz dopasowanie także dla 0%, 100% i braku odczytu. Zapas pamięci dla 16 termostatów pozostał bez zmian.

Walidacja: `DSR_UI_FOCUS_TRV=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS; tylko 10 podglądów karty, trzy skórki i PL/EN/DE. Regresja sprawdza rzeczywiście narysowane cztery krawędzie ramki; przed poprawką wykazała ich brak. Ramka korzysta z istniejącej warstwy rysowania, bez dodatkowych obiektów LVGL. Dla 16 termostatów zapas pamięci pozostał bez zmian: 12 256 B, największy blok 12 200 B, pula 128 KiB. To walidacja hosta; odbiór na pilocie i nowy DEV pozostają po zakończeniu ankiety.

### Runda 5 — wspólna oś baterii i temperatury oraz znaki +/−

Właściciel zaakceptował krótszą szerokość ramki baterii. Zlecił wyśrodkowanie całego pola baterii względem pola temperatury odczytanej „21.5 °C” oraz wyśrodkowanie znaków + i − wewnątrz przycisków. Ramka baterii wraz z zawartością ma teraz środek zgodny ze środkiem rzeczywistego pola odczytu, także dla 0%, 100% i braku danych. Przyciski pozycjonują widoczny kształt znaku z uwzględnieniem jego wymiarów i linii bazowej fontu, zamiast stałego przesunięcia etykiety.

Walidacja: ten sam tryb `DSR_UI_FOCUS_TRV=1` — PASS; 10 podglądów zmienionej karty, trzy skórki i PL/EN/DE. Sprawdzono środek rzeczywistych pikseli obu znaków oraz wspólną oś pól. Ponieważ ten sam helper obsługuje również przyciski 48 px w szczegółach termostatu, sprawdzono ich wyśrodkowanie w trzech skórkach, bez generowania dodatkowego atlasu. Regresja przed poprawką wykazała przesunięte znaki. Dla 16 termostatów pozostaje 12 632 B wolnej pamięci, największy blok 12 576 B, w puli 128 KiB. Podgląd kodu roboczego do oceny; odbiór na pilocie i nowy DEV po zakończeniu ankiety.

**Odbiór ankiety:** właściciel potwierdził „Tak, karta zaakceptowana” po wyśrodkowaniu ramki baterii względem temperatury i znaków +/−. Punkt UI-07 zamknięty na poziomie wyglądu podglądu; nie jest to potwierdzenie wdrożenia ani odbioru na fizycznym pilocie. Dalsza ankieta dotyczy kolejnych ekranów.

### UI-08 — zaakceptowany podgląd kart okna i drzwi

Właściciel wybrał ten punkt jako następny po zaakceptowaniu karty termostatu. Przygotowano świeży podgląd okna i drzwi w obu stanach dla Cyber Blue, Grafit i Jasny, z dodatkowym przykładem dłuższej nazwy oraz rzeczywistych odczytów 0 lx i baterii 92% z modelu testowego.

Wykryta niespójność: wiersz „pomieszczenie • stan” skraca pomieszczenie „Schlafzimmer” przy „Offen”, a dłuższe „Geschlossen” usuwa pomieszczenie z tego wiersza. Nazwa urządzenia i przypisanie do pomieszczenia są odrębnymi polami. Grafika po prawej poprawnie rozróżnia okno/drzwi i stan otwarty/zamknięty, bez powielania tekstu stanu w swoim panelu.

Właściciel zaakceptował wielkość i wygląd grafik. Ostateczne doprecyzowanie układu zastępuje wcześniejszą propozycję pełnego wiersza pomieszczenia: **nazwa urządzenia u góry; mała ikona po lewej; zaakceptowana duża grafika po prawej; w środkowej kolumnie od góry pomieszczenie, ikona i wartość lx, następnie ikona baterii i procent**.

Ten wariant jest w kodzie roboczym. Pomieszczenie zawija się w środkowej kolumnie bez skracania i bez rywalizacji z tekstem stanu; otwarcie/zamknięcie wskazują zaakceptowane grafiki. Odczyty lx i baterii mają osobne, krótkie ramki oraz ikony. Rzeczywiste 0 lx pozostaje zerem, brak albo nieprawidłowy odczyt jest oznaczony „—”. Zachowano dotychczasowe komunikaty offline/braku danych.

**Odbiór ankiety:** właściciel potwierdził „Tak, układ zaakceptowany” po obejrzeniu tego wariantu. Punkt UI-08 zamknięty na poziomie wyglądu podglądu, obejmując grafiki oraz układ kart okna i drzwi. Odbiór na fizycznym pilocie pozostaje otwarty po nowym DEV.

Walidacja: `DSR_UI_FOCUS_CONTACT=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS na przypiętym LVGL 9.3. Tylko 15 podglądów wybranych kart: 12 kombinacji skórki/typu/stanu i trzy dłuższe nazwy z odczytami. Sprawdzono PL/EN/DE, długie nazwy pomieszczeń do granicy pola modelu, odstępy i wyśrodkowanie wierszy pomiarowych, brak odczytów oraz offline. Grafika 52 px zachowała rozmiar. Pomieszczenie i odczyty są rysowane w istniejącej karcie; dla 16 urządzeń pozostaje 61 008 B wolnej pamięci, największy blok 60 952 B, w puli 128 KiB. To host z modelem danych, bez odbioru na pilocie; nowy DEV po zakończeniu ankiety.

### UI-09 — zaakceptowany podgląd rolety, przycisków i animacji

Po zaakceptowaniu termostatu oraz kart okna i drzwi właściciel wybrał roletę. Przygotowano aktualne piksele LVGL 9.3 dla Grafit, Jasny i Cyber Blue: zatrzymana roleta, przeciąganie z oddzielnym podglądem celu, ruch z czerwonym STOP, nieznana pozycja oraz karta Dashboardu w spoczynku i w ruchu. Krótki GIF pokazuje klatki istniejącej animacji otwierania 50→80% i zamykania 80→20%, z własną kolorystyką każdej skórki. Przyciski szczegółów mają wysokość 48 px i 16 px odstępu przed nawigacją.

**Wykryta niespójność pierwszego podglądu:** na karcie Dashboardu przypisane pomieszczenie „Wohnzimmer” znikało przy stanie „Gestoppt” oraz „Wird geöffnet”. Współdzielony wiersz pomieszczenia i stanu oddawał całe miejsce dłuższemu stanowi.

**Odbiór ankiety:** właściciel potwierdził „Tak, te elementy zaakceptowane” dla przycisków, odstępu od nawigacji, skali oraz animacji otwierania/zamykania. Dla karty Dashboardu zlecił: **„Stan zastąp nazwą pomieszczenia”**. To zastępuje propozycję dodania osobnego wiersza pomieszczenia nad stanem.

W kodzie roboczym karta rolety pokazuje teraz nazwę pomieszczenia w miejscu zwykłego stanu ruchu. Pod nią pozostaje pozycja, a po prawej dotychczasowe sterowanie lub czerwony STOP w ruchu. Długie nazwy zawijają się z automatycznym przesunięciem pozycji i dopasowaniem wysokości karty. Użyto istniejącej etykiety, bez dodawania obiektów LVGL. Brak przypisania ma lokalizowany komunikat; błędy i nieaktualny odczyt zachowują ostrzeżenie w tym miejscu. Przy okazji usunięto przycinanie dłuższego niemieckiego komunikatu błędu logowania na karcie rolety. Szczegóły i zaakceptowana animacja nie zostały zmienione.

**Końcowa akceptacja podglądu:** właściciel potwierdził „Tak, karta rolety zaakceptowana” dla wariantu z pomieszczeniem zamiast stanu. W połączeniu z wcześniejszą akceptacją przycisków, skali i animacji zamyka to punkt UI-09 na poziomie wyglądu podglądu. Nie jest to potwierdzenie wdrożenia ani odbioru na fizycznym pilocie; nowy DEV pozostaje po zakończeniu ankiety i walidacji wydania.

Walidacja pierwszego podglądu: `DSR_UI_FOCUS_COVER=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS. Tryb uruchamia wyłącznie roletę; 60 klatek obejmuje 42 klatki dwóch animacji w trzech skórkach. Sprawdzono rzeczywistą zmianę pikseli rolety, odstęp akcji i brak zapisu pozycji podczas przeciągania, z pojedynczym wywołaniem po puszczeniu. Podgląd używa aktywnych atrap callbacków otwierania, zamykania i STOP; nie są to polecenia wysyłane do sprzętu. Sam pierwszy przegląd nie zmieniał kodu firmware i nie generował pełnego atlasu. Późniejsza zmiana karty została opisana powyżej; odbiór na pilocie pozostaje otwarty.

Istniejąca regresja `python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_cover_ui` — PASS, osiem klatek. Potwierdza pojedyncze żądanie po puszczeniu, pierwszeństwo STOP i końcowy potwierdzony odczyt, odzyskanie odczytu bez powtarzania ruchu po błędzie oraz blokady przy kalibracji i błędzie autoryzacji. Napisy sprawdza w PL/EN/DE. Radio, worker i zapis pozostają modelem hosta.

Walidacja zmienionej karty: `DSR_UI_FOCUS_COVER_CARD=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, tylko dziewięć klatek kart, bez powtarzania animacji ani szczegółów. Sprawdzono trzy skórki i PL/EN/DE, spoczynek/otwieranie/zamykanie, pełne nazwy do granicy pola modelu, brak przypisania oraz zachowanie ostrzeżeń offline i błędu logowania. Regresja przed poprawką wykazała brak pomieszczenia; podczas korekty wykazała też przycięty niemiecki komunikat błędu. Rzeczywiste piksele krótkiego i długiego pomieszczenia sprawdzono na renderach LVGL 9.3. To wynik hosta, bez wdrożenia na pilocie i bez nowego DEV.

### NET-01 / UI-10 — podgląd dodawania urządzeń

Po akceptacji rolety właściciel wybrał dodawanie urządzeń: kroki, napisy i ramki. Przygotowano świeży podgląd wyboru metody, ręcznego IP, AP oraz LAN w trzech skórkach, a także etykiet BLE w PL/EN/DE w Cyber Blue. Pierwszy ekran zachowuje zaakceptowane położenie „Krok 1 z 3”. Na kolejnych ekranach numer kroku znajduje się pod nagłówkiem, a nie obok zegara. Zgody połączenia z AP i wysłania Wi-Fi oraz ekran błędu mają turkusowe, sfazowane ramki Cyber Blue. Etykiety BLE mówią „urządzenie / device / Gerät”; RSSI i opis typu mają oddzielne, czytelne ramki.

**Niespójność pierwszego podglądu:** w kroku LAN „Nazwa i pomieszczenie” dolna część pola kanału była ucięta przez przewijany kontener formularza. Kontener miał wysokość 184 px, a pole kanału zaczynało się na 156 px i miało wysokość 44 px, więc jego dolne 16 px wykraczało poza początkowy obszar widoczny. Opis pomocniczy pod formularzem pozostawiał zbyt mało miejsca na pełny widok wszystkich pól. Problem występował we wszystkich trzech skórkach. Właściciel zaakceptował układ z pełnymi polami i opisem w przewijanej treści; wdrożenie oraz nowy podgląd opisano poniżej.

Walidacja podglądu: `DSR_UI_FOCUS_ONBOARDING=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden przypadek ograniczony do dodawania urządzeń, 48 wybranych klatek, bez pełnego atlasu ani ponawiania zaakceptowanych kart. Obejmuje pustą listę AP, błąd i ponowienie, dwie odrębne zgody, anulowanie z powrotem do domowego Wi-Fi w modelu hosta, wyniki LAN i konfigurację oraz rozpoznane/nierozpoznane BLE i języki PL/EN/DE. Obejrzenie rzeczywistych pikseli ujawniło opisane przycięcie pola kanału mimo przejścia dotychczasowych sprawdzeń. To walidacja hosta z atrapami radia i zapisu; akceptacja wyglądu oraz rzeczywiste dodawanie urządzeń na pilocie pozostają otwarte. Żaden nowy DEV nie został opublikowany.

### NET-01 — przycisk „Dalej / Continue / Weiter”

Właściciel oznaczył podgląd kreatora jako wymagający korekty i zlecił: **„Przycisk Weiter dodajmy ikonę oraz kolor zmieńmy na ten niebieski z przycisku Speichern”**. Dodano strzałkę w prawo z istniejącego zestawu ikon, z miejscem oddzielonym od etykiety. Zmiana dotyczy przycisku Dalej w stopce kreatora i działa w trzech skórkach oraz PL/EN/DE.

Aktywny Dalej już korzystał z tego samego stylu podstawowej akcji co Zapisz; poprzedni podgląd pokazywał stan przed wyborem metody. Nowy podgląd obejmuje oba stany oraz aktywny niebieski przycisk po wyborze. Porównanie ze Zapisz potwierdza identyczny kolor tła, kolor gradientu i kierunek gradientu. Wymóg wyboru metody pozostaje aktywny; samo zaznaczenie metody nie uruchamia skanowania, a przejście następuje po naciśnięciu Dalej.

Walidacja: `DSR_UI_FOCUS_ONBOARDING_NEXT=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, tylko 12 klatek pierwszego ekranu, trzy skórki i PL/EN/DE. Sprawdzono ikonę, czytelność pełnej etykiety i odstępy, zgodność aktywnego stylu ze Zapisz oraz brak uruchomienia skanowania przed wyborem i naciśnięciem Dalej. Przed poprawką regresja wykazała brak ikony. Te same sprawdzenia dla DE w trzech skórkach są także w standardowym przebiegu tego przypadku UI, przewidzianym na końcowe CI. Pozostałych ekranów i atlasu nie uruchamiano ponownie w tej rundzie. Podgląd oczekuje na ocenę właściciela. Przycięte pole kanału oraz pozostałe uwagi do kreatora nadal są otwarte; odpowiedź dotycząca przycisku nie zamyka tych punktów.

Doprecyzowanie właściciela po podglądzie: strzałkę przenieść na prawą stronę przycisku, a po wybraniu Bluetooth, Wi-Fi AP lub sieci lokalnej dodać białą kropkę wewnątrz niebieskiego okręgu. Obie zmiany są w kodzie roboczym. Znacznik jest wyśrodkowany i pojawia się tylko przy jednej wybranej metodzie; przełączenie usuwa go z poprzedniej opcji. Rysowanie korzysta z istniejącej kontrolki, bez dodatkowych obiektów ani zasobów graficznych. Etykieta Dalej/Continue/Weiter ma wydzielone miejsce po lewej od strzałki. Aktywny niebieski styl Zapisz pozostaje wspólny.

Walidacja tej korekty: ten sam tryb `DSR_UI_FOCUS_ONBOARDING_NEXT=1` — PASS, 14 wybranych klatek pierwszego ekranu, trzy skórki i PL/EN/DE. Sprawdzono rzeczywiste białe piksele w środku zaznaczonego okręgu oraz ich brak przy pozostałych opcjach, zmianę zaznaczenia między trzema metodami, położenie strzałki po prawej i pełne etykiety bez kolizji. Wybór opcji nie uruchamia transportu; skan startuje dopiero po Dalej. Nie powtarzano pozostałych ekranów, animacji ani pełnego atlasu.

**Odbiór ankiety:** właściciel potwierdził „Tak, te poprawki zaakceptowane” dla strzałki po prawej oraz białej kropki wybranej metody. Te elementy pierwszego ekranu są zaakceptowane na podglądzie. Przycięte pole kanału, pozostały zakres NET-01 / UI-10 i odbiór na fizycznym pilocie pozostają otwarte; akceptacja nie oznacza nowego wydania DEV.

### NET-01 — formularz „Nazwa i pomieszczenie”

Właściciel odpowiedział „Tak, taki układ odpowiada” na propozycję zachowania szerokości i wysokości pól, powiększenia formularza oraz przeniesienia opisu pod pola do przewijanej treści. Jest to akceptacja układu do wdrożenia; końcowa ocena poniższego podglądu pozostaje otwarta.

Obszar formularza ma teraz 208 px wysokości zamiast 184 px. Pole kanału 288×44 px mieści się w całości, a pola nazwy i pomieszczenia zachowują swoje rozmiary. Opis pomocniczy znajduje się za polami. Walidacja pustej nazwy, rozpoczęcie zapisu, błędy i niepotwierdzony zapis automatycznie przewijają komunikat do widocznej części, nad stopką. Nieobsługiwany kanał nadal blokuje zapis i od razu pokazuje wyjaśnienie.

Przy sprawdzaniu klawiatury wykryto także kolizję pola nazwy z kartą tożsamości urządzenia. Na czas wpisywania karta jest ukrywana, a aktywny wiersz ma własny obszar nad klawiaturą. Po zamknięciu klawiatury wracają karta, pełny formularz i początkowe przewinięcie. Formularz danych logowania zachowuje osobną wysokość 184 px. Radio, reguły dodawania i etapy zapisu nie zostały zmienione.

Walidacja: `DSR_UI_FOCUS_LAN_FORM=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden przypadek ograniczony do tego formularza, dziewięć wybranych klatek, trzy skórki i PL/EN/DE. Sprawdzono pełne pole kanału przed i po klawiaturze, dostępność opisu, widoczność komunikatów walidacji/niepotwierdzonego zapisu/limitu urządzeń oraz powrót do logowania po błędzie autoryzacji. Pusta nazwa nie uruchamia żądania BIND. Regresja uruchomiona na oryginalnym formularzu potwierdziła przycięcie pola. Podgląd używa przypiętego LVGL 9.3 i właściwych motywów natywnych pól; sprawdzono rzeczywiste piksele. Nie generowano pełnego atlasu ani nie ponawiano zaakceptowanych kart. To wynik hosta z modelem radia i zapisu, bez nowego DEV ani odbioru na fizycznym pilocie.

## Konsolidacja starych zgłoszeń

Zamknięcie oznacza przeniesienie dalszej koordynacji tutaj, a nie potwierdzenie usunięcia wszystkich defektów ani wykonania fizycznych testów.

| Stare issue | Dalszy status |
| --- | --- |
| #6 — UI/UX quality gate: Shelly-style design system validated on ES3C35P | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #7 — Device onboarding: real local Shelly discovery wizard (LAN, Wi-Fi AP, BLE) | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #8 — Wi-Fi keyboard: larger iOS-inspired keys and readable 320×480 touch layout | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #12 — Physical Gen3 test: confirm relay ON/OFF, expose real V/A and command errors | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #15 — Shelly-style cover details: vertical position indicator and large Open/Close controls | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #18 — PILNE: rzeczywiste skanowanie Bluetooth LE na ES3C35P — Shelly Wi-Fi i Shelly BLU | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #19 — Wi-Fi AP: lokalne dodawanie nowych urządzeń Shelly z fabrycznego punktu dostępowego | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #20 — Silnik wielu urządzeń Shelly: niezależne połączenia, kolejki i stan w czasie rzeczywistym | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #21 — Grupy, sceny i aktywność: wykonanie lokalnych akcji zamiast samych edytorów | Grupy/sceny i backend aktywności są; widok LCD oraz odbiór pozostają w #74 |
| #22 — Energia, wykresy i statystyki Shelly: prawdziwe pomiary, zapis historii, spójne UI | Wycofane decyzją właściciela; pomiary bieżące zostają |
| #23 — Rzeczywiste typy urządzeń Shelly: Cover, dimmer, RGB/RGBW, sensory i termostaty | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #24 — Kompletne aktualizacje OTA i ustawienia: GitHub Releases, kontrola wersji i przywracanie | Zakres częściowy: OTA/ustawienia są; eksport/reset i odbiór pozostają w #74 |
| #25 — MASTER QUALITY GATE: aktywować i przetestować wszystkie widoczne funkcje DIY Shelly Remote | Zakres częściowy: OTA/ustawienia są; eksport/reset i odbiór pozostają w #74 |
| #28 — Pomieszczenia na pilocie: bezpośrednie Dodaj/Edytuj/Usuń z widoku pomieszczeń | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #29 — Szczegóły urządzenia: działająca zakładka Ustawienia oraz wejście do historii i Wykresów | Ustawienia są; historia/wykresy wycofane decyzją właściciela |
| #32 — UI/UX: nowoczesny kreator dodawania Shelly inspirowany bottom-sheet Shelly Smart Control | Implementacja jest; odbiór/pozostały zakres przeniesiony do listy powyżej |
| #66 — Koordynacja: status prac i decyzje właściciela | Koordynacja przeniesiona do #74 |

PR-y zamknięte bez merge: #73, #72, #71, #70, #69, #67, #65. Ich nowszy kod znajduje się w historii 0.7.70; #67 zastąpiony późniejszymi rozwiązaniami.

## Wszystkie gałęzie — stan przed porządkami

Przejrzano 62 zdalne gałęzie. Zachowano `main` i `work/dev-0.7.70`; pozostałe 60 usunięto po konsolidacji PR-ów. Pięć ma łącznie sześć unikalnych historycznych commitów, bez utraty funkcji w aktualnym kodzie; zachowano plik odzyskiwania 64 KB `.local/audit-cleanup-2026-10-22/legacy-unique-commits.bundle`, zweryfikowany przez `git bundle verify`. Pozostałe końcówki są przodkami bieżącej gałęzi.

| Gałąź | SHA | Decyzja / zachowanie historii |
| --- | --- | --- |
| `main` | `51c5840e4098` | Zachowano |
| `reference/0.2.0-local-dev` | `51c5840e4098` | Usunięto; commit w historii 0.7.70 |
| `ui/shelly-smart-control` | `c496479e3e8c` | Usunięto; commit w historii 0.7.70 |
| `work/battery-calibration` | `799861169231` | Usunięto; commit w historii 0.7.70 |
| `work/codex-p0-review` | `4a4d69cb200f` | Usunięto; commit w historii 0.7.70 |
| `work/config-save-photo-fixes` | `fdce36fa47d5` | Usunięto; commit w historii 0.7.70 |
| `work/config-web-panel` | `5f6a653e9a85` | Usunięto; commit w historii 0.7.70 |
| `work/cyber-blue-complete` | `8a8dffdc7961` | Usunięto; commit w historii 0.7.70 |
| `work/cyber-blue-final-polish` | `c138c990ae6e` | Usunięto; commit w historii 0.7.70 |
| `work/cyber-blue-quality-pass` | `d2290f21a0fd` | Usunięto; commit w historii 0.7.70 |
| `work/cyber-blue-reference-refinements` | `5d33c2c9a755` | Usunięto; commit w historii 0.7.70 |
| `work/dev-0.7.68` | `5fbd86dc554d` | Usunięto; commit w historii 0.7.70 |
| `work/dev-0.7.70` | `d43da439f757` | Zachowano |
| `work/device-settings-context` | `693d869f54c1` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/es3c35p-bringup` | `f592876f532f` | Usunięto; commit w historii 0.7.70 |
| `work/fix-071-boot-memory` | `5a21559b7548` | Usunięto; commit w historii 0.7.70 |
| `work/fix-startup-recovery` | `aa1969a32c8c` | Usunięto; commit w historii 0.7.70 |
| `work/foundation` | `4a40e428e722` | Usunięto; commit w historii 0.7.70 |
| `work/issue-5-web-locale` | `863729646280` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-8-unified-ui` | `b9daba3cd937` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-display-settings` | `2641ecda325e` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-global-settings` | `f7fb16d97ac5` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-lcd-restart` | `1765187de0ff` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-visual-system` | `54bbef68095b` | Usunięto; commit w historii 0.7.70 |
| `work/issue-6-web-pairing-revoke` | `b1286945ef05` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/issue-7-lan-discovery` | `895480c3436a` | Usunięto; commit w historii 0.7.70 |
| `work/issue-12-stable-polling` | `ed2ef9fdfc73` | Usunięto; commit w historii 0.7.70 |
| `work/issue-12-telemetry-errors` | `9436220300df` | Usunięto; commit w historii 0.7.70 |
| `work/issue-15-cover-details` | `41137b62a7b1` | Usunięto; commit w historii 0.7.70 |
| `work/issue-18-ble-ota-full` | `3b96534ea312` | Usunięto; commit w historii 0.7.70 |
| `work/issue-18-ble-scan` | `c4ca8728ea62` | Usunięto; commit w historii 0.7.70 |
| `work/issue-19-ap-onboarding-full` | `267c32f31d7d` | Usunięto; commit w historii 0.7.70 |
| `work/issue-19-ap-visit` | `633863fd7e8c` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/issue-20-independent-shelly` | `07331b829a22` | Usunięto; commit w historii 0.7.70 |
| `work/issue-21-actions-full` | `b5a36ef927b9` | Usunięto; commit w historii 0.7.70 |
| `work/issue-22-history-integrated` | `f240d15bc402` | Usunięto; commit w historii 0.7.70 |
| `work/issue-23-cover-full` | `a194554a811c` | Usunięto; commit w historii 0.7.70 |
| `work/issue-23-ht-full` | `ab8a728ebb42` | Usunięto; commit w historii 0.7.70 |
| `work/issue-23-light-full` | `18f91a61ea91` | Usunięto; commit w historii 0.7.70 |
| `work/issue-23-rgb-full` | `16481fa226d9` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-app-download` | `1153d7104554` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-image-stream` | `2a3e7dd74a97` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-manifest-fetch` | `a298e6862475` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-manifest-verifier` | `c9dd6f712cd7` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-native-controller` | `e69e24a9be67` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-release-candidate` | `2fb3e452d6c8` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-release-chain` | `0790e6e803c0` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-release-fetch` | `784f99aefe2f` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-safe-restart` | `e72074c86bcd` | Usunięto; commit w historii 0.7.70 |
| `work/issue-24-upload-stream` | `3501a67213b1` | Usunięto; commit w historii 0.7.70 |
| `work/issue-25-battery-voltage` | `37dd18763b14` | Usunięto; commit w historii 0.7.70 |
| `work/issue-25-feature-audit` | `d5ce4024b81c` | Usunięto; commit w historii 0.7.70 |
| `work/issue-28-bind-factory` | `51124b4df202` | Usunięto; commit w historii 0.7.70 |
| `work/issue-28-editor-acceptance` | `f9bac663dd1d` | Usunięto; commit w historii 0.7.70 |
| `work/issue-29-settings-current` | `dc5a42ea7e28` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/issue-32-onboarding-full` | `0ee4dd2455f4` | Usunięto; commit w historii 0.7.70 |
| `work/local-switch` | `76683a0cc513` | Usunięto; commit w historii 0.7.70 |
| `work/p0-spiffs-ota-brand` | `e6e23597a966` | Usunięto; commit w historii 0.7.70 |
| `work/rooms-context-editor` | `81e734d38fe8` | Usunięto; commit w historii 0.7.70 |
| `work/shelly-hud-cyber-blue` | `e201332d8d0b` | Usunięto; commit w historii 0.7.70 |
| `work/ux-battery-clock-ble` | `b82a922d5314` | Usunięto; historyczne warianty w pliku odzyskiwania, funkcje mają późniejsze implementacje |
| `work/wifi-onboarding` | `81c0753fe5e8` | Usunięto; commit w historii 0.7.70 |

### UI-10 / NET-01 — górny przycisk „Zurück” w Cyber Blue

Po podglądzie formularza właściciel wskazał jako jedyne zastrzeżenie brak podwójnego turkusowego obramowania górnego przycisku powrotu i zbyt cienką strzałkę. Wspólny renderer dodawał wewnętrzny kontur dopiero od szerokości 80 px, podczas gdy ten przycisk ma 44×44 px.

Górny przycisk otrzymał wewnętrzny sfazowany kontur przy zachowaniu dotychczasowej ramki zewnętrznej. Strzałka w istniejącym obszarze ikony ma teraz zaokrąglony obrys 3 px. Zmiana dotyczy wspólnego nagłówka Cyber Blue; pozostałe skórki i zaakceptowane karty zachowują wygląd. Nie dodano obiektów LVGL, zasobów bitmapowych ani timerów. Obszar dotyku i przypisywanie funkcji powrotu pozostają bez zmian.

Walidacja: `DSR_UI_FOCUS_HEADER_BACK=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden przypadek ograniczony do przycisku nagłówka, trzy skórki i PL/EN/DE, cztery klatki. Sprawdzono stan naciśnięcia i rzeczywisty powrót z formularza do listy przez zdarzenia wskaźnika. Obejrzano rzeczywiste piksele LVGL 9.3, w tym porównanie przed/po i stan naciśnięty. Właściciel potwierdził „Tak, przycisk zaakceptowany”. Bez pełnego atlasu, nowego DEV i odbioru na fizycznym pilocie.

### NET-01 — pozostałe ekrany i błędy dodawania urządzeń

Właściciel wybrał dalszy przegląd dodawania urządzeń. Bieżący render ujawnił trzy konkretne niespójności: karta pustej listy AP miała 274 px przy widocznym obszarze 258 px, co przycinało jej dolny kontur; wyszukiwanie i puste wyniki LAN/BLE nie miały paneli zgodnych z Cyber Blue; IP, MAC i oznaczenie hasła na ekranie logowania LAN dzieliły za krótki wiersz i kończyły się wielokropkiem.

Poprawiono wysokość pustej karty AP do 258 px, dodano wspólne panele wyszukiwania LAN/BLE i ramki pustych wyników, w tym stanu niedostępnego Bluetooth. Grafit i Jasny używają tych samych układów z własną kolorystyką. W karcie tożsamości bez odczytanych kanałów IP i MAC mają osobne wiersze, a MAC zachowuje odstęp od dolnego konturu. Komunikat dostępności BLE bez Wi-Fi znajduje się wewnątrz panelu, nad stopką. Zachowano ekranowe zgody AP, dotychczasowe działanie ponowienia, anulowania i logowania; nie zmieniano backendu radia ani zapisu danych.

Walidacja: `DSR_UI_FOCUS_ONBOARDING_STATES=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden ukierunkowany przypadek w trzech skórkach i PL/EN/DE; zapisano 30 wybranych klatek po niemiecku. Sprawdzono wyszukiwanie, brak wyników, ponowienie i wyniki AP, brak ponownego wysyłania danych Wi-Fi przy retry, pusty LAN i błąd logowania z możliwością powrotu oraz pusty/niedostępny Bluetooth z ponowieniem także bez Wi-Fi. Kontrole geometrii obejmują pełną kartę AP, MAC nad konturem i komunikat offline nad stopką. Obejrzano rzeczywiste rendery przypiętego LVGL 9.3; podglądy czekają na ocenę właściciela. To modele dostawców sieci/BLE, bez fizycznego skanowania, nowego DEV, pełnego atlasu ani pełnego CI.

Odbiór tej rundy: właściciel potwierdził „Tak, te ekrany LAN zaakceptowane” oraz „Tak, te ekrany Bluetooth zaakceptowane”. Akceptacja obejmuje pokazane stany wymienione powyżej, bez rozszerzania jej na cały onboarding i działanie radia na sprzęcie. Dla Wi-Fi AP wybrał „Wymagają korekty — opiszę”; ten punkt pozostaje otwarty. Przekazane następnie wymagania i ich wdrożenie opisano poniżej.

### NET-01 — wskaźnik AP, większe ikony i złoty komunikat

Właściciel wskazał konkretną korektę AP: taki sam okrągły wskaźnik jak w LAN, większa ikona sieci przy braku wyników, większa ikona ostrzeżenia na ekranie wyniku oraz ważny tekst w pomarańczowo-złotym kolorze i istniejącym obramowaniu ostrzeżeń.

Wskaźnik AP używa teraz standardowego stylu LVGL tak jak LAN, rozmiaru 48×48 px i tej samej animacji 1000 ms / 200°. Ikona sieci ma obszar 72×72 px, ostrzeżenia 64×64 px. Duże symbole Cyber Blue oraz trójkąt ostrzegawczy są rysowane z łuków i linii w docelowym rozmiarze; nie powiększają małych masek bitmapowych. Główny komunikat wyniku ma złoty tekst i własną ramkę o wysokości dopasowanej do treści, z istniejącym stylem ostrzeżenia Cyber Blue. Jasny używa ciemniejszego odcienia dla kontrastu. Opis postępowania pozostaje pod komunikatem, z odstępem nad stopką. Zmiany nie dotyczą zaakceptowanych ekranów LAN i Bluetooth ani backendu AP.

Walidacja: `DSR_UI_FOCUS_AP_STATES=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden przypadek ograniczony do AP, trzy skórki i PL/EN/DE; 12 wybranych klatek po niemiecku. Zachowano sprawdzenie ponowienia samego skanu bez wysyłania danych Wi-Fi, pustej listy i znalezionego AP. Sprawdzono odstępy głównego komunikatu w ramce oraz opisu pomocniczego od ramki i dolnej krawędzi karty. Obejrzano rzeczywiste piksele LVGL 9.3, w tym ostrość większych ikon i kontrast Jasnego. Nie ponawiano zaakceptowanych LAN/Bluetooth, pełnego atlasu ani CI. Właściciel potwierdził „Tak, te ekrany AP zaakceptowane”. Akceptacja dotyczy pokazanych ekranów; bez nowego DEV i potwierdzenia fizycznego radia.

### UI-05 / UI-06 — nazwy, pomiary i odstępy Dashboardu

Właściciel wybrał przegląd Dashboardu. Przygotowano aktualne podglądy Grafit, Jasny i Cyber Blue: typowe karty przekaźników/gniazdek, dłuższe nazwy pulpitu i urządzenia oraz większe wartości pomiarów. Nie zmieniano układów zaakceptowanych kart termostatu, okna, drzwi i rolety.

Rzeczywiste piksele potwierdziły dwie niespójności. Selektor ma 146 px i trzy sąsiednie przyciski akcji; jego pole nazwy ma około 100 px. Dłuższa nazwa „Erdgeschoss – Wohnen, Küche und Terrasse” jest skracana mimo dwóch linii. W kartach przekaźników i gniazdek pomieszczenie współdzieli wąski wiersz ze stanem, a helper ogranicza je do części szerokości i wstawia wielokropek. Nawet „Küche und Esszimmer” jest niepełne. Nazwa urządzenia w przykładzie mieści się w dwóch liniach, a pomiary `0.0 W | 231.8 V` i `12345.6 W | 240.2 V` pozostają pełne.

Pierwsza ankieta przedstawiła wybór pełnej szerokości nazwy pulpitu z osobnym wierszem akcji albo zachowania jednego wiersza z przewijaną nazwą. Dla przekaźników/gniazdek zaproponowano oddzielne wiersze pomieszczenia, stanu i pomiarów obok przycisku po prawej. Właściciel wybrał jeden wiersz z przewijaną nazwą, zatwierdził osobne wiersze na kartach oraz doprecyzował korektę pomiarów: w Cyber Blue tekst nie był wyśrodkowany między górą i dołem ramki. Przyciski zaakceptował.

Walidacja podglądu: `DSR_UI_FOCUS_DASHBOARD=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden przypadek tylko dla Dashboardu przekaźników/gniazdek, trzy skórki i PL/EN/DE, dziewięć zapisanych klatek po niemiecku. PASS potwierdza wykonanie scenariusza i obecność pomiarów, nie rozwiązuje opisanych wad wizualnych. Przejrzano rzeczywiste rendery LVGL 9.3. Bez ponownego przeglądu zaakceptowanych kart, pełnego atlasu, CI i nowego DEV.

### UI-05 / UI-06 — przewijana nazwa i osobne wiersze kart

Wdrożono wybrany układ. Selektor zachowuje 146×44 px i sąsiednie akcje 44 px w jednym wierszu. Długie nazwy przesuwają się poziomo w swoim polu bez wielokropka; natywna animacja LVGL zatrzymuje się na końcach i wraca. Dłuższa nazwa korzysta z fontu 14 px zamiast zmniejszania do 12 px i ucinania po dwóch liniach. Usunięcie ekranu usuwa również animację.

Karty urządzeń z przełączanym wyjściem mają pełne pomieszczenie w osobnym, zawijanym wierszu, pod nim stan oraz pomiary. Wysokość karty wynika z wysokości tekstu, z odstępem pod ramką pomiarów. Dotyczy przekaźników, gniazdek i świateł powiązanych z endpointem Switch; zaakceptowane układy termostatu, kontaktów i rolety nie zostały przebudowane. Pełne pomieszczenie jest rysowane w istniejącej warstwie karty, bez dodatkowego obiektu etykiety. W/V mają wysokość tekstu dopasowaną do fontu i pionowy środek ramki zarówno po utworzeniu, jak i aktualizacji wartości. Przyciski zachowują zaakceptowany rozmiar i położenie.

Walidacja poprawki: ten sam tryb `DSR_UI_FOCUS_DASHBOARD=1` — PASS, trzy skórki i PL/EN/DE, 17 wybranych klatek (w tym osiem klatek fragmentu przewijania Cyber Blue). Regresja na poprzednim kodzie wykazała brak przewijania nazwy. Sprawdzono pełną nazwę pomieszczenia w zadaniach rysowania LVGL, odstępy pomieszczenie–stan–pomiary, pionowy środek tekstu W/V przed i po zmianie liczb, rzeczywisty ruch animacji i jej usunięcie po wyjściu. Obejrzano rzeczywiste rendery LVGL 9.3. Właściciel potwierdził „Tak, układ Dashboardu zaakceptowany” oraz „Tak, pomiary zaakceptowane”. Akceptacja obejmuje pokazany układ i pomiary; bez nowego DEV i fizycznego odbioru.

### UI-04 — listy ustawień i przewijanie

Właściciel wybrał przegląd ustawień. Sprawdzono menu główne, podmenu System oraz Sieć i połączenia w trzech skórkach i PL/EN/DE. Rzeczywiste zdarzenia wskaźnika przewijają menu do całej pozycji System nad dolną nawigacją; dotknięcie tej pozycji otwiera właściwe podmenu. W ustawieniach sieci ostatni przycisk Odśwież jest w pełni dostępny nad stałą stopką Wstecz. Sprawdzono również powrót między ekranami.

W pokazanych stanach nie wykryto kolizji list ze stopką ani agresywnego skracania nazw. Dłuższe niemieckie etykiety zawijają się w wierszach. Nie zmieniano kodu interfejsu; dodano ograniczony scenariusz przeglądu z kontrolą odstępów etykiet od ikon i strzałek oraz dostępności końca listy.

Walidacja: `DSR_UI_FOCUS_SETTINGS=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS; jeden przypadek ograniczony do ustawień, trzy skórki i PL/EN/DE, 15 wybranych klatek po niemiecku. Obejrzano rzeczywiste rendery LVGL 9.3. Nie oznacza to odbioru pozostałych ekranów ustawień ani fizycznego dotyku na pilocie. Bez pełnego atlasu, ponawiania zaakceptowanych kart, pełnego CI i nowego DEV.

Odbiór 2026-10-23: dla głównej listy oraz pokazanych podmenu System i Sieć właściciel wybrał „Wymagają korekty — opiszę”. Oba punkty pozostają otwarte do czasu doprecyzowania uwag i poprawionego podglądu. Nie zmieniano układu ani nie ponawiano testów na podstawie samej tej odpowiedzi.

### UI-04 — sześć pozycji bez przewijania i ikona dodawania Shelly

Właściciel doprecyzował dwie zmiany: nieco mniejsze ramki i odstępy mają zmieścić wszystkie sześć sekcji ustawień na jednym ekranie, a Netzwerk und Verbindungen ma wrócić do jednej linii; na podstronie brakuje ikony przy Shelly-Gerät hinzufügen.

Główne menu używa teraz przycisków 48 px z odstępem 6 px. Cała lista mieści się nad nawigacją, z 24 px odstępu pod ostatnim przyciskiem. Zachowano marginesy ikony, tekstu i strzałki oraz podwójne sfazowane ramki Cyber Blue. Etykiety zachowują font 16 px, a dłuższe napisy korzystają z 14 px po pomiarze ich szerokości; nazwa sieci jest pełna i jednoliniowa w PL/EN/DE. Lista nie wymaga przewijania. W Cyber Blue przycisk dodawania Shelly dostał istniejącą ikonę plus, tak jak pozostałe akcje tej skórki. Podmenu System oraz przewijana zawartość Sieci zachowują dotychczasowy układ.

Kontrola gestów wykryła, że domyślna blokada naciśnięcia LVGL na nieruchomej liście mogła otworzyć pierwotnie dotkniętą pozycję po przeciągnięciu poza nią. Wyłączono ją lokalnie dla sześciu przycisków menu. Przeciągnięcie nie zmienia już ekranu, a zwykłe dotknięcie Systemu i Sieci nadal otwiera odpowiednie podmenu.

Walidacja poprawki: `DSR_UI_FOCUS_SETTINGS=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, trzy skórki i PL/EN/DE, 12 wybranych klatek po niemiecku. Regresja przed poprawką wykazała brak widoczności wszystkich sekcji. Sprawdzono dostępność sześciu pozycji bez gestu, jednoliniową nazwę sieci, odstępy tekstu od ramek, zachowanie przy przeciąganiu, wejście do Systemu/Sieci, powrót i dostępność ostatniej akcji podmenu Sieć. Obejrzano rzeczywiste piksele LVGL 9.3. Właściciel potwierdził „Tak, te poprawki zaakceptowane” oraz wyraził zgodę na wysłanie commita `d4683ba`; commit jest na `work/dev-0.7.70`. Akceptacja dotyczy pokazanego menu i poprawionej ikony; bez pełnego atlasu, pełnego CI, nowego DEV i odbioru na urządzeniu.

### UI-10 / UI-11 — dimmer i RGB: przegląd wyglądu i sterowania

Właściciel wybrał ten punkt po zaakceptowaniu ustawień. Przygotowano rzeczywiste podglądy dimmera, zmiany jasności, RGB, edycji wartości R/G/B i stanu offline w trzech skórkach. Cyber Blue używa pokrętła jasności oraz koła barw, a Grafit i Jasny suwaków. Nie zmieniano kodu produktu w tej rundzie przeglądu.

Wykryte niespójności:

- Akcja włączenia/wyłączenia RGB w Cyber Blue ma tylko 4 px do dolnej nawigacji i wysokość 40 px. Przyciski Powrót/Zastosuj w edytorze R/G/B mają 8 px do nawigacji. Dla porównania zaakceptowany ekran rolety zachowuje 16 px odstępu dolnych akcji.
- Helper `cyber_box` celowo rysuje pojedynczy cienki kontur. Wykorzystują go moduły pomiarów i szybkich ustawień dimmera oraz panele RGB. To pozostałość poprzedniego wyglądu, niespójna z późniejszym zleceniem wspólnych podwójnych, sfazowanych ramek Cyber Blue.
- Grafit i Jasny zachowują techniczne nagłówki Light RPC / RGB RPC. Nie pokazują w szczegółach dimmera pomiarów W/V/A, przycisków −/+ i szybkich poziomów, a w RGB nie mają gotowych kolorów ani koła barw dostępnych w Cyber Blue. Podstawowe ustawienie jasności i wartości R/G/B działa; braki dotyczą wygody i zakresu interfejsu. Wybór między przeniesieniem układu Cyber Blue z kolorystyką skórek a uzupełnieniem obecnych suwaków wymaga oceny właściciela.

Walidacja: `DSR_UI_FOCUS_LIGHTING=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS; jeden przypadek ograniczony do dimmera i RGB, trzy skórki i PL/EN/DE, 18 wybranych klatek po niemiecku. Sprawdzono brak wysyłania jasności podczas przeciągania, pojedynczą akcję po puszczeniu, odrębne potwierdzenie wartości przez model, koło barw Cyber Blue bez wysyłania podczas przeciągania, brak zapisu suwaków R/G/B przed Zastosuj, prawidłowe trzy składowe po Zastosuj oraz blokadę edycji i wysyłania offline. Podgląd uwzględnia dostępność akcji Zużycie/Sceny tak jak wiązanie produkcyjne; nawigacja do tych modułów nie jest odbierana w tym scenariuszu. Obejrzano rzeczywiste piksele LVGL 9.3. Dostawcy akcji i odczytu są modelami hosta, więc nie potwierdza to transmisji RPC do fizycznego Shelly. Bez pełnego atlasu, ponawiania zaakceptowanych ekranów, pełnego CI i nowego DEV. Ocena właściciela pozostaje otwarta.

### UI-10 / UI-11 — wspólny dimmer i RGB dla trzech skórek

Właściciel wybrał „Przenieść układ z pokrętłem i kołem barw z Cyber Blue” do Grafit i Jasny oraz potwierdził „Tak, rozmiary sterowania odpowiadają”. Te rozmiary pozostały: promień pokrętła 70 px, koła barw 64 px, przyciski −/+ 44 px i dotychczasowe suwaki R/G/B.

Wspólny renderer obsługuje teraz trzy skórki: pomiary W/V/A, pokrętło, zmianę jasności co 5 punktów, szybkie poziomy 10/25/50/100%, koło barw, cztery gotowe kolory oraz edytor R/G/B z Zastosuj. Grafit zachowuje ciemne powierzchnie i niebieski akcent, Jasny jasne powierzchnie i ciemniejszy kolor ostrzeżenia dla kontrastu. Cyber Blue używa wspólnego sfazowanego, podwójnego konturu paneli. Ekrany nie pokazują już technicznych nagłówków Light RPC / RGB RPC. Potwierdzony przełącznik stanu pokazuje właściwy kolor od razu po wejściu na ekran.

Dolny przycisk RGB ma wysokość 44 px i pozycję y=368; przyciski powrotu i Zastosuj mają wysokość 48 px i y=364. W obu widokach pozostaje 16 px do nawigacji. Dopasowano środkowy moduł jasności i odstępy gotowych kolorów. Odczyty W/V/A dostały bezpieczny odstęp od wewnętrznego konturu małych ramek. Usunięto zastąpione gałęzie dwóch starych widoków i ich nieużywaną etykietę podglądu; backend sterowania pozostaje wspólny, a ręczne odświeżenie jest nadal dostępne w ustawieniach urządzenia.

W regresjach Light/RGB zachowano sprawdzenie rzeczywistego wiązania urządzenia, wysyłania po puszczeniu, zatwierdzania koloru, zachowania podglądu przy zmianie języka/odpytywaniu, anulowania po utracie połączenia i blokady przejścia/kalibracji. Test Light korzysta teraz z rzeczywistego pokrętła, a RGB z aktualnych przycisków Kolor/Powrót. Nowy scenariusz podglądu sprawdza szybkie akcje we wszystkich skórkach, geometrię pomiarów oraz odstęp dolnych akcji.

Walidacja poprawki: `DSR_UI_FOCUS_LIGHTING=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, trzy skórki i PL/EN/DE, 18 wybranych klatek po niemiecku. Regresja przed zmianą wykazała brak pokrętła w Grafit. `python3 tools/dev/run_host.py test_ui.py --jobs 4 --cases test_light_ui test_rgb_ui` — PASS obu istniejących regresji; kontrola pamięci po 40 cyklach nawigacji w każdym przypadku nie wykazała ubytku. Przy końcowej korekcie kontrastu wskaźnika i geometrii pomiarów ponawiano tylko scenariusz podglądu, nie cały zestaw. Użyto zgodnego z LVGL 9.3 deskryptora przejścia zeroczasowego zamiast nieobsługiwanego pustego wskaźnika. Obejrzano rzeczywiste piksele końcowych renderów LVGL 9.3. Nowy podgląd oczekuje na ocenę właściciela; akcje sieciowe są modelami hosta, bez nowego DEV, pełnego atlasu, pełnego CI i fizycznego testu Shelly.

Odbiór tej rundy: właściciel potwierdził „Tak, dimmer zaakceptowany na podglądzie” oraz „Tak, RGB zaakceptowane na podglądzie”. Akceptacja obejmuje pokazane widoki trzech skórek, pomiary, pokrętło i −/+, szybkie poziomy, koło barw i gotowe kolory, edytor R/G/B, stan offline, ramki i odstęp dolnych akcji. Zmiany są zapisane lokalnie w commicie `6f5b077`; nie opublikowano nowego DEV ani nie potwierdzono jeszcze działania na fizycznym Shelly.

### UI-02 — status bar: bateria, zegar i Wi-Fi

Po akceptacji dimmera i RGB właściciel wybrał status bar. Przygotowano rzeczywiste wycinki LVGL 9.3 dla Grafit, Jasny i Cyber Blue: Dashboard, ustawienia z powrotem oraz szczegóły dimmera z długą nazwą urządzenia. Nie zmieniano kodu produktu w tej rundzie.

Procent pozostaje wewnątrz obrysu baterii, także dla najdłuższego `~100%`. W tym samym miejscu mieszczą się `0%`, `5%`, napięcie `3.90V` i kreska dla błędu/odczytu starszego niż 30 s. Sprawdzono środki pola tekstowego, odstęp od Wi-Fi, miejsce dla zegara `23:59` oraz brak kolizji z przyciskiem powrotu i następnym wierszem treści. Niski poziom ma czerwony obrys. Zmienność wypełnienia oraz zatrzymanie podczas uśpienia sprawdzono na modelu dostawcy stanu ładowania; nie potwierdza to wykrywania ładowarki przez sprzęt.

Ujawniona niespójność: w szczegółach dimmera/RGB nazwa między zegarem i Wi-Fi ma 92 px szerokości. Dla „Deckenbeleuchtung Küche” dwa wiersze fontu 12 łamią pierwsze słowo. Tekst nie nachodzi na zegar ani Wi-Fi, ale jest trudniej czytelny. Do oceny właściciela pozostaje zachowanie dwóch linii albo przewijanie jednego wiersza, jak w zaakceptowanym wyborze Dashboardu; nie zmieniano zaakceptowanego układu sterowania.

Ikona Wi-Fi w status barze obecnie rozróżnia połączenie i jego brak kolorem (akcent/szary). Wspólny nagłówek otrzymuje tylko `bool wifi`, nie RSSI. Ten przegląd nie udaje wizualizacji siły sygnału ani nie rozszerza backendu.

Walidacja: `DSR_UI_FOCUS_STATUS=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, wyłącznie przypadek status bara dla trzech skórek i PL/EN/DE. Obejrzano rzeczywiste piksele normalnego nagłówka, nagłówka z długą nazwą, stanów baterii i dwóch faz wypełnienia. Podglądy ograniczono do paska oraz sąsiedztwa nagłówka; bez pełnego atlasu, pełnego builda i CI. Ocena właściciela pozostaje otwarta; nowy DEV po zakończeniu uzgodnionych poprawek i walidacji wydania.

### UI-02 — akceptacja status bara i usunięcie nazwy przy zegarze

Właściciel potwierdził „Tak, status bar zaakceptowany na podglądzie” i zamiast przewijania nazwy dimmera/RGB wybrał „usuńmy tę nazwę”. Nagłówki szczegółów dimmera i RGB pokazują teraz powrót, zegar, Wi-Fi oraz baterię, bez nazwy urządzenia pomiędzy nimi. Zmiana dotyczy wszystkich trzech skórek.

Walidacja poprawki: `DSR_UI_FOCUS_STATUS_NAME=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, tylko nagłówki dimmera i RGB, trzy skórki i PL/EN/DE, sześć wybranych klatek. Obejrzano rzeczywiste piksele LVGL 9.3; nie ponawiano pozostałych stanów status bara ani zaakceptowanych ekranów sterowania. Poprawiono również dane scenariusza: Light/RGB są alternatywnymi polami unii modelu, więc wypełnia się je osobno. To zmiana kodu roboczego, bez nowego DEV i odbioru na fizycznym pilocie.

### UI-03 — dolna nawigacja: ikony, odstępy i dotyk

Właściciel wybrał przegląd dolnej nawigacji. Dashboard ma ikonę domu, Pomieszczenia siatkę, trzeci przycisk otwiera „Bateria i zasilanie”, dalej są Ustawienia i kontekstowe „+”. Po przedstawieniu znaczenia trzeciego przycisku właściciel wybrał **„Pozostawić ikonę procesora”**; nie zmieniano tej ikony ani przypisanej funkcji.

Ukierunkowany scenariusz rzeczywistego dotyku potwierdził w trzech skórkach dwa błędy: szczegóły otwarte z Dashboardu podświetlały Pomieszczenia, a przeciągnięcie poza „+” nadal uruchamiało dodawanie. Poprawiono zachowanie wspólnego paska: szczegóły i ustawienia urządzenia zachowują kontekst strony wejścia, również przy bezpośrednim otwarciu ustawień urządzenia; wyjście palca poza pole anuluje dotknięcie. Niedostępne akcje są teraz przygaszone i mają kolor nieaktywnej ikony. Samo przygaszenie rodzica nie wystarczało dla ręcznie rysowanych ikon, dlatego korekta koloru jest lokalna dla paska.

Sprawdzono pięć nieprzecinających się pól 64×47 px, ikonę „+” 42 px wewnątrz większego pola dotyku, stany aktywne Dashboard/Pomieszczenia/Ustawienia, szczegóły i powroty do strony wejścia, pojedyncze wywołanie po dotknięciu, punkty na granicy sąsiednich pól, przeciągnięcie do treści i sąsiedniego przycisku bez przypadkowej akcji, dodawanie urządzenia versus pomieszczenia oraz brak działania niedostępnych przycisków. Kontrola pikseli potwierdza odmienny wygląd niedostępnych akcji w każdej skórce.

Walidacja: `DSR_UI_FOCUS_NAVIGATION=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, tylko przypadek dolnej nawigacji, trzy skórki i 18 wybranych klatek z niemieckim interfejsem. Pasek nie ma etykiet zależnych od języka. Regresja przed poprawką wykazała oba błędy interakcji; po korekcie koloru ikon ponowiono tylko ten sam scenariusz. Obejrzano rzeczywiste piksele LVGL 9.3. Dostawcy operacji są modelami hosta; nie uruchamiano pełnego atlasu, pełnej regresji, builda docelowego ani CI. Wygląd czeka na ocenę właściciela, a odbiór na pilocie po nowym DEV pozostaje otwarty.

Dodatkowa uwaga z sąsiedztwa paska: dolne pola napięcia i natężenia w szczegółach przekaźnika Cyber Blue kończą się na y=425, a nawigacja zaczyna na y=428, więc odstęp ma tylko 2 px. Nie ma nakładania, ale odstęp jest znacznie mniejszy od 16 px zaakceptowanych przy dolnych akcjach rolety/RGB. Ten układ pomiarów pozostawiono do osobnej oceny; pokazano właścicielowi rzeczywisty fragment ekranu.

### UI-03 — akceptacja nawigacji i odstęp 16 px przy pomiarach przekaźnika

Właściciel potwierdził „Tak, dolna nawigacja zaakceptowana na podglądzie” oraz „Tak, zwiększyć do 16 px” dla pól V/A w szczegółach przekaźnika Cyber Blue. Odbiór samego paska jest zamknięty na poziomie podglądu; po wykonaniu poprawki zaakceptowano także układ sąsiedniego ekranu, jak odnotowano poniżej.

Pola V/A przesunięto na y=364 przy wysokości 48 px, co daje dokładnie 16 px pustego miejsca do nawigacji na y=428. Wiersz ostatniego odczytu przesunięto na y=328, zachowując 8 px odstępu nad polami V/A. Główny panel ma wysokość 192 px, a napis Włącz/Wyłącz wykorzystuje wolną przestrzeń po lewej stronie przycisku zasilania i jest wyśrodkowany na jego osi. Przycisk odświeżania wyrównano w tym samym wierszu. Układ mieści duży przycisk 86 px i dotychczasowe rozmiary tekstów pomiarowych.

Walidacja: `DSR_UI_FOCUS_RELAY_SPACING=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, tylko szczegóły przekaźnika Cyber Blue dla PL/EN/DE, siedem wybranych klatek. Sprawdzono odstęp 16 px, brak kolizji paneli, pomiarów i opisu akcji, pełne napisy Włącz/Wyłącz w trzech językach, pojedyncze wywołanie sterowania i odświeżania, potwierdzenie stanu po aktualizacji modelu oraz stany oczekiwania, offline i braku odczytu. Obejrzano rzeczywiste piksele LVGL 9.3. Operacje i odczyty Shelly są modelami hosta. Bez ponawiania zaakceptowanego paska, innych skórek, pełnego atlasu, builda docelowego i CI; odbiór na pilocie po nowym DEV pozostaje otwarty.

**Odbiór układu przekaźnika:** właściciel potwierdził „Tak, ten układ zaakceptowany na podglądzie” dla wariantu PL/EN/DE z odstępem V/A 16 px, wierszem ostatniego odczytu wyżej oraz napisem Włącz/Wyłącz po lewej stronie przycisku zasilania. Zamyka to UI-03 na poziomie podglądu. Zmiana kodu jest zapisana w lokalnym commicie `3ede008`; odbiór na pilocie po nowym DEV pozostaje otwarty. Przy zapisie tej akceptacji sprawdzono tylko dokumentację i diff, bez ponawiania testów firmware ani renderów.

### NET-01 — konfiguracja i zapis urządzenia

Po akceptacji nawigacji i szczegółów przekaźnika właściciel wybrał formularz konfiguracji i zapis. Przygotowano rzeczywiste podglądy LVGL 9.3 w trzech skórkach z niemieckimi etykietami: nazwa, pomieszczenie i kanał zgłoszony przez API; weryfikacja; niepotwierdzony zapis i ponowienie; zakończenie dodawania z nową kartą na Dashboardzie; ponowne logowanie oraz brak obsługiwanego kanału. Użyto wyłącznie danych przykładowych i dostawców hosta, bez odczytu danych pilota.

**Wykryty i poprawiony błąd:** błąd uwierzytelniania podczas weryfikacji zapisu powodował utratę wpisanej nazwy, wybranego pomieszczenia i kanału po ponownym logowaniu. Formularz zachowuje teraz te wartości dla tego samego wybranego urządzenia. Aktualna lista możliwości API pozostaje rozstrzygająca: usunięty kanał nie jest przywracany. Szkic jest czyszczony po opuszczeniu kreatora, ponownym skanowaniu, wybraniu innego urządzenia lub zakończeniu dodawania. Nie obejmuje haseł i nie zmienia modelu trwałego zapisu.

Walidacja: `DSR_UI_FOCUS_ADD_SAVE=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, jeden przypadek, 257 sprawdzeń i 27 wybranych klatek, trzy skórki, język DE. Przed poprawką test wykazał utratę szkicu we wszystkich trzech skórkach. Po korekcie sprawdzono zachowanie pól po logowaniu, odrzucenie nieaktualnego kanału, czyszczenie szkicu dla innego urządzenia, blokadę podwójnego zapisu, brak zmiany dotychczasowej konfiguracji przy symulowanym błędzie jej zapisu oraz udane ponowienie. Osobne potwierdzenie zadania publikacji prywatnego połączenia poprzedza powrót do Dashboardu; wcześniejsze urządzenie pozostaje w konfiguracji. Test korzysta z rzeczywistej ścieżki UI i modelu zapisu, lecz radio, pamięć i odpowiedzi Shelly zastępują dostawcy hosta. Nie potwierdza to działania na fizycznym urządzeniu. Nie ponawiano pełnego atlasu, regresji ani builda docelowego i CI.

**Niespójności wizualne pozostawione do oceny w ankiecie:**

- Błąd zapisu i komunikat o nieobsługiwanym typie używają małego, przygaszonego tekstu. Brakuje wyróżnienia ważnej informacji, podobnego do zaakceptowanej złotej ramki AP.
- W Cyber Blue nieaktywne „Zapisz” nadal wygląda jak aktywna niebieska akcja; dotknięcie jest zablokowane. Grafit i Jasny pokazują przygaszenie.
- Po przewinięciu do komunikatu górna część pola nazwy wychodzi poza widoczny obszar formularza. Nie zachodzi na kartę tożsamości, ale widoczny fragment wymaga oceny wyglądu.
- Podczas oczekiwania na końcowe potwierdzenie nadal widnieje ogólny tekst sprawdzania przed zapisem. Po sukcesie następuje powrót do Dashboardu z nową kartą, bez osobnego okna sukcesu.

**Odbiór:** pierwszy podgląd formularza i przebiegu zapisu wymagał korekty. W kolejnym zgłoszeniu właściciel wskazał także dublowanie informacji o pomiarach na karcie Dashboardu po zapisie, opisane poniżej. Po poleceniu „dalej?” dopracowano wyróżnienie błędów formularza i nieaktywnego „Zapisz”; poprawiony etap konfiguracji i zapisu został następnie zaakceptowany na podglądzie, jak odnotowano w następnej rundzie. Wcześniejsze akceptacje pokazanych ekranów LAN, Bluetooth i AP pozostają w mocy. NET-01 jako całość, pozostałe kroki konfiguracji AP/BLE i odbiór na pilocie po nowym DEV pozostają otwarte.

### UI-06 — pojedynczy komunikat o niedostępnych pomiarach

Właściciel wskazał podwójny komunikat na pokazanej karcie Dashboardu: „Messwert veraltet” w wierszu stanu i „Keine Messwerte” w ramce pomiarów. Zlecił pozostawienie tylko komunikatu w ramce. Dodatkowy napis o nieaktualnym odczycie jest teraz pomijany, jeżeli karta ma już ramkę informującą o niedostępnych pomiarach. Zmiana dotyczy wspólnych kart w trzech skórkach i PL/EN/DE; nie zmienia geometrii zaakceptowanych kart.

Osobne przyczyny, takie jak brak Wi-Fi, brak odpowiedzi, błąd logowania i oczekiwanie na pierwszy odczyt, pozostają widoczne. Na karcie z wyłączonymi polami pomiarowymi zachowano jedyny komunikat o nieaktualnym odczycie. Po ponownym uzyskaniu odczytu wracają zwykły stan oraz pomiary W/V.

Walidacja: `DSR_UI_FOCUS_CARD_NOTICE=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, 83 sprawdzenia i trzy wybrane klatki, trzy skórki i PL/EN/DE. Najpierw poprawiono dane scenariusza karty bez ramki; prawidłowy test przed zmianą kodu następnie wykazał duplikat we wszystkich dziewięciu wariantach skórki/języka. Po poprawce przeszedł ten sam przypadek. Obejrzano rzeczywiste piksele LVGL 9.3. Dostawcy radia i odczytów są modelami hosta; nie ponawiano pozostałych ekranów, atlasu, pełnej regresji ani builda docelowego i CI. Kod pozostaje roboczy, bez nowego DEV i odbioru na pilocie. Późniejszy odbiór poprawionego formularza konfiguracji opisano poniżej.

### NET-01 — złote komunikaty, nieaktywne „Zapisz” i przewinięcie formularza

Kontynuowano poprawki po poleceniu właściciela „dalej?”. Błędy walidacji, zapisu i logowania oraz informacja o nieobsługiwanym urządzeniu mają teraz złoty tekst 14 px w ramce. Cyber Blue korzysta z istniejącego helpera sfazowanych ostrzeżeń AP; Grafit i Jasny zachowują swoje powierzchnie i odpowiedni kontrast. Zwykły opis i trwająca weryfikacja nadal mają neutralny wygląd.

Wyróżniony komunikat znajduje się pod polami w przewijanym formularzu. Wysokość ramki wynika z długości tłumaczenia. Automatyczne przewinięcie ustawia początek pełnego wiersza, tak aby nie pozostawiać przypadkowego fragmentu poprzedniego pola. Początkowy widok formularza i pełna wysokość pola kanału pozostają zachowane. Podgląd ujawnił zbyt duże przewinięcie po błędzie logowania mimo poprawnych granic samej ramki; skorygowano je i dodano kontrolę widoczności pola hasła wraz z komunikatem.

Wyłącznie przycisk zapisu tego formularza w Cyber Blue otrzymał ciemne tło, przygaszony podpis, ikonę i kontur w stanie nieaktywnym. Po nieudanym zapisie odzyskuje aktywny niebieski wygląd. Blokada powtórnego wysłania pozostaje zachowana. Nie zmieniano zaakceptowanej nawigacji ani pozostałych akcji skórki.

Walidacja: `DSR_UI_FOCUS_ADD_SAVE=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, 338 sprawdzeń i 27 klatek, zapis w trzech skórkach z etykietami DE. Następnie uruchomiono ten sam, świeżo zbudowany plik `build-ui/native/test_dev_069_ui` z `DSR_UI_FOCUS_LAN_FORM=1`, bez powtarzania kompilacji — PASS, 597 sprawdzeń i dziewięć klatek, formularz w trzech skórkach i PL/EN/DE. Sprawdzono pełną ramkę i tekst w obszarze przewijania, widoczne pole hasła po błędzie logowania, zamknięcie klawiatury, zachowanie szkicu, nieudany zapis i ponowienie. Dodatkowa kontrola pikseli początkowego renderu wykazała brak złotego konturu, a po poprawce potwierdziła jego obecność. Obejrzano rzeczywiste piksele LVGL 9.3: błędy zapisu, logowania, brak kanału, stan oczekiwania i przygaszone „Zapisz”. Pierwsza kompilacja wykryła brak nagłówka helpera Cyber Blue; dodano właściwy nagłówek, bez wyciszania ostrzeżenia. Dostawcy radia i zapisu są modelami hosta. Bez pełnego atlasu, pozostałych regresji, builda docelowego i CI.

**Odbiór:** właściciel potwierdził „Tak, ten etap zaakceptowany na podglądzie”. Akceptacja obejmuje wspólny formularz nazwy/pomieszczenia/kanału i przebieg zapisu z powrotem do Dashboardu, zachowanie wpisów po ponownym logowaniu, złote ramki błędów, czytelne nieaktywne „Zapisz” i jednocześnie widoczne pole hasła oraz komunikat logowania. Zmiana kodu jest zapisana w lokalnym commicie `8b89e18`. Ogólny opis oczekiwania na końcowe potwierdzenie i powrót po sukcesie bez osobnego okna pozostają w pokazanej postaci. NET-01 jako całość, pozostałe kroki konfiguracji AP/BLE i odbiór na fizycznym pilocie pozostają otwarte. Przy zapisie tej akceptacji sprawdzono dokumentację i diff, bez ponawiania testów firmware i renderów oraz bez nowego DEV.

### CFG-01 — pomieszczenia i dashboardy: tworzenie, edycja i usuwanie

Po zaakceptowaniu konfiguracji i zapisu urządzeń właściciel wybrał pomieszczenia i dashboardy. Przygotowano aktualne piksele LVGL 9.3 dla Grafit, Jasny i Cyber Blue z etykietami DE: lista pomieszczeń, tworzenie i edycja nazwy/ikony/zdjęcia, nieudany zapis, odrzucenie zmian, odmowa usunięcia używanego pomieszczenia i potwierdzenie usunięcia; wybór dashboardu, lista, nowy dashboard, edycja kart, potwierdzenie usunięcia oraz ochrona ostatniego dashboardu. Dane są przykładowe; nie usuwano danych pilota.

Ukierunkowany przebieg sprawdził tworzenie i zmianę nazw, zachowanie dotychczasowych danych po błędzie zapisu, udane ponowienie, odrzucenie niezapisanej zmiany nazwy pomieszczenia oraz wybór zdjęcia. Usunięcie wymaga potwierdzenia i Zapisz, a przed zapisem dokument pozostaje bez zmian. Pomieszczenie z urządzeniem i kartą nie może zostać usunięte. Po usunięciu aktywnego dashboardu ustawiany jest istniejący dashboard zastępczy; ostatni dashboard pozostaje chroniony. Pozostałe pomieszczenia i urządzenie testowe zostały zachowane.

**Wykryta kolizja i poprawka:** w edytorze dashboardu Grafit/Jasny przyciski zaczynały się 22 px pod nazwą karty, nawet gdy nazwa zajmowała dwie linie. Rzeczywisty render wykazał nachodzenie nazwy na przyciski we wszystkich PL/EN/DE. Cyber Blue rezerwował już 40 px na nazwę i 48 px przed pierwszym rzędem akcji. Wspólny edytor stosuje teraz tę geometrię we wszystkich skórkach, z odpowiednim odstępem przed następną kartą. Kolorystyka i funkcje przycisków pozostały zachowane.

**Niespójności pierwszego przeglądu** (późniejsze korekty i odbiór opisano w kolejnej rundzie):

- „Zapisz” w formularzach Grafit/Jasny ma wygląd zwykłej szarej akcji, podobny do Wstecz; zaakceptowany kreator dodawania urządzeń wyróżnia zapis na niebiesko.
- Błąd zapisu i ochrona ostatniego dashboardu używają małego tekstu nad formularzem. Brakuje wyróżnienia ważnych informacji, takiego jak złote ramki zaakceptowanego kreatora.
- Edytor dashboardu ma ogólny nagłówek konfiguracji i wiele skróconych akcji Obiekt/Układ/Typ/Wys./Dane. Wymaga oceny czytelności i wygody na dotyku.
- Z kodu wynika różne zachowanie powrotu: formularz pomieszczenia pyta o odrzucenie zmian, a ogólna edycja dashboardu przechodzi przez listę i menu; wyjście do Home czyści szkic bez takiego dialogu. Docelowy przebieg tego wyjścia wymaga dopracowania.

Walidacja pierwszego przeglądu: `DSR_UI_FOCUS_ROOM_DASHBOARD=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, 436 sprawdzeń i 39 wybranych klatek, trzy skórki, etykiety DE, bez pełnego atlasu i regresji innych modułów. Następnie `DSR_UI_FOCUS_CONFIG_LONG=1` z tym samym poleceniem — po poprawce PASS, 170 sprawdzeń i trzy klatki; tylko układ długich nazw kart, trzy skórki i PL/EN/DE, w tym odstęp następnej karty oraz dostępność dolnej akcji po przewinięciu. Pierwszy wariant danych testowych wymagał zmiany nazwy dashboardu, aby helper nie traktował jej jako etykiety tłumaczonej. Prawidłowy test przed zmianą kodu wykazał sześć kolizji w Grafit/Jasny; po zmianie ponowiono tylko ten scenariusz. Przeglądu CRUD nie powtarzano po korekcie samej geometrii edytora kart. Obejrzano rzeczywiste piksele LVGL 9.3 przed i po poprawce.

Odbiór CFG-01 pozostaje otwarty. Dostawcy zapisu i radia są modelami hosta; przegląd nie potwierdza trwałości na fizycznej pamięci po restarcie. Nie publikowano nowego DEV.

### CFG-01 — kolejność Dashboardu i Urządzeń, opisy kontrolek i złote komunikaty

**Decyzja właściciela:** edycja i usuwanie pomieszczeń są zaakceptowane. Właściciel wskazał nieczytelne lub pozornie nieczynne Obiekt/Breite/Typ/Höhe, brak widocznych przycisków kolejności oraz potrzebę złotego koloru ważnych tekstów. Doprecyzował: „Musimy wykonać taką opcję zmiany kolejności dla Dashboard oraz dla urządzeń”. Akceptacja pomieszczeń nie oznacza automatycznej akceptacji ich tworzenia ani całego CFG-01.

- **Dashboard:** Góra/Dół są teraz pierwszym rzędem pod nazwą karty. Pierwsza karta ma nieaktywne Góra, ostatnia Dół. Przesunięcie przez granicę strony edytora przenosi widok do przesuniętej karty. Zapis zmienia kolejność kart tego dashboardu, niezależnie od listy Urządzeń.
- **Urządzenia:** ołówek w zakładce Urządzenia otwiera listę z nazwą i przyciskami Góra/Dół przy każdym wpisie. Nazwy mogą zajmować więcej wierszy. Zapis utrwala kolejność tablicy urządzeń przez dotychczasowy mechanizm rewizji i potwierdzenia zapisu. Identyfikatory, kanały, powiązania i kolejność kart dashboardów pozostają zachowane. Po potwierdzeniu widoczna jest nowa kolejność listy. Powrót z niezapisanymi zmianami oferuje Zostań/Odrzuć; podczas zapisu zmiana kolejności i wyjście są blokowane.
- **Czytelność edytora:** nagłówek Dashboard bearbeiten / Edit dashboard / Edytuj dashboard. Obiekt opisuje teraz „Następny obiekt”, a przy jednym obiekcie jest nieaktywny z podpisem „Tylko jeden obiekt”. Typ, szerokość i rozmiar pokazują bieżące wartości. Szerokość można zmieniać dla kart pomieszczeń; urządzenia, sceny i grupy zachowują pełną szerokość. Dane na karcie są aktywne dla urządzeń. Zmiany wartości zachowują pozycję przewijania. Obszary dotyku mają 48 px wysokości, a strzałki listy Urządzeń 48×48 px. Przeciągnięcie poza przycisk nie wykonuje przesunięcia.
- **Ważne teksty:** błędy i ostrzeżenia wspólnego edytora konfiguracji oraz treść dialogów ochrony pomieszczeń są złote, z ciemniejszym odcieniem w Jasny. Zwykły opis, trwający zapis i udane potwierdzenie są neutralne. Dotychczasowe ramki pozostają; zmiana nie oznacza przeglądu wszystkich komunikatów całego firmware.

Walidacja dotyczyła wyłącznie tej rundy: `DSR_UI_FOCUS_CONFIG_CONTROLS=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_dev_069_ui` — PASS, dziewięć kombinacji trzech skórek i PL/EN/DE, 1059 asercji, 18 wybranych klatek. Przed implementacją ten scenariusz wykazał brak widoku zmiany kolejności Urządzeń. Po wdrożeniu sprawdzono faktyczne kliknięcia, zapis obu kolejności, granicę strony Dashboardu, działanie wyboru obiektu/typu/rozmiaru/szerokości, nieaktywną szerokość urządzenia i dane pomieszczenia, błąd zapisu bez zmiany dokumentu, ponowienie, blokadę podczas zapisu i Zostań/Odrzuć. Sprawdzono zachowanie pozostałych kolekcji konfiguracji. Następnie ten sam plik wykonywalny z `DSR_UI_FOCUS_CONFIG_LONG=1`, bez ponownej kompilacji — PASS, 170 asercji, trzy klatki; długie nazwy kart i dostępność ostatniej akcji w trzech skórkach oraz PL/EN/DE.

Obejrzano rzeczywiste piksele LVGL 9.3: listę Urządzeń, edytor Dashboardu przed i po przesunięciu przez granicę strony, pełne rzędy właściwości, błąd zapisu oraz dialog używanego pomieszczenia. Kontrola wizualna wykryła symbol rolety w miejscu strzałki Cyber Blue; zastąpiono go istniejącymi prostymi strzałkami przed końcowym podglądem. Nie generowano pełnego atlasu, nie uruchamiano pełnego CI ani builda docelowego. Zapis i sieć mają dostawców hosta; trwałość po restarcie pilota wymaga późniejszego odbioru sprzętowego.

**Odbiór:** właściciel potwierdził „Tak, te elementy zaakceptowane na podglądzie” dla Góra/Dół w obu miejscach, podpisów i przygaszenia niedostępnych opcji oraz „Tak, złote komunikaty zaakceptowane” dla ważnych tekstów konfiguracji i ostrzeżeń w trzech skórkach. Kod tej rundy jest zapisany w lokalnym commicie `970ee07`. Przy zapisie akceptacji sprawdzono dokumentację i diff, bez ponawiania testów firmware i renderów. Nadal otwarte: wyróżnienie Zapisz w Grafit/Jasny, ochrona niezapisanych zmian przy wyjściu z ogólnej edycji dashboardów do Home oraz pozostały odbiór CFG-01. Akceptacja podglądu nie zastępuje odbioru na pilocie. Nowy DEV dopiero po zakończeniu ankiety i walidacji commita wydania.

### CTRL-02 — grupy i sceny: tworzenie, edycja i wynik uruchomienia

Właściciel wybrał kolejny punkt ankiety: „Grupy i sceny — tworzenie, edycja i działanie”. Przegląd obejmuje istniejący kreator LCD, bez nowego modelu konfiguracji. Grupy obsługują przekaźniki i gniazdka; ręczne sceny także polecenia rolet. Automatyczne warunki i harmonogramy pozostają poza tym zakresem.

Wykryto i poprawiono:

- Górny powrót w pierwszym kroku omijał ochronę szkicu. Teraz używa tej samej obsługi co Anuluj/Wstecz, z dialogiem Zostań/Odrzuć i blokadą wyjścia podczas zapisu. Po zamknięciu dialogu formularz kończy się nad stopką we wszystkich skórkach.
- Wybór urządzenia do sceny i podsumowanie skracały dłuższe nazwy. Przyciski wyboru zawijają tekst, a wysokość pozycji podsumowania wynika z pomiaru tekstu. Natywny checkbox LVGL rysował nazwę grupowanego urządzenia w jednej linii mimo szerokości pola; otrzymał osobną, zawijaną etykietę w tym samym obszarze dotyku.
- Karty grup/scen bez zdjęcia miały biały tekst także na jasnej powierzchni. Używają teraz koloru tekstu skórki; na zdjęciach zachowano biały tekst i ciemne tło opisu.
- Odwzorowanie zapisanych zaznaczeń rozpoczynało animację od pustego pola, szczególnie nieczytelnego w Jasny. Znany stan jest teraz odtwarzany przed ponownym zastosowaniem motywu i lokalnych stylów; późniejsze dotknięcia zachowują animacje motywu. Cyber Blue ma sfazowane ramki wierszy wyboru.
- Strzałki kolejności scen w Cyber Blue używają prostych symboli góra/dół, z przygaszeniem niedostępnego kierunku. Błędy i niepewne wyniki poszczególnych kroków oraz ważny opis wyniku są złote, z ciemniejszym odcieniem w Jasny. Wyśrodkowano opisy przycisków kart i wykonania.

Walidacja: `DSR_UI_FOCUS_ACTION_REVIEW=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_actions_editor_ui` — końcowy PASS, 922 sprawdzenia, 33 wybrane klatki; trzy skórki i PL/EN/DE, zapisane podglądy DE. Sprawdzono tworzenie i edycję grupy, przypisanie pomieszczenia, wybór dwóch urządzeń, błąd zapisu bez zmiany dokumentu i udane ponowienie; utworzenie sceny, zmianę polecenia, kolejności i zapis. Próby Włącz/Wyłącz grupę i uruchomienia sceny dla niepowiązanych urządzeń modelu kończą się osobnym błędem dla każdego kroku, bez wysłania SET/Cover. Nie jest to potwierdzenie udanego sterowania rzeczywistym Shelly. Test przed zmianami wykazał pomijanie ochrony powrotu i skracanie wyboru w dziewięciu kombinacjach skórki/języka. Obejrzano rzeczywiste piksele LVGL 9.3 przed i po poprawkach, w tym brak kontrastu Jasny, pełne nazwy, zaznaczenia, opcjonalne zdjęcia i złote błędy.

Następnie uruchomiono istniejący `test_actions_editor_ui` bez zmiennej przeglądu, na tym samym pliku wykonywalnym i bez ponownej kompilacji — PASS, 367 sprawdzeń, 27 klatek; między innymi zachowanie późniejszych edycji przy potwierdzeniu wcześniejszego zapisu, nieudany zapis/odrzucenie oraz cykl pamięci edytorów. Nie uruchamiano pełnego atlasu, testów innych modułów, CI ani builda docelowego. W toku diagnozy dopasowano helpery do zakończenia układu LVGL i tłumaczonego licznika. Próba użycia pustego wskaźnika przejścia została odrzucona po awarii testu na LVGL 9.3; wersja końcowa korzysta z publicznego usunięcia stylów i ponownego zastosowania motywu, bez zmiany wersji LVGL ani wyciszania ostrzeżeń.

**Odbiór podglądu:** właściciel zaakceptował pokazany kreator grup oraz listę poleceń, kolejność, podsumowanie i zapis scen. Polecił pozostawić identyfikatory `#3` w wynikach wykonania. Akceptacja dotyczy pokazanych ekranów, nie pełnej funkcjonalności grup.

**Rozszerzenie zlecone po przeglądzie:** nowa grupa ma mieć sterowanie na Dashboardzie i liście Urządzeń, właściwe dla jej członków: przekaźniki, rolety, światła, RGB lub termostaty. Właściciel wybrał oddzielne grupy według sterowania oraz jedną wspólną temperaturę dla grupy termostatów. Wdrożenie jest w toku; stare grupy Włącz/Wyłącz pozostają zgodne. Wykonawca ma zachować indywidualny wynik każdego członka, potwierdzenie odczytem, kontrolę tożsamości i brak automatycznego ponowienia niepewnego zapisu.

**Wymaganie wyglądu doprecyzowane przez właściciela:** grupy mają korzystać z zaakceptowanego designu Dashboardu i Urządzeń. Oddzielny nowy układ kart został odrzucony. W kodzie roboczym obie karty korzystają teraz z jednego renderera: te same ramki, odstępy, ikony, pola pomiarów, przycisk zasilania 58 px oraz ramka 104×56 px z okrągłymi kontrolkami 44 px dla rolety i termostatu. Grupy światła i RGB wykorzystują istniejące pokrętło i koło barw. Dane podglądu grupy nie mają identyfikatora fizycznego urządzenia, hosta ani tożsamości do RPC; osobne callbacki przekazują wyłącznie identyfikator grupy i wspólne polecenie. Wyniki pozostają rozdzielone na członków.

**Odbiór kart grup po korekcie designu:** właściciel potwierdził: „Tak, ten wygląd kart grup odpowiada”. Akceptacja dotyczy pokazanego porównania kart pojedynczego urządzenia i grupy, korzystających ze wspólnego układu i kontrolek Dashboardu oraz Urządzeń. Nie zamyka odbioru pozostałych ekranów sterowania grupą, wspólnego wyboru temperatury ani prób na sprzęcie.

**Korekta szczegółów grupy termostatów:** właściciel wycofał akceptację osobnego formularza wspólnej temperatury i polecił użyć widoku pojedynczego termostatu, dodając dwa wskaźniki baterii oraz dwa wskaźniki zaworu dla dwóch członków. Grupa korzysta teraz z tego samego renderera pokrętła, skali, temperatury i okrągłych przycisków −/+ co pojedynczy TRV w każdej z trzech skórek. Pod pokrętłem są odczyty zaworu i baterii osobno dla każdego termostatu, z nazwami członków; brak odczytu nie jest zastępowany wymyśloną wartością. Pokrętło i −/+ wybierają jedną wspólną temperaturę absolutną, z opóźnieniem 1,2 s jak przy pojedynczym TRV. Podczas przeciągania nie ma wysłania; wyjście z ekranu i zmiana tożsamości/powiązania członka anulują niewysłany szkic. Usunięto zastąpioną ścieżkę formularza TRV, bez zmiany wykonawcy RPC. Właściciel zaakceptował trzy korekty odstępów opisane niżej; zaakceptowany wygląd kart grup pozostaje bez zmian.

Walidacja korekty: `DSR_UI_FOCUS_GROUP_TRV=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_actions_editor_ui` — PASS w LVGL 9.3, Grafit/Jasny/Cyber Blue i PL/EN/DE. Sprawdzono zgodność geometrii pokrętła urządzenia i grupy, oddzielne odczyty dwóch członków, dotyk −/+ i przeciąganie, wspólne polecenie po zwolnieniu, anulowanie po wyjściu/rebindzie oraz dotychczasowe sterowanie pojedynczym TRV. Obejrzano sześć wybranych klatek urządzenie/grupa. Przed poprawką test wykazał brak pokrętła grupy; sondę ikon dostosowano do istniejących stylów Grafit/Jasny, a gest testowy do rzeczywistej zmiany nastawy. Nie uruchamiano pełnego atlasu, pełnego CI ani builda docelowego; przechwytywane polecenia są modelem hosta, nie potwierdzeniem RPC na sprzęcie.

**Doprecyzowanie odstępów termostatów po odrzuceniu podglądu:** właściciel wskazał kolizję ikony Online/Offline z ramką w Cyber Blue, zbyt bliskie dolnej ramki podpisy Min./Max. oraz ikonę zaworu grupy zbyt blisko lewej krawędzi. Wspólny widok pojedynczego TRV i grupy ma teraz ikonę statusu odsuniętą w prawo i w dół, z wyrównanym tekstem. Podpisy Min./Max. podniesiono i zbliżono do wartości 4.0/30.0 °C. W obu polach członków grupy odsunięto ikonę zaworu oraz jej procent od lewego obramowania, zachowując miejsce na baterię. Właściciel potwierdził: „Tak, te trzy poprawki zaakceptowane”. Akceptacja obejmuje pokazane położenie statusu Online/Offline, podpisów Min./Max. i ikon zaworów w termostacie pojedynczym oraz grupie. Pozostałe ekrany sterowania grupami i odbiór sprzętowy pozostają otwarte.

Walidacja odstępów: powtórzono wyłącznie istniejący `DSR_UI_FOCUS_GROUP_TRV=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_actions_editor_ui` — PASS dla trzech skórek i PL/EN/DE. Obejrzano osiem wybranych rzeczywistych klatek LVGL 9.3: termostat/grupa w trzech skórkach oraz dwa stany offline Cyber Blue. Bez pełnego atlasu, pełnego CI, builda docelowego i zmian transportu sterowania.

**Odbiór grup dimmerów i RGB:** właściciel potwierdził: „Tak, widok grupy dimmerów zaakceptowany” oraz „Tak, widoki grupy RGB zaakceptowane”. Akceptacja dotyczy pokazanych ekranów Grafit/Jasny/Cyber Blue: pokrętła, −/+, szybkich poziomów i ramek grupy dimmerów oraz koła barw, jasności, gotowych kolorów i edycji R/G/B grupy RGB. Użyto istniejących podglądów, bez powtarzania renderów ani testów firmware. Odrębny formularz szczegółów grup przekaźników i rolet zastąpiono współdzielonymi widokami pojedynczych urządzeń; odbiór nowych podglądów opisano poniżej. Zakres scen BLE/TRV i czujników, rzeczywiste RPC/readback oraz odbiór na pilocie pozostają otwarte.

Tę akceptację zapisano w [komentarzu do #74](https://github.com/voltex-at/DIY-Shelly-Remote/issues/74#issuecomment-6364575932). Głównej treści issue nie synchronizowano w tej rundzie z pełnym raportem lokalnym; aktualny odbiór dimmera/RGB jest w powyższym komentarzu i tabeli CTRL-02 tego dokumentu.

**Szczegóły grup rolet i przekaźników — podgląd zaakceptowany:** właściciel wybrał ten punkt jako następny. Zastąpiono osobne formularze grup istniejącymi rendererami pojedynczego przekaźnika i rolety w trzech skórkach: wspólne zakładki i ustawienia grupy, włącznik, pola pomiarów, grafika rolety, skala, gest, animacja oraz przyciski Otwórz/Zamknij/Stop z odstępem 16 px od nawigacji. Polecenia trafiają do stabilnego ID grupy przez istniejącego wykonawcę; numer grupy nie jest indeksem urządzenia fizycznego. Odświeżanie przekaźników kopiuje ID rzeczywistych członków przed callbackami i zleca ich istniejące odczyty. Moc jest sumowana tylko przy kompletnych poprawnych odczytach; wspólnego napięcia ani prądu nie wymyślano. Wiersz odczytu grupy pokazuje liczbę urządzeń i stan online zamiast czasu przypadkowego urządzenia.

Różne stany Włącz/Wyłącz oraz różne pozycje są jawnie opisane. Gest rolety wybiera jedną pozycję absolutną dla grupy, wysyła dopiero po zwolnieniu i nie zmienia potwierdzonej telemetrii. Podgląd mieszanych pozycji nie rysuje fałszywego wspólnego znacznika potwierdzenia. Zmiana powiązania, utrata możliwości ustawiania pozycji lub przejście między wspólną i mieszaną pozycją w trakcie gestu natychmiast anulują podgląd i przebudowują ekran, bez odłożenia za aktywnym dotykiem. Animacja odtwarza wyłącznie nowe potwierdzone wspólne pozycje. Model danych urządzenia i backend RPC pozostały bez zmian; aktualne reguły przyjmowania grupowych operacji pozostają w istniejącym wykonawcy.

Walidacja tej rundy: `DSR_UI_FOCUS_GROUP_DETAILS=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_actions_editor_ui` — PASS w LVGL 9.3 dla Grafit/Jasny/Cyber Blue i PL/EN/DE, z 24 wybranymi klatkami urządzenia/grupy i stanów mieszanych/ruchu. Test przed poprawką wykazał brak wspólnych kontrolek grup. Sprawdzono właściwe intencje i ID, gest pozycji, brak wysłania w trakcie dotyku, anulowanie po rebindzie, odświeżanie wszystkich członków i zmianę pikseli animacji. Oczekiwanie etykiety pozycji uwzględnia tłumaczony prefiks, a zegar animacji jest odświeżany przy zamrożonych timerach dostawców hosta. Niezależny przegląd wykrył dwa przypadki unieważnienia gestu; dodane regresje wykazały również opóźnione sprzątanie podglądu, a końcowa poprawka i ponowny przegląd nie mają dalszych istotnych uwag. Na tym samym pliku wykonywalnym przeszedł zaakceptowany widok grupy TRV; osobno przeszedł istniejący `test_cover_ui` dla pojedynczej rolety, STOP i readback. Są to przechwytywane intencje oraz modele hosta, nie odbiór sterowania Shelly na sprzęcie. Bez pełnego atlasu, pełnego CI i builda docelowego; Pokazane szczegóły grup rolet i przekaźników zostały zaakceptowane; odbiór sprzętowy i nowy DEV pozostają otwarte.

Krótki status nowych szczegółów grup zapisano w [komentarzu do #74](https://github.com/voltex-at/DIY-Shelly-Remote/issues/74#issuecomment-6365153955); właściciel następnie zaakceptował podglądy rolet i przekaźników.

**Odbiór szczegółów grup rolet i przekaźników:** właściciel potwierdził „Tak, szczegóły grupy rolet zaakceptowane” oraz „Tak, szczegóły grupy przekaźników zaakceptowane”. Akceptacja obejmuje pokazany wspólny układ z pojedynczym urządzeniem, grafikę/skalę i akcje rolety, odstęp od nawigacji, włącznik przekaźnika, sumę mocy, odświeżanie członków i jawny stan mieszany. Nie powtarzano testów ani renderów po samej akceptacji. Następny otwarty zakres tej części to sceny BLE/TRV i czujniki; odbiór rzeczywistego sterowania oraz walidacja wydania pozostają osobnymi etapami.

**Stan lokalnego rozszerzenia:** wykonawca obsługuje pozycję rolety, jasność, kolor RGB i wspólną nastawę BLU TRV przez istniejący worker i dotychczasowe procedury kontroli oraz odczytu. Grupa mieszana lub pasywny czujnik są odrzucane przed sterowaniem. Kreator filtruje członków według rodzaju sterowania. Nowa grupa i jej kafel aktywnego Dashboardu są zapisywane atomowo, bez duplikowania po błędzie/ponowieniu i edycji. Grupy są również na liście Urządzeń. Przy limicie 24 kafli podsumowanie uprzedza, że grupa pozostanie na liście Urządzeń, a kafel trzeba dodać po zwolnieniu miejsca. Stan mieszany nie jest przedstawiany jako jedna potwierdzona wartość; termostaty otrzymują jawnie wybraną wspólną temperaturę. Moc grupy jest sumowana tylko przy kompletnych odczytach; nie wyliczano fikcyjnego wspólnego napięcia, temperatury ani procentu baterii z różnych wartości.

Walidacja tej rundy obejmuje wyłącznie zmieniony zakres: `tools/test_group_controls.py` dla parsera/modelu/runtime, następnie workera i schematu — PASS. Obejmuje ścisłe typowane wartości, stare pole `on`, tożsamość TRV, odrzucanie mieszanych grup i czujników, wspólną nastawę, guards, anulowanie oraz wynik niepewny bez automatycznego ponowienia. Na Windows wzorce schema1 są identyczne z obiektami Git po normalizacji CRLF; lokalny adapter normalizował tylko końce linii podczas kontroli ich skrótów, bez zmiany wzorców lub produkcji. `DSR_UI_FOCUS_GROUP_CONTROLS=1 python3 tools/dev/run_host.py test_ui.py --jobs 4 --case test_actions_editor_ui` — PASS dla trzech skórek i PL/EN/DE: właściwe polecenia kart, Dashboard/Urządzenia, nastawy wspólne i mieszane, odświeżenie po odczycie oraz zapis i ponowienie bez duplikatu. Osobno przeszedł `DSR_UI_FOCUS_LIGHTING=1` dla istniejącego dimmera/RGB. Regresja dotychczasowego edytora przeszła na tym samym pliku wykonywalnym, bez ponownej kompilacji. Obejrzano piksele LVGL 9.3 i bezpośrednie porównanie urządzenia z grupą; nie generowano pełnego atlasu ani pełnego CI. Backend i zapis mają dostawców hosta. Wygląd kart grup oraz pokazane korekty odstępów widoku termostatu/grupy zostały zaakceptowane na podglądzie; pokazane szczegóły grup przekaźników i rolet zostały zaakceptowane; kompilacja docelowa, rzeczywiste RPC/readback i trwałość na pilocie nie są potwierdzone tą rundą. Starszy firmware 0.7.70 może odmówić konfiguracji z rozszerzonymi typami grup; nie zmieniano zainstalowanego firmware ani danych pilota.

**Audyt scen i BLE — aktualizacja:** właściciel zlecił dodanie konkretnej temperatury do ręcznych scen. W kodzie roboczym sceny obsługują teraz `target` z polem `value` w połówkach stopnia (8..60 = 4..30°C). Schemat 1 pozostaje zamrożony; dotychczasowe kroki schematu 2 zachowują znaczenie. Edytor LCD i panel WWW pozwalają wybrać rzeczywiście rozpoznany TRV, ustawić temperaturę i ponownie edytować zapisany krok. Podsumowanie zawiera nastawę. Czujnik pasywny nie otrzymuje sterowania termostatem; aktor i worker nadal sprawdzają rzeczywiste powiązanie oraz możliwości przed I/O. Starszy edytor w ustawieniach korzysta z tego samego wyboru temperatury i zachowuje pozostały draft; zmiana urządzenia usuwa poprzedni parametr nastawy.

**Weryfikacja ręcznych scen TRV:** ukierunkowany test parsera/konfiguracji (w tym granice i odrzucanie brakującej, ułamkowej, powielonej lub obcej wartości), test aktora/workera sceny z anulowaniem, zmianą powiązania i niepewnym wynikiem bez ponownego zapisu oraz natywny edytor LVGL 9.3 w Grafit/Jasny/Cyber Blue i PL/EN/DE przeszły. Edytor sprawdzono także przy błędzie zapisu, ponowieniu, anulowaniu i zmianie urządzenia w rejestrze ustawień; sama edycja nie wysyła sterowania. Istniejąca regresja edytora scen/grup przeszła bez ponownej kompilacji. Chromium potwierdził wybór TRV/czujnik pasywny, walidację co 0,5°C, zapis/ponowne otwarcie, zmianę języka i powrót do wyboru podczas opóźnionego odczytu typów. Podgląd: `.local/ui-survey-0.7.71/groups/scene-target.png` (sześć wybranych klatek, bez pełnego atlasu). Przegląd kodu wykrył i po poprawce potwierdził usunięcie problemu starszego edytora; bez pozostałych potwierdzonych P1/P2 w tym zakresie. Lokalny Cygwin nie ma biblioteki UBSan — te uruchomienia hostowe były bez sanitizerów. Właściciel zaakceptował pokazany podgląd nowego kroku sceny TRV: wybór temperatury −/+, zapisane podsumowanie oraz wygląd trzech skórek. Wykonanie/readback na sprzęcie i walidacja wydania pozostają otwarte. Po samej akceptacji nie powtarzano testów ani renderów. Bez publikacji DEV, push ani flashowania.

**Automatyzacja czujników — uzgodnienia ankiety:** pierwszy przypadek to okno/drzwi → ogrzewanie. Właściciel wybrał reakcję natychmiast po odebraniu zmiany stanu, bez celowego opóźnienia. Temperatura przy otwarciu ma być wybierana osobno w regule, domyślnie 8°C; po zamknięciu planowany jest powrót do poprzedniej nastawy. Właściciel zdecydował, że ręczna zmiana temperatury podczas otwarcia ma zostać zachowana po zamknięciu — automatyzacja nie przywraca wtedy starszej nastawy. Właściciel zaakceptował obsługę kilku czujników sterujących tym samym termostatem: otwarcie dowolnego obniża temperaturę, a powrót do wcześniejszej nastawy następuje dopiero po zamknięciu wszystkich, z zachowaniem ręcznej zmiany. Właściciel zaakceptował automatyczne wznowienie po restarcie lub utracie połączenia dopiero po świeżych odczytach wszystkich przypisanych czujników i termostatów, z zachowaniem ręcznych zmian. Główne zachowanie reguły jest uzgodnione. Ruch i inne odczyty BLE nadal nie wyzwalają automatycznie scen.

**Projekt wykonawcy automatyzacji — zatwierdzony:** [Opis okno/drzwi → ogrzewanie](superpowers/specs/2026-10-23-window-heating-design.md) utrwala decyzje ankiety oraz proponowaną integrację z aktorem/workerem, dziennik odzyskiwania, migrację konfiguracji i ukierunkowaną walidację. Rozpoznanie wykazało, że świeże HTTP do bramki nie potwierdza świeżości konkretnego kontaktu ani lustrzanej nastawy TRV. Projekt wymaga czasu raportu kontaktu oraz bezpośredniego odczytu TRV. Bez okresowych raportów kontakt może wymagać oczekiwania na nową transmisję; beacon nie jest włączany samoczynnie. Dodano granice trwałego zapisu ręcznego override we wszystkich fazach cyklu. Właściciel zatwierdził dokument („Tak, projekt zatwierdzony”). [Plan wdrożenia](superpowers/plans/2026-10-23-window-heating.md) obejmuje model, dziennik, dowody świeżości, migrację schematu 3, wykonawcę oraz LCD/WWW. Właściciel zatwierdził plan i wykonanie tutaj. Etapy 1–7 są wdrożone lokalnie; stan końcowej walidacji opisano w aktualizacji z 2026-10-23.


Niezależny przegląd kodu tej rundy wykrył i doprowadził do poprawy trzech nowych ścieżek grup: powrót ze strony koloru RGB zachowuje teraz grupę, zamiast traktować jej numer na liście jak indeks fizycznego urządzenia; wspólny stan Włącz/Wyłącz jest liczony oddzielnie od różnych jasności i kolorów; odrzucenie szkicu RGB odtwarza pozostałe kanały z aktualnych odczytów. Ustawienia otwierają edycję właściwej grupy, a Zużycie dla grupy dimmerów jest nieaktywne. Ponowiono wyłącznie nowe przypadki grup i reprezentatywne istniejące dimmer/RGB — PASS dla trzech skórek i PL/EN/DE. Kontrola obejmuje fizyczne kliknięcia zakładek, Wyłącz przy różnych kolorach/poziomach i zmianę odczytu podczas niezapisanego szkicu; dostawcy poleceń pozostają modelami hosta. Końcowy przegląd poprawek nie zgłosił dalszych istotnych uwag w tym zakresie. Wygląd kart grup oraz pokazane widoki sterowania grupami dimmerów, RGB, przekaźników i rolet są zaakceptowane; docelowy build i odbiór sprzętowy pozostają otwarte.

## Porządkowanie — wykonane

- [x] Audyt lokalnego stanu, wszystkich 62 gałęzi, 17 issues i 7 otwartych PR-ów; lista wymagań i źródeł zachowana.
- [x] Odróżniono implementację od starych planów, wycofanych wymagań i fizycznego odbioru.
- [x] Zamknięto 17 starych issues i 7 PR-ów bez merge; dalszy zakres przeniesiono do #74.
- [x] Usunięto 60 zdalnych i dwie nieaktywne lokalne gałęzie; zachowano main i work/dev-0.7.70 oraz mały plik odzyskiwania unikalnych commitów.
- [x] Usunięto stare wyniki Actions i wszystkie artefakty; zachowano ostatnie CI i Signed release 0.7.70 oraz publiczne pliki wszystkich wydań.
- [x] Usunięto lokalne build-ui/build-ai, stare handoffy/logi i wygenerowane obrazy/raporty z docs. Zachowano źródła, testy, zasoby firmware, referencje właściciela i aktywne środowisko.
- [x] Zarchiwizowano osiem rozmów projektu ChatGPT; zachowano bieżący czat Codex.
- [x] Uporządkowano dokumentację i odwołania do usuniętych obrazów; walidacja ograniczona do dokumentacji, diff i stanu repozytorium. Nie uruchamiano nowych testów firmware ani atlasu.

Etap porządkowania nie publikuje nowej wersji. Dalszy etap, zlecony przez właściciela, obejmuje ankietę, uzgodnione poprawki na bieżąco i nowy DEV po ich zakończeniu oraz walidacji. Flashowanie, reset danych, zmiana klucza OTA i scalenie do main nie są częścią porządkowania.

Bieżąca koordynacja: https://github.com/voltex-at/DIY-Shelly-Remote/issues/74
