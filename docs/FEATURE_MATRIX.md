# Matryca funkcji — 0.7.27, rewizja 27

## #29A: ustawienia wybranego Shelly z aktualnym runtime

Integracja istniejącego PR34 na bazie PR53: aktywna zakładka Ustawienia,
tożsamość i dostępne odczyty właściwego endpointu, odświeżanie wybranego ID,
lokalna nazwa/pokój przez dotychczasowy CAS i trwały ACK. Po zapisie powrót
do właściwej karty; IO/CAS zachowuje edycję. Reorder/remove WWW nie przekierowuje
akcji na drugi Shelly. Host/kanał/sekrety pozostają poza formularzem.
Kod i regresje są dodane; wynik bieżącego CI i fizyczny odbiór to osobne bramki.
Nie wdraża historii/Wykresów, RPC ustawień, naprawy SPIFFS ani atomowego BIND.

## #19: rzeczywiste dodawanie przez otwarty fabryczny AP

[Kontrakt i ograniczenia](ISSUE19_AP_PL.md): skan Wi-Fi, BSSID-pinned RAM join,
tożsamość API, dwie fizyczne zgody, jeden Gen1 form/Gen2+ JSON POST i powrót
do domu. Exact ID/MAC/generation + STA SSID/IP readback w LAN przed istniejącym
formularzem i wielourządzeniowym BIND. Dane domowe/rejestr/pokoje nie są kasowane.
LAN suspend/drain przed zmianą radia, fail-closed SDK restore, cancel/timeout/
hide-reopen/lateACK i relative deadlines. PL/EN/DE LCD oraz authenticated WWW
handoff do LCD (bez password API), utrata LAN wyjaśniona, podgląd wstrzymany.
Zakres: open AP i unauthenticated HTTP; HTTPS/PKI/enhanced-security oraz chronione
AP jawnie unsupported. To nie pełna zgodność wszystkich generacji/firmware.
Pełny target CI/artefakt i fizyczny AP→LAN→BIND odbiór pozostają osobnymi bramkami.

## #5/#6: język Web Panelu niezależny od pilota

