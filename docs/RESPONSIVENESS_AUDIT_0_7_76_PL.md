# Odczyt po utracie odpowiedzi Cover — 2026-10-25

Baza: opublikowane `dev-v0.7.76`, commit `8557e72`. Zmiany opisane poniżej
są lokalne; nie oznaczają potwierdzonej naprawy fizycznego timeoutu HTTP.

## Dowód z płytki

Log właściciela potwierdza aplikację 0.7.76-dev i ESP-IDF 5.5.4.
Wyciąg zawiera tylko znaczniki czasu i publiczne wyniki operacji:

| Job komendy / layout | Wynik komendy | Następny odczyt tej samej rolety | Odstęp po wyniku komendy |
| --- | --- | --- | --- |
| 17 / 4 | 20,927 s: WRITE, OFFLINE | 22,437 s: zatrzymana, 93% | 1,510 s |
| 42 / 4 | 34,947 s: ACK, OK | 35,357 s: zamykanie, 79% | 0,410 s |
| 78 / 4 | 59,427 s: WRITE, OFFLINE | 61,557 s: zatrzymana, 6% | 2,130 s |
| 89 / 4 | 68,627 s: WRITE, OFFLINE | 70,667 s: otwieranie, 39% | 2,040 s |

Nie każdy późniejszy odczyt dowodzi ruchu: po jobach 17 i 78 pierwszy GET
pokazuje tę samą pozycję. Job 89 daje odczyt ruchu. W żadnym przypadku sam
GET nie dowodzi otrzymania ACK konkretnego polecenia. Faza WRITE jest
ustawiana przed `esp_http_client_perform()`; nie dowodzi pełnego wysłania
POST. Dokładne miejsce i przyczyna awarii transportu pozostają nieustalone.

Oddzielne pomiary: budowa strony 0 trwa 907–909 ms; pojawiają się przebiegi
LVGL 202–287 ms i odświeżenia ekranu do 274 ms. Nie ma dowodu, że powodują
timeouty HTTP. Ta poprawka nie usuwa kosztu początkowej budowy Dashboardu.

## Potwierdzone opóźnienia w kodzie i zmiana

- Fleet po niepewnym wysłaniu Cover czekał 1 s przed GET. W logu w tę lukę
  wchodziły odczyty przekaźników i innej rolety. Teraz GET jest należny od
  razu; właściciel daje mu jedną okazję przed zwykłymi odczytami, także po
  zmianie ekranu. Ręczne polecenia i pilny STOP nadal mają pierwszeństwo.
  Rozpoczęcie PROBE kończy ten specjalny priorytet; błędy GET nie tworzą
  pętli z pominięciem backoff.
- `dsr_http_refresh_info()` po poprawnym statusie wykonywał jeszcze
  opcjonalne `Sys.GetConfig` (limit 650 ms) i aktualizację nazwy w NVS,
  zanim worker oddał wynik. Ruch i odzyskiwanie używają teraz wariantu
  telemetry-only istniejącego PROBE, z tą samą weryfikacją tożsamości i
  uwierzytelnienia, lecz bez tego dodatkowego I/O. Zwykłe odczyty nadal
  aktualizują nazwę i metadane.
- Dwa istniejące bajty intencji są współdzielone przez rodzaje zleceń.
  Niezależny przegląd wykrył, że actor przy zamianie CLOSE na PROBE
  pozostawiał w nich wartość komendy 2, nieprawidłową dla nowej flagi bool.
  Regresja odtworzyła problem; konwersja do normalnego PROBE zeruje teraz
  oba bajty. Stara intencja nadal pozostaje w modelu actora do późniejszego
  sprawdzonego wykonania, a nie w payloadzie odczytu.
- Dodano ograniczone logi `Cover HTTP`: identity/status/name/command,
  kod SDK i HTTP, errno, kompletność, liczba wysłań nagłówków i fragmentów,
  liczba odebranych/zachowanych bajtów, przekroczenie terminu i czas.
  Logowane są komendy, błędy i odczyty od 500 ms. Nie ma hostów, identyfikatorów,
  haseł, nagłówków ani treści odpowiedzi. Wysłanie nagłówków nie jest ACK.

Zachowano rzeczywisty komunikat niepewnej komendy, zakaz replay POST,
oddzielenie żądanej i odczytanej pozycji oraz niezależny STOP. Nie zmieniono
limitu 3 s, transportu HTTP, ustawień Wi-Fi, wersji bibliotek ani skórek.

## Walidacja i ograniczenia

Regresje obejmują realny fleet, actor, worker i LVGL z modelami transportu:
natychmiastowy GET po utracie ACK, jeden odczyt przed nowym viewportem,
powrót zwykłego odświeżania, STOP, brak replay, prawdziwy stan, normalne
metadane i konwersję starej komendy CLOSE do PROBE. Odrębne czerwone testy
odtworzyły brak trybu telemetry-only i konflikt payloadu przed poprawkami.

