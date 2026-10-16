# Magazyn — inicjalizacja 0.7.7 i kopia bez zapisu (#28 / PR #33)

## Wznowienie przerwanego pierwszego formatu (2026-10-17, lokalny patch, NIE na GitHubie)

**Zmiana zachowania.** Do tej pory przerwany pierwszy format (prefiks 109/992
lub 313/992) był traktowany jak „niepusty FS” i blokował magazyn na zawsze.
Test hostowy wręcz to utrwalał. Jedno przerwanie w ~1 min formatu (ekran LCD
jest wtedy czarny, bo `dsr_ui_init` startuje później) oznaczało trwałą utratę
magazynu do ręcznego recovery.

`dsr_storage_mount()` po nieudanym montażu skanuje całą partycję i rozróżnia
trzy przypadki:

- **pusta** (same 0xFF) — jeden format, jak dotąd;
- **resztka przerwanego formatu** — wznowienie jednego pełnego formatu;
- **wszystko inne** — odmowa, każdy bajt bez zmian, jak dotąd.

Resztka musi wyglądać dokładnie tak, jak zostawia ją `SPIFFS_format()` z
IDF 5.5.4 dla TEJ geometrii (992 bloki, strona 256 B, magia z długością):

- bloki 0..N−1 mają dokładny nagłówek formatera: magia
  `0x20140529^256^(992−blok)` pod +252 oraz licznik kasowań 0 pod +254;
- blok N może mieć częściowo zaprogramowany nagłówek (bity skasowane tylko
  tam, gdzie wzorzec ma zera);
- wszystkie dalsze bloki i każdy inny bajt każdego bloku to 0xFF.

Wpis lookup, strona danych, nazwa pliku, nagłówek innej geometrii, licznik ≠ 0,
dwa częściowe bloki albo „dziura” (nagłówek po pierwszym niedokończonym
bloku) → odmowa. Taka resztka z definicji nie zawiera bajtu użytkownika.
Brak dodatkowych zadań, watchdogów, zmian priorytetów ani partycji.

**Testy (prawdziwy IDF 5.5.4 SPIFFS, symulowany NOR — NIE sprzęt):**

- prefiksy 1/109/313 → jeden pełny format (992 kasowania), zapis/odczyt pliku
  i remount bez zapisów;
- prefiks 991 → naprawia go sam `SPIFFS_mount` (1 kasowanie);
- prawdziwy formatter przerwany I/O po 109 i 313 sektorach daje bajt w bajt
  ten sam prefiks, a następny start go kończy;
- 6 wariantów częściowego nagłówka (także blok 0) → wznowienie;
- 12 wariantów „obcych” bajtów, w tym realny FS z plikiem i uszkodzoną magią
  → odmowa, bajty identyczne.

Test był czerwony na starym `dsr_storage.c` i jest zielony po zmianie.
Pełne `tools/test.py --sanitize` i `tools/test_config.py --sanitize` są zielone.

**Ekran postępu formatowania (2026-10-17, lokalny patch, NIE na GitHubie).**
LCD i LVGL są już zainicjowane przed magazynem (`dsr_board_init`). Teraz
na czas jedynego przebiegu formatowania pojawia się ekran „Przygotowanie
pamięci” (PL/EN/DE):

- animacja oczekiwania (spinner) i pasek postępu;
- licznik `N / 992 bloków • %` **odczytywany z Flash** (nagłówki formatera,
  wyszukiwanie binarne, ≤10 odczytów po 4 B co 500 ms), nie szacowany;
- czas od startu oraz ostrzeżenie „Nie odłączaj zasilania”;
- przy wznowieniu: komunikat o przerwanym formatowaniu, a postęp liczony
  dopiero powyżej starego prefiksu;
- na koniec „Pamięć gotowa” (0,8 s) albo komunikat o błędzie (4 s).

