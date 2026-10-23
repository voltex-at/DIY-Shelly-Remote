# SYS-02 — Aktywność, szyfrowany backup i reset danych

Data: 2026-10-24. **Status: projekt zatwierdzony przez właściciela w rozmowie**
(„Tak, projekt zatwierdzony”). Podstawa przygotowania planów wdrożenia SYS-02.
Podstawa: ankieta #74, kod na `work/dev-0.7.70` po `1baf1a0`.
Dokument opisuje przyszłe funkcje; nie jest poleceniem resetu urządzenia.
Niezależny przegląd wykonalności wskazał brak określenia enrollment reguł po imporcie.
Projekt uzupełniono; ponowny przegląd tej części nie pozostawił istotnych pytań
blokujących plan. Nie zastępuje to testów implementacji ani odbioru sprzętowego.

## Cel i decyzje właściciela

Pilot ma pokazywać historię działań na LCD i pozwalać odzyskać pełną konfigurację
po resecie bez ponownego wprowadzania wszystkich połączeń. Uzgodniono:

1. Aktywność korzysta z obecnych 128 wpisów RAM od uruchomienia. Restart ją czyści.
2. Backup i odtwarzanie przez WWW obejmują konfigurację, zdjęcia i połączenia,
   w tym hasła oraz powiązania BLE. Plik jest szyfrowany hasłem użytkownika.
3. Odtwarzanie działa **tylko na tym samym fizycznym pilocie**, także po pełnym
   resecie. Migracja na inny egzemplarz nie należy do tego zakresu.
4. Pełny reset danych wymaga potwierdzenia na fizycznym LCD i usuwa również
   Wi-Fi, Shelly, BLE oraz dostęp WWW. Firmware i weryfikacja OTA pozostają.

Poniższe wybory architektury, ograniczenia i szczegóły UX zostały zaproponowane przez
wykonawcę i zatwierdzone wraz z dokumentem; nie przypisujemy ich wcześniejszym
odpowiedziom właściciela. Dotychczas zaakceptowane ekrany i działanie zwykłego restartu pozostają.

## Podejście i granice

Rekomendacja: wersjonowana **kopia logiczna**, walidowana przez adaptery istniejących
magazynów. Surowy zrzut NVS/SPIFFS przenosiłby także niepożądane sesje i dane SDK,
wiązał format z ABI oraz utrudniał kontrolę kompletności. Sam eksport publicznego
JSON pomija hasła i powiązania, więc nie realizuje wybranego zakresu.

Zachowujemy ESP-IDF 5.5.4, LVGL 9.3.0, ST77922 2.1.1, mDNS 1.8.2 i partycje
ES3C35P N16R8. Nie pożyczamy slotu OTA. Interfejs ma Cyber Blue/Grafit/Jasny,
PL/EN/DE, obecne sfazowane podwójne ramki, złote ważne komunikaty i znane przyciski.
Nie dodajemy chmury, historii trwałej, synchronizacji między pilotami ani nowych
typów automatyzacji. Nie wykonujemy na urządzeniach Shelly poleceń jako części
backupu, odtwarzania lub resetu. Nie zmieniamy kluczy podpisu OTA.

## Istniejące elementy i podział odpowiedzialności

- `dsr_actions`: redagowany log RAM, 128 rekordów po 32 B, strony maksymalnie 32;
  WWW już ma zapytania logu. LCD ma używać tego samego modelu i epoki.
- `dsr_workspace` / `dsr_config_store`: konfiguracja schema 3, CAS, dwa checkpointy;
  starsze schematy 1/2 mają zamrożone walidatory i migrację w RAM.
- `dsr_photos`: do 16 zdjęć po 115200 B, identyfikatory treści SHA-256, żywe bufory
  LVGL do restartu. Odtwarzanie zdjęć musi poprzedzać ładowanie tych buforów i GC.
