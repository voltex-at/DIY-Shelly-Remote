# #20: niezależne bindingi Shelly — 0.7.23 DEV

Kod rozszerza istniejący działający workflow switch/relay. Nie zamyka #20 bez
odbioru na ES3C35P z co najmniej dwoma fizycznymi Shelly. Testy hostowe używają
prawdziwego kodu C/LVGL; sieć, NVS, RTOS i urządzenia są jawnie providerami testów.
Nie wykonano operacji na sprzęcie, erase, merge ani publikacji Stable.

## Tożsamość, zapis i migracja

- Rejestr ma maksymalnie 16 endpointów `(device_id, generation, channel)`.
  Nazwa i IP nie są tożsamością. Dwa kanały tego samego Shelly mają osobny stan
  i intencję; zweryfikowana zmiana DHCP/loginu aktualizuje ich wspólny locator/auth.
- Nowy atomowy blob `dsr_bindings/registry`, schema 2, przechowuje endpointy,
  prywatne dane logowania i prywatny `layout_id`. Poprzedni schema-1
  `dsr_device/connection` pozostaje niezmieniony. Odczyt legacy jest leniwy:
  pełny rejestr powstaje przy następnym BIND lub publikacji mapowania.
- Nieprawidłowy rozmiar/schema/licznik/duplikat tożsamości lub prywatnego ID
  zatrzymuje zapis. Nie ma erase, formatowania ani fallbacku z uszkodzonego
  istniejącego rejestru do legacy. Poprawny rejestr z licznikiem 0 jest
  autorytatywny i nie przywraca dawnego pojedynczego urządzenia.
- Każdy zapis rejestru wymaga commit, ponownego otwarcia i porównania całego
  odczytanego blobu. Błąd commit/readback oznacza wynik niepotwierdzony;
  NVS mogło już przyjąć nowy blob. Nie obiecujemy rollbacku zapisu przy błędzie.
  Ponowiony UNBIND potwierdza odczytem także brak już usuniętego endpointu;
  nie zapisuje ponownie rejestru ani nie narusza pozostałych wpisów.
- Workspace zachowuje dotychczasowy schema 1 i numeric stable IDs. Nie dodajemy
  pól do publicznego schematu. Sekrety i identity transportu pozostają poza nim.
  `layout_id=0` oznacza binding oczekujący na publikację; niezerowy wiąże istniejący
  obiekt. Migrujemy zbiorczo jednym CAS z aktualnego snapshotu, zachowując pokoje,
  kafle, sceny i ID. Konflikt CAS ponawia tylko scalenie układu, nigdy SET/BIND.
- Sukces dodawania następuje po weryfikacji API, zapisie rejestru, dokładnym ACK
  workspace i trwałym ACK prywatnego mapowania. Przez cały ten etap działa lease
  blokujący restart. Błąd workspace nie usuwa innych bindingów; rejestr pozostaje
  możliwy do odzyskania bez ponownego logowania/nowego sprawdzania urządzenia.
- Usunięcie opublikowanego numeric ID z poprawnego workspace kolejkuje oddzielny
  lokalny UNBIND wyłącznie tego endpointu. Usunięty opublikowany ID nie jest
  odtwarzany przez zaległą migrację DHCP/BIND po konflikcie CAS; wynik
  `DSR_CONFLICT` wymaga wczytania aktualnego stanu i ponownego dodawania.
  Brak snapshotu/uszkodzony dokument nie
  oznacza usunięcia. Zmiana host/kanał istniejącego ID wyłącza jego routing aż do
  świadomego ponownego BIND; nie kierujemy SET na nowy adres wpisany w edytorze.
- Firmware sprzed tej zmiany nadal zna wyłącznie dawny pojedynczy rekord. Przy
  downgrade może go ponownie użyć, także po UNBIND w nowej wersji. Zachowanie
  legacy zapewnia odwracalność migracji, nie unieważnienie sekretów w starszym
  firmware. NVS pozostaje niezaszyfrowane; nie zmieniono układu partycji ani kluczy.

## Kolejka, stan i granice opóźnienia

Każdy endpoint ma własne online/error, last confirmed sample, auth block,
backoff, oczekującą intencję i serial operacji. Jedna intencja użytkownika na
endpoint jest przyjmowana również podczas I/O innego. Duplikat dostaje jawne
BUSY; intencje trafiają FIFO przed polling. Przy odmowie samej lokalnej kolejki
intencja czeka na kolejne przyjęcie. Po wykonanym, nieudanym SET nie ma replay.
Identity `/shelly` jest sprawdzane przed auth/SET, po udanym SET obowiązuje
GetStatus/readback właściwego kanału. Zdarzenie musi pasować do serialu,
tożsamości, generacji, kanału i locatora; obce odpowiedzi nie nadpisują bindingu.

Transport celowo pozostaje jednym ograniczonym workerem i jednym żądaniem I/O.
To nie są 16 równoległych połączeń. LVGL nie wykonuje HTTP, pozostała karta
pozostaje aktywna i przyjmuje własne polecenie. Polecenie czeka na zakończenie
aktualnej transakcji. Timeout SDK to 3000 ms na żądanie; refresh ma identity +
status, SET ma identity + SET + readback. Obejmuje to dodatkowy czas obsługi
challenge Digest/SDK, więc 3000 ms nie jest gwarancją całkowitego czasu SET.
Offline na pierwszym identity zwykle kosztuje jeden timeout. Odbiór sprzętowy
musi zmierzyć opóźnienia przy dwóch urządzeniach i Wi-Fi/BLE coexistence.

