# Roleta: niezależne potwierdzenie komendy i odczyt stanu

Data: 2026-10-24. Baza kodu: `f5c185e`, po opublikowanym `dev-v0.7.75`.
Gałąź pozostaje `work/dev-0.7.70`. To lokalna poprawka, bez nowego wydania.

## Przyczyna i uzgodnione zachowanie

Poprzednia poprawka rozdzielała tekst błędu zapisu i odczytu, ale
`dsr_http_cover()` nadal zwracał wynik późniejszego GET jako wynik całej
komendy. Dwa nieudane odczyty po poprawnym ACK dawały błąd całego zlecenia.
Worker przekazywał go dopiero po zakończeniu odczytów, a fleet czyścił dane.

Oficjalne API [Cover](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Cover/)
zwraca `null` przy powodzeniu Open/Close/Stop/GoToPosition. Dla używanego
bezpośredniego endpointu `/rpc/<metoda>` HTTP zwraca zawartość wyniku, zgodnie
z [RPC Channels](https://shelly-api-docs.shelly.cloud/gen2/General/RPCChannels/).
Potwierdzenie wywołania i późniejsza obserwacja pozycji są osobnymi zdarzeniami.

## Zmiana

- HTTP Cover kończy zlecenie na poprawnym ACK. Zachowuje sprawdzenie tożsamości,
  uwierzytelnienia i bieżących możliwości przed ruchem. Nie czeka na GET.
- Worker przekazuje zwykły wynik do właściciela UI i zwalnia swój slot.
  Fleet planuje od razu osobne zlecenie odczytu przez istniejący arbitraż.
  Nie dodano zadań ani kolejek. Inne adaptery, w tym działający TRV, zachowują
  dotychczasowy kontrakt zapisu/odczytu.
- Osobny bajt `command_result` przechowuje wynik komendy. ACK nie kopiuje
  telemetrii sprzed POST jako nowego odczytu i nie zmienia `sample_ms`.
- Jeden transportowy błąd odczytu po ACK może ponowić GET po 1 s bez migania
  błędu, jeśli poprzednia próbka mieści się w istniejącym oknie świeżości.
  Ponowna utrata odczytu pokazuje błąd odczytu; auth, zmieniona tożsamość,
  błędne dane i limit zapytań pokazują swój rzeczywisty błąd od razu.
- Brak ACK nadal oznacza niepewny wynik komendy. Odzyskanie używa GET;
  nigdy nie powtarza automatycznie POST. Nowy ruch jest niedostępny podczas
  oczekiwania na odczyt; pilny STOP pozostaje dostępny.
- UI pokazuje „Polecenie przyjęte” w PL/EN/DE, także dla grup i STOP przy
  nieznanej pozycji. Grafika i animacja korzystają tylko z rzeczywistych danych.
  Przyjęcie nowego STOP zeruje jego bieżącą fazę. Późny ACK starego ruchu nie
  potwierdza STOP; anulowanie ruchu przed POST nie usuwa ostatniej obserwacji.
- Logi odróżniają `Cover command`, `Cover readback` i `Cover STOP`, z numerami
  job/layout/phase/result, bez haseł i adresów urządzeń.

## Walidacja

Najpierw odtworzono nieudany przypadek rzeczywistego adaptera HTTP: poprawny
ACK był blokowany przez późniejszy GET. Osobno regresja realnego fleet/LCD
odtworzyła utratę danych przez ruch anulowany pilnym STOP.

| Zakres | Wynik |
| --- | --- |
| Wybrane suites `platform`, `battery`, `actions_worker`, `actions_http_uncertainty` z `tools/test.py` | PASS; HTTP/fleet, osobne wyniki, brak replay, błędy, korelacja, actor/worker i pilny STOP. |
| `test_cover_ack_ui` w przypiętym LVGL 9.3 | PASS; ACK → osobny GET → timeout → odzyskanie, STOP po ACK, STOP przed POST i przyjęcie grupy. |
| `test_cover_refresh_ui`, `test_cover_ui`, `test_group_stop_ui` | PASS; istniejące obiekty, prawdziwa pozycja, STOP i błędy; zmienione informacje sprawdzone w trzech skórkach i PL/EN/DE. |
| ESP-IDF 5.5.4, `ninja -C build-ai -j4` | PASS; aplikacja mieści się w partycji, 29% zapasu. |
| Audyty `check_boot_memory.py`, `check_fleet_target.py`, `check_actions_target.py` | PASS; fleet 8080 B, slot 504 B; stos UI 8192 B i pula LVGL 131072 B bez zwiększenia. |

Liczby asercji w logach nie oznaczają osobnych uruchomień. Nie uruchamiano
pełnego CI ani pełnego atlasu. Jeden niezależny przegląd wskazał brak tekstu ACK
grupy i dwa przypadki dziedziczenia/utraty danych przy STOP; odtworzono je
regresjami i poprawiono. Szczegółowe logi i rendery są w ignorowanym `.local/`.

Przy przygotowaniu 0.7.76 pełne CI pierwszego commita wydania wykazało stare
oczekiwanie STOP przy nieruchomej rolecie w podglądzie DEMO. Dostosowano dwa
fixture podglądu do zaakceptowanego układu: brak STOP w spoczynku, dolny STOP
podczas ruchu/nieznanej pozycji i zachowana kontrola braku komend z DEMO.
Lokalnie oba dotknięte podglądy przeszły; firmware nie wymagał kolejnej zmiany.
Pełny przegląd wskazał także kliknięcie dawnej bocznej pozycji STOP w
`test_cyber_ui`. Test sprawdza teraz brak bocznej akcji oraz prawidłowy dolny
STOP podczas raportowanego ruchu; ten przypadek również przeszedł lokalnie.
Publikacja wymaga pełnego CI 5/5 końcowego commita.

## Pozostaje sprzęt

Host i build nie potwierdzają rzeczywistej latencji sieci, silnika ani dotyku.
Na płytce trzeba sprawdzić czas ACK, ruch, zmianę pozycji i STOP podczas ruchu,
chwilową i trwałą utratę odczytu oraz poprawny błąd logowania. Publikacja nowego
DEV wymaga osobnego commita wydania i pełnych bramek z RELEASE_CHECKLIST_PL.md.
