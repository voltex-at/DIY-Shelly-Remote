# #23 — rzeczywiste endpointy Cover Gen2+

Pion obejmuje aktywny komponent Cover RPC generacji 2 i nowszych: wykrycie
możliwości, wybór typu/kanału, weryfikację, trwałe powiązanie, odczyt i świadome
Open/Close/Stop/GoToPosition na LCD oraz w sparowanym WWW. Nie zmienia profilu
urządzenia, nie kalibruje, nie obsługuje listew/slats ani rolet Gen1.
Źródło protokołu: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Cover/.

## Tożsamość i trwałość

Klucz endpointu to `(device_id, generation, endpoint_kind, channel)`, gdzie
Switch=0, Cover=1. Cover nie udaje Switch: ma własny odczyt, intencję i właściwą
przestrzeń RPC. Nazwa/IP nie zastępują tożsamości. Wybór Cover w formularzu
ręcznym wymaga Gen2+; LAN/AP/BLE handoff oferuje tylko faktyczne kanały API.

Zachowano rozmiary i offsety zainstalowanego `dsr_device_t` (152 B, kind w dawnym
paddingu137, power144), rekord v1 256 B i blob rejestru4104 B, z assertions.
Frozen legacy structs kopiują jawnie pola i ustawiają Switch0; losowy stary
padding nie jest typem. Rejestr pod tym samym autorytatywnym kluczem przechodzi
leniwo v2→v3 podczas kolejnej mutacji, bez zmiany partycji/mount/erase.
Stary firmware rozpoznający tylko v2 odrzuci v3, bez fallbacku do legacy.
Nieznana/uszkodzona wersja/typ pozostają zachowane i nieoperacyjne.

Workspace schema1 i jego hash/klucze pozostają bez zmian; istniejące `kind:cover`
ma teraz rzeczywisty transport. Obowiązuje P0 BIND: weryfikacja wyłącznie RAM →
dokładny trwały ACK workspace → jeden pełny prywatny zapis z finalnym layout ID
→ trwały readback i ponowna kontrola mapowania przed aktywacją. Po błędzie
wymagane jawne dodanie ponownie. V2/v3 ID0 bez prawdziwego legacy origin nie
staje się aktywny przez dopasowanie hosta ani fallback.

## Polecenia i odczyt

Każdy ruch sprawdza `/shelly` przed auth, a następnie odczytuje świeży
`Cover.GetStatus` właściwego kanału. Błąd, kalibracja i niepoprawny odczyt blokują
ruch. GoToPosition wymaga również `pos_control=true` i znanej pozycji0–100;
nieznana pozycja nie jest zerem. Nieznane błędy urządzenia blokują ruch.
Open/Close bez kalibracji pozostają dostępne zgodnie z API.

POST wysyła tylko intencję; wyłącznie udokumentowany ACK `null` pozwala przejść
do odczytu potwierdzającego. ACK nie oznacza zakończenia ruchu. LCD/WWW pokazują
rzeczywisty readback, również gdy STOP nadal odczytuje `calibrating`. Timeout,
błąd lub utracona odpowiedź nie powtarza POST. Nie ma optymistycznej pozycji.

LCD ma pionowy podgląd celu: PRESSING nie wysyła, RELEASED wysyła jedno polecenie.
Anulowanie gestu, utrata aktualności/połączenia lub zmiana celu anuluje podgląd.
Wskaźnik i duża pozycja pozostają ostatnim potwierdzonym odczytem. WWW analogicznie
oddziela potwierdzoną pozycję od podglądu celu i zachowuje DOM przy zmianie języka.
Utrata freshness/online/can_position, pending, zmiana hosta/targetu lub epoki
kasuje gest. Późniejszy change nie odtwarza go po odblokowaniu; wymagany jest
nowy input. IDs operacji porównywane są jako uint32 z uwzględnieniem zawinięcia.

## Priorytetowy STOP i opóźnienie

STOP może być przyjęty podczas oczekiwania na ruch i przy nieznanej pozycji lub
kalibracji; nadal obowiązują identity, auth i dokładne mapowanie układu. Korzysta
z jednego ograniczonego mailboxu z własnym ID/lease i końcowym snapshotem bez
sekretów. Nie korzysta z ID, event queue ani semafora dostępności zewnętrznego
zadania. Nie zastępuje nieodebranego wyniku; drugi endpoint może dostać BUSY.
Sekrety z kopii żądania są wymazywane przed udostępnieniem wyniku.