- `dsr_settings`, `dsr_connection`, profile baterii i styl: osobne magazyny NVS.
  Sterownik Wi-Fi używa `WIFI_STORAGE_RAM`; źródłem trwałym są ustawienia projektu.
- NimBLE ma własne rekordy zabezpieczeń, IRK i lokalnej tożsamości; sam MAC lub klucz
  BTHome nie wystarczają do odtworzenia lokalnego powiązania.
- Nowa warstwa utrzymania danych w `dsr_platform` koordynuje adaptery, zgody,
  spójny snapshot, walidację i odzyskiwanie. Format/walidatory są testowalne na hoście;
  UI i HTTP nie zapisują magazynów bezpośrednio. Nie powstaje drugi worker RPC.

## Aktywność na LCD

System → Aktywność pokazuje stronicowaną historię: czas UTC lub czas od uruchomienia,
źródło, identyfikator, polecenie i wynik. Zachowujemy wcześniej wybrane identyfikatory
numeryczne. Obserwacja zmiany nie wygląda jak potwierdzenie wykonania komendy.
Nowe/starsze wpisy, Odśwież i Wyczyść mają jednoznaczne etykiety; czyszczenie wymaga
potwierdzenia i dotyczy tylko logu RAM. Widok obsługuje pusty log i zmianę epoki
podczas przeglądania. Propozycja: do 6 wpisów na stronie LCD z przewijaną treścią;
docelową wysokość ustali pomiar rzeczywistych napisów LVGL, bez ucinania wyników.
Nazwy hostów, SSID, MAC, hasła, PIN i klucze nie trafiają do nowych wpisów.

## Zawartość kopii i zgodność

| Składnik | Reguła |
|---|---|
| Konfiguracja publiczna | Pomieszczenia, urządzenia, kolejność, dashboardy, grupy, sceny, definicje reguł, język i wygląd; pełna walidacja powiązań ID |
| Zdjęcia | Wszystkie aktualnie referencjonowane zdjęcia; stały wymiar/format, długość i skrót pełnej treści; brak ścieżek dostarczanych przez klienta |
| Ustawienia | Wi-Fi, jasność, wygaszanie, profil i kalibracja baterii, wskazówka stylu |
| Połączenia prywatne | Rejestr Shelly, poświadczenia, tożsamości kanałów, klucze BLU/BTHome; bez starych odczytów |
| Powiązania NimBLE | Własne i zdalne zabezpieczenia, lokalny IRK/tożsamość, CCCD i pozostałe wymagane rekordy skonfigurowanego store |
| Zgody reguł | Definicje i zaufane tożsamości uczestników już zatwierdzonych reguł; bez historii cykli i starych nastaw |
| Wyłączone z kopii | Log RAM, telemetria, kolejki, zgody OTA, sesje WWW, token parowania panelu, prywatny certyfikat/klucz serwera TLS, firmware, PHY i eFuse |

Manifest zawiera wersję formatu, model sprzętu, wersję firmware/schema, wersję adaptera
NimBLE, datę UTC lub jawne „czas nieustawiony” i identyfikator pilota wyliczony
z fabrycznej tożsamości eFuse oraz modelu.
Identyfikator jest wewnątrz szyfrowanej części i nie zależy od resetowanych ustawień.
To kontrola zgodności egzemplarza, nie zabezpieczenie przed właścicielem modyfikującym
własną kopię. Starsza wersja firmware odmawia nieznanego formatu/schema zamiast
pomijać elementy. Nowy format wymaga jawnego adaptera migracji i regresji.

