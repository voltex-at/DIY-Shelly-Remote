# #24 — integracja strumienia w istniejącym dev uploadzie (0.7.13)

## Izolacja obrazu diagnostycznego — dodatkowa bramka review

PR #39 wymuszał linkowanie helpera obrazu również w diagnostyce. Recenzja
koordynatora i pobrany dokładny artefakt diagnostycznego ESP32-S3 potwierdziły
pełny marker z NUL w loadable `.flash.rodata` (0x3c0283ee), nie tylko debug
strings. To podważa separację profilu board przy późniejszym wygenerowaniu
diagnostycznego app BIN; nie próbowano fizycznej instalacji takiego obrazu.
Teraz wymuszenie helpera dotyczy wyłącznie `DSR_BOARD=lcdwiki-es3c35p`.
CI obu diagnostycznych targetów sprawdza **rzeczywisty wygenerowany app BIN**
pod kątem braku markera. Nie publikuje diagnostycznych BIN-ów. Prawdziwy
target LCDWiki nadal musi zachować marker i realne helper entrypoints.

To naprawa istniejącej ścieżki lokalnego Web uploadu, **nie natywny signed OTA**.
Kontynuuje niezależnie zrecenzowany helper PR #39 (2a3e7dd, CI 5/5).
P0 #28 / PR #33 i właścicielski odbiór fizyczny pozostają nadrzędne.
Nie jest to polecenie wgrania nowej wersji, erase ani publikacji Stable.

## Bez zmiany autoryzacji i partycji

Kod `api()`, `local_request()` i `authorized()` pozostaje bez zmian: local
same-origin, boot-ready, cookie/pairing i istniejący routing. `ota()` nadal
jednorazowo zużywa zgodę LCD 120 s pod istniejącym lockiem, także przy błędnym
żądaniu. Nie ma automatycznego restartu. Nieaktywny slot i limit długości nadal
wybiera/sprawdza istniejący caller. Boot slot zmienia się wyłącznie po sukcesie
receivera i końcowego `esp_ota_end`; jego błąd nadal daje HTTP 500.

Legacy dev upload sprawdza **klientowy checksum**, nie podpis wydawcy. Nowy
jawny `create_dev_checksum` współdzieli walidację bajtów, ale nie tworzy
fikcyjnego `dsr_ota_verified_t`. Native `create/reset(verified)` nadal wymaga
zaakceptowanego manifestu. Produkcyjny truststore jest pusty; żadnego klucza
testowego/TLS w roli wydawcy. Downloader, podpisane wydanie, progres LCD,
anulowanie i zdrowy boot/rollback native OTA nadal są otwarte.

Dotychczas tylko 64 małe znaki hex `X-SHA256` mogły zakończyć upload sukcesem:
duże przechodziły `isxdigit`, lecz nie końcowe porównanie z małym hex. Teraz
duże/niepoprawne znaki są odrzucane **przed Flash**, zamiast po całym transferze.
Nie poszerzono zbioru akceptowanych checksumów.

## Rzeczywisty receiver

`dsr_ota_upload_receive` jest tym samym kodem kompilowanym dla IDF i testów.
Odbiera każdą porcję raz i przekazuje wszystkie bajty do SHA/KMP helpera,
łącznie z prefiksem 512 B. Dopiero po jego walidacji wykonuje `begin`, zapisuje
buforowany prefiks i resztę bieżącej porcji dokładnie raz, w pierwotnej kolejności.
To usuwa oba odtworzone błędy rozpoznawania markera z PR #39.

Brak `recv`/timeout retry policy change: `n<=0` kończy próbę. Niemożliwe
`n>want` jest odrzucane przed użyciem danych. Błędy read/SHA/hash/marker/write
po udanym begin powodują dokładnie jeden abort i brak end/boot selection.
Begin failure nie tworzy live handle. Po `esp_ota_end` handle jest nieważny
**również przy błędzie**, zgodnie z przypiętym IDF v5.5.4
[kontraktem](https://github.com/espressif/esp-idf/blob/v5.5.4/components/app_update/include/esp_ota_ops.h).
Nie wolno go ponownie abortować. Receiver nie wybiera boot ani nie restartuje.

Bufory 4096+512 B są w jednym heap workspace, docelowy `_Static_assert`
wymaga dokładnie 4608 B; kontekst SHA/KMP z PR #39 ma 724 B na ESP32-S3.
Nie ma `probe[4096+80]` ani dużej lokalnej tablicy w ścieżce uploadu.
OOM jest obsługiwane przed begin, bez nowego taska/timera. Target `.su`/obj
audit dotyczy bezpośrednich ramek i zależności, nie peak stosu TLS/mbedTLS.

## Dowody i brakujące testy fizyczne

Hostowe minimalne SDK shims nie zapisują urządzenia: przechwytują dokładne
bajty sinka i kolejność begin/write/end/abort. Test rzeczywistej funkcji
`ota()` jest generowany z aktualnego źródła, nie drugą implementacją endpointu.
Oddziela test jednorazowej zgody/odpowiedzi/boot wyboru od niezmienionych
zewnętrznych bramek auth. ASan/UBSan/LSan i docelowe kompilacje wymagane w CI.

Lokalnie UBSan: 136002 kontroli rzeczywistego receivera/endpointu, 2741363
helpera, pełny native/config265 oraz LVGL2237/96 przechodzą. Recenzent i
koordynator nie znaleźli blokera przepływu receivera. Wynik nowego target CI
i integralność jego artefaktów pozostają oddzielnymi bramkami.

Fizycznie nadal potrzebne: P0 storage/workspace/HTTPS READY i trwały Salon,
następnie świadomie zatwierdzony lokalny transfer/odmowy/błędny checksum,
przerwanie i zachowanie poprzedniego boot oraz Wi-Fi/Rooms/zdjęć. Test hostowy
ani zielone CI tego nie zastępują. Nie kasować niepustych danych, nie zmieniać
NVS/partycji, nie zmieniać testowanej instalacji bez zgody właściciela.
