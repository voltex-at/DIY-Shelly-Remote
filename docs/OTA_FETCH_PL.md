# #24 — ograniczony transport manifestu HTTPS (0.7.14-dev)

Backend po PR #40, nie gotowy updater LCD. Nie pobiera app BIN, nie wywołuje
Flash/OTA, nie wybiera slotu i nie restartuje. Nie dodaje zadania/timera, cache,
release pickera, automatycznego sprawdzania ani przycisku udającego instalację.
P0 #28 / PR #33 nadal czeka na właścicielski odbiór sprzętowy; bez erase/main/Stable.

## Granica transport–zaufanie

Caller podaje jawny URL manifestu i zaufaną lokalną politykę. URL początkowy:
wyłącznie `https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/tag/file`,
do 256 B, dwa niepuste segmenty ASCII alnum/-/underscore/dot, bez `..`, escapes,
query, fragmentu, credentials, IP ani niestandardowego portu. To **nie** selekcja
zatwierdzonego stable Release; przyszły release picker musi jeszcze sprawdzić
draft/prerelease/channel i świadomy wybór użytkownika.

Dozwolony najwyżej jeden ręczny redirect z tego źródła do dokładnego hosta
`https://release-assets.githubusercontent.com/`. GitHub dokumentuje ten host
[dla release assets](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).
Bez wildcardów, względnych URL, zmiany repo/hosta na kolejny GitHub URL,
userinfo/port/fragmentów i dalszych redirectów. CDN path jest ograniczony do
niepustych zwykłych segmentów ASCII, bez traversal/escape. Query jest traktowane
jako ograniczony opaque URL query: widoczny ASCII, poprawne percent escapes,
bez kontroli, fragmentu i backslash. Nie dowodzi przynależności CDN path do repo;
powiązanie pochodzi z pierwszej zweryfikowanej odpowiedzi TLS GitHub.
Query ani odpowiedzi nie są logowane przez ten moduł.

Każdy hop ma świeży klient i nową walidację TLS, bez cookies, Authorization,
klucza klienta, automatycznego redirectu i ponawiania auth. CA bundle **nie jest**
kluczem wydawcy. Dopiero exact odebrane bajty przechodzą istniejący strict JSON
v2 / canonical Ed25519 verifier. Każdy błąd zeruje wynik. Produkcyjny truststore
pozostaje pusty, więc transport nie może obecnie autoryzować żadnego wydania.
Klucze RFC/testowe i prywatne TLS identity nie stają się publisher anchor.

## TLS, zegar i limity

