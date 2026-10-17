# Metadane jednego Release przez HTTPS — backend 0.7.17, #24

`dsr_ota_fetch_release(policy, control, out, release_result)` pobiera jeden jawnie
wskazany tag. Wynikiem jest **UNVERIFIED CANDIDATE**, nie zatwierdzone wydanie,
podpisany manifest ani instalacja. Brak produkcyjnego callera, UI/workera,
latest/list/discovery/cache/retry, mostu do podpisanego chainu i pobierania app.
Nie ustala produkcyjnego nazewnictwa tagów/assetów ani polityki DEV/wersji.

## Kontrakt

- Wspólna walidacja policy z parserem: jawny tag 1..64, asset 1..96, bezpieczne
  ASCII alnum/`-_.`, bez `.` i `..`; wyłącznie STABLE. Błąd przed HTTP.
- URL jest budowany, nie przyjmowany od callera:
  `https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/tags/{tag}`.
  Wewnętrzny transport ponownie sprawdza dokładny origin/repo/path/tag.
- ZERO przekierowań, parametrów query, credentials, tokenów, auth retry i
  insecure fallback. Nie przyjmujemy dowolnego Location. GET z istniejącym
  User-Agent, `Accept: application/vnd.github+json`, `Accept-Encoding: identity`
  i jawnym `X-GitHub-Api-Version: 2022-11-28`.
- Tylko HTTP200, canonical pojedynczy CL lub pojedyncze chunked, identity,
  dokładna kompletność i rzeczywiste body 1..16384 B. Read do1024 B; także cache
  SDK przy complete=true musi zostać opróżniony. Dodatkowy bajt wykrywa oversize.
  3xx odmawia bez drugiego klienta; 304/403/404/429/5xx to błędy statusu, nigdy
  „brak nowej wersji”. Prywatne zasoby/public API mogą zwrócić404 — bez PAT.
- Wspólny transport TLS/CA bundle/hostname/X.509 dates z manifestem/app; UTC
  przed2025-01-01 odmawia. Nie dodano SNTP ani źródła zaufanego czasu.
- Każdy utworzony klient jest close/cleanup przed rzeczywistym parserem cJSON.
  Kontekst transportu jest zwolniony przed parserem. Dokładne odebrane bajty
  trafiają do [istniejącego ścisłego parsera](OTA_RELEASE_PL.md), bez kopii.
- Każdy błąd zeruje candidate, także cleanup, parser, końcowy cancel/deadline.
  Tylko cały `DSR_FETCH_OK` publikuje wynik. `release_result=OK` może towarzyszyć
  późniejszemu timeout/cancel; nie jest samodzielną zgodą ani dowodem zaufania.
  NOT_FOUND oznacza brak assetu w danym obiekcie, nie brak aktualizacji.

## Pamięć i ograniczenia SDK

Nowy osobny heap context `release_fetch_t<=16640 B` zawiera body16385 i URL160.
Nie zwiększono istniejącego `fetch_t<=8192 B`, nie dodano dużego bufora na stos.
W fazie HTTP łączny własny bound wynosi24832 B; dodatkowo SDK/TLS/headers.
W fazie parsera pozostaje context Release + cJSON, bez contextu HTTP. Input,
node i depth caps nie są pomiarem heap cJSON. Raport OTA_RELEASE_FETCH.json
sprawdza realny target object/ELF/.su, nie peak runtime/transitive stack.

SDK timeout5s i cooperative budget30s nie są twardym deadline dla DNS/TLS,
headers ani read z wieloma positive-progress transport reads. Wrapper obejmuje
końcowym sprawdzeniem również parser. Cancellation tylko z zadania właściciela;
bez zamykania klienta z obcego taska. Policy/control i pointed data niezmienne
przez cały call; output wyłącznie dla tej operacji do powrotu.

Header callback dzieli limity/error latch z dotychczasowym transportem. SDK
alokuje nagłówki wcześniej (domyślnie80KiB); nasze limity ich nie obejmują.
IDF nie emituje ostatniego pending trailera przy message_complete. Końcowy
guard obejmuje SDK-visible Location, nie wszystkie surowe trailery. Żadnego
podążania za Location w200 ani claim pełnej zgodności modelu z SDK/TLS.

## Testy i bramki

`tools/test_ota_release_fetch.py`: rzeczywisty wspólny fetch + candidate parser,
SDK/TLS/transport jawnie MODEL; granice/chunki/cache/headers/status/kompletność,
policy/clock/cancel/time/cleanup/OOM oraz negatywy realnego JSON parsera.
Opcja `--cjson-source` wymusza istniejący dokładny pin i SHA obu plików cJSON;
CI instrumentuje cały oryginalny cJSON ASan/UBSan/LSan, nie tylko wrapper.
Lokalny UBSan nie zastępuje pełnych sanitizerów ani prawdziwego TLS/urządzenia.
Niezależny korpus:129009 checks z system cJSON,133415 z original pinned cJSON,
29/31 rzeczywistych błędów alokacji. Root powtórzył pinned corpus; stare native,
manifest/app, config265 i rawHTTP3012 cases/provider bez regresji. Review kodu
i budżetu koordynatora bez blockerów. Exact-source CI i artefakt osobno w PR.

Produkcja nadal nie ma publisher anchor/signed Release; HTTPS API/id/author/
digest/immutable nie uwierzytelniają wydawcy. Pełne OTA wymaga osobnego bridge,
jawnych naming/version/channel/time decyzji, UI/consent/power/serialized staging,
install/health/rollback/data-retention i fizycznego odbioru. Nie dodano kluczy,
Release ani instalacji. P0 SPIFFS nadal bez hardware PASS, bez nowego erase.

## Źródła pierwotne

- [GitHub: get Release by tag](https://docs.github.com/en/rest/releases/releases#get-a-release-by-tag-name)
- [GitHub API versions](https://docs.github.com/en/rest/about-the-rest-api/api-versions)
- [GitHub REST troubleshooting](https://docs.github.com/en/rest/using-the-rest-api/troubleshooting-the-rest-api)
- [IDF5.5.4 esp_http_client source](https://github.com/espressif/esp-idf/blob/735507283d5b2f9fb363a1901172dbd9e847945d/components/esp_http_client/esp_http_client.c)
- [Istniejący kontrakt TLS/SDK](OTA_FETCH_PL.md)