Renderuje wyłącznie zadanie UI (8 KiB stosu), nigdy stos `app_main`.
`app_main` buduje i zmienia obiekty tylko przy zamkniętej bramce i po
potwierdzeniu, że zadanie UI jest poza `lv_timer_handler` (handshake na
atomikach, `dsr_ui_task_quiesce`). Bramka ekranu nie oznacza „boot READY”:
nie potwierdza OTA i nie uruchamia usług. Gdy zadanie UI nie wystartuje,
zachowanie jest jak dawniej (ciemny ekran, format bez zmian).

Testy hostowe: prawdziwy formatter z sondą przed każdym kasowaniem (postęp
dokładnie = ukończone bloki, przy wznowieniu nigdy powyżej prefiksu); hooki
dokładnie raz wokół formatu, nigdy dla istniejącego lub odrzuconego FS;
7 klatek prawdziwego LVGL 320×480 i pamięć LVGL stała po 20 cyklach.
**Nie sprawdzono na płytce:** płynność animacji przy kasowaniu Flash
(cache wyłączony na czas każdego sektora) i wpływ na łączny czas formatu.

**Przyczyna przerwania nadal NIEUSTALONA.** Hipotezy, w kolejności
prawdopodobieństwa:

1. zewnętrzny reset w trakcie ~45–60 s czarnego ekranu (odłączenie zasilania,
   EN, ponowne połączenie ESPConnect/USB-Serial-JTAG);
2. brownout przy kasowaniu Flash z aktywnym Wi-Fi;
3. watchdog.

Różne punkty przerwania (109 vs 313) przemawiają raczej za resetem
zewnętrznym niż za stałym limitem czasu. Rozstrzyga `reset_reason` z
następnego startu (logowany już w `app_main`): POWERON, USB, BROWNOUT,
TASK_WDT, INT_WDT lub PANIC.

## Dwa warianty instalacji FACTORY i UPDATE (2026-10-16)