| Końcowe sprawdzenie | Wynik |
| --- | --- |
| `tools/test.py`: wybrane `platform`, `actions_worker`, `actions_http_uncertainty`, uruchomione lokalnym filtrem bez pozostałych runnerów | PASS |
| `tools/test_actions_runtime.py` | PASS; konwersja PROBE nie zmienia intencji zapisanej w actorze ani jego bramek |
| `tools/test_ui.py --cases test_cover_ack_ui test_cover_refresh_ui test_group_stop_ui test_priority_ui` | PASS; realny LVGL 9.3, transport modelowany |
| Nowy `test_cover_readback_priority_ui` | PASS; zmiana ekranu, utracony ACK, odczyt przed zaległym przekaźnikiem, powrót tła i urgent STOP |
| ESP-IDF 5.5.4, `ninja -C build-ai -j4` | PASS; aplikacja 0x441c50 B, 29% wolnej partycji |
| `check_boot_memory.py`, `check_fleet_target.py`, `check_actions_target.py` na końcowym ELF | PASS; fleet 8080 B, stos UI 8192 B i pula LVGL 131072 B bez zwiększania |
| Niezależny przegląd po poprawce payloadu PROBE | Brak dalszych konkretnych usterek; bez prób radiowych |
| `check_docs.py` i `git diff --check` | PASS |

Liczby asercji w logach nie są liczbą uruchomionych testów. Nie wykonywano
pełnego atlasu ani ponownej publikacji w ramach diagnostyki.
Sprzęt musi jeszcze rozróżnić przyczynę timeoutu, czas odpowiedzi urządzenia,
kolejkę i reakcję ekranu; host nie zalicza tego odbioru.

