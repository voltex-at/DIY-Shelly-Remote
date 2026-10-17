# Globalne Ustawienia pilota — etap #6

Wersja 0.7.10-global-settings-dev kontynuuje istniejący projekt na PR #36.
Nie zastępuje obrazu 0.7.7 używanego do odbioru SPIFFS. Brak zgody na kolejny
erase, main albo Stable. Ta wersja nie stanowi ukończenia #6/#24/#25.

Dolne Ustawienia otwierają przewijaną stronę z sześcioma sekcjami: Sieć,
Wygląd i język, Ekran, Bateria, Panel WWW, System. Ołówek HOME pozostawia
wyłącznie pulpity i pomieszczenia; dotychczasowa konfiguracja lokalnych wpisów
urządzeń/scen jest w System. Nie jest to adapter ustawień RPC wybranego Shelly.

Wygląd używa istniejącego wspólnego JSON i CAS. Otwiera świeży snapshot; Save
zachowuje pozostałe kolekcje. Trwający zapis blokuje opuszczenie formularza;
niezapisane zmiany wymagają Zostań/Odrzuć. ACK wersji A może odświeżyć język
ekranu bez zgubienia późniejszych zmian B. Brak workspace powoduje jawny błąd
i powrót do Ustawień, nie zastępczą lub sformatowaną konfigurację.

Sieć pokazuje rzeczywisty status, SSID/IP i istniejące Wi-Fi/BLE. BLE jest
observer-only, AP provisioning pozostaje niewdrożony. Ekran i bateria używają
wcześniejszych rzeczywistych backendów; procent i ładowanie nadal nieznane.

Panel WWW pokazuje rzeczywisty IPv4, adres mDNS, gotowość HTTPS, kod stanu
SPIFFS i dostępny odcisk certyfikatu. Samo wejście nie generuje kodu. Osobny
fizyczny przycisk żąda 16-hex kodu/120 s/5 prób; brak gotowości lub locka
nie jest sukcesem. UI usuwa wygasły kod. Podczas ważności backend blokuje
podgląd ekranu. Brak nowego hasła, publikowania tokenu lub captive portalu.
Widok sesji/unieważnianie pozostają niewdrożone na LCD.

System odczytuje build/board i rzeczywiste liczniki wolnej pamięci, największy
blok internal, PSRAM oraz dostępny skonfigurowany rozmiar Flash i kod błędu.
To odczyt, nie nowa sonda lub zapis. Odśwież ponownie pobiera stan.

Natywne podpisane OTA #24 ma jawnie niedostępne akcje: backend pobierania,
manifestu i instalacji nie jest tu implementowany. Zaufany klucz wydawcy,
TLS/board/layout/channel/semver/size/SHA/podpis, strumieniowy transfer,
bezpieczne anulowanie, potwierdzona instalacja, zdrowy start/rollback i
zatwierdzone podpisane wydanie pozostają wymagane. Istniejące lokalne OTA
Web Panelu ma odrębną zgodę: prawdziwy bool accepted pod trylockiem, 120 s;
UI nie potwierdza odmowy ani nie pokazuje zgody po wygaśnięciu. Restart z LCD,
eksport/import i factory reset nadal jawnie niewdrożone; nie dodano atrapy.

## Weryfikacja

Rzeczywiste C/LVGL, nie HTML-mock: globalne kliknięcia i przewijanie, PL/DE,
offline/workspace niedostępny, długość SSID, 40 cykli bez ubytku heap,
CAS/kolekcje, pending i dirty Zostań/Odrzuć, A/B ACK oraz konflikt Web,
odmowa/wygaśnięcie pairing i lokalnej zgody OTA. Nowe testy wykonują produkcyjny
algorytm zgody OTA (22 kontrole) i snapshot pamięci (30 kontroli) z SDK shims.
Pełne target CI jest osobną bramką dla końcowego SHA.

Po odbiorze SPIFFS: zweryfikować na płytce pełną mapę dotyku i scroll,
przyciski co najmniej 44 px, zapis języka/motywu/akcentu po restarcie i sync
Web, zachowanie pozostałych kolekcji, rzeczywiste IPv4/fingerprint oraz pairing
telefonu, wygaśnięcie kodu i zgody, brak sekretów w preview oraz brak regresji
LAN/ON-OFF/BLE/jasności/wake. Host i CI nie są fizycznym odbiorem.