NimBLE eksportuje pola typowane, z jawnymi rozmiarami i limitem 16 bonds; nie zapisujemy
surowych struktur C. Adapter musi objąć wszystkie rodziny faktycznie włączone w
przypiętym SDK (`our_sec`, `peer_sec`, CCCD, CSFC, peer-device, IRK, RPA, ewentualnie EAD).
Nieobsługiwany obecny rekord oznacza odmowę pełnego eksportu/importu, a nie cichą stratę.
Import jest wykonywany przy zatrzymanym hoście, przed inicjalizacją jego pamięci store
i radia; adapter respektuje ABI oraz inicjalizację pinned SDK. Potrzebne odtworzenie
lokalnego adresu następuje przed startem hosta. Odbiór sprzętowy musi sprawdzić ponowne
połączenie. Jeżeli drugie urządzenie zapomniało swoją stronę powiązania, może być
konieczne ponowne parowanie — lokalny backup nie odtwarza pamięci zdalnego urządzenia.

### Reguły ogrzewania i stara historia sterowania

Kopia przechowuje definicje, flagi włączenia i część enrollment zaufanych tożsamości
reguł, lecz nie historyczny dziennik operacyjny `dsr_auto` ze starymi nastawami
do przywrócenia. Propozycja: eksport oraz
przyjęcie odtwarzania wymagają braku aktywnego cyklu ograniczenia, oczekującego polecenia
i nierozstrzygniętego wyniku automatyzacji. W przeciwnym razie UI prosi o zakończenie
cyklu albo świadome wyłączenie reguły; samo otwarcie dialogu niczego nie przełącza.
Eksport czeka także na zakończenie enrollment wszystkich włączonych reguł; nie utrwala
niepotwierdzonej tożsamości. Samo usunięcie `dsr_auto` uniemożliwiałoby automatyczne
wznowienie: obecny `configure()` wymaga zatwierdzonego enrollment.
Dlatego fizyczne potwierdzenie importu jawnie obejmuje przywrócenie włączonych reguł
dla pokazanej liczby zapisanych urządzeń. Adapter tworzy świeży poprawny rekord
`dsr_auto` z definicjami i zatwierdzonymi tożsamościami z kopii, pustymi cyklami oraz
bez dawnych baseline/desired/manual-intent/override. Waliduje zgodność definicji,
prywatnych połączeń i tożsamości; nie nadaje zgody na dowolne nowe urządzenia.
Po starcie włączone reguły przechodzą istniejącą barierę świeżych odczytów czujników
i bezpośredniego odczytu TRV, tworząc nowe podstawy działania. Zmiana tożsamości
oznacza konflikt i wymaga normalnego ponownego zatwierdzenia reguły. Nie odtwarzamy
z archiwum dawnych temperatur urządzeń zewnętrznych.
Zwykły restart nadal korzysta z obecnego dziennika i uzgodnionego zachowania.

## Szyfrowanie i przepływ przez przeglądarkę

Propozycja: standardowe Web Crypto w panelu HTTPS wykonuje PBKDF2-HMAC-SHA256
(600000 iteracji) i AES-256-GCM. Każdy plik ma losową sól 16 B, nonce 12 B i tag
16 B. Nagłówek jest AAD. Hasło nigdy nie jest wysyłane pilotowi ani zapisywane
w localStorage/sessionStorage. Eksport wymaga dwukrotnego hasła, minimum 12 znaków
i maksymalnie 256 B UTF-8, bez przycinania lub normalizacji; import używa dokładnie
tych samych bajtów. Brak hasła oznacza brak możliwości odzyskania pliku.

Format v1: 64-bajtowy nagłówek, little-endian: magic `DSRBKP1\0` (8 B), wersja u16,
długość nagłówka u16, iteracje u32, sól 16 B, nonce 12 B, długość szyfrogramu z tagiem
u32 i 16 zerowych bajtów zarezerwowanych. Dokładna długość, znane parametry i limit
pliku są sprawdzane przed KDF. Payload ma ograniczony manifest UTF-8 i rekordy zdjęć
o jawnych długościach; bez ZIP, kompresji, zagnieżdżonych archiwów i dowolnych ścieżek.
Odrzucamy duplikaty kluczy/rekordów, obce pola, przepełnienia i dane po końcu.

