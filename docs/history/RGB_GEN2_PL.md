# #23 — Shelly Plus RGBW PM w istniejącym profilu RGB

Ten pion dodaje prywatną rodzinę RGB3 oraz rzeczywiste RGB RPC na LCD i WWW.
Bazą jest zintegrowany Light/#21/#32 `df93106`, odpowiadający źródłu PR62
`18f91a61ea9114837bdc068a7553f98a77687e61`. Wersja projektu: `0.7.34-rgb-dev`.
Nie wykonano testów fizycznego Shelly, zapisu Flash pilota ani zmiany profilu.

## Konkretny zakres i źródła

Oficjalne źródła producenta, sprawdzone 2026-10-16:

- [RGB](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/RGB/):
  `RGB.GetStatus`, `RGB.Set`, output, brightness0–100, rgb=[R,G,B]0–255;
  udokumentowany transition.target zawiera output, brightness i rgb.
- [Plus RGBW PM](https://shelly-api-docs.shelly.cloud/gen2/Devices/Gen2/ShellyPlusRGBWPM/):
  odrębne profile light/rgb/rgbw; w profilu rgb istnieje jeden komponent rgb:0.
- [Identyfikacja modelu](https://kb.shelly.cloud/knowledge-base/shelly-plus-rgbw-pm):
  kod SNDC-0D4P10WW, Gen2.
- [Shelly.GetDeviceInfo](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly/):
  identyfikator, model, generation i profil urządzenia wieloprofilowego.
- [RGBW](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/RGBW/):
  odrębny komponent i white; nie jest aliasem RGB i pozostaje niewspierany.

Obsługiwany jest dokładnie Gen2 SNDC-0D4P10WW działający **już w profilu rgb**,
komponent0, z istniejącym transportem HTTP. Profile light i rgbw nie są
konwertowane. Brak modelu/profilu, inny model/generacja/kanał lub zła identity
zatrzymują typed RGB BIND i polecenia przed użyciem credentials. Nie wywołuje
się Shelly.SetProfile, PlusRGBWPM.SetConfig, kalibracji, Toggle, offset,
SetAll, white, CCT ani API Gen1. Nie deklaruje się obsługi każdej rodziny RGB.

## Odczyt, intencja i potwierdzenie

Output jest osobnym booleanem; jasność0 i RGB[0,0,0] nie oznaczają OFF.
Polecenia są rozłączne: on/off wysyła tylko on, brightness tylko brightness,
rgb tylko trzy całkowite składowe koloru. Jeden jawny POST RGB.Set przechodzi
przez istniejący mailbox i serialny worker. Wcześniej następuje publiczna
identity bez credentials oraz autoryzowany świeży RGB.GetStatus; po ACK null
obowiązuje odczyt zwrotny. Wspólne callback guards, write-boundary hook,
restart leases i pilny Cover STOP między nogami HTTP pozostają zachowane.

Przy aktywnym transition **cel rozstrzyga o zgodności konkretnej intencji**.
Chwilowo zgodny current przy przeciwnym target nie jest sukcesem. Cały target
jest walidowany i porównywany przed zwolnieniem JSON. Do trwałych modeli trafia
wyłącznie rzeczywisty current r/g/b/brightness oraz flagi VALID/TRANSITION/ERROR.
Nie przechowuje się niepełnego celu ani nie zastępuje odczytu intencją.
Podczas transition nowy RGB/brightness/on/off jest jawnie zablokowany do
stabilnego GET. To świadomy limit kompaktowego modelu, a nie symulacja celu.

Brak/null/wrong type wymaganych output/brightness/rgb, niecałkowita wartość,
niewłaściwa długość tablicy, przekroczony zakres, duplikat klucza lub błędny
transition nie tworzą próbki. Każdy niepusty errors, również nieznany kod,
blokuje sterowanie. Opcjonalne brakujące/null W/V/A/Wh pozostają nieobecne;
poprawne zero pozostaje zerem. Próbka starsza niż15s blokuje operację.
Timeout/auth/error/niezgodny readback nie powodują ponowienia SET. Kolejne GET,
reconnect lub zmiana języka nie odtwarzają intencji. Serialny transport może
opóźnić inne polecenie o trwającą nogę HTTP; nie ma nowego tasku ani obietnicy
równoległych połączeń. Fizycznego czasu odpowiedzi nie zmierzono.

## BIND, publiczna kategoria i prywatne wersje

Publiczny workspace pozostaje schema2. Kategoria `light` jest kategorią
wizualną wspólną dla Light2 i RGB3; rzeczywisty widok i routing wyznacza tylko
prywatny `(device_id,generation,kind,channel)` z opublikowanym numeric layoutID.
Sam host/channel ani zmiana kategorii na light nie konwertują Light→RGB i nie
przenoszą credentials. RGB layout0 bez rzeczywistego legacy origin pozostaje
w kwarantannie. Nowy typed RGB BIND nie adoptuje host-only karty Light.

Migracja tworzy świeży numericID i zachowuje go jako kandydata konkretnej
wersji migracji. Dopiero dokładny workspace ACK przekazuje ID do tego samego
aktywnego typed BIND. Następnie istniejący prywatny store zapisuje i odczytuje
mapowanie; pełny sukces wymaga obu ACK. Cancel/failure/new intent zerują lub
usuwają kandydata. Zmiana workspace bez exact ACK daje CONFLICT, zamiast
ponownego automatycznego tworzenia RGB karty. Użytkownik powinien odświeżyć
konfigurację przed kolejnym dodaniem; niepotwierdzona karta layoutu nie
uzyskuje sterowania. Istniejące numericID innych endpointów są zastrzeżone.

Prywatny rejestr v5 nadal ma4104B. Frozen v1/v2 importuje wyłącznie Switch,
v3 Switch/Cover, v4 także biały Light. Kind3 zapisany pod v3/v4 jest korupcją,
nie rozszerzeniem starej wersji. Poprawny import jest wyłącznie RAM/lazy;
boot nie przepisuje rejestru. Dopiero rzeczywista mutacja zapisuje v5 z exact
readback. Future/corrupt/OOM odmawiają modyfikacji bez erase i bez fallbacku do
starego single bindingu. Layout0 w każdej nowszej wersji pozostaje w kwarantannie;
wyjątek dotyczy tylko rzeczywistego starego single v1.

Downgrade do v4 nie odczyta v5, ale zachowa jego bajty, workspace schema2 i
zdjęcia. Jeszcze starszy single firmware może zobaczyć zachowany v1; nie
obiecuje się unieważnienia dawnych credentials we wszystkich wersjach.
Dawne displaykind light przypisane do Switch0 nadal wymagają jawnej korekty
relay/plug albo typed BIND; RGB nie odgadnie rodziny z historycznej kategorii.
Geometria SPIFFS, zdjęcia i surowe wcześniejsze ABI pozostają bez zmian.

## Discovery, B1, LCD i WWW

LAN rozpoznaje wyłącznie prawdziwy rgb:0 z poprawnymi output/brightness/rgb.
RGBW nie tworzy wiersza RGB. Ręczne LCD/WWW wybiera jawnie RGB API. B1 wymaga
rzeczywistych RGB.GetStatus/RGB.Set w ListMethods, modelu/profilu oraz wszystkich
istniejących guards otwartego pending/confirmed setup. Pozostają dwie fizyczne
zgody, wymazanie danych RAM, brak bondów oraz zgodna identity/MAC/generation/
SSID/IP na LAN przed BIND. Nie obchodzi się enhanced security/fabrycznego TLS2.0;
sama reklama lub GATT nie jest kryptograficznym dowodem własności.

LCD ma osobny widok koloru z trzema suwakami0–255, podglądem i „Zastosuj”.
Podgląd/zmiana widoku nie wykonują I/O. Jasność ma osobny suwak0–100; release
wysyła jedną intencję. WWW ma osobny natywny color picker z Apply; input/change
sam nie wysyła koloru. Poll i PL/EN/DE zachowują widoczny szkic i jego widget;
identity/host/epoch/freshness/error/transition lub zewnętrzna zmiana koloru
kończą szkic. Cancel/navigation nie wysyłają go później.

Authenticated POST `/api/v1/devices/rgb` przyjmuje wyłącznie device_id,
generation2, endpoint_kind3, channel0, command on/off/brightness/rgb oraz
odpowiednio brightness lub dokładnie3 liczby rgb. Brak host/password/API
konfiguracji profilu. Przyjęcie zwraca operation_id/boot_id; wynik jest
korelowany istniejącymi wrap-safe serialami. Nie ma automatycznego POST retry.

Historia RGB jest jawnie unsupported; kind3 nie jest aliasem Switch0. Grupy
i sceny pozostają ograniczone do Switch/Cover, odrzucają RGB przed I/O.
Dziennik aktywności nie interpretuje RGB output jako komendy Switch OFF.

## Dowody hostowe i budżet

```sh
python3 tools/test.py --ubsan-only
UBSAN_OPTIONS=halt_on_error=1 python3 tools/test_ui.py --ubsan-only
node tests/test_panel_runtime_model.cjs
node tests/test_panel_history_model.cjs
node tests/test_web_locale_dictionary.cjs
```

Genuine kod testowany z jawnymi modelami SDK/Flash/HTTP: pełne0–255 każdego
kanału i0–100 jasności, zero≠OFF, wszystkie intencje z przeciwnymi celami,
błędne identity/profile/JSON, auth/timeout/no replay, registry v1..v5/future,
quarantine i typed mailbox. cJSON parser/serializer podlega awarii każdej
alokacji. Actual LVGL obejmuje dwie zgody B1, typed BIND i osobne Light77/RGB78,
natywne gesty, local preview/Apply, poll/locale, stale/transition oraz40 cykli
nawigacji z identycznym wolnym heap. JS model testuje osobno draft i jego
unieważnienie po7 rodzajach zmiany. Shipped-assets Playwright test jest dodany;
bez dostępnego Chromium lokalnego browser PASS nie deklaruje się.

| Obiekt | Bajty native | Zmiana względem PR62 |
| --- | ---: | ---: |
| connection / telemetry | 296 / 32 | 0 |
| job / event | 560 / 1192 | 0 |
| mobile device / model | 320 / 6944 | 0 |
| output mailbox entry | 160 | 0 |
| fleet slot / fleet | 504 / 8080 | +8 / +128 |
| migrations[16] | 2944 | 0; candidateID z paddingu |

Intent RGB4B używa istniejącego AP payloadunion offset408, nie powiększa
job ani event. Current RGB5B używa istniejącej union telemetrii/mobile.
Nowy mały stan widoku koloru nie jest kopią modelu. Audytory zachowują
internal BSS≤81920B, fleet≤8192, UI stack8192, worker10240 i LVGL pool131072;
dodane publiczne funkcje RGB są wymagane w target .su. Rzeczywisty Xtensa
BSS/own-frame i browser CI pozostają osobnymi dowodami. Host sizeof ani własna
ramka nie mierzą transitive SDK stack/heap ani fizycznego bootu/retencji.
