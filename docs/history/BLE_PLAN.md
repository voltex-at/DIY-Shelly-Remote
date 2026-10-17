# BLE Scan — 0.7.0-ble-scan-dev (Issue #18 / #7B)

Osobna gałąź `work/issue-18-ble-scan`, oparta na `ed2ef9f` (PR #17).
To implementacja skanera radiowego, nie makieta listy. Nie oznacza jeszcze
ukończonego parowania, konfiguracji Wi-Fi przez BLE, sensorów BLU ani sterowania
RPC przez BLE. Nie zmienia działającego transportu HTTP/NVS przekaźnika.

## Zakres pierwszego etapu

`+ → Dodaj urządzenie → Bluetooth LE → Szukaj` uruchamia ESP-NimBLE
w ESP-IDF 5.5.4 na ESP32-S3. Skan działa także bez Wi-Fi; trwa najwyżej 20 s.
Aktywne zapytania scan request pozwalają uzyskać nazwy i manufacturer data ze
scan response. Nie są połączeniem GATT i nie zapisują ustawień urządzenia.
Okno/odstęp 25/100 ms ogranicza obciążenie radia; współistnienie Wi-Fi/BLE
jest włączone. Host NimBLE i manager pracują na core 1, Wi-Fi na domyślnym core 0.
Pamięć hosta NimBLE korzysta z potwierdzonej PSRAM N16R8; kontroler zachowuje
własne wymagania pamięci. Sterowniki LCD, dotyku i podświetlenia są zachowane.

Osobne zadanie i krótkie callbacki NimBLE przekazują ograniczony snapshot.
Callbacki nie wywołują LVGL. Anuluj zatrzymuje procedurę radiową, a powrót z
ekranu pilota anuluje jego skan. Timeout/cancel zamykają procedurę; kolejne żądanie
jest dozwolone dopiero po cleanup. Token generacji odrzuca spóźnione wyniki.
NimBLE uruchamia się dopiero przy pierwszym skanie i pozostaje zainicjalizowany.
Stały koszt hosta nie jest wyciekiem; po pierwszym uruchomieniu trzeba sprawdzić
stabilność minimum 10 cykli. Nie obiecujemy odzyskania całej pamięci hosta po skanie.

Ekrany PL/EN/DE pokazują czas, liczbę wyników, anulowanie, pustą listę, brak BLE,
RSSI, wiek reklamy oraz jawny stan `Wykryto • nie sparowano`. Lista przewija się.
Wynik RPC może przejść do istniejącego rzeczywistego wyszukiwania LAN; nie
przekazujemy po cichu danych Wi-Fi przez Bluetooth.

## Rozpoznawanie z reklam

- UUID usługi RPC `5f6d4f53-5f52-5043-5f53-56435f49445f`: grupa reklamowanego
  Shelly RPC. Nie zgadujemy Gen2/Gen3/Gen4 lub funkcji na podstawie nazwy.
  Ten UUID nie dowodzi, że urządzenie ma Wi-Fi — może należeć też do BLU TRV.
- Shelly BLU: udokumentowana usługa BLU lub manufacturer ID `0x0BA9` z
  rozpoznanym blokiem modelu `0x0B`, ewentualnie z prawidłowym BTHome v2.
  Mapowanie nazw modeli pochodzi z tabeli producenta. Nieznany ID pozostaje
  nieznany; nie przypisujemy wymyślonej generacji.
- Service data `0xFCD2` z nagłówkiem BTHome v2 bez danych Shelly trafia do
  osobnej grupy. **BTHome jest otwartym standardem i samo nie potwierdza Shelly.**
- Sama nazwa Shelly/SB jest tylko wskazówką niezweryfikowanej reklamy. Nazwy,
  UUID i manufacturer data nie są uwierzytelnieniem urządzenia.
- Legacy AD do 31 bajtów, maks. 16 wyników; walidacja długości, sanitizacja nazw,
  deduplikacja adres + typ adresu, scalanie scan response, świeży RSSI i czas.
  Rotacja prywatnego adresu bez bondingu może dać kolejny wpis. Nie dekodujemy
  zaszyfrowanych pomiarów, nie odczytujemy kluczy i nie obsługujemy Extended ADV.

## Panel WWW i API

Sparowany lokalny panel HTTPS oferuje `Urządzenia → Wyszukaj Shelly w LAN →
Bluetooth LE • skan`. Korzysta z tego samego skanera co LCD:

| API v1 | Działanie |
| --- | --- |
| GET `/api/v1/ble` | snapshot grup, parametrów i stanu; bez MAC |
| POST `/api/v1/ble/scan` | rozpoczęcie, 202 lub 409 przy zajętości |
| POST `/api/v1/ble/cancel` | żądanie anulowania |

Wszystkie ścieżki są za istniejącą autoryzacją i kontrolą local/same-origin.
Brak nowych portów, publicznej telemetrii i chmury. Logi aplikacji zawierają
wyłącznie etapy i liczniki pamięci, bez obcych MAC, identyfikatorów i haseł.

## Automatyczne sprawdzenia i ich granice

`tools/test.py --ubsan-only`: rzeczywisty parser i adapter skanera C z jawnymi
stubami NimBLE/FreeRTOS. Obejmuje błędne długości, 2000 deterministycznych ramek,
manufacturer/model, BTHome nie-Shelly, UUID RPC, scalenie odpowiedzi, limit,
public/random adres, zawinięcie zegara, 20 s, anulowanie, 10 cykli, reset hosta,
nieudaną inicjalizację, zajętość i odrzucenie spóźnionych callbacków.

`tools/test_ui.py --ubsan-only`: rzeczywiste LVGL 9.3.0 z jawnym dostawcą radia
hosta. Kadry `80`–`88` 320×480 mają TEST / DEMO. Skrypt testuje ścieżkę z kreatora,
wyniki, szczegóły, cancel/retry/empty/unavailable, PL/EN/DE, pracę bez Wi-Fi,
niezależny polling przekaźnika i 10 cykli pamięci UI. Test przeglądarki używa
rzeczywistych zasobów panelu i parsera C. Te testy **nie dowodzą odbioru radia**.

Pełny build ES3C35P, ASan/UBSan, przeglądarka i dwa buildy diagnostyczne są
kontrolami nowego Draft PR. Fizyczny test jest otwarty; nie oznaczamy Stable.

## Odbiór na rzeczywistym ES3C35P — wymagany

1. Zachowaj wersję/SHA i wgraj aplikacyjny BIN z paczki przez dotychczasowe OTA
   lub osobne pliki USB z instrukcji, bez kasowania NVS. Sprawdź zachowanie Wi-Fi,
   zapisanych pomieszczeń, zdjęć i działającego ON/OFF Shelly 1PM Mini Gen3.
2. W tym samym pomieszczeniu umieść rzeczywiste Shelly z włączonym BLE i
   reklamowaniem usługi. Nie każda skonfigurowana wersja firmware stale reklamuje
   BLE. Sprawdź ustawienia urządzenia w oficjalnej aplikacji; nie resetuj go.
3. Otwórz Bluetooth LE i Szukaj. Zapisz czas zakończenia, liczbę rzeczywistych
   wyników, ich zgodność z posiadanymi modelami i brak resetu/utraty dotyku.
   Zrzut radio-fixture nie jest takim dowodem.
4. Jeśli masz BLU, **krótko** naciśnij jego przycisk podczas skanu. Nie przytrzymuj
   go i nie uruchamiaj factory reset. Sprawdź grupę BLU, rzeczywisty RSSI i opis
   Wykryto/nie sparowano. Gdy model/producent jest nieustalony, wynik ma pozostać
   nieznany. Brak egzemplarza BLU oznacza ograniczony, jawnie opisany odbiór.
5. Powtórz co najmniej 10 razy: zakończenie po 20 s, Anuluj w trakcie, powrót,
   szybkie Ponów. Podczas skanu sprawdź dotyk/przewijanie, aktualność GET mocy,
   lokalny HTTPS i ON/OFF z drugiego interfejsu lub po powrocie. Powtórz bez Wi-Fi.
6. Zapisz oczyszczone logi `dsr_ble: before/scanning/idle internal_free=...
   largest=... psram_free=...` z pierwszego i dziesiątego cyklu. Pierwszy wzrost
   kosztu inicjalizacji jest oczekiwany, dalszy stały spadek wymaga poprawki.
   Obserwuj płynność i brak restartów; nie traktuj hostowego heap jako PSRAM ESP32.
7. W sparowanym panelu powtórz start/cancel i porównaj wyniki z LCD. Sprawdź
   odmowę API bez sesji. Zrestartuj pilot, sprawdź stary binding i OTA.

Nie publikuj MAC/SSID/haseł w PR. Przed akceptacją potrzebny jest opis przynajmniej
jednej faktycznej reklamy na płytce. Nie mamy dostępu do sprzętu użytkownika;
ten dokument nie jest deklaracją zaliczonego testu fizycznego.

## Kolejne etapy — poza tym PR

1. BLE GATT/RPC provisioning zgodnego modelu, uwierzytelnianie/bonding zgodne
   z firmware 2.0+, jawny wybór sieci, weryfikacja powrotu do LAN i rollback.
2. BTHome: trwałe dodanie czujników, parser parametrów i ewentualne klucze
   uzyskane przez uprawnione parowanie. Sam scan nie jest integracją czujnika.
3. Sterowanie przez BLE RPC — dopiero po osobnym wdrożeniu i testach.

Źródła pierwotne: [IDF 5.5.4 coexistence](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-guides/coexist.html),
[NimBLE example 5.5.4](https://github.com/espressif/esp-idf/blob/v5.5.4/examples/bluetooth/nimble/blecent/main/main.c),
[Shelly BLU common](https://shelly-api-docs.shelly.cloud/docs-ble/common/),
[Shelly BLE security](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/BLE/),
[Shelly RPC UUID](https://kb.shelly.cloud/knowledge-base/kbsa-communicating-with-shelly-devices-via-bluetoo),
[BTHome v2](https://bthome.io/format/).