Panel pobiera prywatny snapshot po uwierzytelnionym HTTPS i dopiero szyfruje plik
do pobrania. Podczas importu odszyfrowuje go lokalnie i wysyła jawny snapshot
**wewnątrz HTTPS** do niezależnej walidacji na ESP. Oznacza to chwilową obecność
sekretów w pamięci uprawnionej przeglądarki; nie obiecujemy, że nie opuszczają pilota.
Odpowiedzi mają `no-store`, bez cache service workera, URL z danymi lub logowania body.
Panel nie oferuje jawnego pobierania snapshotu ani raportu sekretów. Brak Web Crypto
lub secure context kończy operację komunikatem, bez słabszego zamiennika szyfrowania.
Kod pochodzi z firmware, zgodnie z CSP; bez CDN, inline JS i nowej biblioteki crypto.
Bufory są zwalniane/zerowane na końcu, pola hasła czyszczone; GC nie gwarantuje
wymazania wszystkich kopii niezmiennych łańcuchów JavaScript.

## Zgody i spójny snapshot

System → Kopia i reset zawiera instrukcję otwarcia sparowanego panelu oraz Reset.
WWW ma osobne sekcje Utwórz kopię i Przywróć. Eksport prywatnych danych wymaga
jednorazowej zgody na fizycznym LCD. Import wymaga zgody na przesłanie oraz osobnego
potwierdzenia konkretnej zwalidowanej kopii na LCD. Zgody są różne typami operacji,
związane z sesją, losowym nonce i generacją danych, wygasają po 120 s i nie zastępują
uwierzytelnienia panelu. Żądania mają dotychczasowe kontrole pochodzenia i metod;
żaden GET nie zmienia danych. Nie zapisujemy sekretów w podglądzie LCD/WWW.

`dsr_activity_begin()` jest licznikiem aktywności, a nie wyłączną blokadą zapisu.
Nowy protokół admission zatrzymuje przyjmowanie nowych mutacji, opróżnia/odmawia
bieżące zadania i uzyskuje bariery właścicieli konfiguracji, ustawień, połączeń,
automatyzacji oraz NimBLE store. Nie trzymamy mutexu podczas HTTP, flash lub ADC.
Operacja czeka na potwierdzone zatrzymanie właścicieli; timeout zwalnia wszystko.
Nie używa wymuszonego restartu. Pilne STOP pozostaje dostępne w normalnym runtime.
OTA (także staged/uncertain), provisioning, restart i inne utrzymanie danych wykluczają
start. Sama kopia nie uruchamia odpytywania ani komend do Shelly.

Snapshot jest niezmienną kopią w PSRAM. Krótka bariera trwa tylko podczas jego
zebrania; szyfrowanie i pobieranie nie zamrażają sterowania. Import dostaje własny
niezmienny kandydat. Potwierdzenie dotyczy jego SHA-256, identyfikatora i generacji
bieżących magazynów. Zmiana któregokolwiek magazynu unieważnia potwierdzenie; commit
ponownie uzyskuje bariery i sprawdza generację przed zapisem intencji. Nie ma luki
między sprawdzeniem a zamrożeniem. Anulowanie, timeout lub odejście zwalnia bufory
i zgody przed commit; po commit operacja musi się dokończyć przez recovery.

## Limity i miejsce na bezpieczne odtworzenie

Maksymalny snapshot: 2 MiB, w tym maksymalnie 64 KiB metadanych i 16 zdjęć po
115200 B (razem 1843200 B). Limity nie zwiększają obecnego JSON 24576 B ani liczby
urządzeń. Cały plik jest najwyżej 2 MiB + 64 B + 16 B. Alokacja PSRAM tylko na żądanie,
z kontrolą największego bloku i wolnej pamięci wewnętrznej; porażka przed zmianą danych.
Strumienie HTTP/flash mają bufory do 4096 B; brak dużych obiektów na stosie LVGL,
nowego stałego wielkiego BSS lub zmiany stosu UI 8192 B i puli LVGL 128 KiB.

