# #24 — pobranie i walidacja app BIN bez instalacji (0.7.15-dev)

Kontynuacja PR #41, nie kompletny updater. P0 SPIFFS #28 / PR #33 nadal
wymaga fizycznego odbioru. Bez nowego erase/formatu nonempty, main ani Stable.

## Łańcuch i granica autoryzacji

`dsr_ota_fetch_app(manifest_url, policy, control, out, manifest_result,
image_result)` najpierw wykonuje rzeczywisty bounded HTTPS manifest fetch
i Ed25519 verifier. **Wyłącznie DSR_FETCH_OK** pozwala rozpocząć żądanie app.
API nie przyjmuje `dsr_ota_verified_t` od callera jako dowodu zaufania. Pusty
produkcyjny truststore kończy przed jakimkolwiek żądaniem BIN. Testowe klucze
RFC, TLS identity i CA bundle nigdy nie stają się wydawcą.

Podpisany URL, size i SHA trafiają do istniejącego `dsr_ota_image_create`;
nie używa się legacy `create_dev_checksum`. Nowe app żądanie zaczyna od
podpisanego GitHub repo URL, nie od Location manifestu. Osobny maksymalnie
jeden hop do dokładnego release-assets hosta, świeży TLS/hostname/CA client
na każdy hop, te same limity URL/nagłówków/framing i brak auth/credentials/
cookies/autoredirect. [Szczegóły transportu i zegara](OTA_FETCH_PL.md).

CL aplikacji musi być równy podpisanemu rozmiarowi; chunked ma ten sam
rzeczywisty limit. Heap buffer jest używany ponownie, read<=1024 B; każdy
odebrany bajt trafia do helpera dokładnie raz, także cache razem z nagłówkami.
Nadmiar jest odrzucany przed helperem, bez whole-image alokacji. Na końcu
wymagane SDK complete i image finish: exact size, rzeczywisty SHA256,
ESP32-S3/project/header i pełny board marker z NUL.

Out jest publikowany dopiero po całym sukcesie i finalnym cancel/budget check;
każdy błąd go zeruje. `manifest_result`/`image_result` są diagnostyką, nie
autoryzacją: mogą być OK mimo późniejszego błędu transportu/cancel/cleanup.
Wszystkie utworzone clients są close/cleanup przed finalnym verifierem;
helper i konteksty zwalniane na każdej ścieżce. Argumenty pozostają immutable,
out należy wyłącznie do operacji do jej zakończenia.

**Bajty aplikacji są odrzucane.** Brak sinka instalacyjnego, Flash/esp_ota,
wyboru slotu/reboot, taska/UI/progress, release pickera czy automatycznej
aktywacji. Funkcja nie ma obecnie produkcyjnego callera. Sukces nie daje
dostępu do wcześniejszych bajtów i nie upoważnia do późniejszej instalacji:
przyszły installer musi pobrać/walidować ponownie, pod własnymi bramkami.
Helper nie zastępuje pełnego IDF segment/checksum/revision/bootability check,
nie porównuje semver z dev stringiem w descriptorze i nie zmienia polityki
manifestu/version/channel/layout/board.

## Budżet i źródła

Każda faza osobno ma kooperacyjny budget30s i SDK I/O timeout5s. App read
1024 B to świadomy kompromis throughput vs częstotliwość owner-task polls.
Źródło [przypiętego klienta IDF](https://github.com/espressif/esp-idf/blob/735507283d5b2f9fb363a1901172dbd9e847945d/components/esp_http_client/esp_http_client.c)
pokazuje wewnętrzne wielokrotne odczyty do żądanej długości. Positive-progress,
DNS/TLS i headers mogą przekroczyć budget przed powrotem: **nie twardy total
deadline**. Nie uruchamiać w UI task, nie zamykać client z innego taska.

Manifest fetch context jest zwolniony przed app fetch. Target-compiled bounds:
fetch<=8192 B + chain<=512 B; osobno image<=1024 B (aktualny helper724 B).
To nie całkowity RAM/peak: JSON/SDK/TLS/header allocations i transitive stack
są dodatkowe. Target .obj/.su/ELF/sdkconfig audit sprawdza rzeczywiste symbole,
ramki<=512 B, wymagane image API i brak direct dev-checksum/Flash/slot/restart.
Rzeczywisty peak, czas, Wi-Fi/TLS i zachowanie sprzętowe pozostają otwarte.

## Regresje i granice dowodów

Raw-wire test najpierw potwierdził, że SDK-visible Location w chunked trailerze omijał
kontrolę final-200 wykonywaną przy nagłówkach. Końcowa kontrola po body/cleanup
odrzuca taki callback przed akceptacją. UBSan3012 przypadków/provider:1615474 HOST/
2110802 production-empty PASS. To hostowa luka polityki, nie fizyczny failure
ani przyczyna przerwanego SPIFFS. Oryginalny IDF parser jest przypięty; header
assembly/cache/transport są nadal modelami, nie pełnym esp_http_client/TLS.
Primary-source review ujawniło też, że IDF5.5.4 nie emituje ostatniego pending
trailera przy message-complete, a public API nie ma jego read-only gettera.
Nie gwarantujemy odrzucenia wszystkich surowych trailerów; kontrolujemy headers
udostępnione przez HTTP_EVENT_ON_HEADER. Raw harness już nie flushuje ostatniego
trailera; negatywy mają X-After, aby wymusić SDK-visible event. Nie podążamy za
Location z 200. Podpis, exact rozmiar/SHA i image finish nadal są wymagane;
to hardening callback policy, nie dowód obejścia autentyczności czy hardware.

Niezależne testy app chain używają faktycznego fetch/verifier/Ed25519/image
i realnego SHA, podpisów Python cryptography wyłącznie HOST oraz oddzielnego
production-empty wariantu. Artifact fixture ma podpis TEST dla jego exact
size/hash; nie staje się zatwierdzonym Release. Fizyczny TLS, instalacja,
power interruption, rollback i zachowanie danych nie wynikają z hostowych CI.
Lokalny niezależny UBSan:220127 HOST/168689 production-empty kontroli,36/34
rzeczywiste błędy alokacji (jawne cJSON hooks + konteksty). Podpisany size511
odrzucany przed app request; bad URL, app HTTP500/206, duplikat CL/SDK framing
mismatch, chunked exact/truncation/extra, header/marker/hash, oba redirecty,
cache, IO/cleanup/OOM/SHA, post-finish cancel/deadline i zero output przechodzą.
Rzeczywisty BIN PR41 1772528 B SHA256
`516700a62369e957abb9ddf6d6e8e090a68f9b893eae27ea0fa799670365e076`
przeszedł ten chain jako niezależnie podpisany fixture HOST:10013219/169661
kontroli, produkcja nadal zero app requests. Numeric TEST manifest1.2.4 nie
jest porównywany z embedded dev stringiem ani zatwierdzonym Release.
Full native/config265, real UI2237/96 i review2635/24 UBSan PASS. CI z pełnymi
sanitizerami i realnym target .obj/.su/ELF oraz jego pobrany artefakt osobno.

Nadal otwarte #24: poprawny czas/UTC, zatwierdzony publisher anchor/rotacja/
signed Release, discovery draft/prerelease/channel, świadoma zgoda i power
policy, serializacja worker/LCD/progress/cancel, Flash staging/slot/IDF final
validation i zdrowy boot/rollback. Istniejący Web dev upload nie jest zmieniany.
