# #22: rzeczywista historia INTERNAL 24 h

Ten pion dodaje wspólną historię wybranego Switch/relay: dolna **Energia**, karta
**Wykresy**, HTTPS Devices → Energy/history. Zakresy to 1 h i 24 h, pomiary W/V/A
oraz rzeczywisty licznik Wh, jeśli urządzenie go zwraca. Nie ma SD, 7 dni,
estymacji Wh z W, lokalnej strefy zgadywanej z języka ani danych DEMO w firmware.

## Dane i tożsamość

Próbka powstaje wyłącznie po nowym, skorelowanym i zaakceptowanym zakończeniu
PROBE albo SET z mandatory readback, przy nadal zgodnym bindingu i domowym Wi-Fi.
BIND, błąd, stary event, opublikowany snapshot i odświeżenie ekranu nie dopisują
próbek. request ID zapobiega ponownemu naliczeniu tej samej odpowiedzi.

Klucz to `(device_id, generation, endpoint_kind, channel)`. Host, nazwa i numer
pozycji na liście nie są kluczem. Numeric layout ID jest dodatkowym warunkiem
wybranego LCD bindingu. Dziś `kind=0` oznacza Switch/relay; inne kind są jawnie
odrzucane i zachowywane na dysku. Następna integracja Cover musi rozszerzyć
registry/metadane świadomie, bez aliasowania `switch:0` z `cover:0`.

