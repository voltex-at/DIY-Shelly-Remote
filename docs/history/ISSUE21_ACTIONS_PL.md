# Grupy, sceny i aktywność (#21)

Ten pion dodaje wspólny wykonawca LCD/WWW, grupy Switch ON/OFF, uruchamianie istniejących scen oraz ograniczony dziennik RAM. Nie wykonuje poleceń podczas otwierania widoku ani przy odświeżaniu strony. Wyniki są osobne dla każdego kroku; operacja może zakończyć się częściowo. Nie ma obietnicy atomowości ani automatycznego ponawiania niepewnych poleceń.

## Konfiguracja i zachowanie danych

Schema2 wymaga `groups` (maksymalnie8 grup, każda do16 unikalnych ID urządzeń Switch). Sceny zachowują limit8×8 jawnych akcji ON/OFF/OPEN/CLOSE/STOP. Typ urządzenia i rzeczywista zdolność endpointu są sprawdzane przed wykonaniem; Cover nie może wejść do grupy Switch. Kafelek sceny ma osobne RUN i edycję, a kafelek grupy ON/OFF. Usunięcie używanego urządzenia/grupy jest blokowane przez referencje.

Osobny, zamrożony walidator schema1 sprawdza znane pola i odrzuca nieznane. Dopiero poprawny dokument1 jest podnoszony w RAM do2, bez zapisu przy starcie ani kasowania poprzedniego checkpointu. Pierwszy jawny Save używa dotychczasowego CAS, revision+1 i dokładnego potwierdzenia trwałego odczytu. Po potwierdzonym zapisie publikacja revision nie wymaga kolejnej alokacji JSON. Mieszane checkpointy1/2 są czytane; przyszła schema3 i uszkodzony/niedostępny magazyn pozostają odmową z zachowaniem danych. Domyślna schema2 powstaje wyłącznie dla faktycznie pustego magazynu. Stary firmware, który rozumie tylko1, zachowa przyszłe dane zamiast ich naprawiać.

## Przyjęcie i wykonanie

Jeden aktor RAM ma do16 kroków i korzysta z istniejącego pojedynczego workera. Przy przyjęciu zamraża ID/gen/kind/channel/host/layout, nigdy hasła ani tokeny. Każda próba I/O i każdy etap zapisu ponownie sprawdzają bieżącą tożsamość, mapowanie, uprawnienia i termin180s. Znana blokada auth/identity/rate nie powoduje I/O; brak świeżego statusu dopuszcza jeden GET przed pojedynczą komendą i jej readbackiem. Zmiana hosta, ponowne powiązanie lub usunięcie kończy dany krok, bez przekierowania polecenia.

Publiczne przyjęcie wymaga bootID, revision, admission_generation i16 znaków nonce. Duplikat ostatniego nonce zwraca ten sam run także po utracie odpowiedzi lub edycji konfiguracji. Nowe wykonanie zużywa generation; stary formularz po innym run jest odrzucany bez komendy. Restart zmienia bootID i nie wznawia aktora. WWW po utracie POST pozostaje przy GET; nie wysyła nowego POST automatycznie. Wynik UNCERTAIN nie jest potwierdzeniem zmiany stanu. Osobny hook oznacza rzeczywistą próbę sterującego requestu tuż przed SDK perform, po przygotowaniu klienta/body (także mutujący GET Gen1). Po tej granicy również auth/identity/unsupported z readback lub guarda zachowują kod przyczyny, ale dają UNCERTAIN. Przed próbą znane odmowy pozostają FAILED. Ręczny wewnętrzny STOP zawiesza i przywraca oba hooki zewnętrzne; STOP aktora ma własny exact-job hook. Po niepewnym zapisie późniejsze kroki tego samego endpointu są anulowane, aby plan nie stał się ukrytym retry.

Anulowanie i termin blokują dalsze I/O, ale już przyjęty job zachowuje rezerwację i activity lease do dokładnego ACK. Ręczny pilny STOP ma pierwszeństwo i anuluje resztę planu. Osobny registered-job guard nie zmienia znaczenia bool urgent-hook: wewnętrzny ręczny STOP zawiesza tylko guard zewnętrznego aktora, następnie przywraca go na każdej ścieżce. STOP będący krokiem aktora ma własny guard. Tombstone wcześniejszych zarejestrowanych Cover MOVE zachowuje się niezależnie od przestrzeni jobID/WWWoperationID.

## Aktywność i pamięć

