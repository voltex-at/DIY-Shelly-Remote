# #32 — wspólny kreator dodawania na LCD i WWW

Pion UI na istniejących rzeczywistych transportach LAN, AP i BLE B1. Pierwotny
checkpoint na Cover (`faa623f`) jest obecnie zintegrowany z grupami/scenami
PR60 jako kandydat `0.7.32-onboarding-dev` w PR61. Nie tworzy nowego skanera, magazynu połączeń ani
modelu urządzeń. Nie zawiera prywatnych zdjęć, SSID, adresów ani logo Shelly.
Kompozycja jest własna; używa istniejącej typografii Roboto i atlasu Material
Symbols Rounded z zachowanymi licencjami repozytorium.

## Zachowanie

Na LCD „+” otwiera arkusz od y=72 nad przyciemnionym rzeczywistym HOME. HOME
jest zatrzymany (`mobile_hide`, widok NONE), a pełny modal przechwytuje dotyk.
Arkusz ma uchwyt, tytuł, „Krok 1 z 3”, trzy segmenty i trzy karty metod.
Ręczny IP jest czwartą opcją w przewijanej liście. Stopka Anuluj/Dalej ma
przyciski wysokości 48 px i pozostaje poza obszarem przewijania.

Zaznaczenie metody zmienia wyłącznie radio i obramowanie. Nie wywołuje SDK,
HTTP, skanu ani zapisu. Dalej jest nieaktywne bez wyboru. Offline blokuje AP
i LAN z wyjaśnieniem; skan BLE oraz formularz ręczny pozostają dostępne.
Aktywne kończenie AP/BLE, operacja OTA, oczekujący pilny STOP Cover lub
BIND blokuje rozpoczęcie kolejnego pionu. BIND obejmuje także kończenie
workspace oraz prywatnego exact ACK; stary sukces nie zamyka nowej metody.
Zmiana Wi-Fi i okresowy poll nie przebudowują HOME pod otwartym modalem.

Dalej uruchamia istniejący rzeczywisty skan wybranej metody LAN, BLE lub AP;
nie wymaga ponownego przycisku Szukaj. Ponów pozostaje przy wynikach. Wszystkie używają wspólnego nagłówka i segmentów nad y=80,
listy wyników oraz stopki od y=408. Krok 2 oznacza wyszukanie/wybór; krok 3
weryfikację i konfigurację, a nie deklarację zakończonego zapisu. Klawiatura
chowa stopkę i zmniejsza przewijany formularz, aby nie zasłonić aktywnego pola.
Powrót z formularza ręcznego prowadzi do wyboru metody.

Wynik BLE pokazuje odrębnie rozpoznany model i `Reklama: …`. Nieznana nazwa,
w tym podobna do `ShellyPro2Cover`, daje „Nierozpoznane urządzenie”, bez
wywnioskowanej generacji/modelu i bez fikcyjnej akcji dodania. RSSI, wiek,
„wykryto, niesparowane”, reklama RPC i wynik API są różnymi dowodami. Więcej
wyników przewija się wewnątrz listy; nagłówek i przyciski pozostają nieruchome.

AP zachowuje obie fizyczne zgody, RAM-only zmianę Wi-Fi, fail-closed powrót,
barierę LAN oraz ten sam identity/SSID/IP readback przed BIND. B1 zachowuje
osobne fizyczne zgody, otwarte okno setup i jeden WiFi.SetConfig bez replay.
Zmiana współdzieli wyłącznie elementy UI; zasady transportów są opisane w
[AP](ISSUE19_AP_PL.md) i [BLE B1](BLE_RPC_B1_PL.md). BIND nadal musi otrzymać
rzeczywisty wynik istniejących dwóch magazynów. Odzyskiwanie SPIFFS i trwałość
BIND mają osobne bramki; test UI nie dowodzi poprawności nośnika ani zasilania.
Typowane endpointy Switch/Cover, Cover-only B1, kolejka i potwierdzenia STOP,
historia, format zdjęć i diagnostyka przyczyny resetu pozostają z bazy Cover.
Kreator nie zmienia ich transportu ani magazynów.

WWW używa tych samych trzech etapów i lokalnego wyboru z jawnym Dalej.
Zmiana locale aktualizuje istniejące węzły i zachowuje wybór, formularz, focus
oraz wpisane wartości. Ręczny IP używa istniejącego `devices/bind`, a nie
zapisu fikcyjnej karty. Błędy i utracone odpowiedzi nie powodują ponowienia
POST. AP i BLE B1 przekazują pracę do LCD bez password API; fizyczne zgody
pozostają na pilocie. Brak połączenia z serwerem jest odróżniony od dostępności
samego radia. Panel nie może wykonać operacji na odłączonym pilocie.

