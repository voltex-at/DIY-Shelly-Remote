# FACTORY-CLEAN-16MB — wyłącznie czysta instalacja ES3C35P N16R8

**Ten wariant usuwa wcześniejsze Wi-Fi, powiązania Shelly, pomieszczenia,
zdjęcia i ustawienia. Nie służy do aktualizacji działającego pilota.**
Paczka jest przygotowanym artefaktem do późniejszego świadomego testu na nowej
płytce lub egzemplarzu przeznaczonym do czystego testu. Nie uruchamia żadnego
polecenia na sprzęcie. Przed testem istniejących danych zachowaj prywatną kopię
według STORAGE_RECOVERY_PL.md. Samo przygotowanie paczki nie jest zgodą na erase.

Po osobnym zatwierdzeniu czystego testu w ESPConnect wybiera się jeden plik
`DIY-Shelly-Remote-es3c35p-FACTORY-CLEAN-16MB.bin`, offset **0x0**, Flash **16 MB**,
ESP32-S3, DIO/80 MHz. W tym wariancie czysta instalacja obejmuje Erase All.
To pełne **16777216 bajtów**, kończące się dokładnie na 0x1000000, wraz z całym
przygotowanym SPIFFS pod 0xC20000. Nie używaj krótszego historycznego `local.bin`
jako pełnego obrazu Flash.

| Segment alternatywnej instalacji | Offset | Granica/rozmiar partycji |
| --- | --- | --- |
| bootloader/bootloader.bin | 0x0 | przed 0x8000 |
| partition_table/partition-table.bin | 0x8000 | 0x1000 |
| ota_data_initial.bin | 0x19000 | 0x2000 |
| diy_shelly_remote.bin | 0x20000 | ota_0: 0x600000 |
| dsrdata.bin | 0xC20000 | dokładnie 0x3E0000 |

`flasher_args.json` wskazuje jeden pełny obraz pod 0x0.
`flasher_args-segments.json` opisuje powyższe segmenty z dsrdata.
NVS 0x9000+0x10000, phy_init 0x1B000+0x1000, ota_1 0x620000+0x600000 i wszystkie
pozostałe przerwy pełnego obrazu mają 0xFF. Obraz nie zawiera haseł,
certyfikatów, ustawień użytkownika ani prywatnego dumpu. Firmware dopiero
utworzy własną domyślną konfigurację na zamontowanym pustym systemie plików.

`FACTORY_MANIFEST.json` zawiera rozmiary, CRC32 i SHA256 pełnego obrazu,
dsrdata i każdego segmentu. `SPIFFS_MOUNT_PROOF.json` musi pasować do SHA
dsrdata i potwierdzać montowanie na niezmienionej bibliotece IDF 5.5.4 bez
formatowania i mutacji, a także próbę zapisu i ponownego odczytu konfiguracji
na symulowanej kopii NOR. `SHA256SUMS.txt` obejmuje wszystkie pliki BIN.
To kontrola artefaktów i modelu, nie odbiór sprzętowy.

Po zatwierdzonym teście potrzebny jest pełny log USB pierwszego bootu,
storage/workspace/HTTPS READY, utworzenie pokoju → Zapisz → restart → zachowany
pokój, następnie co najmniej dwa Shelly i powrót LAN. Zapisz exact source/hash
z tej paczki. Nie publikuj Wi-Fi/MAC/PIN ani prywatnych kopii na GitHubie.

Factory omija koszt pierwszego pełnego formatowania na płytce. **Nie ustala
przyczyny przerwania automatycznego formatu** i nie zamyka tego P0. Relacje
o prefiksach 109/992 oraz wcześniejszym 313/992 są oddzielnymi dowodami;
potrzebny jest pierwszy log przerwania/reset reason. Main i Stable pozostają
bez zmian.
