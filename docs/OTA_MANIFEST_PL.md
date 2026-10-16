# Podpisany manifest OTA — backend weryfikacji #24

0.7.11-manifest-verifier-dev kontynuuje istniejący projekt po PR #37. To
rzeczywisty weryfikator, nie pobieranie/instalacja z LCD. Bez zmiany NVS,
partycji, SPIFFS i testowanej instalacji. P0 nadal PR #33; bez erase/main/Stable.

## Kontrakt v2

V1 było nieimplementowaną propozycją. V2 wymaga dokładnie 11 pól opisanych
w update-manifest.schema.json. Wire: maks. 2048 bajtów, jeden płaski ASCII
obiekt, dowolna kolejność pól i whitespace poza stringami. Bez JSON escapes,
NUL/control, dupes, unknown fields, nesting i trailing input. Liczby: decimal
integer bez leading zero/frakcji/wykładnika. Schema nie zastępuje wire lexingu.

Podpis: czysty Ed25519, nie Ed25519ph lub EdDSA/BLAKE2b. Podpisywane bajty:

1. ASCII DSR-OTA-MANIFEST z końcowym NUL, następnie bajt 0x02.
2. Siedem stringów z długością U16 big-endian przed każdym: hardware_id,
   partition_layout, version, channel, url, key_id, signature_algorithm.
3. Rozmiar aplikacji U64 big-endian, potem 32 surowe bajty SHA-256.

Signature nie jest podpisywane. Transport: kanoniczne base64 64 bajtów,
88 znaków, końcowe == i zerowe unused pad bits. Hash: mały hex; alg: ed25519.
Key ID wybiera wyłącznie kompilowany prywatny provider, nie klucz z JSON/URL/NVS.

Board lcdwiki-es3c35p-n16r8, layout es3c35p-16m-ota6m-dsrdata3e0000-v1:
NVS 0x9000+0x10000, otadata 0x19000+0x2000, PHY 0x1b000+0x1000,
app0 0x20000+0x600000, app1 0x620000+0x600000, dsrdata 0xc20000+0x3e0000.
Caller podaje realną pojemność nieaktywnego slotu; manifest jej nie zwiększa.
Wersja: kanoniczne X.Y.Z nowsze numerycznie od zaufanej lokalnej wersji bazowej.
Build z sufiksem nie jest automatycznie normalizowany z manifestu. Kanał
stable/dev musi odpowiadać jawnemu wyborowi caller, bez fallbacku. URL: dokładnie
repo voltex-at/DIY-Shelly-Remote, releases/download/tag/file, bez query,
fragmentu, credentials, escapes, podwójnych segmentów lub dwóch kolejnych kropek.

## Zaufanie i ograniczenia

Produkcyjny truststore jest pusty: obecny firmware nigdy nie przyjmie manifestu
jako zaufanego wydania. Klucz testowy nie jest kompilowany przez IDF. Publiczny
klucz wydawcy, rotacja/recovery i zatwierdzony podpisany Release wymagają review.
Pustego store nie zastępować kluczem RFC/testowym.

Monocypher 4.0.3: pin/licencja/checksum w components/dsr_crypto/README.md,
oryginalne bajty BSD-2-Clause OR CC0, opcjonalny moduł SHA-512 Ed25519.
Semantyka punktów: cofactored Monocypher, nie identyczne reguły strict libsodium;
S musi być mniejsze od L. Trust anchor musi być autentycznym, poprawnym kluczem,
nie zerowym, małego rzędu lub fixture. Bez własnej arytmetyki krzywych.
Źródła: [RFC 8032](https://www.rfc-editor.org/rfc/rfc8032),
[Monocypher Ed25519](https://monocypher.org/manual/ed25519).

Każdy błąd zeruje output; dopiero pełna polityka, zaufany klucz i podpis
publikują metadane. Allocation failure także odmawia; cJSON nie rozróżnia go
od parse failure. Bez cache/task/timer/network/Flash/reboot. API nie upoważnia
do instalacji bez pozostałych bramek.

## Weryfikacja i dalsze bramki

RFC KAT i mutacje podpisu: 2315 kontroli. Niezależny Python cryptography signer:
117 przypadków, 1494 kontrole host trust i 1329 pustego produkcyjnego truststore,
w tym po 32 allocation failures. Hash checker kontroluje oryginalny vendor.
Target CI wymaga parsera/Ed25519/SHA-512 w ELF, bez signer/keygen i testowego
klucza w BIN. Nie jest to fizyczny test czasu/stosu lub instalacji ESP32-S3.

Otwarte: HTTPS i redirect allowlist, streaming download, rozmiar/hash/ESP image
metadata, rzeczywisty layout, progres/anulowanie, zasilanie, fizyczna zgoda na
instalację, slot selection, zdrowy boot/rollback i zachowanie ustawień/zdjęć.
Lokalne dev OTA Web Panelu pozostaje niezależne, nie jest podpisanym updaterem.
UI nadal uczciwie pokazuje, że natywny przepływ nie jest gotowy.