`dsrdata` ma 0x3e0000 B; nie utrzymamy dwóch pełnych zestawów zdjęć i dodatkowego
archiwum. Przed commit zapisujemy i odczytujemy kontrolnie **osobne szyfrowane pliki
staging**: metadane i do 16 zdjęć. Nowy losowy klucz AES-256 jest niezależny od hasła
archiwum i zapisany tylko w prywatnym stanie operacji. Każdy rekord ma unikalne nonce,
tag oraz AAD wiążące operację, pilota, typ, indeks, długość i hash manifestu. Staging
nie jest nazwą/ścieżką z żądania i nie trafia do zwykłego przeglądu zdjęć ani GC.
Przed staging utrwalamy stan PREPARING z identyfikatorem operacji i kluczem.
PREPARING nie oznacza zgody na zmianę aktywnych danych: po restarcie recovery
usuwa tylko należące do niego pliki tymczasowe i klucz, potem dopuszcza normalny
start. Nieosierocone dane użytkownika pozostają nietknięte. Sprzątanie nigdy nie
wybiera plików na podstawie ścieżki z archiwum. Dopiero osobny COMMITTED pending
po fizycznej zgodzie dopuszcza właściwy import; niejednoznaczny stan blokuje start.

Preflight obejmuje aktualne pliki + staging + jedno odtwarzane zdjęcie + koszty FS
i co najmniej 65536 B zapasu, pozostając poniżej dotychczasowego progu 75% SPIFFS.
Sprawdzamy również miejsce NVS na dziennik i docelowe rekordy. Brak miejsca oznacza
odmowę bez kasowania aktualnych danych. Można najpierw wykonać kopię, jawnie zresetować
pilot i odtworzyć na pustym magazynie; reset nigdy nie jest automatycznym krokiem
zwalniania miejsca. Przy pełnym zestawie zdjęć odtwarzanie na pustym pilocie musi
mieścić się w budżecie. Eksport nie wymaga staging w flash.

## Odtwarzanie i przerwanie zasilania

1. Użytkownik wybiera plik i hasło. Zły tag/hasło lub obcy egzemplarz kończy import.
   ESP sam sprawdza format, całą zawartość i referencje; walidacja JS nie jest zaufana.
   Odszyfrowanie w przeglądarce poprzedza prośbę o krótkotrwałą zgodę przesłania;
   wolny KDF nie zużywa czasu zgody. Przyjęty transfer ma odrębny, ograniczony timeout.
2. Panel/LCD pokazują datę, wersję i liczby elementów oraz ostrzeżenie o zastąpieniu
   danych i nowym parowaniu WWW. Po sprawdzeniu miejsca powstaje zweryfikowany staging.
   Do tej chwili można anulować, a istniejąca konfiguracja pozostaje niezmieniona.
3. Fizyczne potwierdzenie LCD, ponowna kontrola baterii/generacji i bariera zapisu.
   Przed pierwszą zmianą aktywnych danych utrwalamy intencję i boot-latch w nowym,
   odrębnym `dsr_maint`. Dwa rekordy z CRC/wersją/sekwencją mają readback; choćby
   jeden wiarygodny pending blokuje normalny start, niespójność prowadzi do recovery.
4. Kontrolowany restart. Hook recovery działa po inicjalizacji NVS/niezbędnego LCD,
   **przed** settings, Wi-Fi, workspace/checkpointami/GC, cache zdjęć, TLS i BLE.
   Prosta ścieżka LCD pokazuje postęp bez startu zwykłego runtime i zadań sterujących.
