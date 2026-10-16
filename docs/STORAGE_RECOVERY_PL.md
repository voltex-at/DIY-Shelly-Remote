# 0.7.6 — niedostępny magazyn, diagnostyka i kopia bez zapisu (#28 / PR #33)

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
workspace, negatywne mount/read/ENOMEM oraz 8 testów offline analizy i backupu.
Nie zastępują testu USB/radia/Flash na ES3C35P. PR #33 pozostaje Draft;
odbiór #28 i zależny rozwój #20/#29 są zablokowane do naprawy magazynu.