W [decyzji właściciela](https://github.com/voltex-at/DIY-Shelly-Remote/issues/28#issuecomment-6217161680)
potwierdzono włączone Erase All. Właściciel następnie
[sprostował wersję fizyczną na 0.7.23 / PR51](https://github.com/voltex-at/DIY-Shelly-Remote/issues/28#issuecomment-6217253815),
nie 0.7.25 / PR53. Erase All kasuje
wcześniejszą konfigurację; krótki merged BIN kończył się przed dsrdata i nie
zawierał przygotowanego systemu plików. Nowa relacja o 109/992 blokach pochodzi
z analizy w odrębnej rozmowie; aktualny dump nie jest dostępny w tej sesji.
Nie utożsamiamy go z historycznym 313/992 ani nie przypisujemy reset reason.

CI przygotowuje teraz dwa wyraźnie oddzielne artefakty:

- **FACTORY-CLEAN-16MB:** jeden pełny obraz pod0x0, dokładnie16MiB, pusty kompletny
  SPIFFS992/992 pod0xC20000, oficjalny spiffsgen IDF5.5.4, identyczna geometria
  i granice. Zawiera też osobne segmenty/offsety. Ten wariant usuwa dane i jest
  przeznaczony wyłącznie do osobno zatwierdzonego czystego testu.
- **UPDATE-APP-ONLY:** tylko aplikacja do właściwego slotu, bez erase/DSR obrazu,
  bootloadera/partycji/otadata. Produkcyjny OTA truststore nadal EMPTY; CI BIN
  nie jest zatwierdzonym podpisanym wydaniem.

Pełna tablica/MD5,16MiB boundary, każdy segment i każda przerwaFF, CRC32/SHA256,
app checksum/hash/chip9 oraz montowanie dokładnie tego obrazu na oryginalnym
IDF są obowiązkowymi bramkami przed uploadem. Weryfikacja hostowa nie zapisuje
obrazu wejściowego: zapis testowej konfiguracji/remount odbywa się na kopii NOR.
[Instrukcja FACTORY](INSTALL_FACTORY_PL.md) i [UPDATE](INSTALL_UPDATE_PL.md).
Nie wykonano device writes, nie proponuje się kolejnego erase przed gotowym
artefaktem. Factory omija koszt pierwszego formatu na płytce, **nie zamyka
przyczyny przerwania automatycznego formatowania** ani testu fizycznego.

## Sprostowany raport 0.7.23 / PR51 i test trwałości (2026-10-16)

Fizyczny raport dotyczy PR51/source `07331b8`, wersji 0.7.23; PR53 nie został
w tym teście zainstalowany. Według sprostowania właściciela odrębna analiza
kopii znalazła 109 z 992 kolejnych nagłówków i 883 bloki 0xFF, bez plików.
Aktualny dump nie jest dostępny w tej sesji; nie jest to nasza analiza kopii.
Nie ma pełnego logu pierwszego nieudanego startu. Dawny prefiks 313 bloków
jest oddzielną obserwacją. Erase All wyjaśnia utratę danych, lecz **nie powód
przerwania formatu na bloku 109**. To nie dowód fizycznej regresji AP/PR53.

`dsr_storage_mount()` ma identyczny blob
`baa433f649d69d768877d492b5a4036984d8d817` w PR51 `07331b8` i PR53 `267c32f`.
Nie zmieniamy watchdogów, yield ani czasu oczekiwania na podstawie samego
prefiksu. Przed ewentualnym osobno zatwierdzonym testem monitor USB musi
działać **przed pierwszym EN**. Aplikacja loguje `reset_reason` jeszcze przed
inicjalizacją NVS/board/storage, następnie istniejące `blank_check`,
`format_started`, `format_finished` i READY. Brak końcowego logu wraz z kodem
resetu następnego bootu pomaga rozróżnić reset; nie przesądza jego przyczyny.
Bez nowego Erase All lub formatowania. Ewentualna osobno uzgodniona reprodukcja
dotyczy wyłącznie dsrdata po prywatnej kopii, nigdy NVS ani OTA.

Porównanie `0790e6e803c0bb8e091117e0cc37a47f8eb7782b` →
`267c32f31d7d955b26cc611e0ee7685226813ffb` daje **zero zmian** w
`dsr_storage.c`, `dsr_config_store.c`, obu `sdkconfig.defaults`,
`boards/lcdwiki-es3c35p/partitions.csv` i `dependencies.lock`.
Sama zbieżność awarii z aktualizacją nie ustala zmiany geometrii ani kodu
montowania. Ochrona niepustego magazynu przed formatowaniem pozostaje właściwa.

`python3 tools/test_idf_spiffs.py --ubsan-only` kompiluje niezmienione źródła
przypiętego IDF 5.5.4 i SPIFFS oraz **rzeczywiste** `dsr_config.c` i
`dsr_config_store.c`. Ograniczony most hostowy przekazuje ich wywołania plikowe
do oryginalnej zarejestrowanej tabeli VFS. Nie udaje libc FILE buffering,
RTOS ani fizycznego sterownika. Model NOR wykonuje AND bitów: zaprogramowane
zera pozostają zerami, także gdy SPIFFS zapisuje maski flag. Odpowiada to
domyślnemu `CONFIG_SPI_FLASH_VERIFY_WRITE=n`, potwierdzonemu w retained
target sdkconfig; nie zmieniamy konfiguracji produkcyjnej ani zależności SDK.

Zestaw zachowuje poprzednie testy blank-check/jednego formatu, przerwania
formatu po 313 sektorach, błędów I/O, dziesięciu remountów i niezależnego
obrazu spiffsgen. Nowy przypadek zapisuje 8 pomieszczeń i 16 urządzeń przez
**1112 rewizji**, aż realny SPIFFS uruchomi GC. Zapis wywołujący GC ma
70 operacji NOR, w tym jedno kasowanie sektora. Dla każdej operacji odrębny
proces kończy się natychmiast przed, w połowie i po operacji: **210 przerwań**,
bez close/unmount/flush po przerwaniu. Po ponownym montowaniu dostępny jest
dokładnie poprzedni albo kompletny nowy checkpoint; niezmienny plik kontrolny
64 KiB też pozostaje zgodny. Osobno sprawdzany jest błąd zapisu i brak pełnego
formatowania podczas remountu. Oryginalne wznowienie przerwanego GC przy mount
może dokończyć kasowanie jednego sektora; to nie jest format całej partycji.

To **test syntetyczny**, nie odtworzenie raportu właściciela i nie naprawa
obecnego egzemplarza. Nie obejmuje przerwań zasilania sprzętu, uszkodzeń Flash,
pełnego VFS/libc, konfliktów z RTOS ani retencji rzeczywistych zdjęć. Procedura
poniżej służy zachowaniu aktualnej kopii; analizujemy ją dopiero po dołączeniu
w bieżącej rozmowie, wraz z pierwszym logiem startu i sposobem instalacji
(APP/OTA czy obraz scalony/Erase All). Nie ma automatycznego formatowania,
odtwarzania pustego obrazu ani zgody na zapis do urządzenia.

## Aktualizacja P0: 0.7.6 nie przeszła czystej instalacji

Recenzja opisuje analizę prywatnego odczytu właściciela: poprawny prefiks 313
nagłówków SPIFFS z 992 bloków i reszta 0xff, bez checkpointów/plików. Późniejszy
boot wykazuje niedostępny workspace przy działającym dotyku/callbacku. To dowód
częściowego formatowania, nie konkretnego watchdog/resetu. Nie publikujemy
dumpu ani nie zlecamy powtórnego pełnego erase/instalacji 0.7.6.

0.7.7 usuwa dwa formaty wykonywane przez IDF 5.5.4 przy osobnym esp_spiffs_format
niezamontowanej partycji. Po **pełnym sprawdzeniu 0xff** wykonuje register z
format_if_mount_failed=true, jeden przebieg. Na czas formatu zadanie startowe
dzieli CPU z idle (priorytet 0, preempcja/time slicing IDF); priorytet wraca także
po błędzie. Zachowujemy watchdog i domyślne yield Flash, bez nowego zadania.
Blank-check oddaje tick co 256 KiB.

Pierwszy log: `blank_check checked=... total=4063232`, `format_started pass=1/1
blocks=992`, `format_finished result=0x0 elapsed_ms=...`, `mount_ready`,
`workspace_ready`, `https_ready`. Przy przerwaniu istotne są brak końcowego logu
i reset reason następnego bootu. Publiczny wrapper nie ma callbacku sektorów;
licznik to postęp sprawdzania oraz start/zakończenie jednego przebiegu formatu.

**Niepusty prefiks 313 pozostaje chroniony.** Sam zapis aplikacji 0.7.7 nie kasuje
go i nie naprawia automatycznie. Test pustej dsrdata lub preformatowanego obrazu
wymaga osobno uzgodnionej konkretnej jednorazowej operacji po zachowaniu kopii.
To nie jest zgoda na ponowny pełny erase, automatyczny format niepustego
magazynu albo nadpisanie NVS. DEV nadal wymaga płytki; [bramki](PR33_ACCEPTANCE.md).

Poniżej procedura kopii/analizy bez zapisu oraz historyczna diagnostyka 0.7.6.

## Ustalona przyczyna i granice poprawki

Recenzja #28 zawiera oczyszczony log fizycznej instalacji 0.7.4:
`SPIFFS mount failed, -10025` = `SPIFFS_ERR_NOT_A_FS`, niepusta partycja,
`Configuration storage unavailable: ESP_FAIL`. Dotyk działa. Magazyn nie
startuje, więc stary callback edycji zwracał bez komunikatu; serwer HTTPS
był pomijany. To potwierdzony FAIL odbioru 0.7.4, mimo poprzedniego CI.
Nie znamy jeszcze przyczyny niezgodnych bajtów ani ich wartości dla właściciela.
Nie przypisujemy logu do commitu wyłącznie po numerze wersji.

0.7.6 naprawia **ciche błędy i diagnostykę**. Nie naprawia przez zgadywanie
nieznanej zawartości Flash. Przy niedostępnym SPIFFS edycja nadal nie może
zapisać pokoju; pokazuje konkretną informację i ekran diagnostyczny. Sterowniki,
dotyk, podświetlenie, transporty, stare powiązanie/NVS, HTTPS i partycje OTA
pozostają w dotychczasowej architekturze. Nie dodano formatowania, captive
portalu ani konta/hasła domyślnego. Dotychczasowa inicjalizacja FS tylko po
potwierdzeniu całej partycji jako 0xff pozostaje bez zmian.

LCD pokazuje kategorię i wynik ESP-IDF, np. niepusty niezamontowany SPIFFS /
ESP_FAIL. Rejestrator SPIFFS zwraca aplikacji ogólny ESP_FAIL; dokładnego
wewnętrznego -10025 nie da się z niego odtworzyć. Odczytaj go z logu USB;
nie oznaczamy każdego ESP_FAIL jako NOT_A_FS. Osobno widoczny jest stan
workspace i HTTPS. Brak gotowego serwera oznacza brak generowania PIN.

## 1. Kopia przed jakimkolwiek flashowaniem lub naprawą

Na komputerze z podłączonym pilotem zamknij ESPConnect/monitor portu.
Z paczki firmware uruchom w terminalu (Windows, zamień COM5 na właściwy port):

```powershell
py -m pip install "esptool>=5,<6"
py storage_recovery.py backup --port COM5 --out PRIVATE-backup
```

Z repozytorium plik znajduje się w `tools/storage_recovery.py`. Linux/macOS:
użyj `python3` zamiast `py` i właściwego portu, np. `/dev/ttyACM0`.
Przy błędach transmisji użyj **nowego** katalogu i `--baud 115200`.
Narzędzie nie nadpisuje istniejącego katalogu. Odczyt 3,875 MiB dwa razy
może potrwać kilka minut. Po ukończeniu naciśnij EN/reset, aby wrócić do UI.
Przy przerwaniu zachowaj częściowe kopie.

Wykonywane są wyłącznie polecenia `read-flash` (esptool 5.x) lub `read_flash`
(4.x); stub ładowany jest do RAM, nie do Flash. Pierwsze połączenie wprowadza
tryb odczytu, a kolejne nie resetują urządzenia do działającej aplikacji.
To zapobiega zmianie konfiguracji przez firmware pomiędzy dwiema kopiami.

| Prywatny plik | Offset | Długość |
| --- | --- | --- |
| partition-table.bin | 0x8000 | 0x1000 |
| PRIVATE-nvs.bin | 0x9000 | 0x10000 |
| dsrdata.bin | 0xC20000 | 0x3E0000 |

Tablica partycji i MD5 są sprawdzane **przed** odczytami pozostałych obszarów;
inna tablica zatrzymuje skrypt. Każdy obszar jest czytany dwukrotnie,
porównywany bajt po bajcie i zapisywany z SHA-256 w
`PRIVATE-backup-report.json`. Skrypt nie wypisuje treści Flash ani logów
esptool zawierających identyfikatory.

**Nie publikuj żadnej kopii w GitHubie. NVS zawiera sekrety i zostaje u Ciebie.**
Do dalszej prywatnej analizy wystarczą początkowo dsrdata.bin, partition-table.bin
i raport, dołączone w bieżącej rozmowie. Dane mogą zawierać nazwy/IP/zdjęcia.
Nie ma tu zdalnego dostępu do portu USB komputera właściciela.

## 2. Analiza kopii na komputerze

```powershell
py storage_recovery.py analyze PRIVATE-backup/dsrdata.bin --out PRIVATE-analysis --extract
```

Analizator nie dotyka ESP32. Weryfikuje rozmiar, SHA, całkowite 0xff,
geometrię/magic length SPIFFS i kompletność indeksowanych plików. Dla domyślnej
geometrii IDF 5.5.4: blok 4096, strona 256, nazwa 32, meta 4, little endian,
indeksy nie są wyrównywane. Inne parametry podaj dopiero po porównaniu
efektywnego sdkconfig poprzedniego firmware; nie zmieniaj ich w pilocie na próbę.
Zgodne magic dla innego rozmiaru mogą wskazywać dawny format innej długości.

Odzysk wymaga zgodnego lookup, pełnych indeksów i właściwych ID/span/flag stron.
Zduplikowane aktywne nagłówki lub brakujące/niezgodne strony są odrzucane.
Wydobyte pliki to **kandydaci offline**, nie zatwierdzona naprawa FS. Dla
config0.dat/config1.dat dodatkowo sprawdzane są długość rekordu DSR1,
CRC32 i zgodność rewizji JSON. Pełna walidacja modelu C i wybór wersji pozostają
osobnym krokiem przed odtworzeniem. Brak kandydata nie dowodzi braku wartościowych
danych. Analizator nie jest pełnym checkerem/mounterem SPIFFS ani journal replay.

Źródła formatu:
[SPIFFS generator IDF v5.5.4](https://github.com/espressif/esp-idf/blob/v5.5.4/components/spiffs/spiffsgen.py),
[konfiguracja SPIFFS](https://github.com/espressif/esp-idf/blob/v5.5.4/components/spiffs/include/spiffs_config.h),
[Kconfig](https://github.com/espressif/esp-idf/blob/v5.5.4/components/spiffs/Kconfig).

## 3. Dalszy odzysk i zgoda

Najpierw oceniamy kopie oraz możliwość odczytu istniejących plików.
Jeżeli dane da się odzyskać, zachowujemy oryginał i pracujemy na kopii.
Jeśli brak odzyskiwalnego systemu i właściciel potwierdzi brak potrzebnych
danych, można dopiero przedstawić mu konkretną inicjalizację **samej dsrdata**.
Ta wersja **nie zawiera** polecenia kasowania/formatowania i nie ma zgody na
usunięcie niepustej partycji. NVS, bootloader, OTA0/OTA1 zostają zachowane.

Nie wgrywaj scalonego local.bin przed kopią: nawet bez erase nadpisuje NVS
w przerwach obrazu. Ewentualny test 0.7.6 używa samego
`diy_shelly_remote.bin` i **aktualnego aktywnego slotu potwierdzonego logiem**
(0x20000 lub 0x620000), przy identycznej tablicy. Nie zakładaj 0x20000 po OTA.
Pełna instrukcja instalacji jest również w BOOT_MEMORY_FIX_PL.md.

## Test fizyczny wymagany po analizie / zatwierdzonym odzysku

1. Zapisz exact app SHA-256 i wersję, log USB 115200 bez SSID/MAC/JSON/sekretów.
   Brak pętli restartu, obraz i dotyk; liczby internal_free/largest/psram_free.
2. Na obecnym niedostępnym magazynie: ołówek HOME oraz górny, środkowy i dolny
   Add w Rooms pokazują błąd jednym dotknięciem; Diagnostyka działa i Back
   wraca. Settings → Diagnostyka konfiguracji działa bez workspace.
   USB: pencil_click/add_room_click → callback → snapshot → open_failed.
3. Po bezpiecznym odzyskaniu magazynu: ołówek otwiera Edytuj pilot; Add otwiera
   formularz; Salon → Zapisz → powrót → reset → Salon zachowany.
   Drugi pokój, edycja, referencje, Web CAS; HTTPS przez IP/mDNS i prawdziwy
   16-znakowy kod/fingerprint. Nie pokazuj kodu przy niedostępnym HTTPS.
4. Wi-Fi/LAN/ON-OFF istniejącego Shelly, BLE scan z Wi-Fi i bez, jasność,
   wybudzanie. To nie jest test BLE pairing/provisioning.
5. Istniejący FS po upgrade ma zachować dane; uszkodzone config nie są
   zastępowane domyślnymi. Przypadek pierwszej pustej partycji tylko na
   osobnej płytce/kopii testowej, bez niszczenia obecnego egzemplarza.

Automaty: rzeczywisty LVGL, błędy alokatora cJSON, siedem procesów startu
workspace, negatywne mount/read/ENOMEM oraz 9 testów offline analizy i backupu,
w tym stały syntetyczny obraz z rzeczywistego spiffsgen.py ESP-IDF v5.5.4
(ścieżki /config0.dat i plik 51200 B z kolejnymi indeksami).
Nie zastępują testu USB/radia/Flash na ES3C35P. PR #33 pozostaje Draft;
odbiór #28 i zależny rozwój #20/#29 są zablokowane do naprawy magazynu.