Źródła: [Shelly RPC Channels](https://shelly-api-docs.shelly.cloud/gen2/General/RPCChannels/),
[Cover](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Cover/)
oraz lokalny kod `esp_http_client.c` oznaczony tagiem ESP-IDF `v5.5.4`.
Usługa Espressif zwracała również kod master, którego nie użyto jako dowodu
semantyki przypiętej wersji.

## Dalsze zgłoszenia: restart OTA, Pomieszczenia i pamięć

Właściciel zgłosił pętlę odliczania 5 s, odmowę ręcznego restartu jako
„zajęte”, przycinanie Pomieszczeń oraz odwrotnie wypełnione paski pamięci.
Trzy powiązane regresje odtworzyły błędy przed zmianami.

- Restart automatyczny był wywoływany z poll, który sam trzymał
  `dsr_activity_begin()`. Zwykła rezerwacja restartu musiała więc zwrócić BUSY;
  UI uzbrajało kolejne 5 s. Teraz tylko próba restartu następuje po zwolnieniu
  tej blokady. Aktualizacja snapshotu i baterii nadal działa po odebraniu wyniku
  workera w tym samym poll. Ręczny przycisk zapamiętuje prośbę; odliczanie
  automatyczne pozostaje przy zerze do przyjęcia żądania. „Później” odracza.
- Podczas oczekiwania nie przyjmujemy kolejnych zwykłych odczytów tła ani
  nowych zadań automatyzacji. Przyjęte komendy, STOP i zapis mogą się zakończyć.
  Przegląd wykrył dodatkową lukę: intencja TRV może czekać w RAM na potwierdzenie
  zapisu bez aktywnej blokady. Zachowano możliwość jej wysłania i dodano osobną
  kontrolę oczekiwania przed restartem, zarówno ręcznym, jak automatycznym.
  Nie użyto wymuszonego restartu ani nie zmieniono bramek boot/OTA/zapisu.
- Pomieszczenia nie korzystały ze ścieżki aktualizacji kart, więc zmiana
  telemetrii odtwarzała ekran ze zdjęciami. Pokoje i ich kafelki Dashboardu
  zachowują teraz obiekty, zdjęcia i przewinięcie; zmienia się tylko istniejąca
  etykieta sumy mocy/liczby urządzeń. Zmiana struktury, nazwy, zdjęcia, języka
  lub skórki nadal korzysta z normalnej przebudowy. Pierwsze wejście nadal
  wymaga utworzenia widoku — nie zmierzono jeszcze jego czasu na płytce.
- Paski RAM/PSRAM/grafiki pokazują `(całość − wolne) / całość`, a stos —
  szczyt wykorzystania na podstawie minimalnej wolnej wartości. Opisy nadal
  podają wolne/minimalne wolne bajty; legendę zmieniono w PL/EN/DE. BLE
  pozostaje stanem modułu. Dla 80 wolnych z 294 KiB zajętość wynosi około 73%
  (72% po obcięciu do całkowitej wartości paska). Nieznana pojemność pozostaje
  stanem nieaktywnym, a nie fałszywym pomiarem.

| Sprawdzenie tej rundy | Wynik |
| --- | --- |
| `test_room_refresh_ui` i `test_memory_ui` | PASS, trzy skórki × PL/EN/DE; zachowanie obiektów/scrolla, wartości graniczne, rzeczywiste rendery |
| `test_native_ota_ui` | PASS po zachowaniu kolejności wynik → snapshot; komunikaty baterii, odroczenie, restart i nawigacja |
| `test_ota_restart_admission_ui` | PASS, osobne procesy auto/manual/TRV; rzeczywista bramka aktywności, zajęty writer, zaległe tło, intencja bez blokady, brak obejścia i jednokrotne przyjęcie |
| `test_priority_ui`, `test_cover_readback_priority_ui` | PASS, dotychczasowe priorytety i poprawki Cover zachowane |
| `python3 tools/test_restart.py` | PASS, moduł restartu i ochrony zapisu; scheduler/SDK/reset modelowane |
| `ninja -C build-ai -j4`, ESP-IDF 5.5.4 | PASS, aplikacja 0x441e40 B, 29% wolnej partycji |
| `check_boot_memory.py`, `check_fleet_target.py`, `check_actions_target.py` | PASS, internal BSS 61872 B, LVGL 131072 B w PSRAM, stos UI 8192 B, fleet 8080 B |
| Niezależny końcowy przegląd | Brak dalszych konkretnych usterek po naprawie oczekującej intencji TRV |

Kontrolowano tylko powiązane przypadki i reprezentatywne obrazy; nie tworzono
pełnego atlasu. Logi i rendery pozostają pod ignorowanym `.local/`/build.
Kompilacja obejmuje też wcześniejsze lokalne zmiany Cover opisane wyżej.
Nie wykonano fizycznego resetu, aktualizacji, flashowania ani publikacji.
Rzeczywisty restart po OTA i czasy klatek/dotyku wymagają odbioru na płytce.

## Kafelki Pomieszczeń i odświeżanie kart na innych stronach

Właściciel doprecyzował, że przycinają również inne ekrany. Dotychczasowa
naprawa Pomieszczeń nie zamyka więc problemu płynności całego interfejsu.

Właściciel doprecyzował, że chodzi o pustą część ramki pod zdjęciem,
nie o pomniejszenie grafiki. Przywrócono rozmiar zdjęcia z bazowego kafelka
186 px. Ramka kończy się teraz 6 px pod paskiem
pomiarów, który jest 4 px pod rzeczywistą zawiniętą nazwą. Krótsza nazwa
nie rezerwuje już pustych 46 px; dłuższa nadal ma potrzebną wysokość do
dotychczasowego limitu. Odstęp między rzędami wynosi 8 px od wyższej ramki
w parze, zamiast rezerwować stary skok 194 px po skróceniu ramek. Wysokość
jest znana ze stylu w pikselach; układ nie wymaga dodatkowego przeliczania
całego drzewa LVGL po każdej karcie. Dwukolumnowy układ, przyciski edycji
i kafelki Dashboardu zachowują poprzednie wymiary. Obejrzano prawdziwe rendery przed/po
trzech skórek oraz długie nazwy i wariant bez zdjęcia; sprawdzano PL/EN/DE.

Wspólna ścieżka kart miała jeszcze odrębny koszt: zmiana samego
`blu_age_s` odtwarzała kartę BLU na Dashboardzie, Urządzeniach i liście
urządzeń wybranego pomieszczenia, choć te karty nie pokazują wieku sygnału.
Regresja `sensor_card_age` w `test_priority_ui` zawiodła przed poprawką na
usuwaniu obiektu karty. Teraz pełny model przyjmuje nowy wiek bez przebudowy
karty; zmiana kontaktu, możliwości sterowania, tożsamości lub innego pola
nadal odświeża widok. Ignorowanie samego wieku dotyczy wyłącznie BLU:
pozycja rolety współdzieli te bajty modelu i pozostaje sygnałem odmalowania.
Ekrany szczegółów nadal otrzymują wiek i pokazują go normalną ścieżką.

PASS: `tools/test_ui.py --cases test_room_refresh_ui test_priority_ui
test_cover_refresh_ui`, przez lokalny adapter Cygwin, LVGL 9.3.0. Przypadki
obejmują trzy skórki, PL/EN/DE, zachowanie kart, rzeczywistą zmianę kontaktu
i aktualizację grafiki rolety. Przyrostowy `ninja -C build-ai -j4` przeszedł:
aplikacja 0x441f70 B, 29% wolnej partycji. Niezależny przegląd źródła nie
wskazał dalszych usterek w tej zmianie. Nie wykonywano pełnego atlasu, CI,
flashowania ani publikacji. Początkowa budowa ekranów, koszt rysowania i
rzeczywiste czasy reakcji całego UI pozostają do pomiaru na płytce.

Po korekcie samej ramki i odstępów powtarzano wyłącznie `test_room_refresh_ui`
(PASS, trzy skórki i PL/EN/DE) oraz oględziny zmienionych renderów.
Nie powtarzano kompilacji całego firmware ani testów niezwiązanych z tą
korektą wyglądu. Podany wyżej rozmiar BIN dotyczy wcześniejszego etapu.

Odbiór właściciela 2026-10-25: ramki Pomieszczeń i odstęp 8 px między
rzędami zaakceptowane na podglądzie LVGL. Płynność pozostałych ekranów
i odbiór na płytce pozostają otwarte.
