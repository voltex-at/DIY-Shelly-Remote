# Konfiguracja Shelly i dodawanie BLU z pilota

Stan dokumentacji producenta: **2026-10-18**. Implementacja: `0.7.55-dev`,
`work/p0-spiffs-ota-brand` (Draft #65). Część API poniżej opisuje szersze możliwości
producenta; zakres tej wersji jest określony tu i w RELEASE_NOTES. Odbiór fizyczny
parowania i zapisu pozostaje do wykonania.

## Zaimplementowany zakres 0.7.55

- `dsr_device_settings_model.c` i prywatny actor `dsr_device_settings.c`: formularze
  LAN Gen2/3/4, capability discovery, jedna zmiana, kontrola konfliktu i read-back
  także po utracie ACK. Osobny formularz Wi-Fi SSID+PSK. Sekrety write-only nie
  otrzymują fałszywego potwierdzenia z GetConfig.
- `dsr_blu.c`: współdzielony central NimBLE, trwałe bondy NVS SDK, PIN, odwołanie
  jednego powiązania, brak automatycznej eksmisji; generacje callbacków i cleanup.
  Dotychczasowy B1 dla otwartego setupu nadal odmawia SMP w swoim przepływie.
- `dsr_blu_model.c`: klasyczne modele 1/2/3/5/6/7/8; Motion, Door Window,
  Button i TRV mają walidowane charakterystyki GATT. TRV ma osobne flagi z RMW,
  podpisany offset 0.1 °C i limity temperatur. H&T zachowuje stale aktywny beacon.
- Rejestracja na bramce: idempotentne rozpoznanie adresu, AddDevice i AddSensor
  z odczytem po każdym możliwym zapisie, bez ponawiania mutacji po timeout.
  Dla TRV: osobne StartAssociations(target=8), tylko wybrana głowica w trybie
  parowania i weryfikacja jej adresu oraz powiązania BluTrv/BTHome.
- Ekrany LCD PL/EN/DE, maskowane sekrety, zatwierdzenie i blokada podglądu WWW.
  Zapis karty jest oddzielony od bondingu oraz rejestracji odbiornika.

Nadal poza tym DEV: chmurowe przypisanie do konta, nowe BLU/ZB, Zigbee/Matter
commissioning, harmonogramy, OTA/reset zewnętrznych urządzeń i BTHomeControl.
Nie podnoszono limitów BSS, stosów ani kolejki. NVS prototypu nie jest szyfrowane.

Cel właściciela: dodawanie i konfiguracja z ekranu urządzenia oraz kreatora
„Dodaj urządzenie”, bez obowiązkowego przechodzenia do konfiguratora WWW.
Zdjęcia referencyjne pokazują wybór sposobu dodania, listę wyników Bluetooth
i oczekiwanie na naciśnięcie przycisku. Są referencją zachowania i wyglądu;
same nie dokumentują protokołu używanego wewnętrznie przez aplikację.

## Co potwierdza publiczna dokumentacja

| Operacja | Mechanizm | Znaczenie dla pilota |
| --- | --- | --- |
| Ustawienia urządzenia Wi-Fi Gen2+/Pro | RPC komponentów, zwykle `GetConfig` / `SetConfig` | Rzeczywisty edytor ustawień Shelly jest możliwy |
| Ustawienia klasycznych czujników BLU | Charakterystyki GATT usługi Shelly i bonding BLE | Klient GATT/bonding dodany w 0.7.55 |
| Dodanie odbioru BLU na bramce LAN | Dynamiczne komponenty BTHome | Można tworzyć lokalne powiązanie z wybraną, zgodną bramką |
| Powiązanie BLU TRV z BLU Gateway Gen3 | `BLE.StartAssociations`, potem komponent `BluTrv` | Osobny przepływ od Motion/Door Window |
| Przypisanie BLU do Shelly Cloud | Mechanizm konta/chmury | Publiczne lokalne API nie daje podstaw do obietnicy odtworzenia całego procesu aplikacji |

Wniosek projektowy: pilot może stać się lokalnym konfiguratorem. Wykrycie
reklamy, zapis karty w pilocie, utworzenie komponentu na bramce i bonding
Bluetooth muszą mieć oddzielne, prawdziwe wyniki.

## Ustawienia urządzeń Wi-Fi

Poniższa tabela określa możliwości API, nie deklarację gotowego UI.
Zakres każdego formularza musi zależeć od konkretnego modelu, firmware,
profilu, komponentu oraz uprawnień. Metoda obecna w `Shelly.ListMethods`
potwierdza dostępność operacji na danym kanale, ale nie wszystkie jej pola.
Lista komponentów i bieżący `GetConfig` są dodatkowymi warunkami.
Źródło: [Shelly RPC i enumeracja komponentów][rpc].

| Ekran ustawień | Co można udostępnić | API / źródło |
| --- | --- | --- |
| Ogólne | Nazwa zapisana w Shelly, strefa czasowa, lokalizacja, SNTP; ECO jako opcja zależna od modelu | `Sys.GetConfig`, `Sys.SetConfig` [System][sys] |
| Przekaźnik / gniazdo | Stan po powrocie zasilania, automatyczne włączenie i wyłączenie z opóźnieniem, tryb wejścia; dostępne limity ochronne | `Switch.GetConfig`, `Switch.SetConfig` [Switch][switch] |
| Wejścia fizyczne | Typ wejścia, włączenie, odwrócenie logiki; tylko typy obsługiwane przez urządzenie | `Input.GetConfig`, `Input.SetConfig` [Input][input] |
| Rolety | Tryb wejść, stan po zasileniu, maksymalne czasy ruchu, kierunki i wykrywanie przeszkody, jeśli dostępne; kalibracja jako osobna czynność | `Cover.GetConfig`, `Cover.SetConfig`, `Cover.Calibrate` [Cover][cover] |
| Światło / dimmer | Timery, zachowanie po zasileniu, przejścia jasności, tryb nocny, ustawienia przycisku zależne od modelu | `Light.GetConfig`, `Light.SetConfig` [Light][light] |
| Sieć | Wi-Fi podstawowe i zapasowe, DHCP/statyczne IP, AP, dostępny range extender | `WiFi.GetConfig`, `WiFi.SetConfig`, `WiFi.Scan` [WiFi][wifi] |
| Bluetooth | Dostęp do RPC, uruchomienie okna parowania i zarządzanie bondami tam, gdzie firmware je udostępnia | `BLE.GetConfig`, `BLE.SetConfig`, metody pairing [BLE][ble] |
| Integracje | Połączenie Cloud, MQTT i jego parametry; osobne formularze | `Cloud.SetConfig` [Cloud][cloud], `MQTT.SetConfig` [MQTT][mqtt] |
| Harmonogramy / akcje urządzenia | Edycja zadań wykonywanych przez Shelly oraz webhooków; nie jest to obecny ręczny edytor scen pilota | `Schedule.*` [Schedule][schedule], `Webhook.*` [Webhook][webhook] |

Proponowane wejście: **Urządzenie → Ustawienia → Ustawienia Shelly**.
Obecne „Edytuj kartę” nadal oznacza nazwę/pokój/zdjęcie w pilocie.
Zmiana nazwy na urządzeniu wymaga osobnej, wyraźnie opisanej operacji;
nie zmienia przyjętej zasady pierwszeństwa lokalnej nazwy karty.

Pierwszy edytor LAN: nazwa urządzenia, timery i stan po zasileniu przekaźnika,
następnie wejścia. Ustawienia sieci, kalibracja silnika i parametry ochronne
wymagają osobnych przepływów z potwierdzeniem skutku. Gen1 pozostaje poza
zakresem zgodnie z decyzją właściciela.

## BLU: przycisk, parowanie i dostęp do ustawień

**Krótki przycisk do wykrycia** i **wejście w tryb parowania** to dwie różne
czynności. Dla klasycznych BLU producent opisuje przytrzymanie ponad 10 s,
do sygnalizacji parowania; okno kończy się po minucie bezczynności albo
po połączeniu. Urządzenie przechowuje do czterech sparowanych peerów;
kolejny może wyprzeć najmniej używany. Zapis konfiguracji wymaga bondingu.
Usługa klasycznych BLU ma UUID `de8a5aac-a99b-c315-0c80-60d4cbb51225`.
Źródło: [Common features][common].

Dla już szyfrowanego BLU potrzebny jest jego PIN. Klucz BTHome można
odczytać po uprawnionym sparowaniu. Jest odrębny od kluczy bondingu BLE.
Zmiana Passkey włącza szyfrowanie i generuje nowy klucz; według dokumentacji
powrót do transmisji jawnej wymaga resetu fabrycznego. Dlatego zwykłe
„Dodaj” ma zachować dotychczasowy PIN i klucz. Zmiana szyfrowania musi być
odrębną operacją z opisem wpływu na istniejące odbiorniki.
Źródło: [Encryption][encryption].

### Potwierdzone ustawienia klasycznych modeli

| Model | Ustawienia dostępne do wdrożenia | Źródło |
| --- | --- | --- |
| BLU Motion `SBMO-003Z` | Czułość 0/1/2; liczba impulsów 1–4; czas podtrzymania 30–600 s; LED; beacon. Podwójny przycisk uruchamia minutowy test ruchu | [Motion][motion] |
| BLU Door Window `SBDW-002C` | Beacon i minimalna zmiana kąta raportowana przez czujnik; odczyt otwarcia, kąta, lux i baterii | [Door Window][dw] |
| BLU Button1 `SBBT-002C` | Beacon, buzzer, „znajdź urządzenie”, szybkość wielokliku. Zdalny buzzer wymaga beacon | [Button][button] |
| BLU H&T `SBHT-003C` | Ustawienia wspólne/parowanie; brak odrębnych ustawień modelu w tej dokumentacji. Beacon jest stale aktywny | [H&T][ht] |
| BLU TRV | Blokada dziecięca, wyświetlacz, offset temperatury, boost/override, kalibracja i harmonogramy; m.in. przez `BluTrv.Call` na bramce Gen3 | [TRV][trv], [BLU Gateway Gen3][gw] |

Przydatne charakterystyki dla dwóch pierwszych adapterów:

| Ustawienie | UUID | Format z dokumentacji |
| --- | --- | --- |
| Motion: czułość | `21b5b57b-da8d-4ea4-baf8-7654a2214650` | 1 bajt, 0–2 |
| Motion: czas podtrzymania | `219a1ecc-2567-4378-9dbd-0c97d10630ad` | 2 bajty, 30–600 s |
| Motion: liczba impulsów | `22f36e64-e682-4fbc-8dd6-a87f6e7b7d92` | 1 bajt, 1–4 |
| Motion: LED | `24f52308-6cc6-4065-acf0-1d4574d9ba0f` | 1 bajt |
| Door Window: próg kąta | `86e7cc43-19f4-4f38-b5ad-1ae586237e2a` | 1 bajt; zakres wymaga sprawdzenia dla firmware |
| Beacon | `cb9e957e-952d-4761-a7e1-4416494a5bfa` | 1 bajt; tylko modele z przełączanym beacon |

Identyfikacja modelu jest konieczna: np. **Motion ZB `SBMO-103Z`** ma osobne
zasady parowania (4 naciśnięcia albo 10 s) i trójstanowy poziom LED, mimo
ponownego użycia niektórych UUID. Nie należy automatycznie stosować adaptera
klasycznego Motion. Źródło: [Motion ZB][motionzb].
Kolejność bajtów, dozwolone wartości i read-back wymagają testu konkretnego
adaptera; obecność charakterystyki nie wystarcza do zgadywania jej semantyki.

## Co oznacza „wybierz bramkę”

BLU rozgłasza pakiety. W lokalnym wariancie pilot konfiguruje wybrany odbiornik
Shelly; nie wpisuje do czujnika adresu IP bramki. Więcej niż jeden odbiornik
może odbierać reklamy. Zmiana bramki używanej przez kartę pilota nie powinna
usuwać powiązań innych odbiorników.

Publiczny proces BTHome: wykrycie, `BTHome.AddDevice`, rozpoznanie obiektów,
`BTHome.AddSensor`. Dokumentacja ogranicza komponenty do wybranych generacji
(Gen2 Pro, Gen3, Gen4 w trybie Matter), więc sam napis Bluetooth/Plus nie
kwalifikuje urządzenia jako zgodnej bramki. Od 1.6 nie tworzy się nowego
`BTHomeSensor` dla przycisku — używa się zdarzeń `BTHomeDevice`.
Źródło: [BTHome][bthome].

`BTHomeDevice.SetConfig` ustawia m.in. nazwę i klucz na odbiorniku. Nie jest
API czułości Motion. Klucza nie zwraca `GetConfig`; jego poprawność trzeba
potwierdzić odszyfrowanym, świeżym pomiarem. Każdy sensor ma własne
`obj_id`, `idx`, adres i identyfikator komponentu.
Źródła: [BTHomeDevice][bdevice], [BTHomeSensor][bsensor].

`BLE.StartAssociations` w dokumentacji API obsługuje aktualnie powiązania
TRV na BLU Gateway Gen3; nie jest ogólnym poleceniem parowania Motion.
`BluTrv.Call` jest udokumentowanym proxy RPC TRV, nie dowodem na uniwersalne
proxy GATT dla dowolnego BLU. Źródła: [BLE][ble], [Gateway Gen3][gw].

`BLE.CloudRelay` publicznie wystawia odczyt list i informacji; pozostałe
funkcje zarządzania są opisane jako wewnętrzne. `Cloud.SetConfig` na bramce
nie dodaje automatycznie BLU do konta użytkownika.
Źródła: [BLE CloudRelay][ble], [Cloud][cloud].

Osobna możliwość konfiguracji przycisków: zgodne Shelly udostępniają
`BTHomeControl.StartLearning` i `Create`/`Update`, aby mapować zdarzenia BLU
na wyjścia Switch, Cover albo Light. To powiązanie wykonywane przez Shelly,
przydatne również wtedy, gdy pilot śpi. Nie zastępuje ustawień czujnika.
Źródło: [BTHomeControl][bcontrol].

## Kreator LCD według referencji — projekt

1. **Sposób dodania:** Bluetooth, AP Wi-Fi albo LAN, w istniejącym kreatorze.
2. **Wybór urządzenia:** lista z wyszukiwaniem i filtrem. Wyraźny stan „już
   dodane do tego pilota”, odrębny od potwierdzonego istnienia na bramce.
   Nazwa i RSSI służą wyborowi, nie uwierzytelnianiu.
3. **Potwierdź na urządzeniu:** instrukcja dla rozpoznanego modelu. Do samego
   wykrycia krótki przycisk; do konfiguracji BLU właściwe okno parowania,
   potem bonding i PIN, jeśli wymagany. Widoczny czas i możliwość anulowania.
4. **Brama i pomiary:** wybór zgodnej bramki LAN, sprawdzenie jej tożsamości,
   uprawnień, istniejącego adresu BLU i dostępnej pojemności. Utworzenie
   brakujących komponentów, bez duplikowania już obecnych.
5. **Nazwa, pokój, zdjęcie:** lokalna karta pilota; osobna opcja nazwy Shelly.
6. **Wynik:** osobno wynik sparowania, odbioru przez bramkę i trwałego zapisu
   karty. Sukces odbioru dopiero po świeżym prawidłowym pomiarze. Brak próbki
   oznacza oczekiwanie/niepotwierdzony odbiór, a nie zakończoną konfigurację.

Na stronie BLU: **Sterowanie/pomiary → Ustawienia → Bramka / Ustawienia
Bluetooth / Informacje**. Przy braku bramki można sparować i skonfigurować
czujnik w zasięgu BLE, ale stałe odczyty przez LAN pozostają nieaktywne.
To propozycja zachowania do implementacji, a nie opis gotowego ekranu.

## Luka w aktualnym kodzie i kolejność prac

| Miejsce w repozytorium | Implementacja 0.7.55 |
| --- | --- |
| `dsr_ble.c`, `dsr_blu.c`, `dsr_ui_blu.c` | Jeden manager radia, skan i bonding/PIN, GATT i kreator |
| `sdkconfig.defaults` | SMP legacy+SC, NVS persist, 16 bondów, debug keys i auto-deletion wyłączone |
| `dsr_device_settings.c`, `dsr_http.c` | Kontrolowane RPC LAN, dynamiczne BTHome i osobna asocjacja TRV |
| `dsr_ui_device_settings.c` | Edytor rzeczywistych ustawień z odczytem i potwierdzeniem |
| `dsr_ui_mobile.c:device_settings` | Osobne wejścia do edycji karty i ustawień Shelly/BLU |

Rozpoczęty bonding i zapis wymagają właściciela operacji, generacji sesji,
jednej aktywnej operacji radia i cleanup także przy wyjściu z ekranu.
Obecne automatyczne zwalnianie BLE po bezczynności musi respektować nową sesję.
Znaczenie komunikatów „sparowano”, „dodano”, „zapisano” nie może być wspólne.

Od firmware Shelly 2.0 BLE RPC poza otwartym setupem wymaga parowania.
Nie wolno pomylić tego z czterema peerami klasycznego BLU; limit klientów
BLE RPC urządzeń Wi-Fi jest inny. Zmieniło się też zarządzanie skanowaniem:
starsze flagi `enable`/`observer.enable` nie są uniwersalnym sposobem
włączania bramki. Źródło: [BLE][ble].

Obecny prototyp opisuje NVS jako niezaszyfrowane (`dsr_platform.h`). Projekt
przechowywania bondów i kluczy musi jawnie uwzględnić ten stan; nie oznaczać
go jako szyfrowanego magazynu. PIN i klucze nie trafiają do modelu kart,
logów, eksportu konfiguracji ani odpowiedzi panelu. Formularz może pokazać
jedynie stan „klucz zapisany”.

## Weryfikacja DEV i odbiór sprzętowy

- Parsery i kontrakty modeli: długości GATT, wartości graniczne, nieznany
  model/UUID, klasyczny BLU vs ZB, błędny PIN i brak wymaganej charakterystyki.
- Bonding: anulowanie, timeout, restart, ponowne połączenie, usunięcie tylko
  wybranego bondu, pełny magazyn, spóźnione callbacki i rozłączenie po zapisie.
- BTHome: istniejący adres, brak uprawnień, pełna bramka, częściowe dodanie,
  utrata ACK, błędny klucz, stara próbka i brak nowego pomiaru. Dokumentacja
  `AddDevice` pokazuje różne pola wyniku (`key` w opisie, `added` w przykładzie);
  fixture kontraktu trzeba przypiąć do rzeczywistego firmware.
- Utrata odpowiedzi po zapisie oznacza stan niepewny: najpierw odczyt i
  porównanie z istniejącymi komponentami, bez automatycznego ponowienia
  `AddDevice`, parowania, zmiany PIN ani resetu.
- Anulowanie po częściowym sukcesie nie usuwa cudzych komponentów i nie
  deklaruje cofnięcia wykonanego zapisu. Wznowienie rozpoznaje już utworzone
  elementy; stan częściowy pozostaje widoczny także po restarcie pilota.
- Formularze LAN: obca tożsamość pod tym samym IP, zmiana konfiguracji
  równolegle z aplikacji, niespójny odczyt, auth, ponowne wyszukanie po zmianie
  Wi-Fi. `cfg_rev` jest kontrolą zmiany, nie obietnicą atomowego CAS na Shelly.
- LCD/WWW PL/EN/DE: klawiatura PIN, skan, czas, anulowanie, częściowy wynik,
  przewijanie przy 320×480; wszystkie zapisy przez te same kontrolowane operacje.
- Pełny build targetu i dotychczasowe kontrole pamięci: BSS ≤81920 B,
  stos UI 8192 B, bez podnoszenia limitów. Aktywny BLE, HTTPS, polling i dotyk
  sprawdzić razem na ES3C35P, w co najmniej 10 cyklach.
- Odbiór fizyczny: rzeczywisty Motion i Door Window, wersje firmware, ponowny
  odczyt ustawień po restarcie, zachowanie odbioru w dotychczasowej aplikacji
  i bramce. Test hostowy ani sam zielony build nie zamyka tego odbioru.

Wydanie 0.7.54 pozostaje niezmienione. Nowy kod 0.7.55 nie potwierdza jeszcze wykonania parowania na sprzęcie.

[rpc]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly/
[sys]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Sys/
[switch]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Switch/
[input]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Input/
[cover]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Cover/
[light]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Light/
[wifi]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/WiFi/
[ble]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/BLE/
[cloud]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Cloud/
[mqtt]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Mqtt/
[schedule]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Schedule/
[webhook]: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Webhook/
[common]: https://shelly-api-docs.shelly.cloud/docs-ble/common/
[encryption]: https://shelly-api-docs.shelly.cloud/docs-ble/encryption/
[motion]: https://shelly-api-docs.shelly.cloud/docs-ble/Devices/BLU/motion/
[dw]: https://shelly-api-docs.shelly.cloud/docs-ble/Devices/BLU/dw/
[button]: https://shelly-api-docs.shelly.cloud/docs-ble/Devices/BLU/button/
[ht]: https://shelly-api-docs.shelly.cloud/docs-ble/Devices/BLU/ht/
[trv]: https://shelly-api-docs.shelly.cloud/docs-ble/Devices/BLU_ZB/trv/
[motionzb]: https://shelly-api-docs.shelly.cloud/docs-ble/Devices/BLU_ZB/BluMotionZB/
[bthome]: https://shelly-api-docs.shelly.cloud/gen2/DynamicComponents/BTHome/
[bdevice]: https://shelly-api-docs.shelly.cloud/gen2/DynamicComponents/BTHome/BTHomeDevice/
[bsensor]: https://shelly-api-docs.shelly.cloud/gen2/DynamicComponents/BTHome/BTHomeSensor/
[bcontrol]: https://shelly-api-docs.shelly.cloud/gen2/DynamicComponents/BTHome/BTHomeControl/
[gw]: https://shelly-api-docs.shelly.cloud/gen2/Devices/Gen3/ShellyBluGwG3/
