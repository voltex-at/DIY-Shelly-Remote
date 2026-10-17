# #23 — biały Light Gen2+, cel Shelly Dimmer Gen3

Ten pion dodaje rzeczywisty lokalny transport `Light`, osobny od Switch i
Cover. Pierwotny pełny pion `7693fc0` bazuje na Cover `568dbcf`. Ta integracja
obejmuje PR60/#21 i PR61/#32, kandydat `0.7.33-light-dev`.
Nie wykonano operacji na fizycznym Shelly ani zapisu Flash pilota.

## Zakres i źródła

Dokumentacja producenta sprawdzona 2026-10-16:

- [Light RPC](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Light/):
  `Light.GetStatus`, `Light.Set`, odrębne `output` i `brightness`, zakres 0–100%,
  przejście do celu, stan kalibracji, errors i flags. Przykład Set zwraca null.
- [Shelly Dimmer Gen3](https://shelly-api-docs.shelly.cloud/gen2/Devices/Gen3/ShellyDimmerG3/):
  jeden komponent `light:0`, dwa wejścia, Wi-Fi i BLE.
- [Identyfikacja Dimmer Gen3](https://kb.shelly.cloud/knowledge-base/shelly-dimmer-gen3):
  model S3DM-0A101WWL; pomiary zależą od obecności przewodu neutralnego.

| Rodzina | Implementacja | Dowód fizyczny |
| --- | --- | --- |
| Dimmer Gen3 / udokumentowany biały `Light` Gen2+ | GET, ON, OFF, jasność, typed BIND, LCD, WWW | Nie wykonano |
| Switch i Cover | Istniejące osobne ścieżki zachowane | Nie poszerza się dotychczasowych dowodów |
| Gen1 dimmer, RGB/RGBW/CCT, termostaty i TRV | Poza tym pionem | Nie deklaruje się obsługi |

Wykrycie nazwy w reklamie nie nadaje typu. LAN musi zwrócić rzeczywisty
`light:N` z poprawnym ID, boolean output i całkowitą jasnością 0–100.
Odczyt `Light.GetStatus` przy BIND potwierdza wybrany komponent. Nie zmienia
się profilu urządzenia, kalibracji, gamma, range_map ani konfiguracji obciążenia.
Dimmer z nieukończoną kalibracją pozostaje zablokowany; ten pilot jej nie inicjuje.

## Stan i polecenia

Jasność 0 jest poprawnym odczytem. Nie oznacza OFF: stan wyjścia ma osobną
wartość boolean. Podczas aktywnego przejścia to jego cel, również target.output,
rozstrzyga o zgodności komendy; chwilowo zgodny current nie maskuje przeciwnego
celu. Przy regulacji wysyłany jest wyłącznie `brightness`, a przy
ON/OFF wyłącznie `on`. Brak Toggle, SetAll, offset i automatycznego replay.
Odczyt przejścia zachowuje rzeczywistą bieżącą jasność i osobny cel. ACK null
nie ustawia lokalnie jasności ani stanu wyjścia.

Każda intencja przechodzi przez istniejący bounded mailbox i serialny worker:
identity bez credentials → autoryzowany świeży Light status → jeden jawny
Set → odczyt zwrotny. Niezgodny odczyt/cel jest błędem. Timeout SET pozostaje
niepewny; żadne późniejsze GET ani reconnect nie ponawia zapisu. Pilny STOP
Cover nadal przerywa pomiędzy nogami HTTP. Jeden wolny endpoint może opóźnić
kolejny o trwającą nogę HTTP; nie deklaruje się równoległych połączeń.

Błędy, kalibracja, uncalibrated/no_load i nieznane warunki blokują sterowanie.
Niepoprawny ID, typ JSON, duplikat klucza, brak/null jasności lub wartość poza
zakresem nie tworzą próbki. Brakujące/null opcjonalne W/V/A/Wh są nieobecne,
a poprawne zero pozostaje zerem. Odczyty starsze niż 15 s blokują sterowanie.

## Onboarding i prywatny rejestr

Discovery i ręczny formularz wybierają jawnie rodzinę Light. B1 używa tych
samych dwóch fizycznych zgód i nie wysyła hasła z samego wyboru reklamy.
Otwarte okno setup musi przejść istniejące guards tożsamości, provision,
wersji, metod Wi-Fi, enhanced security i braku baterii. Light dodatkowo wymaga
`Light.GetStatus` oraz `Light.Set` w rzeczywistej liście metod. Po provisioning
następuje zgodne ID/MAC/generation/SSID/IP w LAN i istniejący dwumagazynowy BIND.
Samo GATT ani Just Works nie jest kryptograficznym dowodem własności urządzenia.
Fabryczne HTTPS/enhanced security nie jest obchodzone; obowiązują limity B1.

Prywatny endpoint ma `(device_id, generation, kind=2, channel)`; adres IP to
locator. Rodzaj layoutu musi pasować do prywatnego endpointu. Zmiana karty na
inną rodzinę blokuje routing, zamiast dziedziczyć credentials innego API.
Zwykłe karty light bez trwałego prywatnego bindingu nie zyskują sterowania.

Rejestr v4 ma nadal 4104 B. Frozen v1/v2 importuje tylko Switch; v3 dopuszcza
wyłącznie Switch/Cover i odrzuca kind2 jako uszkodzenie. Import jest RAM-only,
bez zapisu podczas bootu. Dopiero rzeczywista mutacja użytkownika zapisuje v4
z exact readback. Nieznana wersja/uszkodzenie/OOM blokują modyfikacje i nie
przywracają legacy przez fallback. Layout0 w v4 pozostaje w kwarantannie;
jedyny wyjątek origin dotyczy rzeczywistego starego single v1.

Downgrade do firmware znającego najwyżej v3 nie odczyta rejestru v4, zachowa
jednak jego bajty. Jeszcze starszy single loader może zobaczyć zachowany stary
binding v1. Nie deklaruje się usunięcia dawnych credentials we wszystkich
wersjach. Dawna publiczna kategoria `light` mogła wskazywać prywatny
Switch0. Po tej zmianie karta wymaga typed Light2; niepasujący Switch pozostaje
zachowany, ale zablokowany. Dla rzeczywistego przekaźnika trzeba jawnie zmienić
kategorię karty na relay/plug; dla Light wykonać typed BIND. Nie jest to utrata NVS. Nie zmienia się publicznego formatu workspace ani geometrii SPIFFS.

## LCD, WWW i historia

LCD pokazuje oddzielne ON/OFF i jasność z API. Natywny suwak ma lokalny podgląd;
dopiero release wysyła jedną intencję. Poll i zmiana języka zachowują wartość
gestu. Offline, błąd, zmiana identity/locator/target i cancel kończą gest.
WWW utrzymuje DOM i stosuje te same zasady. Authenticated POST
`/api/v1/devices/light` przyjmuje wyłącznie dokładną identity, kind2, channel,
command on/off/brightness i jasność tylko dla brightness. Brak adresu,
credentials czy parametrów kalibracji w API. Accepted operation_id/boot_id
koreluje wynik; stary GET/ACK nie odblokowuje niepewnego polecenia.

Historia Light jest jawnie niewspierana. Kind2 nie trafia do zapisu, query,
forget ani serii Switch0. WWW nie wysyła query Switch dla wybranego Light.
Grupy/sceny #21 nie są rozszerzane w tym pionie. Light2 jest odrzucany przed
I/O przez typed model aktora. Edytor grup nie dodaje Light; istniejący wpis
można usunąć albo poprawić kategorię dawnej karty Switch. Walidacja zachowuje
stary workspace bez wymuszania zmiany schema ani zapisu podczas bootu.
Ręczne WWW wybiera rodzinę Switch/Cover/Light przed zweryfikowanym BIND.
Wspólne HTTP guard i write-boundary hooks obejmują także Light, bez dodawania
obsługi Light do planów grup/scen.

## Weryfikacja i pamięć

```sh
python3 tools/test.py --ubsan-only
python3 tools/test_ui.py --ubsan-only
python3 tools/test_ui.py --ubsan-only --review
node tests/test_panel_runtime_model.cjs
node tests/test_panel_history_model.cjs
node tests/test_web_locale_dictionary.cjs
```

Testy wykonują produkcyjny parser, HTTP adapter, rejestr, scheduler i prawdziwe
LVGL z jawnymi providerami MODEL. Obejmują każdą jasność 0–100, 0≠OFF,
transition/readback, auth/identity, lost SET i brak replay, kalibrację, błędne
JSON, v3/v4/future, quarantine, pełne cJSON allocation faults i typed mailbox.
LCD testuje obie zgody B1, typed BIND, prawdziwe pointer input/release,
poll+locale zachowujące widget, release poza obszarem dotyku, offline cancellation
i 40 cykli nawigacji z identycznym wolnym heap98728 B. Końcowy host checkpoint:
platform6448, worker4106, runtime parser/serializer12869, mailbox681,
B1 protocol152564/driver20203; pełny native UBSan PASS. Actual LVGL4619/161,
boot BIND14/23/18/25, Cover105/7, Light81/6 i review2635/24 PASS.
Modele WWW runtime324/history35/AP46/BLE28 i słownik2101 PASS.
Chromium wykonuje oddzielnie prawdziwe shipped assets; jego lokalnego PASS
nie deklaruje się bez dostępnego browsera.

Static asserts zachowują connection296, telemetry32, job560, event1192,
mobile device320/model6440 i fleet≤8192 B. Light status i intent używają
istniejących union/padding. Brak nowego tasku i dużego stałego bufora.
Audytory targetu nadal wymagają internal BSS≤81920 B, UI stack8192,
worker10240, LVGL pool131072 oraz bounded własnych ramek. Dodane Light public
funkcje i jednostka mobile są objęte raportami .su. To nie pomiar szczytu
stosu/heap SDK. Rzeczywisty target ELF i fizyczny Dimmer są osobnymi bramkami.

Integracja zachowuje device320/model6944 B z parenta #21 (wcześniejsza baza
Cover miała model6440 B). Light używa paddingu karty, nie powiększa modelu
parenta; publiczny limit8192 B pozostaje bez zmian. Actual native/LCD/JS
wyniki integracji są w PROJECT_STATUS; target i browser mają własne CI.
