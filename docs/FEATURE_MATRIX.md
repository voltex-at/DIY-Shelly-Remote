# Matryca funkcji — 0.7.9, rewizja 10

Etap niezależny #6/#24: [trwała jasność i wygaszanie](DISPLAY_SETTINGS_PL.md), bez nowego schematu NVS; odbiór fizyczny otwarty.

Etap niezależny #6/#25: [napięcie BAT na żądanie](BATTERY_VOLTAGE_PL.md).

| Zasilanie pilota | Stan | Bramka |
| --- | --- | --- |
| GPIO8 / kalibracja / napięcie / wiek | CI 37dd187: 5/5, run 38838860433 | Pomiar multimetrem |
| Procent / niski poziom / czas pracy / monitoring | Otwarte | Typ ogniwa, kalibracja i istniejący backlog |
| Obecność ogniwa / USB / ładowanie | Nieznane | Wiarygodny sygnał sprzętowy |

**Aktualny P0:** czysta 0.7.6 jest **FAIL** według nowych fizycznych dowodów
w PR #33: 313/992 nagłówków formatu, brak workspace, działający callback ołówka
i diagnostyka. 0.7.7 zmienia inicjalizację pustej partycji na jeden format po
pełnym blank-check, współdzielenie CPU z idle i logi etapów/czasu/wyniku.
Test oryginalnego IDF wymaga dokładnie 992 erase i zachowania prefiksu 313.
CI poprawki 81e734d: 5/5, [run 38835176199](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38835176199). Fizyczny pierwszy boot do odbioru; zielone CI
nie jest physical pass. [Aktualne dowody i blokady](PR33_ACCEPTANCE.md).

**Historyczny test 0.7.6:** właściciel zaakceptował jednorazowe pełne erase swojej płytki
i testuje czystą instalację source `ecdac4e`. CI tego źródła sprawdzone: **5/5**,
[run 38831522391](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38831522391).
Dodany test oryginalnego wrappera/core IDF SPIFFS na symulowanej NOR przechodzi
lokalnie (nowe CI osobno). Nie zmienia firmware i nie potwierdza fizycznego
odbioru. [Dowody i otwarte bramki](PR33_ACCEPTANCE.md). To nie jest zgoda na
automatyczny lub ponowny erase ani merge/Stable.

**Blokada fizyczna #28:** instalacja 0.7.4 nie montuje niepustego dsrdata
(log SPIFFS_ERR_NOT_A_FS), więc edytory i HTTPS nie działają. 0.7.4 nie
przeszła odbioru. 0.7.6 w tym samym PR #33 dodaje widoczne błędy, diagnostykę,
blokadę API niegotowego workspace oraz narzędzie kopii/analizy bez zapisu.
[Procedura](STORAGE_RECOVERY_PL.md). Faktyczny odzysk danych i test sprzętowy
pozostają otwarte; nowy firmware nie formatuje nieznanej niepustej partycji.
Zależne #20/#29 pozostają zablokowane. Dotychczasowe statusy CI poniżej
dotyczą testów automatycznych, nie potwierdzenia działania tej płytki.

