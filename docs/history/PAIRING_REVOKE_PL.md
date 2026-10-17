# Unieważnienie wspólnego parowania WWW — istniejący PR50, #6 punkt5

Port istniejącego [PR50](https://github.com/voltex-at/DIY-Shelly-Remote/pull/50),
autor `voltex-at`, source `b1286945ef0551a934644ce2adffa2f2bdc71119` na aktualny
Cover/history/P0/native/BLE parent `faa623fca5f7bb7cf63808fca963ccc1b9dee667`.
Poniższe historyczne wyniki PR50 zachowano jako proweniencję; aktualne wyniki
portu są odrębne. Brak nowego PR, zmiany wersji albo operacji na sprzęcie.

To istniejące wymaganie z [uzgodnienia właściciela](https://github.com/voltex-at/DIY-Shelly-Remote/issues/6#issuecomment-6206285114),
nie rejestr sesji. Panel przechowuje jeden `dsr_panel/token_hash`.
Wszystkie przeglądarki z kopią tego samego cookie korzystają z jednego uprawnienia.
Nie znamy liczby połączeń, listy klientów ani ich aktywności. Snapshoty nie
zawierają sekretu, cookie, hasha ani kodu.

## Stan i granica unieważnienia

Read-only snapshot z trylock0 nie wykonuje NVS ani I/O. `paired` oznacza tylko
efektywny `has_token` w RAM bieżącego uruchomienia, nie dowód trwałej obecności
klucza czy aktywnych sesji. Startup NVS read może zawieść. Brak gotowości lub
lock contention zwraca false i zero output, nie fikcyjne „brak parowania”.

Fizyczne potwierdzenie przyjmuje job do istniejącego pojedynczego workera.
Nie wymaga Wi-Fi/Shelly bindingu; nie zapisuje NVS w LVGL. Activity obejmuje
queue→worker→event receive, backend dodatkowo nested lease do ostatniego NVS
close. Restart nie przechodzi w środku. Panel lock serializuje revoke z pair
i przyszłym authorized check. Worker może czekać na lock; LCD tylko trylock0.

Po uzyskaniu locka backend najpierw blokuje stare uprawnienie RAM, zeruje hash,
kod/okno/próby parowania i zgodę Web OTA. Potem usuwa tylko `dsr_panel/token_hash`,
sprawdza erase/commit/readback NOT_FOUND i zamyka uchwyt. NOT_FOUND erase jest
idempotentne. Nie kasuje namespace, Wi-Fi, Shelly, workspace, zdjęć czy TLS
identity. Nie restartuje, nie formatuje, nie dodaje zdalnego endpointu.

Autoryzacja zakończona przed unieważnieniem może dokończyć przyjętą operację;
nie abortujemy transferu/Flash/config Save. Kolejny auth check starego cookie
odmawia. Stary kod nie odtworzy dostępu; nowa późniejsza fizyczna akcja
wygenerowania kodu i udane parowanie pozostają dozwolone.

## NVS nie jest transakcją z rollbackiem

Osobiście sprawdzony pin IDF5.5.4 `735507283d5b2f9fb363a1901172dbd9e847945d`:
`nvs_commit()` jest obecnie no-op po walidacji uchwytu; `erase_item()` wykonuje
storage erase bez oczekiwania na commit. Nie obiecujemy zachowania starego
trwałego klucza po późniejszym błędzie ani jego usunięcia po błędzie erase.

Po rozpoczęciu unieważnienia RAM błąd NVS nigdy nie przywraca starego credential
do RAM. `revocation_unconfirmed` pozostaje true. Stary hash może pozostać na
Flash **albo** już zniknąć. Restart po błędzie może odtworzyć stare uprawnienie;
nie ma gwarancji trwałości. UI jawnie mówi „Nie potwierdzono trwałego
unieważnienia” i dopuszcza świadome ponowienie. Błąd precondition/admission/lock
przed invalidation nie twierdzi, że RAM zmieniono. Dopiero potwierdzony revoke
lub późniejsze udane parowanie usuwa flagę niepewności. Brak automatycznego retry.

## LCD i cykl życia

Ustawienia → Panel WWW / bezpieczeństwo → Unieważnij parowanie WWW. Osobny
ekran ostrzega o starych cookie, dokończeniu trwających operacji i zachowaniu
Wi-Fi/danych. Otwarcie, Anuluj i Wstecz/Home nie zgłaszają joba; tylko confirm
przyjmuje queue job. BUSY nie udaje wykonania.

Pending blokuje duplikaty i oba action buttons; Wstecz/Home pozwalają opuścić
ekran bez cofania operacji. Hide zeruje pointery, nie pending. Powrót pokazuje
pending; matching result aktualizuje stan także po ukryciu. Nie dereferencjonuje
starych obiektów. Generate code jest blokowany podczas własnego pending revoke
i odblokowywany po matching ACK również na otwartej stronie WWW. Sukces
opisuje zakończoną operację, nie zakaz przyszłego fizycznego parowania.

## Dowody i pozostały odbiór

Rzeczywiste backendy/auth/pair/worker/UI; NVS, HTTP, mutexy i RTOS jawnie MODEL,
genuine SHA/activity i rzeczywisty LVGL. Testy obejmują open/erase/commit/readback,
natychmiastowy erase bez rollbacku, symulowany boot, stare cookie/kody, nowe
parowanie, idempotencję, odmowę admission, restart w etapach, trylock/zero out,
pending hide/reopen/matching result, duplikaty i PL/EN/DE/44px/heap. Target audit
dotyczy nowych funkcji i własnych ramek, nie SDK/NVS transitive stack peak.

Wznowienie na PR49ba1591b: pełne native UBSan PASS, panel786/worker3570,
config265; realny LVGL3741/119 kadrów, revoke40cycles heap105488→105488 PASS.
Guardy SHA local_request/authorized/api bez zmian. Locale dictionary1885/237
PASS; zachowano dotychczasowe źródła edytora i shipped locale assets. Są to
dowody hostowe; rzeczywisty target/CI i odbiór sprzętu pozostają osobno.

CI/host nie zastępują fizycznego ponownego parowania, odmowy starego cookie po
restarcie, HTTPS/Origin/Host regresji i zachowania danych. Nie uruchamiano
operacji na urządzeniu. Właściciel potwierdził retencję pokojów po resecie i
działający Web Panel na0.7.18 po celowym przywróceniu PRE-FORMATTED dsrdata
([PR33comment6214082938](https://github.com/voltex-at/DIY-Shelly-Remote/pull/33#issuecomment-6214082938)).
To nie blank-init ani pełny TLS/pairing PASS; nowy revoke wymaga osobnego
odbioru fizycznego. Bez kolejnego erase/przywracania dsrdata, main/Stable,
partycji czy factory reset. Wznowiono istniejącą implementację na PR49 source8637296,
z zachowaniem poprawki edytora PR48 oraz locale WWW; nie wdrażano jej od nowa.

Po integracji root powtórzył LVGL3741/119, panel786/config265 i dictionary1885/237;
pełny native/worker3570 UBSan PASS. Revoke40cycles heap105488→105488, P0 Save/
Delete/refs/CAS i poprzednie funkcje zachowane. Source SHA guards auth/routes
pozostają dokładne. Parent locale exact CI38894174497 ma5/5 oraz browser688/28
i18renderów, dowody artefaktów w PR49. Własny target/CI/artefakt oraz fizyczne
unieważnienie/nowe parowanie pozostają osobnymi bramkami.

## Aktualny port na Cover/history/P0

Backend revoke i snapshot oraz fizyczny ekran pochodzą z PR50; dopasowano
wyłącznie bieżący worker enum/admission i skorelowany dispatch, zachowując
urgent STOP, historię, OTA i AP/B1. `PAIRING_REVOKE` dopisano po istniejących
job kinds. Job560/event1192, telemetry32 i wszystkie limity pozostają bez zmian.
Przy finishing BIND lub oczekującym STOP żądanie zwraca BUSY. Potwierdzenie
revoke nie staje się próbką endpointu/historii i nie przejmuje wyniku STOP.

Pełny native UBSan exit0; pairing786, worker4143, platform2173/API9719/mailbox635,
wszystkie poprzednie history/OTA/photo/P0/B1/AP regresje PASS. Dodatkowy actual
worker integration test daje4170 PASS: STOP przyjęty podczas revoke ma oddzielny
snapshot/lease nawet przy identycznym numericID6200; zwykły event pozostaje
revoke. LCD5137/169 kadrów + Cover105/7 + restart14/23/18/25, review2635/24 PASS.
Revoke40cycles heap105488→105488; PL/EN/DE potwierdzenie obejrzane. Istniejące
WWW modele runtime165/history32/AP46/BLE28 i dictionary2101 pozostają PASS.
Dokładne bytes local_request/authorized i kompletnego API current-parent są
niezmienione; API guard odnosi się teraz do faa623f, bez normalizacji tras.

Rzeczywiste Xtensa/SDK dependency/frame CI oraz fizyczny retest nie zostały
wykonane lokalnie. Zmiana usuwa wyłącznie jeden klucz NVS i jego uprawnienie RAM
wraz z oknem kodu/zgody Web OTA; nie jest factory resetem. Nie publikowano
nowego PR ani wersji i nie zmieniano mount, partycji, limitów lub urządzenia.

Źródła pierwotne:

- [NVS API commit](https://github.com/espressif/esp-idf/blob/735507283d5b2f9fb363a1901172dbd9e847945d/components/nvs_flash/src/nvs_api.cpp)
- [NVS handle erase/commit](https://github.com/espressif/esp-idf/blob/735507283d5b2f9fb363a1901172dbd9e847945d/components/nvs_flash/src/nvs_handle_simple.cpp)
