# #24 — walidator strumienia app BIN, nie instalator (0.7.12-dev)

Opis etapu PR #39 poniżej jest historyczny. Jego późniejsza integracja legacy
dev uploadu ma [osobny kontrakt i testy](OTA_UPLOAD_STREAM_PL.md); nie jest
podpisanym updaterem i nie omija manifest-first native API.

Ten etap kontynuuje verifier manifestu z PR #38. Nie zmienia lokalnego
uploadera, autoryzacji, partycji, SPIFFS, NVS ani testowanego obrazu P0.
Nie pobiera danych, nie wywołuje `esp_ota_begin/write/end`, nie wybiera slotu
i nie zmienia boot/restart. Nie zamyka #24 ani fizycznego odbioru #28.

## Kontrakt

`dsr_ota_image_create/reset` przyjmują kopię rozmiaru i SHA256 z wyniku
**zaakceptowanego** `dsr_ota_manifest_verify`. Sam jawny C struct nie dowodzi
autentyczności — przyszły caller musi zachować tę kolejność. Produkcyjny
truststore nadal pusty, bez kluczy testowych, TLS identity i sekretów wydawcy.
Kanał/model/layout/wersja/URL są wiązane podpisem manifestu i hashem BIN;
ten helper nie zmienia polityki manifestu ani nie porównuje dev suffix do semver.

Każdy bajt od pierwszego `update` trafia dokładnie raz do prawdziwego
mbedTLS SHA256. Licznik ma limit 512 B–6 MiB; nadmiar jest odrzucany przed
odczytem danych. Końcowy `finish` wymaga dokładnej długości, SHA256 i pełnego
markera board **z NUL**. `update=OK` nie jest finalną akceptacją. Błędy są
trwałe do jawnego resetu; reset usuwa poprzedni hash, prefiks i stan markera.
Brak nowego zadania/timera; kontekst jest heap-owned, bez bufora 4096+80 na stosie.

Prefiks 512 B zachowuje kontrolę app magic, ESP32-S3/chip=9, app-desc magic
i dokładną nazwę projektu `diy_shelly_remote` z NUL. Dodatkowo wymaga
1–16 segmentów i pierwszego segmentu mieszczącego descriptor 256 B oraz
zadeklarowanego rozmiaru. Odczyty bajtowe LE nie zakładają wyrównania.
Pola pochodzą z przypiętego IDF v5.5.4:
[image header](https://github.com/espressif/esp-idf/blob/v5.5.4/components/bootloader_support/include/esp_app_format.h),
[app descriptor](https://github.com/espressif/esp-idf/blob/v5.5.4/components/esp_app_format/include/esp_app_desc.h).
To nie pełny parser segmentów/checksum/revision/secure boot. Przyszła instalacja
nadal wymaga końcowego sprawdzenia IDF i osobnego bezpiecznego boot/rollback.

## Potwierdzona regresja starego skanera

Stary `dsr_panel.c::ota()` nie skanuje wcześniejszych porcji podczas zbierania
512 B. Ponadto jego tail zachowuje tylko ostatnią porcję zamiast końca całego
strumienia. Rozpoznanie markera może zależeć od podziału TCP, także dla porcji
1 B po prefiksie. Regresja hostowa odtwarza sam skaner bez Flash. Nie jest to
nowy dowód fizycznej awarii ani wyjaśnienie przerwanego formatu SPIFFS.
Nowy helper używa KMP, włącznie z całym prefiksem i dowolną granicą porcji.

## Weryfikacja i otwarte bramki

Testy obejmują wszystkie dwuporcjowe podziały, porcje 1 B, granice prefiksu
i markera, truncation/extra/hash/header/marker errors, reset/sticky failure,
błędy alokacji i SHA. Hostowy adapter uruchamia prawdziwy OpenSSL SHA256;
target używa przypiętego mbedTLS IDF. CI sprawdza rzeczywisty target i budżet
stosu/kontekstu, a nie tylko hostowe sizeof.

Lokalnie UBSan: **2 741 363 kontroli** w niezależnym zestawie recenzenta,
w tym rzeczywisty strumień 6 MiB, odmowa `SIZE_MAX` przed odczytem bufora 1 B,
kopiowanie metadanych i reset w połowie prefiksu/markera. Native/config 265,
LVGL 2237/96 i izolowany review 2635/24 przechodzą. Recenzent i koordynator
nie znaleźli blokera helpera. To nadal nie pomiar stosu/mocy na sprzęcie.

Integracja lokalnego uploadera jest **osobnym kolejnym etapem**, po testach
i niezależnym review; nie wolno zmieniać autoryzacji ani partycji przy tej okazji.
Native download/progress/cancel/install/rollback/LCD nie są zaimplementowane.
Zaufany publisher anchor i zatwierdzony signed Release pozostają bramkami
rzeczywistej instalacji. P0 SPIFFS nadal czeka na właścicielski test sprzętowy;
bez dodatkowej zgody nie wolno powtarzać erase ani formatować niepustych danych.
