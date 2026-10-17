# Release → podpisany manifest → walidacja app — backend 0.7.18, #24

`dsr_ota_fetch_release_app(release_policy, manifest_policy, control, out, ...)`
łączy istniejące rzeczywiste backendy. To walidacja i **ODRZUCENIE bajtów BIN**,
nie instalator. Brak UI/workera, Flash/sinka, esp_ota/slot/reboot, latest/list,
cache/retry, wyboru produkcyjnych tagów/assetów/dev/time/publisher policy.

## Kolejność i zaufanie

1. Jawne release policy tag/asset/STABLE i zaufana lokalna manifest policy
   current_version/slot_size/STABLE. Obie STABLE i structural checks przed I/O.
   current_version nie pochodzi z tagu, API/id/author/notes ani dev suffix.
   Istniejący rzeczywisty verifier nadal egzekwuje numeric version/channel/
   board/layout/slot/hash/signature. Nie dodano interpretacji taga jako semver.
2. Genuine HTTPS Release fetch i strict candidate parser. Tylko pełne FETCH_OK
   pozwala na kolejną fazę. Metadata/TLS/id/author/digest nie są zgodą wydawcy.
3. Candidate URL trafia do tego samego manifest/app chainu. Dodatkowy prywatny
   parametr wiąże faktyczną liczbę odebranych MANIFEST bytes z
   `candidate.manifest_size` (1..2048): CL exact, chunked actual exact, extra/
   truncation odrzucone przed verifierem i przed app request. To consistency
   check niezaufanych metadanych, nie publisher authentication.
4. Genuine Ed25519 manifest verifier, tylko pełne FETCH_OK pozwala na app.
   `verified.size` oznacza rozmiar BIN — NIGDY nie porównujemy go z długością
   manifestu. Podpisany manifest określa app URL/version/channel/size/hash/board/
   layout; opaque release tag nie narzuca dodatkowej same-tag polityki app URL.
5. Istniejący genuine app fetch/image helper liczy rzeczywisty SHA/rozmiar i
   sprawdza header/project/board marker. Bajty odrzucane, bez możliwości zapisu.
   Końcowy cancel zeruje wynik. Tylko cały FETCH_OK publikuje verified metadata;
   detail release/manifest/image OK może towarzyszyć późniejszemu błędowi.

Produkcja ma nadal PUSTY publisher truststore, więc może pobrać metadane i
manifest, ale nie może wysłać żadnego żądania app. Klucz HOST/RFC, CA lub TLS
identity nie staje się wydawcą. Wynik nie oznacza approved Release ani pełnej
bootability IDF. Przyszła instalacja wymaga ponownego download/validation i
osobnych zgód/zasilania/serialized staging/IDF/health/rollback/data-retention.

## Pamięć, transport i zgodność

Mały osobny `release_chain_t<=1024 B` przechowuje candidate i verified. Nie ma
dużych lokalnych struktur. Release buffer16KiB i cJSON są zwolnione przed
manifest/app. Metadata phase: bridge1024 + własny bound24832, SDK/TLS dodatkowo;
parser phase: bridge+body+cJSON, HTTP context zwolniony. Manifest/app phase:
bridge1024 + appchain512 + fetch8192 + image<=1024 (actual osobno w target audit).
Wszystkie to compile-time bounds, nie pomiar peak SDK/TLS/cJSON/stosu.

Każda faza ma osobny cooperative budget30s/SDK5s; brak hardtotal30s dla całego
chainu, DNS/TLS/headers/positive-progress reads. Dziedziczone TLS/CA/hostname/
X.509 dates/UTC floor, metadata zero redirects i manifest/app one allowed hop.
Ograniczenie SDK ostatniego pending trailera jawne jak w poprzednich etapach.
Nie dodano SNTP/zaufanego czasu/credentials/tokenów/fallbacku.

Publiczne standalone manifest/app APIs mają expected manifest length0 i
zachowują dotychczasową semantykę/limity. Nowy length-bound path jest prywatny,
używa tych samych transportu/verifiera/image helpera — bez create_dev_checksum,
fabricated verified lub nowego trust providera. Public bridge nie przyjmuje
caller-created candidate ani verified. Policy/control/pointed data niezmienne,
out wyłącznie dla tej operacji do powrotu; nie wywoływać z UI taska.

## Dowody i pozostałe bramki

Niezależne testy full genuine chain z HOST-only podpisami i production empty,
modelowany SDK transport/TLS. Manifest length CL/chunked ±1/extra/truncation,
opaque tag, rozdzielone app version, błędy policy/metadata/signature/app/header/
hash/marker/cleanup/IO/time/cancel/OOM; produkcja zawsze zero app requests.
Opcjonalny rzeczywisty BIN jest wyłącznie HOST fixture, nigdy approved Release.
CI instrumentuje original pinned cJSON; target audit sprawdza realne obj/su/ELF.
Final niezależny UBSan:233256 HOST/186385 empty system cJSON (66/64 OOM);
238418/191535 original pinned cJSON (70/68 OOM). Parent PR44 actual app1776368B
SHA1ec31b8ee165a6d99cd2c30d81553c03f0a7989c7a3cd906db3048ac818b21cf:
9045950/192549 checks (70/68 OOM), tylko HOST-signed fixture. Empty production
zawsze zero app requests. Opaque tag i trzy29s fazy pozytywne; metadata heap
free przed manifestem, exact manifest binding przed key lookup, final bridge
cancel i NULL controls/details testowane. Root powtarza korpus; koordynator
code/memory review bez blockerów. Pełne sanitizery/target/exact artefakt w CI/PR.

P0 #28/#33 nadal bez physical PASS; żadnego erase/nonempty format/partycji.
Brak owner-approved anchor/rotacji/signed Release i poprawnego UTC/real TLS.
Naming/version/dev/time/power/consent decyzje, LCD progress, installer i health/
rollback/retained data pozostają otwarte; full #24 NIE ukończone.

Źródła: [OTA](OTA.md), [metadata HTTPS](OTA_RELEASE_FETCH_PL.md),
[strict candidate](OTA_RELEASE_PL.md), [manifest](../OTA_MANIFEST_PL.md),
[app validation/discard](OTA_DOWNLOAD_PL.md), [transport TLS](OTA_FETCH_PL.md).