Dziennik128×32B jest wyłącznie w RAM. Zapisuje numeryczne ID, akcję, wynik, UTC lub0 oraz monotoniczny czas bieżącego startu. Nie zawiera IP, MAC, prywatnego deviceID, nazw ani sekretów. Źródło0 oznacza WWW,1 LCD;2 to obserwowany runtime, bez zgadywania pochodzenia ręcznej komendy z operationID. Stan/błędy są zapisywane przy zmianie, nie co poll. Obserwacja dostępności lub błędu ma osobny kind2 i command0; nie jest ACK polecenia. Tylko zmiana wyjścia Switch może opisywać obserwowany ON/OFF. Utrata łączności Cover daje OFFLINE i oba renderery pokazują zmianę stanu/brak odpowiedzi, nigdy potwierdzone OFF. Clear czyści tylko pierścień RAM i zmienia epoch; restart zaczyna pusty dziennik. Czas jest jawnie UTC albo „brak UTC”; moduł nie uruchamia drugiego SNTP.

Kontekst aktora, cache i dziennik mają7644B w PSRAM (limit12288B), bez nowego taska i bez wzrostu job560B/event1192B. Dziennik ma4108B z metadanymi. Strona do32 wpisów jest alokowana na żądanie; nie trafia jako1036B lokalna zmienna na stos LVGL. Limity internal BSS81920B, LVGL pool131072B, UI task8192B oraz worker/network own-frame2048B pozostają bez podniesienia. `check_actions_target.py` wymaga prawdziwych `.su`/objectów i BOOT_MEMORY.json; raport nie udaje pomiaru runtime peak SDK/heap ani fizycznego testu radia.

## Sprawdzenie hosta i granice dowodu

`tools/test.py --ubsan-only`: pełny native PASS, w tym osobny model135, parser124196, schema2094 (OOM, każda granica przerwania pierwszego Save, mixed1/2/future3), rzeczywisty workspace56+68 OOM, rzeczywisty aktor/API255693+47/501 OOM i worker340. Fixture worker jest jawny; osobne testy używają rzeczywistego HTTP z fake SDK do blokad przed pierwszym GET, między GET/POST i przed readback. Istniejący worker4089 i platform2272 również przechodzą. Native suite zachowuje pozostałe OTA/BLE/AP/history/restart/auth regresje.

Rzeczywisty LVGL:4682 kontrole/167 ekranów, dodatkowo boot14/23/18/23 oraz Cover89/6. Sprawdzono grupę offline, A auth FAILED przy B SUCCESS, osobny RUN sceny z karty, cancel, UTC/clear i rzeczywiste PL/EN/DE.40 cykli grupy/sceny/aktywność utrzymuje heap90848→90848. Radio, worker i trwały magazyn w tym teście są jawnie modelowane, bez zapisów do urządzenia.

NodeVM wysłanych assetów:38 testów klienta aktora oraz24 niezależne kontrole wyścigów odpowiedzi (stary GET tej samej generation, poprzedni boot, przerwany preflight przy request/json). Każdy odczyt ma własny numer świeżości, a run zachowuje właściciela widoku przez await/catch/finally przed POST; stare finally nie zwalnia nowego sending. Testy klienta obejmują, w tym utrata POST i GET-only, boot/generation/nonce, dirty editor, tłumaczenia i redagowany dziennik. Nowy test Chromium używa wysłanych assetów z modelowanym HTTP; lokalnie brak Chromium, wykonanie pozostaje bramką CI. Test audytora ma1 poprawny i6 odrzucanych syntetycznych przypadków; nie jest dowodem targetu. Target IDF, pełny browser oraz rzeczywiste2 Shelly i restart pozostają osobnymi bramkami integracji/odbioru. Ten checkpoint nie zmienia PROJECT_VER, nie publikuje firmware i nie zapisuje Flash urządzenia.

### Wspólny odczyt konfiguracji i statusu WWW

Opóźnione GET/body nie mogą nadpisać późniejszej edycji, Save, wyniku BIND,
nowego Reload ani widoku. Odczyt zachowuje właściciela dokumentu, rewizji,
editSerial i DOM; SaveBEGIN oraz showConfig zmieniają generation także przy
Save→błąd bez zmiany rewizji. Każdy await i finally sprawdza właściciela.
Reload podczas trwającego Save nie rozpoczyna GET. Stary status nie zgłasza
konfliktu ani offline po nowszym sukcesie. Świeży
status z inną, także niższą rewizją nadal chroni szkic, a jawny Reload może
porzucić stary szkic, jeśli podczas jego oczekiwania nie było nowych zmian.
`tests/test_panel_config_reads.cjs` wykonuje wysyłane funkcje: 21 przypadków,
81 kontroli; stare źródło daje niezależne RED. Save A przy późniejszym szkicu B
nadal wymaga exact durable ACK A i zachowuje B. DOM/HTTP providers są MODEL;
przeglądarka ma osobną bramkę CI. Ta ochrona obejmuje też PR61/#32 i następców.