Konfiguracja: SSL, CA bundle, hostname z URL (`common_name=NULL`, skip=false),
auto redirect off, GET, `Accept-Encoding: identity`. Target audit sprawdza
rzeczywisty sdkconfig: HTTPS/bundle/full bundle/HAVE_TIME/HAVE_TIME_DATE włączone,
ESP_TLS_INSECURE i SKIP_SERVER_CERT_VERIFY wyłączone. Primary sources:
[IDF HTTP client](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/protocols/esp_http_client.html),
[przypięty Kconfig dat certyfikatów](https://github.com/espressif/esp-idf/blob/v5.5.4/components/mbedtls/Kconfig).

IDF domyślnie nie sprawdza dat X.509 (`HAVE_TIME_DATE=n`); ten etap włącza je
na płytce. Fetch odmawia przed I/O, jeśli UTC jest wcześniejsze niż 2025-01-01.
To tylko sanity gate, **nie ustanowienie zaufanego czasu**. Projekt nie ma tu
jeszcze synchronizacji zegara/SNTP; obecne urządzenie z nieustawionym czasem
pozostaje zablokowane. Nie wyłączać expiry, CA ani hostname jako obejścia.

Odpowiedź końcowa musi mieć HTTP 200, positive canonical Content-Length
1..2048 B albo pojedyncze Transfer-Encoding: chunked. Bez EOF-only framing,
duplikatów/conflictów CL/TE/Location, gzip/kompresji czy nieoczekiwanego Location
na 200. Limit dotyczy faktycznych zdekodowanych bajtów; kontrola końcowej
kompletności i długości, próba 2049. bajtu. Callback kopiuje nagłówek przed jego
zwolnieniem i latchuje błąd, bo IDF ignoruje zwrot callbacku w tej ścieżce.

Follow-up 0.7.15 wspólnie wykorzystuje ten transport dla app validation/discard.
Końcowa kontrola odrzuca SDK-visible Location z trailerów przed akceptacją.
IDF5.5.4 nie flushuje ostatniego pending trailera przy message-complete;
public API nie ma jego read-only gettera. Polityka callbacków nie gwarantuje
kontroli wszystkich surowych trailerów. Raw harness poprawiono zgodnie z SDK;
X-After wymusza dispatch w negatywach. Nie podążamy za Location z200, a
podpis i exactsize/hash nadal obowiązują. [Aktualny kontrakt](OTA_DOWNLOAD_PL.md).

Read loop opróżnia również dane odebrane razem z nagłówkami, nawet gdy parser
już mówi complete. Nie łączy event body i manual read. Close/cleanup każdego
utworzonego klienta poprzedza verifier. Cancel należy do tego samego ownera;
nie używa `esp_http_client_cancel_request`, który w tym IDF ponownie łączy socket.

SDK timeout 5 s, kooperacyjny budżet 30 s i cancellation między wywołaniami.
Read po 1 B ogranicza positive-progress body waits między pollami. Blokujący
DNS/TLS/odbiór nagłówków SDK **nie ma przez to twardego całkowitego deadline**.
Funkcji nie wolno wywołać w tasku UI; przyszły orchestrator/worker osobno.

Własny heap context ma target-compiled bound <=8192 B. To nie cały RAM:
bufory SDK/TLS i dynamiczne nagłówki są dodatkowe (parser IDF do 80 KiB).
Własne callback limity to 64 headers, key128/value8192 i suma16384 bajtów;
nie ograniczają wcześniejszej alokacji nagłówka wewnątrz SDK. Rzeczywisty peak
stosu/heap/czas sieci i zachowanie na ESP32-S3 pozostają bramką sprzętową.

## Weryfikacja i dalszy etap

Niezależny recenzent testuje faktyczny moduł, oryginalny parser/Monocypher,
Python cryptography podpisy i osobny HOST-only trust provider oraz production
empty provider. HTTP/TLS SDK jest fixture, nie prawdziwą siecią/certyfikatem.
CI dodatkowo buduje i audytuje rzeczywisty target ELF/obj/.su i sdkconfig.
Nie twierdzimy, że hostowe config assertions to fizyczny TLS PASS.

Lokalny UBSan: **350791 kontroli HOST trust / 345251 production empty**, po
33 rzeczywiste błędy alokacji (kontekst + jawne cJSON hooks). Limity URL/body/
headers, framing/dupes, cached body, EOF, oba hopy/faults, config TLS, zegar,
cancel/timeout i podpisy przechodzą. Osobne testy anulują po poprawnym Ed25519:
fetch zwraca błąd i zero output mimo verifier=OK. Caller musi warunkować
akceptację wyłącznie przez **DSR_FETCH_OK**, nie sam result verifiera.
Native/config265, UI2237/96 i review2635/24 PASS. Niezależne review koordynatora
bez blockerów; target CI oraz fizyczny TLS pozostają oddzielnymi bramkami.

PR #41 source7744651: exact-source CI38858870344 **5/5 PASS**. Pobrany BIN
1772528 B (SHA256 `8b3b3ee48bd12c7464a03ab8765544e9ef7b5ddaa2f395c147ed92b80ebc974a`)
i ELF, CRC/SHA ZIP, source/version/chip/project, segmenty, checksum/SHA app,
ELF SHA w descriptorze, licencje i niezmienione partycje sprawdzone. Target:
własne ramki fetch32/48/64 B, bound kontekstu<=8192 B, internal/external BSS
48904/145504 B i UI stack8192 B bez zmian. To nie pomiar transitive SDK/TLS peak.

Test-only follow-up: `tools/test_ota_fetch_parser.py --idf PATH --sanitize`
czyta oryginalne `http_parser.c/h` z commita IDF
`735507283d5b2f9fb363a1901172dbd9e847945d`, sprawdza SHA256 obu plików,
nie pobiera sieci i nie vendoruje SDK. Rzeczywisty parser, fetch i Ed25519,
3010 raw-wire przypadków na provider (UBSan1614269 HOST/2109597 production
empty PASS): każda wielkość fragmentu odpowiedzi CL/chunked, każdy urwany
prefiks, trailers, framing/duplikaty, body razem z headers, malformed bytes
i zmienione podpisane dane. **Header assembly/cache/transport klienta SDK są
nadal modelami**; brak prawdziwej sieci/certyfikatów i bramki sprzętowej.
Pełne ASan/UBSan/LSan dla follow-up sprawdza osobny krok CI; lokalny LSan
jest blokowany przez ograniczony dostęp /proc, nie liczy się jako PASS.

Nadal otwarte: źródło poprawnego czasu, zatwierdzony klucz i signed Release,
release discovery, integracja LCD/worker, app download/progress/cancel,
instalacja, zdrowy boot/rollback oraz fizyczne zachowanie Wi-Fi/Rooms/zdjęć.
Ten backend nie zastępuje istniejącego dev uploadu i nie zamyka #24.
