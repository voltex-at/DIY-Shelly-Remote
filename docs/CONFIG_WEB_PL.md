# Konfiguracja dotykowa i lokalny Web Panel — 0.5.1-review-fixes

Wersja rozwojowa do oceny w PR #9–#11, oparta na PR #5, nie scalona z main. Render HOME i panel są
wygenerowane z kodu projektu. Kompilacja i testy na komputerze przeszły; ta wersja
nie ma jeszcze potwierdzenia działania HTTPS, zapisu i OTA na fizycznym ES3C35P.

## Pierwsze wgranie z 0.2.0

Potrzebny jest USB: stary firmware nie ma instalatora OTA, a tabela partycji
musi otrzymać nowy wpis danych. Profil wyłącznie LCDWiki ES3C35P N16R8, 16 MB.
Zachowaj kopię ustawień i użyj osobnych plików z jednej paczki CI:

```sh
python -m esptool --chip esp32s3 --port PORT write_flash \
  0x0 bootloader/bootloader.bin \
  0x8000 partition_table/partition-table.bin \
  0x19000 ota_data_initial.bin \
  0x20000 diy_shelly_remote.bin
```

Podstaw port urządzenia. Nie wykonuj erase-flash. Nie używaj obrazu łączonego
`DIY-Shelly-Remote-es3c35p-local.bin`, jeżeli chcesz zachować NVS: taki obraz
wypełnia także przerwy pomiędzy plikami. Osobne pliki powyżej omijają NVS
0x9000–0x18fff. Wi-Fi i poprzednie powiązanie Shelly pozostają w NVS.

Sloty OTA nadal mają po 0x600000 bajtów: ota_0 od 0x20000, ota_1 od 0x620000.
SPIFFS dsrdata zajmuje wyłącznie dawniej wolny obszar 0xc20000–0xffffff.
Automatyczne formatowanie następuje tylko po potwierdzeniu, że cała partycja
jest fabrycznie pusta (0xff). Uszkodzona, niepusta partycja nie jest kasowana.

## Edycja na ekranie

Po konfiguracji Wi-Fi wejdź w edycję przez ołówek na pasku pulpitu lub ustawienia.
Edytor pozwala tworzyć i usuwać pulpity, pomieszczenia, wpisy urządzeń i scen;
edytować nazwy klawiaturą; dodawać karty; zmieniać ich kolejność strzałkami,
typ, wysokość i widoczne pola. Pomieszczenia mają pół lub pełną szerokość;
urządzenia i sceny pełną szerokość. Dostępne są język, motyw i kolor akcentu.
Usunięcie obiektu używanego przez kartę/scenę wymaga najpierw usunięcia odwołań.

Zapis jest wspólny z panelem i nie restartuje pilota. Bieżące połączenie Wi-Fi
zachowuje wcześniejszy proces zapisu i restartu. Edytor nie nadpisuje po cichu
nowszej zmiany z drugiego interfejsu: przy konflikcie odczytaj aktualną wersję
i ponów swoją edycję. Zmiany wpisane podczas zapisu pozostają oznaczone jako
niezapisane; potwierdzenie wcześniejszej wersji ich nie nadpisuje. Przed wyjściem
zapisz robocze zmiany.

## Parowanie panelu

Na pilocie otwórz parowanie Web Panelu. Odczytaj adres oraz odcisk SHA-256
certyfikatu. Otwórz `https://shelly-remote.local` lub adres IP w tej samej sieci.
Certyfikat jest generowany indywidualnie w urządzeniu i podpisany przez nie samo;
przy pierwszym zaufaniu porównaj jego odcisk z ekranem pilota. Dla IP przeglądarka
może dodatkowo zgłosić niezgodność nazwy. Port HTTP przekierowuje do HTTPS.

Wprowadź 16-znakowy kod z ekranu. Jest ważny przez 120 sekund i maksymalnie
5 prób. Nie ma fabrycznego hasła. Sesja używa Secure/HttpOnly/SameSite cookie;
ponowne parowanie unieważnia poprzednią przeglądarkę. Podgląd obrazu jest
wstrzymany podczas aktywnego parowania, aby nie ujawnić kodu przez API.
Panel działa lokalnie; nie konfiguruj przekierowania portów na routerze.

