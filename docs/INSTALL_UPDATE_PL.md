# UPDATE-APP-ONLY — zachowanie istniejącej konfiguracji

Ta paczka zawiera wyłącznie `diy_shelly_remote.bin` jako segment firmware,
jego SHA256, identyfikację źródła i licencje. Nie zawiera obrazu dsrdata,
bootloadera, tablicy partycji, otadata ani scalonego obrazu Flash.
**Bez Erase All i bez FACTORY-CLEAN-16MB.**

Aktualizacja app-only wymaga niezmienionej tablicy partycji i poprawnie
zamontowanego dotychczasowego magazynu. Nie naprawia częściowo sformatowanego
lub uszkodzonego SPIFFS. Najpierw zachowuje się kopię i rozstrzyga stan danych.

OTA z pilota wybiera nieaktywny slot przez SDK i wymaga manifestu podpisanego
zatwierdzonym kluczem wydawcy `voltex-ota-2026-1` (jedyny klucz w firmware).
Podpisany jest wyłącznie zestaw z GitHub Release (workflow `release.yml`).
BIN z artefaktu CI nie ma podpisanego manifestu: służy do OTA z Web Panelu
(z potwierdzeniem na LCD i sumą SHA-256) albo do USB app-only.

Ewentualny zatwierdzony test USB zapisuje samą aplikację do aktywnego slotu
potwierdzonego aktualnym logiem: ota_0 0x20000 albo ota_1 0x620000, oba po
0x600000. Nie zakładaj 0x20000 po OTA; wybór złego slotu nie aktualizuje
działającej aplikacji. Nie zapisuj NVS/dsrdata/partition/otadata i nie używaj
krótszego historycznego obrazu `local.bin` do zachowawczej aktualizacji.

Po zatwierdzonym teście potwierdź start i zachowanie Wi-Fi, powiązań,
pomieszczeń, zdjęć oraz sterowania i log pierwszego startu. Host/CI nie zastępuje
tego odbioru. Bez automatycznego restartu, kasowania i publikacji Stable.

## Błąd manifestu `16/6/200/1` w starszym firmware

GitHub dodaje do przekierowań pobierania długi nagłówek przeglądarkowy CSP.
Wersje do 0.7.71 włącznie mogą odrzucić go przed pobraniem i weryfikacją
manifestu. Poprawka jest w 0.7.72-dev; podpis i zaufany klucz pozostają te same.
Wyświetlone `200` pochodzi z wcześniejszego odczytu katalogu wydań i nie
potwierdza pobrania manifestu.

Jednorazowa aktualizacja przez sparowany panel WWW:

1. Pobierz `diy_shelly_remote.bin` z publicznego wydania `dev-v0.7.72` oraz
   `SHA256SUMS.txt`; sprawdź sumę SHA-256 pliku.
2. Otwórz panel HTTPS swojego pilota. W aktualizacji lokalnej wybierz ten BIN.
3. Potwierdź zgodę i stabilne zasilanie na LCD, następnie wyślij plik z panelu.
4. Po poprawnej weryfikacji i instalacji wybierz restart. Sprawdź numer wersji
   oraz zachowanie konfiguracji po uruchomieniu.

Do tej operacji służy aplikacja, nie ZIP ani pełny obraz FACTORY. Alternatywą
jest procedura USB app-only powyżej, po ustaleniu aktywnego slotu.
