# #24 — parser kandydata Stable, nie zatwierdzenie wydania (0.7.16-dev)

Etap po PR #42: **pure parser pojedynczego obiektu GitHub Release**, bez
sieci, UI/workera, pobierania, Flash/OTA, slotu/reboot i produkcyjnego callera.
Nie wybiera latest/listy ani najwyższego semver. Nie publikuje Release,
nie dodaje klucza/tokena i nie zamienia metadanych GitHub w zaufanie wydawcy.
P0 SPIFFS #28 / PR #33 nadal bez nowego physical pass; bez erase/main/Stable.

## Kontrakt i źródła

`dsr_ota_release_candidate_parse(json, length, policy, out)` wymaga jawnej
polityki: `expected_tag` (1..64 B), `manifest_asset` (1..96 B) i STABLE.
Oba segmenty ASCII alnum/-/underscore/dot, bez pojedynczego `.` i bez `..`.
DEV/nieznany kanał odmawia; nie mapujemy go po cichu na stable. Nie narzucamy
produkcyjnej nazwy manifestu ani konwencji tag→semver. Tag jest opaque routing,
nie wersją podpisanego manifestu i nie dowodem nowszego firmware.

Primary sources: [GitHub Get a release by tag name](https://docs.github.com/en/rest/releases/releases#get-a-release-by-tag-name)
i [release assets](https://docs.github.com/en/rest/releases/assets#get-a-release-asset).
Schemat pojedynczego obiektu zawiera id/url/tag_name/draft/prerelease/assets;
`browser_download_url` prowadzi do pobrania, `assets[].url` do API. Parser
nie wykonuje tych żądań ani nie ustala sposobu bezpiecznego transportu metadanych.

Wymagane: root object, positive integer id<=2^53-1 i dokładny własny API URL
`https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/{sameid}`
z canonical decimal, dokładny tag_name oraz bool draft=false/prerelease=false.
Assets to tablica<=32 obiektów ze string name. Dokładnie jeden asset o jawnej
nazwie; zero→NOT_FOUND, więcej→AMBIGUOUS. Wybrany asset musi mieć state=uploaded,
uint size1..2048 i dokładny browser_download_url własnego repo z tym tagiem
i nazwą, kompatybilny z obecnym bounded manifest fetch. Nie akceptujemy query,
escape URL, obcego hosta/repo, credentials, portu ani innego tagu/nazwy.
Escapes JSON są dozwolone; porównanie następuje po ich prawidłowym decode.

Wynik to wyłącznie tag, manifest URL i deklarowany rozmiar: **UNVERIFIED
CANDIDATE**. Root API URL/id również nie uwierzytelnia wejścia — da się je
sfabrykować. Author, digest, immutable, target_commitish, daty i opis są
ignorowane jako źródła zaufania. Nie kopiujemy release notes/HTML do UI.
Candidate nie jest `dsr_ota_verified_t`; przyszła akceptacja wymaga istniejącego
genuine HTTPS→Ed25519→image chain oraz wszystkich późniejszych bramek.
NOT_FOUND dotyczy tylko tej odpowiedzi i nie oznacza `Brak nowej wersji`.

## Ograniczony profil JSON i pamięć

Max16384 B,8 kontenerów w głąb (root=1),512 values (object keys nie są
osobnymi values). Przed alokacją: ścisła gramatyka/punctuation/full consumption,
UTF-8 bez overlong/surrogate/>U+10FFFF, prawidłowe JSON escapes i surrogate pairs,
brak raw/decoded NUL w dowolnym miejscu. Liczby w całym profilu GitHub są
canonical unsigned integers<=9007199254740991: bez minus/fraction/exponent/
leading zeros. To jawny subset typowego integer API, nie ogólny parser każdej
poprawnej liczby JSON. Nowe fractional/signed API extensions wymagają review;
bieżący parser bezpiecznie odmawia zamiast zaokrąglać id/size przez double.

Po preflight **rzeczywisty cJSON** dekoduje i waliduje wejście. W każdym obiekcie,
także ignorowanym/nested, odrzucane są powtórzone decoded keys (case-sensitive,
bez Unicode normalization). Escapowane aliasy nie obchodzą duplikatów.
Wszystkie błędy zerują cały out. NULL/empty/oversize wejście to ARGUMENT;
OOM to INVALID, ponieważ cJSON nie rozróżnia go od błędu parsowania.
Argumenty muszą pozostać immutable, a out należy wyłącznie do operacji.

Nie ma bufora16KiB/candidate na stosie ani własnego whole-input copy.
Input/nodes/depth ograniczają pracę i alokacje, ale **nie są pomiarem heap peak**:
cJSON alokuje nodes, decoded keys/strings i pomocniczy number buffer. Target
audit czyta rzeczywiste .obj/.su/ELF, wymaga parsera/cJSON i dopuszcza tylko
string/memory/compiler arithmetic deps, bez network/crypto/trust/Flash.
Własne ramki<=256 B nie oznaczają całego recursive/transitive stack peak.

## Odbiór i dalsze bramki

Niezależny recenzent buduje korpus Python stdlib JSON + prawdziwy C/cJSON parser,
boundary/duplicate/UTF8/NUL/policy/mutation/allocator fault cases. Koordynator
przeczytał implementację i primary API, przygotował audit targetu: bez blockerów.
Host sanitizer, dokładny target build i pobrany artefakt to osobne dowody;
żaden z nich nie jest fizycznym HTTPS/instalacją/rollback/P0 acceptance.
Niezależny UBSan: **27989 kontroli /1162 przypadki**,34 poprawne kandydatury,
27 rzeczywistych błędów alokacji przez jawne cJSON hooks. Bufory wejścia mają
dokładny rozmiar, bez końcowego NUL. Oracle Python stdlib JSON z decoded-dupe/
UTF8/number/resource checks, wszystkie ucięcia bazowej odpowiedzi i700 seeded
mutations (seed0x2424), granice size/depth/nodes/assets/safe53, escaped keys/URL,
Unicode opisu, arbitrary non-semver tag i max64/96 caller tokens przechodzą.
Koordynator i recenzent niezależnie przeczytali parser: bez blockerów.
Osobny `--cjson-source PATH` kompiluje oryginalny IDF cJSON
`c859b25da02955fef659d658b8f324b5cde87be3`, wymuszając HEAD i SHA256 obu
cJSON.c/h. Tool nie pobiera źródeł i kompiluje dokładnie zweryfikowane bajty.
Pinned UBSan: **29868 kontroli /1162 przypadki /29 allocation faults PASS**.
Różnica względem systemowej biblioteki wynika m.in. z alokowanego number buffer;
nie utożsamiamy 27 systemowych fault points z pełną wersją targetową.
CI osobno instrumentuje także cały oryginalny cJSON ASan/UBSan/LSan, zamiast
polegać tylko na prebuilt libcjson.so. Lokalny LSan jest blokowany przez
ograniczony /proc/ptrace, więc nie jest raportowany jako PASS.

Produkcyjny publisher truststore nadal celowo pusty. Zatwierdzony anchor/rotacja/
signed Release, prawidłowy UTC/TLS, produkcyjna konwencja tag/asset/dev,
release metadata transport/discovery, LCD/worker/progress/consent/power,
serializacja Flash staging/IDF final validation i health/rollback są nadal
otwarte. Nie dodawać ich ukrycie do tego parsera; nie omijać signed chain.
