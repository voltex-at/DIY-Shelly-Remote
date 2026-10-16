# Bezpieczna rezerwacja restartu — backend0.7.19 / LCD0.7.20, #24/#6

Ten etap dodaje wspólny backend i zastępuje bezpośredni restart z istniejącego
uwierzytelnionego Web API. Follow-up0.7.20 dodaje potwierdzenie na LCD opisane
poniżej; nie zmienia kontraktu backendu ani autoryzacji WWW.
Nie dodano resetu ustawień, kasowania, formatowania, zmiany partycji ani OTA
instalatora. Testy nie wywołują restartu ani zapisu na fizycznym urządzeniu.

## Admission zamiast snapshotu busy

`dsr_activity_begin/end` obejmuje czas życia zaakceptowanej operacji aplikacji.
Wspólne 32-bitowe słowo atomowe przechowuje licznik oraz closing/dispatched/
fault. Restart rezerwuje wyłącznie stan0; nowa operacja zwiększa TEN SAM licznik
wyłącznie bez flag closing. Dwie operacje nie mogą jednocześnie wygrać tego
wyścigu. Nie trzymamy mutexu ani sekcji krytycznej aplikacji podczas I/O.
Wywołania muszą być poprawnie zbilansowane; underflow zatrzaskuje fault,
nie zeruje cudzych lease ani nie otwiera restartu. Brak publicznego resetu.

W tym profilu IDF może kierować C11 atomics przez `__atomic_*_4` z obejściem
PSRAM. Nie zakładamy `ATOMIC_INT_LOCK_FREE==2`, wait-free ani twardego latency
bound. IDF dla internal RAM deleguje do S32C1I, a dla EXTRAM używa krótkiej
sekcji krytycznej. Audit realnego ELF sprawdza własny stan w internal BSS,
rozmiar4 B i wąskie zależności atomic32. Nie zmieniamy SDK workaround/config.

| Operacja | Początek ochrony | Koniec ochrony |
| --- | --- | --- |
| Workspace Save | Przed skutecznym queue admission | Durable save/readback oraz publikacja statusu/revision |
| Workspace bind | Przed bind_waiting/bind_busy | Wynik powiązania po admission dalszego Save |
| Każdy job workera | Przed xQueueSend | Odebranie eventu przez konsumenta |
| Przejście event → UI follow-up | Na początku rzeczywistego poll | Po migracji BIND/queued SET/pozostałych admission |
| Settings/connection NVS | Przed nvs_open do zapisu | Po commit/error/close |
| Pairing token | Przed walidacją i write | Po NVS commit i RAM publish/error |
| Photo save | Przed filesystem access do zapisu | Po fsync/fclose i cache publish/error |
| Web OTA endpoint | Przed receiverem | Po receiverze **i** set_boot_partition/error |
| Receiver bezpośredni | Przed alokacją/write | Po end/abort/cleanup |
| OTA boot health | Przed mark_app_valid_cancel_rollback | Po wyniku zapisu otadata |

Zagnieżdżone lease są zamierzone. Przede wszystkim worker event jest chroniony
do receive, a zewnętrzny lease rzeczywistego LVGL poll nakłada się na jego
zwolnienie i przyjęcie kolejnego zapisu. Bez tego restart mógłby wejść między
connection NVS i migrację workspace, mimo pustej kolejki. Każdy przyszły
konsument workera z persistent follow-up musi zachować ten kontrakt.

Startup-only SPIFFS provisioning/checkpoint refresh/GC, TLS identity i NVS
initialization są przed boot-ready; restart przed boot-ready jest niedostępny.
Nie zmieniono krytycznej ścieżki mount/format. Wi-Fi używa WIFI_STORAGE_RAM,
BLE pozostaje observer-only. Pure native OTA validation/discard nie zapisuje.
To bramka aplikacyjnych operacji, nie zapewnienie braku wszystkich wewnętrznych
zapisów sterowników SDK, odporności na odłączenie zasilania ani hardware proof.

## Backend i istniejący Web restart

`dsr_restart_start()` wywoływane raz przed UI tworzy dedykowane zadanie.
Niepowodzenie jest niekrytyczne: funkcja pozostaje unavailable, bez auto-retry.
`dsr_restart_request()` wymaga boot-ready i gotowego dispatchera, nie czeka na
writerów. Wynik: ACCEPTED, BUSY lub UNAVAILABLE. BUSY nie zamyka admission.
Po rezerwacji wysyłana jest notyfikacja; jej failure cofa tylko własne PENDING.

Zadanie priority1, configured stack4096 B, czeka500ms poza LVGL/Web taskiem,
ponownie sprawdza boot readiness i atomowo przejmuje dispatch dokładnie raz.
Nie trzyma panel/workspace/LVGL mutexów przy `esp_restart()`. Nie używa
shutdown hook ani współdzielonego timer daemona. Boot loss odwołuje rezerwację;
po terminalnym dispatch bramka nigdy nie otwiera się w tym procesie. Powracający
hostowy stub nie powoduje ponowienia. Stos4096 to konserwatywna konfiguracja,
nie pomiar kompletnego SDK/shutdown-handler stack peak; TCB/allocator dodatkowo.

Istniejący POST `/api/v1/restart` nadal wymaga tego samego local-only/same-origin,
boot-ready i pairing authorization. Handler zwraca200/restarting przy ACCEPTED,
409 przy BUSY,503 przy UNAVAILABLE, bez opóźnienia/restartu w HTTP callbacku.
ACCEPTED nie gwarantuje dostarczenia odpowiedzi HTTP ani renderu LCD. Błąd
odpowiedzi po przyjęciu nie cofa zaakceptowanego restartu; nie powtarzamy go.
Samo wygenerowanie kodu parowania/TTL OTA nie jest trwającym zapisem. Rezerwacja
restartu odmawia nowym write admission; OTA consent pozostaje jednorazowy.