0.7.4 dodaje kontekstowe Dodaj/Edytuj/Usuń pomieszczenie (#28), z istniejącym
modelem, asynchronicznym potwierdzeniem zapisu, CAS i ochroną zależności.
[Opis i test fizyczny](ROOM_EDITOR_PL.md). Nowy edytor wymaga odbioru na płytce.

0.7.3 dodaje zatrzask diagnostyczny startu po uwadze recenzji PR #30: zwrócony
błąd krytyczny nie wywołuje abort/restart/erase; LVGL i API administracyjne
czekają na gotowość, a OTA nie potwierdza błędnego startu. Test rzeczywistego
app_main z błędami wymuszonymi jest hostowy; nie oznacza odbioru fizycznego.
[Polityka i testy](STARTUP_RECOVERY_PL.md). Odbiór fault recovery nadal otwarty.

## Bramka regresji sprzętowej — 2026-10-16

Start 0.6.3 jest potwierdzony przez użytkownika. 0.7.1 **nie startuje na płytce**:
`ESP_ERR_NO_MEM` przy tworzeniu `dsr_ui`; zielony CI nie jest odbiorem sprzętowym.
0.7.2 poprawia rozmieszczenie pamięci i rezerwację wewnętrznego stosu UI oraz
dodaje kontrolę rzeczywistego ELF i testy błędów startu/magazynu. Start,
Wi-Fi/LAN/przekaźnik, BLE, HTTPS/OTA i trwałość danych wymagają ponownego testu
według [BOOT_MEMORY_FIX_PL.md](BOOT_MEMORY_FIX_PL.md). Nie oznaczaj tej wersji
ani BLE jako sprawdzonych fizycznie na podstawie hostowych fixture lub kompilacji.
Nowsze zgłoszenie właściciela (16.10): płytka startuje, skan BLE odbiera reklamy
(10 wyników na zdjęciu). Exact wersja/SHA nie zostały podane w potwierdzeniu.
To dowód startu i odbioru RF tej instalacji, bez odbioru parowania, recovery,
OTA/rollback lub nowego edytora 0.7.4. Nie publikujemy prywatnych identyfikatorów.

Zakres: LCDWiki ES3C35P N16R8, LVGL 320×480 i lokalny HTTPS Web Panel.
Obowiązek utrzymania: Issue #25. Zmiana callbacku, transportu, capability albo
statusu testów wymaga aktualizacji tej tabeli w tym samym PR.

**To nie jest deklaracja ukończenia produktu.** Konfiguracja 16 wpisów nie daje
16 aktywnych połączeń. Działa transport jednego switch/relay. Produkcja nie
zastępuje brakujących odczytów danymi DEMO. BLE scan nie paruje urządzeń.

Statusy: **Not implemented** — brak wykonawcy; **UI mock** — wyłącznie jawny DEMO;
**Implemented untested** — kod bez adekwatnego wyniku; **CI tested** — test kodu,
parsera, UI lub build, z opisanym zakresem; **Physically tested** — wyłącznie
rzeczywisty udokumentowany wynik na płytce. Każda nowa paczka nadal wymaga regresji
sprzętowej. Właściciel potwierdził wcześniej kolory/jasność/wybudzanie oraz Wi-Fi,
LAN, dodanie 1PM Mini Gen3 i rzeczywisty ON/OFF. Exact SHA tamtej instalacji nie
został zapisany. **Nie przenosimy tego dowodu na 0.7.1 lub na BLE.**

## Pulpit, nawigacja i karty

| Ekran / interakcja LCD | Web Panel | Typ / zakres | Status obecnej implementacji | Issue / PR; wymagany test |
| --- | --- | --- | --- | --- |
| Dolne Pulpit / Dom | Nawigacja edytorów i podgląd LCD | Lokalna konfiguracja | CI tested: przejścia i obszary dotyku | #6, PR #13; zdjęcie LCD i regresja nawigacji |
| Dolne Ustawienia / Dodaj | Urządzenia / System | Pilot, kreatory | CI tested: rzeczywiste callbacki | #6/#7; przejście do Wi-Fi, LAN, BLE |
| Dolna Energia | Brak wykonawcy energii | Modele z licznikiem, docelowo | Not implemented: disabled, bez komendy | #22; licznik i historia na rzeczywistym modelu |
| Górna Aktywność | Brak dziennika | Lokalny ograniczony log | Not implemented: disabled | #21; czas/NTP, retencja, częściowe błędy |
| Górne Grupy / Sceny | Edytor definicji scen | Wymagany multi-device | Not implemented: executor i grupy; zakładki disabled | #20/#21; minimum dwa rzeczywiste Shelly |
| Aktywna zakładka widoku | Aktualna sekcja | Wskaźnik bieżącej strony | CI tested: statyczny wskaźnik, nie martwy przycisk | #25; brak kliknięcia bez działania |
| Pasek pulpitu — nazwa / rozwinięcie | Pulpit startowy | Do 4 pulpitów | CI tested: wybór zapisuje shared config | PR #5/#13; zapis, konflikt, restart |
| Pasek + / ołówek | Dodaj / Edytuj | Lokalny kreator / edytor | CI tested: rzeczywiste przejścia | #6/#7; brak regresji źródeł danych |
| Pasek Info | Stan konfiguracji + podgląd | Liczby z aktualnego modelu | CI tested: modal z rzeczywistą nazwą, definicjami i online; Zamknij | #25, ten PR; pointer hit-test, aktualizacja modelu, PL/EN/DE |
| Pomieszczenie / zdjęcie / powrót | Edycja pomieszczenia / preview | Lokalne dane | CI tested: dwie kolumny, filtr urządzeń, przewijanie | #6/#28; LCD, fotografie i listy po restarcie |
| Pomieszczenia: Dodaj / ołówek / Edytuj pokój / Usuń | Wspólny CRUD i rewizja | Do 8 pokojów, trwałe ID | CI tested na ecdac4e: formularz, foto, CAS, błędy, potwierdzenie usuwania i zależności; odbiór fizyczny otwarty | #28; [test płytki](ROOM_EDITOR_PL.md), restart/OTA i sync |
| Karta urządzenia → szczegóły | Konfiguracja wpisu, LCD preview | Obsługiwany stan bieżącego modelu | CI tested: nawigacja, stable ID, offline | #6/#12; realne dane po zmianie konfiguracji |
| ON/OFF karty i szczegółów | Bezpośredniego SET w WWW brak; podgląd jest read-only | Jeden Gen1 relay lub Gen2+ switch | CI tested: identity/auth/SET/readback; wcześniejsze ON/OFF Gen3 potwierdzone fizycznie | #12, PR #14/#17; ponowny test 1PM Mini Gen3 |
| Odśwież stan | Podgląd LCD / stan konfiguracji | Jeden aktywny switch | CI tested: GET, może dołączyć do bieżącego odczytu | PR #17; dotknięcie w czasie PROBE i brak ponowienia SET |
| W/V/A / wiek próbki / offline | Ten sam framebuffer | Tylko obecne i poprawne pola API | CI tested: parser, zera/braki/błędy, stabilny polling | #12; porównać realne V/A z API |
| Szczegóły Wykresy | Brak historii | Rzeczywista retencja potrzebna | Not implemented: disabled | #22; pomiar po czasie, Flash, restart/offline |
| Szczegóły Ustawienia | Lokalna definicja host/kanał/typ; nie RPC settings | API konkretnego urządzenia | Not implemented: zakładka disabled | #23/#24; potwierdzony model/capability |
| Karta sceny / ikona ołówka | Edytor scen | Zapis definicji, bez wykonawcy | CI tested: edycja; Not implemented: uruchamianie scen | #21; nie przedstawiać otwarcia edytora jako wykonania |

## Dodawanie i rzeczywiste urządzenia

| Element LCD | Web Panel | Wymagany model / generacja | Status | Issue / PR; wymagany test |
| --- | --- | --- | --- | --- |
| Skanuj sieć Wi-Fi (Shelly LAN) | Skanuj sieć Wi-Fi | Gen1 / Gen2+ odpowiadające lokalnie | CI tested; poprzednie 3 realne Gen2/Gen3 potwierdzone fizycznie | #7, PR #10; powtórzyć LAN z BLE włączonym |
| Lista LAN / Sprawdź / Login / hasło / pokaż | Te same kroki | Gen1 Basic / Gen2+ Digest | CI tested: rzeczywisty HTTP parser/identity/capabilities | #7; hasło poprawne/błędne, timeout, DHCP |
| Nazwa / pomieszczenie / kanał / Zapisz | Te same pola i binding | Wspierany pojedynczy switch | CI tested: weryfikacja przed zastąpieniem starego bindingu | #7; restart, trwałość, błąd nie kasuje starego |
| Ponów / Anuluj LAN | Skanuj ponownie / Przerwij | Radio Wi-Fi / LAN | CI tested: oddzielne zadanie i stan | #7; realny timeout i odłączenie Wi-Fi |
| Dodaj ręcznie IP / klawiatura / sprawdź | Edycja hosta jest definicją; operacyjny binding przez LAN | Wspierany relay/switch | CI tested: lokalny bind, walidacja IPv4/kanału/identity | #7/#8; ręczny IP i błędne dane |
| Wi-Fi AP | Wi-Fi AP — niewdrożone | Gen1/Gen2+ zgodne z lokalnym provisioningiem | Not implemented: jawnie disabled | #19; AP → domowy LAN, rollback i zachowanie bindingu |
| Bluetooth LE → Szukaj / Anuluj / Ponów | Bluetooth LE • skan, wspólne `/api/v1/ble*` | ESP32-S3 observer + faktyczne legacy ADV | CI tested: rzeczywisty adapter/parsing/UI; właściciel potwierdza odbiór RF, exact SHA niepodany | #18, PR #26; nadal >=10 cykli coexistence i porównanie payloadu |
| Wynik BLE / szczegóły / RSSI / ostatnia reklama | Grupy i wyniki skanera | RPC UUID, Shelly manufacturer, BTHome v2 | CI tested: bez zgadywania generacji/modelu; wykryto ≠ sparowano | #18; model Wi-Fi i BLU jeśli dostępne, pusty skan, brak radia |
| Wynik BLE RPC → Szukaj w LAN | Powrót do metod i LAN | Urządzenie faktycznie obecne w LAN | CI tested: przejście do istniejącego kreatora | #7/#18; BLE nie dodaje sam do Wi-Fi |
| BLE pairing / provisioning / stałe BLU | Wyłącznie tekst o ograniczeniu, bez atrapy Add | Zgodny GATT/RPC/bonding/BTHome | Not implemented | #18 B1/B2/B3; osobny test modelu, autoryzacji i kluczy |
| Wiele wpisów urządzeń | CRUD do 16 definicji | **Nie 16 transportów** | CI tested: konfiguracja; Not implemented: multi-device executor | #20; co najmniej dwa Shelly bez nadpisania starego |
| Roleta Otwórz / Zamknij / Stop / wskaźnik | Definicja i podgląd | Realny Cover transport nieobecny | UI mock w jawnym DEMO; produkcyjne sterowanie disabled/unknown | #15/#23, PR #16; bezpieczne stanowisko Cover RPC |
| Roleta edycja ołówkiem | Edytuj definicję | Zapisany wpis | CI tested: otwiera istniejący edytor; DEMO disabled | PR #16; właściwe ID po zmianie kolejności |
| Dimmer / RGB/RGBW / termostat | Brak funkcjonalnego kontrolera | API danego modelu | Not implemented | #23; osobny typ/PR, capability i readback |
| Czujnik otwarcia / bateria / temperatura / wilgotność | Definicja typu / preview | Wymagane rzeczywiste źródło danych | UI mock w fixture; Not implemented: adapter czujnika/BLU | #18/#23; odczyt modelu i unavailable, bez fikcyjnego zera |

## Konfiguracja, zdjęcia i system

| Interakcja LCD | Interakcja WWW | Zakres | Status | Issue / test |
| --- | --- | --- | --- | --- |
| Wi-Fi: lista SSID / wybór / Ponów / Wstecz | Łączność panelu przez istniejący LAN | Pilot ESP32-S3, 2.4 GHz | CI tested; wcześniejsze fizyczne wykrywanie Wi-Fi potwierdzone | #7/#8; WPA2/WPA3, restart, słaby sygnał |
| Hasło / pokaż / Connect / Cancel / Continue | Dane Wi-Fi nie są edytowane przez panel w tym etapie | Pilot | CI tested: save-after-IP, rollback i cancel | #8; błędne hasło i utrata AP, bez logowania sekretów |
| Klawiatura litery/cyfry/Shift/Backspace/symbole/ąä | Standardowe pola przeglądarki | PL/EN QWERTY, DE QWERTZ | CI tested: UTF-8, ASCII, hit-test, maska | #8, PR #13; fizyczna czytelność 22 px |
| Klawiatura ← / → / Spacja / Gotowe / IP | Standardowe pola | Jedna wspólna implementacja | CI tested: wstawienie/usunięcie w środku, jeden submit | #8; końce i maskowane hasło |
| Edytuj pilot: Pulpity/Pomieszczenia/Urządzenia/Sceny | Te same kolekcje CRUD | Schema 1, granice 4/8/16/8 | CI tested: wspólny walidowany model i CAS | PR #5/#11; konflikt, A-save/B-edit, błąd zapisu, restart |
| Pola nazwa/typ/IP/kanał/pokój; Usuń wpis | Odpowiednie pola i usuń | Definicje; nie nowe połączenie | CI tested: referencje i walidacja | #20/#23; nie mylić definicji z bindingiem |
| Karty: dodaj typ/obiekt/układ/wysokość/dane/Góra/Dół/usuń/strony | Dodaj, rozmiary, pola, ↑/↓/× oraz drag/drop | Do 24 kart / pulpit | CI tested: save/preview/retencja | #6, PR #5/#13; 24 karty, przewijanie, zmiana kolejności |
| Pokój: ikona / wybór zdjęcia z pamięci | Upload, optymalizacja zdjęcia, zmiana i usunięcie | 320×180 RGB565 w Flash; SD niezweryfikowane | CI tested: rozmiar/cache/GC; upload wymaga testu fizycznego | PR #5/#11; limit miejsca, power loss, zdjęcia po OTA |
| Scena: dodaj/zmień urządzenie/komendę/usuń działanie | Te same definicje | Executor nieobecny | CI tested: edytor, Not implemented: wykonanie | #21; kompatybilne typy i częściowy sukces |
| Wygląd: PL/EN/DE, motyw, akcent, pulpit startowy | Odpowiednie pola, również własny kolor | Lokalny styl/config | CI tested: trwały zapis i rendery językowe | #6/#8; fizyczne kolory/kontrast i aktualizacja bez restartu |
| Zapisz / Wstecz / status konfliktu / błąd | Save / Reload / dirty/conflict | Async durable configuration | CI tested: immutable submitted A, późniejszy B zachowany | PR #11; partial write i restart, oba interfejsy |
| Web Panel: parowanie, kod i fingerprint | Jednorazowy kod, sesja, local HTTPS | Per-device sekret, bez wspólnego hasła | CI tested: logika i browser; fizyczny TLS/pairing do regresji | PR #5; zła sesja, obce Origin/Host, restart i wygasły kod |
| Zezwól na OTA — 2 minuty | Wybór app BIN / Sprawdź i zainstaluj / restart | Właściwy board, dwa sloty 6 MiB | CI tested: dev uploader i image policy; fizyczny OTA/rollback niepotwierdzony | #24, PR #5; zły board/SHA, przerwanie, NVS/zdjęcia |
| Firmware Update: aktualna wersja i instrukcja panelu | Lokalna instalacja wyżej | Bez updatera GitHub Releases | CI tested: realna informacja, Not implemented: sprawdź/pobierz/podpis wydawcy | #24; manifest, klucz, TLS i rollback |
| Suwak jasności + Zapisz | Brak zdalnego suwaka | Istniejący rekord NVS, zachowane Wi-Fi | Implementacja 0.7.9; nowe CI osobno | #6/#24; restart i wybudzenie z zapisaną jasnością |
| Screen off / touch to wake | Brak zdalnego sterowania | Sterownik ES3C35P | Physically tested historycznie; nowa regresja wymagana | #24; sen 60 s, dotyk, brak niechcianego SET po wybudzeniu |
| Timeout ekranu + Zapisz | Brak zdalnego sterowania | Istniejący rekord NVS; 0 = bez autooff | Implementacja 0.7.9; nowe CI osobno | #6/#24; fizyczne wygaszenie i first-touch suppression |
| Eksport/import / przywracanie | Brak kompletnego bezpiecznego procesu | Funkcje systemowe planowane | Not implemented | #24; migracja i recovery bez utraty danych |
| Test dotyku rogi / Back | Podgląd nie zastępuje testu | ST77922 | Physically tested historycznie; nowe CI nie dowodzi hardware | #6; 4 rogi, powrót i orientacja |
| Test kolorów / nawigacja | Preview 320×480 | ST77922 RGB565 | Physically tested historycznie | #6; nowe zdjęcie LCD i zgodność kolorów |
| Podgląd rolety DEMO / Otwórz / Zamknij / Stop / wyjście | Preview odbija rzeczywisty framebuffer | Wyłącznie jawne DEMO, zero komend do sprzętu | CI tested: izolacja i przywrócenie modelu live | PR #16; fizyczny podgląd, zachowanie bindingu |
| Framebuffer 320×480 | Odśwież / cykliczny podgląd | Snapshot z zadania LVGL | CI tested: actual LVGL, read-only | PR #5; heap/PSRAM przy równoległym BLE/HTTPS |

## Wynik audytu połączeń akcji z wykonawcami

- `control → actions.set_output → mobile_output → submit(SET_OUTPUT) → dsr_worker
  → dsr_http_set_output → identity/SET/readback → event → mobile model`:
  operacyjny dla jednego wspieranego switch/relay. PROBE ma osobny stan i jedną
  odroczoną intencję SET; nie ma automatycznego replay po błędzie.
- `LAN choose → HTTP inspect → capabilities → bind_verified → worker BIND →
  zapis po weryfikacji → migration config → UI`: realne API, nie SSID/name guess.
- `BLE Szukaj / POST → queue → manager → ble_gap_disc → GAP event → bounded
  parser/snapshot → LCD tick / authenticated GET`: realny adapter, fizyczny RF
  wymaga odbioru. Brak ścieżki GATT write/provisioning.
- `editor Save → workspace_submit → durable config store → exact ack → model`:
  operacyjny zapis definicji. Nie wykonuje scen lub wielu urządzeń.
- `cover_control → actions.set_cover` i Stop: w produkcyjnym bindingu brak
  takich callbacków; przyciski są disabled. DEMO działa tylko lokalnie.
- `scene_card`: prowadzi do edycji, nie do SET. `Energia`, `Wykresy`, `Grupy`,
  `Sceny`, `Aktywność`, AP i settings RPC nie mają wykonawcy; są disabled.
- `toolbar Info`: usunięto martwą akcję, jest realny modal i Zamknij. Bieżące
  zakładki bez callbacków są statycznymi wskaźnikami, nie przyciskami no-op.
- `brightness/sleep → dsr_board_*`: rzeczywiste wywołania sterownika.
  `firmware` pokazuje wersję; nie udaje klienta GitHub Releases.

## Kolejność i bramki

1. Zachować bazę PR #17 i ustawienia; BLE #18 / PR #26 jest pierwszym pilnym etapem.
2. AP #19 i niezależny multi-device #20; każdy moduł w osobnym Draft PR.
3. Kontrolery #23 i grupy/sceny #21 po spełnieniu zależności; energia/historia #22.
4. Podpisany updater/ustawienia #24, dalszy odbiór wyglądu #6/#8.

Każde przejście statusu musi wskazywać exact source SHA, CI i dowód sprzętowy,
jeśli używa Physically tested. Nie zamykać #25 ani nie publikować Stable, dopóki
komplet nie ma operacyjnych działań dla wspieranych modeli i akceptacji właściciela.