5. Zdjęcia odtwarzamy kolejno: odczyt/decrypt jednego staging → zapis pliku treści →
   flush/readback/hash → trwałe potwierdzenie etapu → usunięcie jego staging.
   Dzięki temu przybywa najwyżej jedno zdjęcie ponad dotychczasowy zajęty staging.
   Powtórzony etap jest idempotentny. Po przerwaniu najpierw weryfikujemy gotowe pliki;
   brak staging jest dopuszczalny tylko dla potwierdzonego i poprawnego pliku wynikowego.
6. Odtwarzamy ustawienia/prywatne połączenia i komplet lokalnego store BLE; usuwamy
   przestarzały fallback `dsr_device`. Publiczna konfiguracja otrzymuje nową rewizję
   większą od obu starych checkpointów; oba sloty zapisujemy i weryfikujemy.
   Przy overflow rewizji import odmawia przed commit. Stary dziennik reguł zastępuje
   świeży rekord enrollment opisany wyżej; token panelu jest usuwany.
   Bieżący certyfikat TLS pozostaje; po resecie jest to nowy certyfikat.
7. Weryfikujemy wszystkie części, usuwamy staging i klucz operacji, zapisujemy
   bezkluczowy wynik, a dopiero na końcu czyścimy pending i wykonujemy normalny start.
   Sprzątanie po zaniku zasilania jest również idempotentne. Stare nieużywane zdjęcia
   zbieramy dopiero po zatwierdzeniu kompletnego nowego zestawu.

Przed commit błąd usuwa tylko pliki tej operacji i zwalnia admission. Po commit
gwarancją jest **dokończenie wprzód**, nie rollback do starego zestawu. Nigdy nie
uruchamiamy normalnego UI/radia z mieszaniną danych. Uszkodzenie, którego nie da się
wznowić, pozostawia ekran recovery z Ponów oraz osobno potwierdzanym pełnym resetem.
Nie zgadujemy domyślnych haseł ani nie kasujemy danych w obsłudze błędu.
Szczegóły trwałości fsync/usuwania/NVS trzeba zweryfikować na przypiętym SDK i sprzęcie;
samo API POSIX nie jest dowodem odporności SPIFFS na przerwanie zasilania.

Po pełnym resecie użytkownik najpierw ponownie łączy pilot z Wi-Fi i paruje panel WWW,
następnie otwiera kopię hasłem. Import przywraca zapisane w niej Wi-Fi i połączenia;
nowe parowanie panelu jest wymagane po zakończeniu. Nie przywracamy dawnych przeglądarek.

## Pełny reset

LCD wymienia usuwane dane, ostrzega, że urządzenia zewnętrzne pozostaną w swoim stanie,
i wymaga przytrzymania potwierdzenia przez 3 s. WWW może otworzyć prośbę na LCD, lecz
nie wykona resetu przez samo HTTP. Anuluj pozostaje dostępne do zapisu intencji.
Reset i commit odtworzenia stosują przyjętą politykę zasilania: świeże >=20%, poniżej
20% odmowa, nieznany/nieaktualny odczyt tylko po fizycznym potwierdzeniu stabilnego USB.
Używamy obecnego dostawcy/progów świeżości, bez wymyślania wykrywania ładowarki.

Po durable RESET pending i restarcie recovery kasuje wyłącznie jawne magazyny danych:
`dsr_cfg`, `dsr_bindings`, `dsr_device`, `dsr_battery`, `dsr_ui`, `dsr_auto`, `dsr_panel`,
`dsr_tls` i `nimble_bond`; następnie formatuje tylko `dsrdata`. Audyt kluczy i starych
fallbacków jest obowiązkowy przy planowaniu adapterów. Dziennik `dsr_maint` przeżywa
do readback braku danych i wyniku COMPLETE. Przerwany reset powtarza kroki bezpiecznie.
Nie ma `nvs_flash_erase()` całej partycji, ruszania OTA/otadata/PHY/eFuse ani urządzeń
Shelly. Profile użytkownika wracają do domyślnych, TLS i dostęp WWW powstają od nowa.
Reset oznacza logiczne usunięcie danych aplikacji; nie obiecujemy wymazania śladów
ze wszystkich historycznych komórek flash ani ochrony przed fizycznym odczytem NVS.