Celowa zmiana source guard: `local_request` i `authorized` są bajtowo niezmienione
wobec2a3e7dd. Cały `api` nadal ma stary SHA po normalizacji **wyłącznie dokładnej
jednej linii restartu** do poprzedniego wariantu. Nie usunięto ani nie poluzowano
pozostałych guardów. To dowód source-invariance, nie runtime auth acceptance.

## Dowody i pozostały odbiór

Niezależne hostowe testy rzeczywistych funkcji, genuine atomics i pthread races;
scheduler/NVS/HTTP/Flash/shutdown są jawnie MODEL. Testy izolują scenariusze
procesami, nie resetują produkcyjnego latcha. Fault cleanup, pending event i
follow-up, write-vs-restart i restart-vs-restart, utrata boot-ready, notify/task
failure, underflow, one-shot dispatch, NVS/pairing/OTA i blokada nowego writera.
Root wykonał pełne native UBSan i LVGL2334/96, w tym próbę rezerwacji w realnym
receive→follow-up gap; worker provider w UI teście jest jawnie fixture.
Pełne ASan/UBSan/LSan oraz realne obj/.su/ELF/footprint w CI są osobną bramką.
Lokalne ograniczenie LSan/proc nie jest PASS. Exact-source proof w opisie PR.
Final actual-writer UBSan: NVS935, worker3534, workspace123, photo/cache101;
queue/provider faults i odmowa po przyjętym restarcie PASS. Testowane rzeczywiste
funkcje, bez fizycznego filesystem/NVS/Flash. Root przeczytał harnessy i powtarza
pełne native. Oba niezależne code reviews bez blockerów.

Nie wykonano restartu sprzętu, nie zaliczono retencji/health/rollback ani P0.
Restart nie potwierdza zdrowia nowej aplikacji ani nie omija rollback IDF.
Stan pending-verify nadal podlega istniejącemu boot-health/rollback kontraktowi.

## LCD potwierdzenie (follow-up0.7.20)

Ustawienia → System → Restart pilota otwiera osobny ekran. Samo otwarcie,
Anuluj oraz Wstecz nie wołają backendu. Dopiero Uruchom ponownie wywołuje
`dsr_restart_request()`. BUSY i UNAVAILABLE pokazują jawny komunikat PL/EN/DE,
pozostawiając możliwość ponownego świadomego potwierdzenia. Nie ma busy snapshotu
zamiast atomic admission ani automatycznego retry. LCD nie wymaga mount SPIFFS.

ACCEPTED zapisuje procesowy latch i wyłącza confirm/cancel. Ukrycie ekranu
zeruje wszystkie nowe pointery LVGL, ale nie latch; po wyjściu i ponownym
otwarciu nie można zgłosić kolejnego restartu. Programmatyczne ponowne eventy
też odmawiają. Wstecz/Home nadal pozwalają opuścić ekran, nie cofają requestu.
Brak publicznego cancel po akceptacji; późniejsza utrata boot-ready może
odwołać backend dispatch bez ponawiania. UI uczciwie mówi „przyjęto żądanie”,
nie obiecuje czasu restartu ani wyrenderowania klatki na fizycznym LCD.

Nie wywołuje się delay/esp_restart/storage z LVGL. Ostrzeżenie rozróżnia
zachowane dane od niezapisanych zmian; restart nie jest resetem fabrycznym.
Przyciski48px, przewijalny kontener318px, napisy PL/EN/DE. Test UI używa jawnego
HOST modelu wyników requestu; prawdziwe admission/dispatcher testuje etap0.7.19.
Niezależne LVGL UBSan2838/102 rendery PASS: wszystkie języki i nieprzecinanie
label/buttonów, zero calls przed confirm, retry po odmowach, sticky accepted
przez programmatic event/Back/Home/reopen.40cykli heap106712→106712. Stare
dirty/pending/CAS appearance testy zachowane. Niezależne code/lifecycle/render
reviews bez blockerów; root powtarza testy. Exact target/CI/artifact osobno.
Fizyczny odbiór wymaga osobno potwierdzić odmowę podczas zapisu/OTA, pojedynczy
restart w idle oraz zachowanie Wi-Fi/pokoi/zdjęć. Tego jeszcze nie wykonano.

P0 SPIFFS nadal wymaga osobnego hardware testu;
żadnego kolejnego erase/nonempty-format bez odrębnej zgody właściciela.

Źródła pierwotne, IDF5.5.4 pin735507283d5b2f9fb363a1901172dbd9e847945d:

- [esp_restart / shutdown](https://github.com/espressif/esp-idf/blob/735507283d5b2f9fb363a1901172dbd9e847945d/components/esp_system/esp_system.c)
- [FreeRTOS task API](https://github.com/espressif/esp-idf/blob/735507283d5b2f9fb363a1901172dbd9e847945d/components/freertos/FreeRTOS-Kernel/include/freertos/task.h)
- [Atomic32 / PSRAM contract](https://github.com/espressif/esp-idf/blob/735507283d5b2f9fb363a1901172dbd9e847945d/components/newlib/priv_include/esp_stdatomic.h)
- [Newlib Kconfig](https://github.com/espressif/esp-idf/blob/735507283d5b2f9fb363a1901172dbd9e847945d/components/newlib/Kconfig)
- [Owner #6 point7](https://github.com/voltex-at/DIY-Shelly-Remote/issues/6#issuecomment-6206285114)