Źródło jednostek: oficjalny [Switch.GetStatus](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Switch/)
ma `apower` w W, `voltage` w V, `current` w A i **`aenergy.total` w Wh**.
`aenergy.by_minute` jest w mWh i nie jest używane jako total. Brak/null total
pozostaje brakiem, rzeczywiste `0` jest punktem. Niewłaściwy typ/ujemny/nieskończony
lub ponadlimitowy total odrzuca odpowiedź. [Gen1](https://shelly-api-docs.shelly.cloud/gen1/)
`/relay/N` nie ma tego pola; odrębne `/meter/N.total` jest w watt-minute. Ten pion
nie dodaje odpytywania Gen1 meter i nie przenosi jego jednostek do Gen2.

W/V/A są średnimi niezależnie obecnych odczytów w oknie 5 min, z własnymi licznikami
próbek. Wh jest ostatnim rzeczywistym stanem licznika, nigdy sumą lifetime między
urządzeniami. Spadek ciągłego licznika oznacza reset; brak pola lub przerwa próbek
ponad 15 s oznacza lukę. Zero, brak, offline/stale i reset pozostają rozróżnione.
Globalna Energia sumuje tylko świeże, porównywalne W i usuwa duplikaty physical key.

## UTC i współdzielenie czasu

Bez poprawnego UTC live telemetry działa, ale historia nie tworzy fałszywych
absolutnych punktów. Licznik `samples_without_time` pokazuje odrzucone odczyty.
Cofnięcie czasu odrzuca kolejne próbki do nadrobienia poprzedniego UTC. Oś jest
zawsze jawnie UTC. Pierwsze 5-minutowe okno może przecinać początek 1 h/24 h;
ring przechowuje 289 okien (24 h + bieżące). Luki nie są ściskane ani interpolowane.

SNTP uruchamia się leniwie tylko po jawnym przycisku LCD/WWW. `dsr_clock_sync`
wykonuje na istniejącym workerze najwyżej 20 oczekiwań po 1 s, respektuje cancel
i ma jeden static initialization owner. Native OTA używa teraz cienkiego bridge
`dsr_native_ota_clock.c` do `dsr_clock.c`, z tym samym kompletem testów162.
Istnieje dokładnie jeden owner `esp_netif_sntp_init`. NTP nie uwierzytelnia wydawcy OTA.

## Footprint i zapis

Model i dziennik mają jawny format little-endian i CRC, niezależny od paddingu C.
Skompilowany proof hosta: point32 B, slot144 B, model168776 B, query9360 B.
Context, query, JSON i immutable checkpoint używają PSRAM bez fallback do dużych
alokacji internal. Compile bounds: context192 KiB, query16 KiB, JSON49152 B,
checkpoint186272 B. Dwie jednoczesne query LCD/HTTPS oraz checkpoint/JSON/context
to około 425 KiB PSRAM, poza istniejącym LVGL i SDK; to nie pomiar peak na ESP32.

Wyłącznie istniejący sole worker ładuje/flushuje/rotuje `/dsr/history0.dat` i
`/dsr/history1.dat`. SPIFFS object `/history0.dat` ma 13 bajtów, mieści się w
zachowanym `CONFIG_SPIFFS_OBJ_NAME_LEN=32`. Nie zmienia się geometria/partycje,
mount/format, NVS, workspace schema ani konfiguracja urządzeń.

Limit każdego pliku to256 KiB, łącznie512 KiB. Zapis zwykły obejmuje zamknięte
okna: max16 ×288 ×40 =184320 B/dobę logicznego append. Największy checkpoint to
32 +16 ×80 +16 ×289 ×40 =186272 B. Przy zapełnionym ring rotacja następuje mniej
więcej co9,8 h, razem około0,61 MiB/dobę; konserwatywnie poniżej1 MiB/dobę bez
churn tożsamości. To nie uwzględnia write amplification SPIFFS, GC ani oceny
żywotności konkretnego układu Flash. Brak zapisu przy każdej klatce/odczycie.

Snapshot powstaje pod krótkim mutexem RAM; mutex jest zwalniany przed wszystkimi
operacjami plikowymi. Obecne worker admission/activity zapewnia wyłączność z AP,
Wi-Fi i restartem do odebrania eventu. Nowe joby nie dodają osobnego payloadu; nie ma nowego
taska ani kolejki próbek. Wspólne struktury job/event przenoszą rozszerzoną
telemetrię, której wzrost podlega istniejącym bramkom pamięci/stosu. Telemetry rozszerza się o
`has_energy` +double (24→32 B; Cover może wykorzystać pozostałe5 flag bytes).

Przed load jest sprawdzona dostępność workspace i SPIFFS. Zapis zachowuje co
najmniej64 KiB wolnego i nie przekracza konserwatywnego progu75% partycji. Nowy
checkpoint trafia do drugiego pliku; flush/fsync i pełny readback muszą przejść,
zanim stanie się bieżącym. Append również wymaga flush/fsync/exact readback.
Po starcie stosuje się tylko kompletny, CRC-valid checkpoint i kompletny append;
ucięty tail własnego znanego dziennika może zostać obcięty do dokładnego valid
boundary. Nie jest to naprawianie SPIFFS ani innych danych.

Brak montowania, oba checkpointy uszkodzone lub nowszy schema/kind → pliki są
zachowane, zapis zablokowany do restartu, nowe próbki mogą działać w RAM. Status
IO/FUTURE i `durable=false` jest widoczny LCD/WWW. Błąd późniejszego zapisu zostawia
dirty points i retry5 s, nie daje fałszywego durable ACK. Bieżące niezamknięte
okno jest RAM-only: restart może stracić do5 min. Poprawny zapis nie dowodzi
fizycznego power-loss zachowania kontrolera/Flash.

## Wspólne API i testy

Authenticated same-origin `POST /api/v1/history/query`, body≤256 B, przyjmuje
wyłącznie exact identity/generation/channel/kind0/hours1|24; nie przyjmuje hosta,
ścieżki pliku, credentials ani polecenia. LCD korzysta z tego samego C query.
`POST /history/clock` i `/history/clock/cancel` mają puste body, nie robią I/O
w HTTP i nie replayują automatycznie żądania. Dotychczasowe auth gates zachowane.

`tools/test_history.py --ubsan-only`: rzeczywiste model/store/wrapper/API/clock,
real cJSON i pliki hosta; SDK/allocator/clock są modelami. Każde byte cut nowego
checkpointu i append, flush/fsync faults, pełne16×289, identity reuse, request
repeat, zero/missing/reset/no-time, future kind/schema, unmounted/both-corrupt
RAM-only/no-writes, HTTP shape, OOM, SNTP init/cancel i restart admission.
`tools/test_ui.py --ubsan-only`: actual pinned LVGL, nowy Charts/Energy flow,
1 h/24 h, W/V/A/Wh, UTC/no-time, PL/EN/DE i40 cycles z równym heap98520→98520.
SCATTER używa tylko finite XY: unika sentinel overflow pinned LVGL LINE oraz
zachowuje realny odstęp UTC bez interpolacji. `tests/test_panel_history_model.cjs`
wykonuje shipped JS; browser regression rozszerza `test_panel_runtime.cjs`.
`check_history_target.py` ma syntetyczne rejection fixtures; final target ELF/.su
musi potwierdzić istniejące BSS81920/worker-own2048/UI8192/LVGL131072 limity.

Integracja opiera się na P0 PR57/source51124b4, exact-head CI38915260953
5/5 i zweryfikowanych FACTORY/UPDATE. Kod BIND i syntetyczny save/restart mają
regresje; przyczyna przerwania blank-format oraz odbiór sprzętowy pozostają
otwarte. Ten Draft nadal nie upoważnia do merge/Stable/device writes. Nie wykonano
sprzętu, format/erase, publikacji ani testu fizycznej retencji/endurance. Dokładny
status target CI i browser jest oddzielną bramką, nie wynika z testów modeli.

## Regresja: puste okno pomiarowe przed restartem

Niezależny review wykrył mask0 bucket w dzienniku: valid → all-fields-missing →
valid, po istniejącym checkpoint i flush, dawało przed poprawką tylko1 valid
point po reload i `repair=true`. Puste okno jest teraz wyłącznie luką UTC: seal
nie ustawia dirty, encoder odmawia mask0, a service defensywnie pomija takie
rekordy z exact version check. Genuine wrapper/store regression po poprawce
odtwarza2 valid points, `repair=false` i pełny odstęp600 s. Reader pozostaje
ścisły; zero pomiaru nadal jest poprawnym punktem z nonzero presence mask.

## Regresja: różny początek zegara ESP i LVGL

Planowanie load/flush/retry używa jednego zegara: dolne32 bity milisekund
`esp_timer_get_time()/1000`. LVGL może wystartować później niż timer ESP;
przekazanie `lv_tick_get()` do historii dodawało ten offset do opóźnienia.
Actual LVGL regression odtwarza120 s inicjalizacji przed startem LVGL, a genuine
wrapper/store sprawdza natychmiastowy load i ponowienie po błędzie dokładnie po5 s.
Oś i zapisane znaczniki UTC pozostają bez zmian; nie powstaje drugi owner SNTP.

Integracja zachowuje lokalny port PR34 i dodatkowe niezależne testy stableID/host
reuse, porównane z równoległym PR55. Nie opublikowano drugiego PR29 i nie
nadpisano gałęzi PR55. Historia jest nowym pionem #22 zależnym od ustawień.
Dołączona poprawka nazw zdjęć zachowuje pełne128bit IDs i geometrię;
[osobny kontrakt](PHOTO_FILE_NAMES_PL.md).