## Planowana walidacja i odbiór

- Format/krypto: znane wektory Web Crypto, Unicode hasła, zły tag/hasło, uszkodzenia,
  limity, przepełnienia, duplikaty, obcy pilot/schema, wszystkie referencje i zdjęcia.
- Granica dostępu: brak eksportu bez osobnej zgody, sesje/origin, jednorazowość,
  timeout/replay, zmiana kandydata/generacji, anulowanie; brak sekretów w logach/cache.
- Adaptery prawdziwego kodu: pełny round-trip ustawień/połączeń/bonds, fallback,
  checkpointy i CAS, NVS full, brak PSRAM/miejsca, wykluczenia OTA/sterowania, STOP.
  Osobno: odtworzona włączona reguła wznawia po świeżych danych bez `DSR_CONFLICT`,
  a podmieniona tożsamość nie dostaje zgody ani polecenia.
- Recovery: fault injection po każdej trwałej granicy, zapis zdjęcia/cleanup,
  oba checkpointy, NVS i usunięcie klucza; dowód blokady zwykłego startu przy pending.
- UI: rzeczywiste LVGL 9.3 dla dotkniętych stanów, trzy skórki i PL/EN/DE, długie
  napisy, klawiatura, przyciski/ramki i odstępy; panel telefon/komputer w Chromium.
- Aktywność: epoka/wrap 128, obserwacja kontra ACK, puste strony i czyszczenie RAM.

Podczas prac tylko właściwe regresje modułu i wybrane kadry, bez pełnego atlasu za
każdą poprawką. Po integracji jeden build targetu i kontrola budżetów pamięci/stosu,
niezależny przegląd bezpieczeństwa/kodu, następnie odbiór widoków w ankiecie.
Przed DEV pełne wymagane CI dla commita wydania oraz osobno autoryzowany odbiór
na sprzęcie: backup → reset → import na tym samym pilocie, realne BLE, Wi-Fi,
powiązania, zanik zasilania i integralność danych. Host i build nie dowodzą tych stanów.
Nie publikujemy DEV ani nie mergujemy main na podstawie tego dokumentu.

## Decyzje pozostawione implementującemu i źródła

Implementujący dobiera nazwy funkcji/pliki, opcodes UI i dokładne rozmiary w ramach
istniejącego stylu i limitów. Nie zmienia zakresu, formatu krypto, barier, wymaganych
potwierdzeń ani reguł odzyskiwania bez aktualizacji projektu. Kolejność: Aktywność,
fundament utrzymania danych i adaptery, backup/import/recovery, reset, odbiór integracji.
Szczegółowy plan powstanie po zatwierdzeniu projektu.

Źródła rozpoznania: `dsr_actions.h`, `web/actions.js`, `dsr_config_store.c`,
`dsr_photos.c`, magazyny `dsr_platform`, `main/app_main.c`, tablica partycji oraz
lokalny kod NimBLE ESP-IDF 5.5.4 (`ble_store_nvs.c`, `ble_store_config.h`).
Espressif MCP pytano jawnie o 5.5.4; odpowiedzi latest sprawdzano w przypiętym źródle.
[NVS 5.5.4](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/storage/nvs_flash.html)
opisuje commit i kasowanie namespace; nie gwarantuje transakcji obejmującej NVS i SPIFFS.
[Web Crypto, rekomendacja W3C](https://www.w3.org/TR/2017/REC-WebCryptoAPI-20170126/)
definiuje PBKDF2 i AES-GCM. Wybór 600000 iteracji jest decyzją projektu, z punktem
odniesienia w [zaleceniach PBKDF2 OWASP](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2);
zalecenia przechowywania haseł nie są specyfikacją formatu backupu.