To nadal jeden worker. STOP może wejść między zwykłymi nogami HTTP i przy
50 ms oczekiwaniu kolejki, lecz nie przerywa trwającego wywołania SDK. Żądanie
HTTP ma limit3000 ms plus obsługę SDK/auth; inne operacje radiowe/OTA także mogą
opóźnić wykonanie. Nie ma gwarancji czasu rzeczywistego ani atomowości ruch/STOP.
Wynik STOP jest stosowany po odebraniu zewnętrznego eventu, aby starszy odczyt
nie nadpisał jego końcowego stanu. Przyjęty, niewykonany ruch z kolejki ustępuje STOP.
Oddzielny secret-free registered slot oznacza starszy ruch jako zastąpiony już
przy przyjęciu STOP, także przed odebraniem joba z kolejki. Zwykły skorelowany
wynik BUSY nie pożycza ID, semafora ani lease STOP. Dopasowanie obejmuje fizyczny
typowany endpoint, host i layout ID; STOP urządzenia A nie kasuje ruchu B.

## WWW

Istniejące local-origin/boot-ready/parowanie obejmują oba endpointy:

- Stary `POST devices/output` zachowuje kontrakt Switch0.
- `POST devices/cover` wymaga dokładnie `{device_id,generation:2,endpoint_kind:1,
  channel,command}`; command to `open`, `close`, `stop` lub `position`. Tylko
  ostatni wymaga dodatkowego całkowitego `position:0..100`. API nie przyjmuje
  hosta/loginu; nieznane/duplikowane pola i inne typy są odrzucane.
- 202 i `{accepted:true,operation_id,boot_id}` oznaczają przyjęcie. GET runtime
  zawiera rzeczywisty typ, pozycję/target z flagami dostępności, stan, błędy oraz
  can_move/can_position/can_stop/stop_pending. Klient nie potwierdza starej operacji
  nową epoką restartu ani samą odpowiedzią202.
- Utracony ACK Cover pozostawia blokadę do świadomego przeładowania lub nowej
  epoki; żaden POST nie jest automatycznie odtwarzany. STOP ma oddzielną lokalną
  blokadę od ruchu. Przeładowanie samo nie wysyła polecenia.

Historia #22 rozpoznaje wyłącznie Switch0. Record/query LCD/WWW przekazują
rzeczywisty endpoint_kind: Cover1 jest jawnie nieobsługiwany, bez aliasu na
Switch0, bez zapisu próbek ani kasowania historii przekaźnika o tym samym ID
urządzenia i kanale. LCD/WWW pokazują to ograniczenie. Telemetry Cover zajmuje
pięć zarezerwowanych bajtów obok flag energy; wspólna struktura nadal ma32 B.

B1 przyjmuje również Cover-only Gen2+ po rzeczywistym setup/preflight. Nadal
wymaga obu zgód na LCD, otwartego setup bez auth/enhanced security, kompletnej
metody WiFi i zasilanego sieciowo urządzenia. Po Wi-Fi.SetConfig nie powtarza
zapisu; domowy station/SSID/identity readback i odczyt kanałów poprzedzają ten
sam typowany BIND. Reklama BLE sama nie uwierzytelnia urządzenia.

## Dowody i odbiór

Testy hostowe wykonują rzeczywisty HTTP/parser, NVS registry, fleet, worker,
workspace, runtime API i LVGL; SDK/NVS/radio oraz UI transport mają jawne
providery MODEL. Obejmują zatrute legacy padding, typowane klucze, v3/future,
bez powtórzenia POST, przerwanie przy trzech granicach HTTP, własność obu ID i
lease STOP, OOM parsera/serializera, realny gest LCD i języki PL/EN/DE.
Pełny native UBSan, istniejący B1/AP/OTA/CRUD i niezależne procesy restartu PASS.

Target CI musi jeszcze sprawdzić rzeczywiste ELF/BSS/ramki SDK i obrazy WWW.
Budżety pozostają81920 B internal BSS/2048 B własnej ramki worker/network,
bez nowego tasku ani powiększenia dużego job/event. Nie jest to dowód runtime
heap/transitive peak ani odbiór fizyczny. Właściciel musi osobno sprawdzić dwa
urządzenia, kalibrację/nieznaną pozycję/błędy, rzeczywisty readback i opóźnienie
STOP przy wolnym lub offline innym urządzeniu. Nie wykonano ruchu ani zapisu
na urządzeniu, erase, publikacji Stable lub merge do main.
