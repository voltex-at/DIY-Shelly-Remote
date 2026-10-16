# Natywne podpisane OTA — #24, 0.7.26 DEV

## Pełny przepływ kodu, osobny odbiór wydania i sprzętu

System → Firmware Update jest rzeczywistym klientem na istniejącym pojedynczym
workerze I/O. Brak nowego zadania aplikacji i wzrostu `dsr_job_t`/`dsr_event_t`.
Kanał domyślny Stable; Dev wybierany jawnie z ostrzeżeniem, bez automatycznego
pobierania/instalacji. CHECK pobiera tylko katalog i podpisany manifest, nie BIN.
Aktualna wersja pochodzi z lokalnego deskryptora aplikacji; zaufana baza X.Y.Z
jest wydzielana z własnego build suffix, nigdy z tagu GitHuba.

Katalog jest **niezaufaną wskazówką**: Stable używa dokładnego own-repo
`/releases/latest`, Dev `/releases?per_page=5`. Pierwszy non-draft prerelease
jest wybierany przed walidacją assetu; błędny pierwszy wpis nie powoduje próby
kolejnego. Nie sortujemy tagów jak semver, nie przechodzimy na inny kanał.
Ścisły parser16KiB/depth8/nodes512, kanoniczne unsigned liczby, poprawny UTF-8,
brak NUL i zduplikowanych decoded keys, dokładne own-repo ID/URL i pojedynczy
asset `DIY-Shelly-Remote-es3c35p-manifest.json`. Nazwa to kontrakt tego klienta,
nie dowód opublikowania prawdziwego wydania. Release notes są plain text,
oddzielnie oznaczone jako niezweryfikowane, do320B na granicy kodpunktu;
LCD ogranicza ich wysokość240px, bez HTML/recolor. Dane te nie autoryzują Flash.

CHECK uruchamia SDK SNTP dopiero na jawne żądanie, jeżeli czas jest nieustawiony.
Maksymalnie20 kooperacyjnych oczekiwań po1s, z anulowaniem pomiędzy nimi.
SNTP nie jest kryptograficznym źródłem czasu ani zaufaniem do wydawcy. Pobrania
nadal wymagają normalnego CA bundle/hostname/date verification i czasu>=2025;
nie ma obejścia dat TLS. Poszczególne fetch zachowują5s SDK I/O timeout oraz
kooperacyjny budżet30s; nie jest to ścisły łączny wall-clock deadline DNS/TLS.

## Zgoda i zapis

Tylko genuine Ed25519 manifest OK dla wybranego kanału, board/layout, nowszej
wersji i rzeczywistego nieaktywnego slotu tworzy AVAILABLE. Dalsza fizyczna
zgoda LCD wyświetla wersję i rozmiar oraz wymaga potwierdzenia stabilnego USB.
Nie udajemy pomiaru ładowania/SOC: ADC nie daje wiarygodnego power-source flag.
Zgoda jest jednorazowa, ważna120s od CHECK; hide/cancel ją unieważnia. Brak
zdalnego HTTP install API, auto-check, automatycznego restartu i replay.

INSTALL świeżo pobiera i weryfikuje manifest. Dokładne version/channel/URL/
size/SHA muszą odpowiadać potwierdzonej krotce; ta krotka sama nie jest kluczem
zaufania. Dopiero potem APP: prefix512/header gate → nieaktywny esp_ota_begin →
każdy bajt raz do esp_ota_write → transport close/cleanup, exact size/hash/image
i late cancel → esp_ota_end (pełna walidacja SDK) → wybór boot partycji.
Nie buforujemy całego BIN. Postęp jest synchronizowanym małym snapshotem;
worker nie dotyka LVGL. Anulowanie jest kooperacyjne, cleanup pozostaje na workerze.

Web development uploader i native installer dzielą opaque staging reservation.
HELD obejmuje cały cleanup i wybór boot. Potwierdzony wybór daje STAGED;
błąd SDK po próbie wyboru daje UNCERTAIN: nie wiemy, czy fizyczne otadata
zostało już zmienione. Oba terminalne stany blokują kolejne Web/native zapisy
do restartu i oddają activity lease. LCD wyjaśnia niepewny stan także po late
cancel, nigdy nie przedstawia go jako bezpiecznie anulowanej instalacji.
Restart używa istniejącego backendu i jawnego przycisku. Żadnego erase/reset.

`dsr_ota_install_ready()` i INSTALL sprawdzają ten sam dokładny układ:
ota_0 0x20000/0x600000, ota_1 0x620000/0x600000,
dsrdata 0xc20000/0x3e0000, running=jedna app, next=druga app. READY boot wymagany;
gotowość CHECK to point-in-time, INSTALL powtarza wszystkie bramki.

## Zaufanie, rollback i dowody

**Produkcja nadal ma EMPTY truststore.** Nie dodano klucza testowego, podpisu
właściciela ani Stable Release. Bez zatwierdzonego autentycznego anchor klient
odmawia przed APP/Flash. Fixture key istnieje wyłącznie w host suite i nie
trafia do firmware. Zatwierdzenie klucza/publikacja wydania wymaga osobnej decyzji
właściciela; kod nie wymusza tej decyzji teraz.

Istniejące delayed health confirmation po UI/HTTPS/touch gotowości pozostaje.
Rollback nowego SDK/bootloadera i zachowanie NVS/workspace/zdjęć wymagają osobnego
fizycznego testu; kompilacja i modele SDK nie dowodzą zachowania realnego Flash.
Nie flashowano ani nie restartowano urządzenia w tym etapie.

Host suite: genuine fetch/signature/SHA/image z modelami HTTP/partition/SDK,
header splits1..512, OOM/cancel/transport cleanup/zmieniona zgoda/SDK faults,
success i UNCERTAIN w świeżych procesach, EMPTY zeroAPP/begin/write/end/boot,
pthread shared reservation/restart race. Niezależny actual actor lifecycle,
120s/stale id/channel/power/one-shot/late cancel, actual SNTP gate i strict
discovery parser/fetch. Rzeczywisty LVGL obejmuje PL/EN/DE, literal notes,
fizyczne potwierdzenie, hide/late result, expired consent, staged/uncertain,
restart i40 cykli nawigacji. Screenshots sukcesu to jawny TEST/DEMO backend.

Target CI kontroluje realny linked ELF/symbole, own static frames i BSS<=80KiB,
worker own frame<=2048B, UI8192/LVGL131072PSRAM zachowane. Nie mierzy szczytu
SDK/TLS/cJSON/transitive heap/stack ani fizycznego jitteru Wi-Fi+BLE.
#24 pozostaje otwarte do autentycznego wydania i niezależnego odbioru sprzętu.