Anulowanie, opuszczenie i ponowne otwarcie WWW odcinają poprzednią sesję
kreatora. Spóźnione odpowiedzi skanu, inspekcji, odczytu lub BIND nie zmieniają
nowego widoku, wybranego celu ani blokady jego przycisku Dalej. Inspekcja
wymaga zgodności ID, adresu i generacji wybranego urządzenia. Anulowanie UI
nie obiecuje cofnięcia polecenia już przyjętego przez pilot; nie ponawia POST.

## Odtwarzalna weryfikacja

```sh
python3 tools/test_ui.py --ubsan-only
python3 tools/test.py --ubsan-only
node tests/test_panel_onboarding_model.cjs
node tests/test_panel_ble_model.cjs
node tests/test_panel_ap_model.cjs
node tests/test_panel_runtime_model.cjs
node tests/test_panel_history_model.cjs
node tests/test_web_locale_dictionary.cjs
python3 tools/test_ble_target.py
```

Nowy test prawdziwego LVGL używa zdarzeń wskaźnika, przeciągnięcia listy,
rzeczywistych granic stopki, offline i busy, pustego skanu, 10 reklam,
nieznanej nazwy, błędu skanera oraz PL/EN/DE ustawionych w modelu workspace.
Po advanced scroll→Anuluj→ponowne otwarcie sprawdza zerowy scroll oraz pełne
współrzędne pierwszej nazwy i karty w widocznym obszarze w każdym języku.
Weryfikuje zero wejść transportów przed Dalej i 10 szybkich cykli anulowania
z identycznym wolnym heap LVGL. Radiowe i storage providers są jawnie MODEL.
Kadry 300–327 pochodzą z prawdziwego renderera LVGL RGB565, nie z concept art;
CI eksportuje je do PNG w istniejącym pakiecie `review-export`.

Test modelu WWW wykonuje funkcje z wysyłanego panel.js, obejmuje wszystkie
cztery metody, offline, podwójne kliknięcie Dalej, brak ponowienia utraconego
POST i zaawansowany bind. To nie dowód DOM/przeglądarki. Osobne Playwright
sprawdza prawdziwe zasoby i zachowanie tego samego DOM podczas zmiany locale.
Lokalnie Chromium nie jest dostępne; te testy i target Xtensa wymagają CI.

Regresje sesji WWW obejmują odpowiedzi opóźnione na granicach request/body,
błędy i finally starego Dalej podczas nowszego żądania, anulowanie przed
POST skanu, obcy adres/generację w inspekcji oraz późny wynik BIND. Wykonują
wysyłane funkcje panel.js z jawnymi providerami DOM/HTTP MODEL.

Checkpoint integracji na Cover: pełny `tools/test.py --ubsan-only` PASS;
prawdziwe LVGL 4947 sprawdzeń / 189 kadrów, osobne procesy boot BIND
14/23/18/25, Cover 112 / 7 kadrów oraz DEMO review 2635 / 24 PASS. Dziesięć
cykli kreatora zachowuje heap 101384 B. Regresja Cover sprawdza też, że
oczekujący STOP blokuje Dalej bez uruchomienia skanera. Model WWW ma 165
sprawdzeń, w tym Cover-only capabilities, typowany POST i brak pełnego
potwierdzenia publikacji, gdy config zawiera Switch zamiast żądanego Cover.
Osobne modele runtime 165, historii 32, AP 46, BLE 28 i słownik locale 2233
PASS. To wyniki host UBSan/modeli; przeglądarka i target wymagają osobnego CI.

Końcowa integracja z #21 i portem PR50: pełny native UBSan PASS, rzeczywisty
LVGL 5552 kontroli / 204 kadry, boot14/23/18/25, Cover112/7 i osobny test
późnego BIND29 PASS. Niezależny test był RED przed blokadą nowej metody,
potem sprawdził osobno zatrzymany workspace ACK i prywatny ACK. Dziesięć
cykli zachowuje heap101336→101336 B. Przy limicie własnej ramki network2048 B
snapshoty event i prywatnego BIND są w osobnych noinline helperach, bez
zmian kolejności/warunków i bez wzrostu limitów. Target ma własną bramkę.

Nowy plik UI ma raport `.su` w buildzie targetu: maksymalnie 512 B własnej
ramki i 256 B stałego BSS, oceniane istniejącym audytorem BLE. Nie zwiększono
80 KiB limitu internal BSS, stosu UI 8192 B, workera 10240 B ani puli LVGL
128 KiB. Własna ramka nie jest pomiarem szczytu stosu SDK ani heap BLE.

## Osobny odbiór fizyczny

Nie wykonano skanu radiowego ani operacji Flash w ramach tego pionu. Właściciel
powinien dopiero na zaakceptowanym artefakcie sprawdzić 320×480 dotyk,
przewijanie 10 wyników, PL/EN/DE, powrót po anulowaniu i rzeczywisty LAN/AP/B1
z zachowaniem zgód. Odbiór wymaga również zakończenia niezależnych bramek
odzyskiwania niepustego SPIFFS i bezstratnego BIND. Same rendery, host testy
lub kompilacja nie zamykają tych bramek.