Polling jest round-robin, normalnie po 5 s od zakończenia odczytu endpointu;
błędy sieci dają 10/20/40/80 s backoff tylko tego endpointu. Auth, rate limit i
identity mismatch zatrzymują jego polling do świadomego odświeżenia/BIND.
Inne urządzenia działają dalej. Po 15 s bez potwierdzonego odczytu LCD/WWW
pokazują stan nieaktualny/offline i nie wysyłają starej intencji SET. Przy 16
wolnych endpointach odczyty mogą przekroczyć ten próg; nie obiecujemy stałych
5 s całej floty ani nie pokazujemy nieaktualnych danych jako bieżących.

## LCD i kontrakt WWW

LAN pokazuje `Już dodane` po stabilnej identity. Wybór nadal pozwala dodać inny
kanał lub zweryfikować zmianę DHCP. Drugi Shelly nie zastępuje pierwszego.
Ręczne dodanie zaczyna pusty formularz; nie kopiuje hasła pierwszego urządzenia.
Każda karta i szczegóły korzystają z własnego stanu/W/V/A; brakujące pomiary
pozostają niedostępne. Gen1 dotychczasowa ścieżka relay nie dopisuje fikcyjnej mocy.

Za istniejącymi bramkami local-origin, boot-ready i parowania:

- `GET /api/v1/devices/runtime` → `{boot_id, devices:[...]}`. Pola endpointu:
  `device_id,generation,channel,host,name,online,output,has_switch,has_power,power_w,
  has_voltage,voltage_v,has_current,current_a,pending,error,has_sample_time,
  sample_age_ms,operation_id`. Żadnych credentials ani secret-bearing connection.
- `POST /api/v1/devices/output` przyjmuje dokładnie
  `{device_id,generation,channel,on}`; `on` musi być boolean. Brak host/index.
  202 oznacza tylko przyjęcie, odpowiedź `{accepted:true,operation_id,boot_id}`.
  400: zły/niejednoznaczny JSON; 404: nieznany endpoint; 409: zajęty/zablokowany;
  503: offline/stale. GET/readback jest dowodem zakończenia, nie sam HTTP 202.
- Snapshoty są publikowane atomowo z licznikiem, również 0. Mailboxy poleceń są
  po tuple, więc usunięcie/kompakcja A nie zmienia celu oczekującej komendy B.
  `boot_id` rozróżnia restart; klient nie może uznać poprzedniego SET za udany
  na podstawie nowej epoki. Polecenia LCD nie zerują obserwowalnego serialu WWW.

## Test odbiorczy właściciela

1. Bez erase wgraj obraz DEV aplikacji; zachowaj kopię obecnego NVS/dsrdata.
2. Sprawdź pierwszy dotychczasowy Shelly. Dodaj drugi przez `+` → LAN;
   pierwszy ma być `Już dodane`, oba mają pozostać na liście po restarcie.
3. Steruj A/B osobno, porównaj odczyty W/V/A ze wspieranym API; przełącz wyjście
   poza pilotem i sprawdź odczyt. LCD i HTTPS muszą pokazać własne wyniki.
4. Odłącz A, wyślij B polecenie podczas oczekiwania na A; zmierz czas readback B
   i płynność dotyku. Przy timeout SET brak automatycznego ponowienia.
5. Zły login/rate-limit jednego, drugi działa; zmiana DHCP zachowuje jego ID,
   drugi binding, kafle i pomieszczenia. Jeśli dostępne, dodaj drugi kanał A.
6. Usuń tylko A z układu po usunięciu jego referencji; po zakończeniu UNBIND i
   restarcie A nie wraca, B i pokoje pozostają. Błąd snapshotu nie usuwa urządzeń.
7. Równolegle BLE scan, sieć offline/reconnect, lokalny restart i ponowne wejścia
   w UI: brak resetów, stabilny heap. Dołącz wersję/SHA i zanonimizowany wynik;
   CI i obrazy hostowe nie zastępują tego testu.

Własne stałe storage: host ABI connection 288 B, fleet około 7.8 KiB; do 16
mailboxów/runtime i do 16 pozycji migracji. Brak nowego tasku i brak dużej tablicy
na stosie LVGL. Rejestr/readback/JSON są alokowane na heap i zwalniane z wymazaniem
sekretów. `.su`, rzeczywisty target BSS/ELF, SDK/TLS peak i fizyczny heap pozostają
oddzielnymi bramkami; wyniki target CI należy dopisać do końcowego PR.

Kontrola kompilacji zapisuje `FLEET.json`: rzeczywiste symbole obiektów i własne
ramki `.su`, globalny internal BSS <=81920 B (parent48952 B), fleet<=8192 B,
runtime/mailbox/migrations po<=4096 B. Wyjątek dotychczasowego formularza
object_choice<=2048 B wynika z istniejącego bufora opcji1024 B; nowe ścieżki
migracji mają limit1024 B. Nie jest to pomiar heap ani sumy stosów SDK.
WWW PL/EN/DE zachowuje formularze przy zmianie locale i lokalnie wygasza
sterowanie po15 s bez aktualnego odczytu, niezależnie od zawieszonej sieci.
Po utraconej odpowiedzi POST nie powtarza polecenia; czeka na nowy serial lub
świadome odświeżenie strony. Nowa epoka restartu nie potwierdza starej komendy.