[Zlecenie właściciela](https://github.com/voltex-at/DIY-Shelly-Remote/pull/5#issuecomment-6214082384)
i [kontrakt](WEB_LOCALE_PL.md): header PL/EN/DE przed pairing/po401, lokalny
dsr.web.locale/browser fallback. Tylko jawne UI text/ARIA bindings, bez render,
API, changed/Save/load i bez zmian shared language/revision. Pola/focus/files/
paircode/dirty/conflict/strona pozostają. User names/SSID/preview nietłumaczone.
Język pilota = wspólny workspace; język WWW = ta przeglądarka.
PR49 source8637296, exact CI38894174497 **5/5 PASS**, real browser/renders
w CI. Lokalny Chromium SIGTRAP nie jest lokalnym PASS. Fizyczny locale retest
otwarty. Sam etap locale nie zmieniał backendu; nowy backend #20 opisany niżej.
Parent PR48f9bac663 CI38892424314 5/5, pobrany exact BIN1780240 B i ELF/segmenty/
partycje sprawdzone, pełne dowody w PR48; jego nowy UI czeka na fizyczny retest.

## #20: niezależne bindingi i sterowanie — nowy pion DEV, fizyczny odbiór otwarty

[Kontrakt/migracja/test sprzętowy](MULTI_DEVICE_PL.md): rzeczywisty rejestr do16
endpointów identity/generation/channel, bez zastępowania pierwszego; lazy legacy
NVS→osobny schema2 blob + exact readback, workspace schema1 bez zmian. Prywatne
numericID, zbiorczy CAS/exact ACK obu stores, targeted UNBIND, sekrety pozaWWW.
Per-device state/auth/backoff/FIFO intent; jeden bounded serial worker, brak SET
replay, obligatory identity+readback, stale15s. LCD LAN `Już dodane`, osobne karty
W/V/A; authenticated WWW runtime/output API z epoką restartu. To NIE 16 równoległych
transportów. Minimum2 fizyczne Shelly, latencja/offline/Wi-Fi+BLE/heap/restart,
fizyczny odbiór pozostaje otwarty. PR51 źródło07331b8 ma exact CI38898040624
5/5 i pobrany BIN/ELF/FLEET/partycje sprawdzone. Nie zamykać #20 na podstawie
testów hostowych; AP rozwija to źródło i wymaga własnego CI.


## P0: rzeczywisty odczyt wrócił; poprawki Save/Usuń

[PR33comment6213932010,16.10.2026 10:54UTC](https://github.com/voltex-at/DIY-Shelly-Remote/pull/33#issuecomment-6213932010):
właściciel przywrócił PRE-FORMATTED SPIFFS, edytor/model/form/klawiatura działają.
[Dalszy wynik11:02UTC](https://github.com/voltex-at/DIY-Shelly-Remote/pull/33#issuecomment-6214082938):
pokoje zachowane po resecie i Web Panel działa na0.7.18/preformatowanym SPIFFS.
To NIE blank-format ani pełny TLS/pairing PASS. Nie ponawiać erase/przywracania dsrdata.
0.7.21: fresh exact ACK + clean A + ta sama screen generation→kontekst poprzedni,
B/IO/CAS/mismatch zostają. Widoczne48px delete dashboard/room, named modal przed
mutacją draft, constraints recheck, osobneSave→ACK. Stabilny ID pokoju/fallback.
[Kontrakt i krótki retest](EDITOR_ACCEPTANCE_PL.md); test/CI/artifact/physical
bramki osobno. Revoke WWW zachowane nieopublikowane, NIE część tego builda.
LVGL UBSan3223/111, PL/EN/DE/max47char/48px i40cycles heap104456→104456 PASS;
native/config265 PASS, niezależny review bez blockerów. Nowy UI fizycznie nieodebrany.

## #6/#24: LCD restart z potwierdzeniem, bez erase/reset

System → Restart pilota: osobny ekran, potwierdź/anuluj/Wstecz. Tylko świadome
potwierdzenie wywołuje wspólny backend; BUSY/unavailable mają retry, accepted
blokuje kolejne żądania także po ponownym otwarciu. Pointery czyszczone na hide,
accepted zachowany do końca procesu. PL/EN/DE i przyciski48px; LVGL nie wywołuje
restartu ani zapisu. Backend PR46, własne testy/CI/artifact oraz fizyczna retencja
pozostają bramkami. LVGL UBSan2838/102, PL/EN/DE geometry,40cycles heap stable
106712→106712 i niezależny review PASS; request provider jawnie MODEL, nie
physical restart. Eksport/import/reset i pełne OTA nie stają się ukończone.

Historyczna aktualizacja09:56UTC (zastąpiona10:54powyżej): właściciel potwierdził boot/UI/Wi-Fi0.7.18, lecz
SPIFFS nadal niepusty/niezamontowany, workspace/HTTPS niedostępne. To nie próba
blank format. Nowa osobna zgoda obejmuje JEDEN lokalny erase tylko dsrdata;
właściciel wykonuje test i zbiera pierwszy log. Brak nowego physical PASS,
bez prawa do full erase/NVS/OTA slotów lub powtórzenia. Prywatne dane poza repo.
[Aktualny dowód i zgoda](https://github.com/voltex-at/DIY-Shelly-Remote/pull/33#issuecomment-6212940294).

## #24/#6: bezpieczny backend restartu (etap0.7.19)

[Kontrakt/inventory](RESTART_PL.md): atomic admission queued/running/event i
real poll handoff; NVS/workspace/bind/pair/photo/OTA+slot/boot health covered.
Dedicated dispatcher stack4096 priority1, one-shot poza LVGL/HTTP; Web restart
zachowuje auth/local-only i jawnie odmawia409/503. Nie ma erase/reset/formatu.
Atomic16052/dispatcher131/panel97/upload234559 UBSan i UI2334/96 PASS;
pozostałe writer faults/exact CI/ELF i physical restart/retention osobne bramki.
LCD follow-up powyżej; #24/#6/P0 niezamknięte.
Actual-writer UBSan: NVS935/worker3534/workspace123/photos101 PASS, failure cleanup
i odmowa po restart admission; niezależny review bez blockerów. Nadal HOST MODEL
providerów NVS/RTOS/filesystem/Flash, nie odbiór fizyczny.

## #24: jawny Release → signed app validation/discard, nadal bez instalacji

[Kontrakt](OTA_RELEASE_CHAIN_PL.md): full FETCH_OK metadata/parser → actual
manifest length binding → genuine Ed25519 → signed app/image validation.
Obie jawne STABLE policies, local version/slot nie z taga; empty production
trust zawsze zero app requests. Każdy błąd zero out. Brak retained BIN/Flash/
UI/worker/discovery/cache/retry i zgody wydawcy. Metadata workspace free przed
następnymi fazami; mały bridge<=1024 i osobne cooperative phase budgets.
Parent PR44 784f99ae CI38877323489 5/5 i rzeczywisty BIN/ELF/partycje sprawdzone;
nowy chain wymaga własnych testów/CI/artefaktu. #24 i P0 pozostają otwarte.
Chain UBSan233256/186385 system,238418/191535 original pinned cJSON PASS;
70/68 pinned allocation faults. Rzeczywisty parent BIN jako wyłącznie HOST
fixture9045950/192549 PASS, production zawsze zero app requests. Transport
SDK/TLS jawnie MODEL, nie hardware/Flash/approved Release. Review bez blockerów.

## #24: HTTPS metadane jednego jawnego tagu — nadal kandydat

[Kontrakt](OTA_RELEASE_FETCH_PL.md): wspólny TLS/framing transport, exact własny
GitHub API URL, zero redirects/auth/query/retry, HTTP200/body<=16384/complete.
Close/cleanup/free transport przed realnym parserem; każdy błąd zero candidate.
Nowy heap<=16640 B osobno, fetch<=8192 bez zmian; SDK/TLS/cJSON peak niezmierzony.
Wynik UNVERIFIED, bez bridge do manifest/app, UI/Flash/latest/discovery/instalacji.
Parent PR43 2fb3e452 ma exact CI38871611118 5/5 i sprawdzone BIN/ELF/partycje;
ten etap wymaga własnych testów/CI/artefaktu, nie zamyka #24/P0.
Niezależne testy UBSan129009/system i133415/original IDF cJSON PASS,29/31 OOM;
root powtórzył pinned korpus, istniejące manifest/app/rawHTTP/config regresje PASS.
Koordynator code/memory review bez blockerów; transport/TLS to MODEL, nie płytka.

## #24: pojedynczy Release → kandydat Stable, nie approved Release

[Kontrakt](OTA_RELEASE_PL.md): pure bounded JSON/cJSON parser, jawny tag/asset/
STABLE policy, typed draft/prerelease=false, own-repo API id i jeden uploaded
manifest asset. UTF8/escapes/NUL/decoded duplicates i limity przed akceptacją.
Wynik candidate, nie verified; brak sieci/UI/Flash/latest/version inference.
Production naming, UTC/publisher/signed Release i pełny updater nadal otwarte.
Parent PR42 1153d710: CI5/5/38865021733 i rzeczywisty BIN/ELF/partycje sprawdzone;
actual app SHA cb04113b3d786e8a32a742bef3a1c9a914c67194557d48cfd490a049a3e5c2a7.
Ten parser wymaga własnych regresji, exact-source CI/artefaktu i późniejszej
integracji; nie zamyka #24 ani fizycznego odbioru P0.
Parser UBSan27989/1162 cases,34 positive candidates,27 genuine cJSON OOM PASS;
niezależny Python oracle, UTF8/decoded-dupes/resource boundaries i mutacje.
Cross-review bez blockerów; target i pełne sanitizery CI osobną bramką.
Original IDF cJSON pin+SHA obu plików: UBSan29868/1162/29 OOM PASS;
osobny CI instrumentuje też dependency, nie tylko wrapper z biblioteką systemową.

## #24: pobranie i walidacja app BIN — bez instalacji

[Kontrakt](OTA_DOWNLOAD_PL.md): publiczny manifest-first chain, osobna TLS/
redirect policy dla podpisanego app URL, exact size/SHA/header/board przez
istniejący helper. Bajty są odrzucane, bez Flash/slot/reboot/UI i bez caller-
created verified struct. Każdy błąd zeruje wynik; produkcyjny truststore pusty.
UTC/publisher anchor/signed Release/real TLS i pełny updater nadal otwarte.
Niezależny chain UBSan220127 HOST/168689 empty PASS,36/34 allocation faults;
empty trust zawsze zero app requests. Actual parent BIN jako HOST fixture
10013219/169661 PASS; to nie approved Release, physical TLS ani instalacja.
Raw HTTP regresja potwierdziła SDK-visible Location w trailerze; końcowa
kontrola go odrzuca. Ostatni pending trailer może nie być emitowany przez SDK;
nie claimujemy kontroli wszystkich raw trailerów ani obejścia autentyczności.
To wynik hostowy, nie P0/hardware failure.
PR #41 a298e686: CI5/5/38859714148 i exact-source BIN/ELF sprawdzone;
SHA app516700a62369e957abb9ddf6d6e8e090a68f9b893eae27ea0fa799670365e076.
Nowszy backend wymaga własnego exact-source CI/artefaktu i testu sprzętowego.

## #24: transport manifestu HTTPS — backend, nie updater

[Kontrakt i bramki](OTA_FETCH_PL.md): strict URL/one CDN hop/TLS/framing/bodycap
i istniejący verifier. Date checks włączone; unset UTC odmawia, nie dodano SNTP.
Produkcja ma pusty publisher truststore. Brak LCD/release pickera/app download/
Flash/instalacji. Fizyczny TLS/time/network peak i pełne OTA nadal otwarte.
PR #41 7744651: CI5/5/38858870344, pobrany BIN1772528 B/ELF/partycje/audit
sprawdzone. Dodatkowy test-only raw HTTP harness:3010 przypadków/provider,
UBSan1614269/2109597 PASS, oryginalny przypięty parser IDF; transport SDK nadal
modelowany. Pełny sanitizer CI tego follow-up osobno, nie physical TLS PASS.
PR #40 3501a67: CI5/5/38854805117 i rzeczywisty BIN/ELF/receiver zweryfikowane.

## #24: istniejący dev Web upload — integracja strumienia

[Kontrakt/testy](OTA_UPLOAD_STREAM_PL.md): receiver wykorzystuje helper dla
wszystkich bajtów, exact sink i begin/write/end/abort; auth/approval/partycje
bez zmian. To nadal klientowy checksum + lokalna zgoda, **nie signed OTA**.
Native truststore pusty; fizyczny transfer/rollback i P0 nadal niezaliczone.
PR #39 2a3e7dd: CI5/5,38853370330, zweryfikowany BIN/ELF/partycje.

## #24: strumień app BIN — backend bez Flash

[Kontrakt i regresje](OTA_IMAGE_STREAM_PL.md). Dokładny rozmiar/SHA256,
prefiks app/ESP32-S3/project i marker z NUL niezależnie od porcji; reset
i fail-closed. Lokalny uploader pozostaje niezmieniony, integracja osobno
po testach/review. Brak download/install/boot wyboru i brak physical pass.
PR #38 c9dd6f7 ma CI 5/5 (38848223833) i zweryfikowany artefakt.

## #24: weryfikator manifestu, nie kompletny updater

[Kontrakt i testy](OTA_MANIFEST_PL.md). Produkcyjny truststore pusty, żadnego
testowego klucza w firmware. Brak pobierania, instalacji i fizycznego odbioru.

| Element | Stan | Dalsza bramka |
| --- | --- | --- |
| JSON v2 / canonical bytes / prawdziwy Ed25519 | Implementacja + 117-case niezależny korpus i RFC KAT | Exact CI / footprint / ESP32 stack |
| Zaufany klucz wydawcy i signed Release | Brak; produkcja zawsze odmawia | Zatwierdzony autentyczny anchor / rotacja / wydanie |
| HTTPS download / progres / install / rollback z LCD | Backend fetch/validation-discard częściowo; LCD/progres/install/rollback nadal nie | Osobna integracja #24 i rzeczywisty test zachowania danych |

PR #37 source f7fb16d: CI 5/5, run 38843761384; firmware integrity zweryfikowana,
brak physical pass. Statusy historycznych etapów poniżej nie zamykają #24.

## Pełne globalne Ustawienia — #6 / #24 / #25

[Nowa mapa kliknięć i odbiór](GLOBAL_SETTINGS_PL.md). Status poniżej to
implementacja + lokalne testy hostowe; exact-source CI nowej gałęzi osobno.
Źródło 2641ecd z PR #36 ma CI 5/5 w run 38839617735, ale brak physical pass.

| Ekran / element LCD | Backend / odpowiednik Web | Stan i bramka fizyczna |
| --- | --- | --- |
| Ustawienia → sześć sekcji | Dedykowane globalne UI | Rzeczywisty scroll/cele; płytka do odbioru |
| Sieć: Wi-Fi/SSID/IP, skan/zmiana | Istniejący Wi-Fi worker | Odczyt + istniejący backend; LAN/RF regresja |
| BLE scan / AP / parowanie | Observer BLE; AP/pairing osobno #18/#19 | Scan działa; AP i pairing niewdrożone |
| Wygląd/język/motyw/akcent/pulpit | Wspólny JSON + CAS / Web config | Save/A-B/conflict testy; restart/sync fizycznie |
| Ekran: jasność/timeout/sleep/wake | Istniejący NVS, worker, PWM | PR #36 CI 5/5; timeout wybrany i 0=off, wake bez SET do testu |
| Bateria | GPIO8 ADC / brak Web procentów | PR #35 CI 5/5; multimeter otwarty, procent/niski poziom/charging nieznane |
| WWW: IPv4/mDNS/TLS/SPIFFS/fingerprint/kod | Istniejący HTTPS/pairing | Jawne generowanie i expiry; realny telefon/preview do testu |
| WWW: sesje i unieważnianie | Brak LCD API | Not implemented; brak pozornej akcji |
| System → Firmware Update | Natywny podpisany updater #24 | Not implemented; brak zweryfikowanego Release |
| System → lokalna zgoda OTA | Istniejący upload HTTPS / 120 s | Bool accepted/expiry i negatywne testy; fizyczny transfer/rollback otwarte |
| System → build/board/RAM/PSRAM/Flash/diagnostyka | Read-only snapshot i health | Testy realnego adaptera; płytka do odbioru |
| System → restart | Wspólny backend #24, LCD confirm/cancel | Host/CI/physical gates; bez erase/reset |
| System → eksport/import/reset | Osobne istniejące zadania | Not implemented; bez destrukcyjnej atrapy |
| Ołówek HOME | Tylko pulpity/pokoje; wpisy/sceny w System | Bez ukrytego języka/pairing/OTA; #28 hardware nadal P0 |

Historia #22: obowiązuje **24 h internal** z rzeczywistych pomiarów i bezpieczną
retencją. Opcjonalna dłuższa historia microSD to osobny późniejszy etap, nie P0.
Nie wdrożono historii przez dodanie menu. Natywne OTA wymaga zaufanego klucza,
podpisu, TLS/model/layout/channel/semver/size/SHA, strumieniowego transferu,
postępu/anulowania, świadomej instalacji, zdrowego boot/rollback oraz faktycznie
zatwierdzonego podpisanego wydania; lokalny dev upload tego nie zastępuje.

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
16 równoległych połączeń. Nowy pion #20 ma niezależne endpointy przy jednym workerze. Produkcja nie
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
| Szczegóły Ustawienia | Aktywne: lokalna nazwa/pokój, stable ID, odczyty i celowane odświeżenie | CAS workspace + właściwy fleet endpoint; bez RPC settings | Kod + regresje 0.7.27; CI i odbiór fizyczny osobno | #29A; ustawienia API pozostają #23 |
| Karta sceny / ikona ołówka | Edytor scen | Zapis definicji, bez wykonawcy | CI tested: edycja; Not implemented: uruchamianie scen | #21; nie przedstawiać otwarcia edytora jako wykonania |

## Dodawanie i rzeczywiste urządzenia

| Element LCD | Web Panel | Wymagany model / generacja | Status | Issue / PR; wymagany test |
| --- | --- | --- | --- | --- |
| Skanuj sieć Wi-Fi (Shelly LAN) | Skanuj sieć Wi-Fi | Gen1 / Gen2+ odpowiadające lokalnie | CI tested; poprzednie 3 realne Gen2/Gen3 potwierdzone fizycznie | #7, PR #10; powtórzyć LAN z BLE włączonym |
| Lista LAN / Sprawdź / Login / hasło / pokaż | Te same kroki | Gen1 Basic / Gen2+ Digest | CI tested: rzeczywisty HTTP parser/identity/capabilities | #7; hasło poprawne/błędne, timeout, DHCP |
| Nazwa / pomieszczenie / kanał / Zapisz | Te same pola i binding | Wspierany pojedynczy switch | CI tested: weryfikacja przed zastąpieniem starego bindingu | #7; restart, trwałość, błąd nie kasuje starego |
| Ponów / Anuluj LAN | Skanuj ponownie / Przerwij | Radio Wi-Fi / LAN | CI tested: oddzielne zadanie i stan | #7; realny timeout i odłączenie Wi-Fi |
| Dodaj ręcznie IP / klawiatura / sprawdź | Edycja hosta jest definicją; operacyjny binding przez LAN | Wspierany relay/switch | CI tested: lokalny bind, walidacja IPv4/kanału/identity | #7/#8; ręczny IP i błędne dane |
| Wi-Fi AP | Kreator na LCD; WWW handoff integrowany osobno | Otwarte fabryczne Gen1/Gen2/3/4 z HTTP API bez hasła | Rzeczywisty driver/HTTP/LVGL; fizyczny odbiór i exact CI otwarte | #19; [zakres i odbiór](ISSUE19_AP_PL.md), dwie fizyczne zgody, RAM AP → home LAN → BIND |
| Bluetooth LE → Szukaj / Anuluj / Ponów | Bluetooth LE • skan, wspólne `/api/v1/ble*` | ESP32-S3 observer + faktyczne legacy ADV | CI tested: rzeczywisty adapter/parsing/UI; właściciel potwierdza odbiór RF, exact SHA niepodany | #18, PR #26; nadal >=10 cykli coexistence i porównanie payloadu |
| Wynik BLE / szczegóły / RSSI / ostatnia reklama | Grupy i wyniki skanera | RPC UUID, Shelly manufacturer, BTHome v2 | CI tested: bez zgadywania generacji/modelu; wykryto ≠ sparowano | #18; model Wi-Fi i BLU jeśli dostępne, pusty skan, brak radia |
| Wynik BLE RPC → Szukaj w LAN | Powrót do metod i LAN | Urządzenie faktycznie obecne w LAN | CI tested: przejście do istniejącego kreatora | #7/#18; BLE nie dodaje sam do Wi-Fi |
| BLE pairing / provisioning / stałe BLU | Wyłącznie tekst o ograniczeniu, bez atrapy Add | Zgodny GATT/RPC/bonding/BTHome | Not implemented | #18 B1/B2/B3; osobny test modelu, autoryzacji i kluczy |
| Wiele urządzeń Shelly | Trwały registry do16 endpointów + własny stan/sterowanie | Bounded serial I/O, nie16 równoległych połączeń | Kod + genuine native/LVGL; exact CI i fizyczny odbiór otwarte | #20; min2 fizyczne Shelly, offline/DHCP/restart/heap |
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
| Suwak jasności + Zapisz | Brak zdalnego suwaka | Istniejący rekord NVS, zachowane Wi-Fi | PR #36 source2641ecd CI 5/5 | #6/#24; restart i wybudzenie z zapisaną jasnością |
| Screen off / touch to wake | Brak zdalnego sterowania | Sterownik ES3C35P | Physically tested historycznie; nowa regresja wymagana | #24; wybrany timeout, 0=off, dotyk, brak niechcianego SET po wybudzeniu |
| Timeout ekranu + Zapisz | Brak zdalnego sterowania | Istniejący rekord NVS; 0 = bez autooff | PR #36 source2641ecd CI 5/5 | #6/#24; fizyczne wygaszenie i first-touch suppression |
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

1. P0 SPIFFS #28 / PR #33: fizyczny pierwszy boot, workspace/HTTPS, Save/restart; bez ponownego erase bez zgody.
2. Niezależne Ustawienia/bateria #6/#25 i backend podpisanego OTA #24; osobne Draft PR, klucz/wydanie i hardware nadal bramkami.
3. AP #19 i BLE #18 według potwierdzonych backendów; #20 oraz #29/#34 respektują zależności magazynu i fizyczny odbiór.
4. Kontrolery #23 i grupy/sceny #21 po zależnościach; historia 24h internal #22 i widoki #29. microSD osobno później.

Każde przejście statusu musi wskazywać exact source SHA, CI i dowód sprzętowy,
jeśli używa Physically tested. Nie zamykać #25 ani nie publikować Stable, dopóki
komplet nie ma operacyjnych działań dla wspieranych modeli i akceptacji właściciela.