W panelu edytuj te same obiekty, przeciągaj karty albo używaj strzałek. Zapis
zwraca najpierw przyjęcie operacji, a potem jej wynik po trwałym zapisie.
Podgląd 320×480 pochodzi z LVGL, a nie z imitacji HTML. Odświeżanie stanu
i konfiguracji odbywa się automatycznie; niezapisany szkic nie jest zastępowany.

## Zdjęcia i pamięć

Panel kadruje JPG/PNG/WebP do 320×180 i konwertuje do RGB565 w przeglądarce.
Do ESP trafia gotowy obraz 115200 bajtów, oznaczony skrótem treści. Obrazy są
lokalne, bez chmury/CDN. Ekran dotykowy może wybrać już przesłane zdjęcie.
Limit wynosi 16 obrazów, z dodatkową rezerwą miejsca Flash. Po usunięciu lub
zastąpieniu zdjęć zapisz konfigurację i uruchom pilot ponownie: podczas startu
odzyska miejsce po nieużywanych zdjęciach, zanim LVGL otrzyma ich bufory.
Uszkodzone kopie konfiguracji wstrzymują sprzątanie. Karta SD nie jest używana;
nie dodano niepotwierdzonego sterownika. Bufory obrazów zajmują do 1,76 MiB PSRAM.

## Urządzenia i sceny — granica obecnego etapu

Model przechowuje do 16 wpisów urządzeń, ale zachowany moduł komunikacji
obsługuje nadal jedno zweryfikowane połączenie Shelly relay/switch. Formularz
powiązania w panelu używa tego samego sprawdzenia tożsamości i zapisu dopiero
po powodzeniu; nowe poprawne powiązanie zastępuje poprzednie. Hasła nie trafiają
do eksportowanej konfiguracji ani logów. NVS nie jest szyfrowane.

Karty rolet i czujników są gotowe wizualnie i konfiguracyjnie; nie dodano ich
nowych protokołów komunikacji ani wykonawcy scen. Bez rzeczywistych danych
urządzenia są offline, a odczyty niedostępne. Liczby na renderach testowych są
oznaczone DEMO i nie wchodzą do firmware. Natywne formularze, kreator i stany
zapisu mają PL/EN/DE i polskie znaki; Web Panel pozostaje w języku polskim.

Przycisk + otwiera kreator z rzeczywistym wykrywaniem LAN. Wybierz urządzenie,
podaj jego hasło, sprawdź kanały z odpowiedzi API i przypisz nazwę/pomieszczenie.
Wi-Fi AP i BLE są jawnie niedostępne. Ręczne IP pozostaje opcją zaawansowaną.
Ograniczenia skanowania i test prawdziwego Shelly opisuje ISSUE7_LAN.md.

## Aktualizacja lokalna OTA

Na ekranie zezwól na OTA na 120 sekund. W sparowanym panelu wybierz plik
**diy_shelly_remote.bin aplikacji**, nie obraz łączony USB. Jedna próba zużywa
zezwolenie. Instalator sprawdza rozmiar, układ ESP32-S3, nazwę projektu, marker
dokładnej płytki, SHA-256 i poprawność obrazu ESP-IDF. Zapisuje nieaktywny slot,
po czym udostępnia jawny restart. Zdjęcia, konfiguracja i NVS nie są zapisywane
przez OTA. Nowy firmware potwierdza start po lokalnym teście UI/dotyku/konfiguracji
i gotowości panelu; nie wymaga dostępu do Internetu ani działającego Shelly.

To rozwojowy instalator pliku wybranego przez operatora. **Nie weryfikuje podpisu
wydawcy i nie pobiera automatycznie GitHub Releases.** SHA-256 chroni spójność
transmisji, nie tożsamość autora. Nie zapisuje eFuse. Powrót do starszej wersji
schema konfiguracji nie może nadpisać nowszego poprawnego zapisu.

Przed uznaniem wersji za stabilną trzeba na sprzęcie sprawdzić parowanie i odcisk,
zmiany w obie strony, zapis po odłączeniu zasilania, połączenie z rzeczywistym
Shelly oraz aktualizację/przerwany transfer/rollback i zachowanie ustawień.
