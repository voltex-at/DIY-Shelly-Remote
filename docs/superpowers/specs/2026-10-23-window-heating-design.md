# Okno/drzwi → ogrzewanie — projekt zatwierdzony

Data: 2026-10-23. Status: projekt zatwierdzony przez właściciela w rozmowie
(„Tak, projekt zatwierdzony”); podstawa planu i wdrożenia wykonawcy automatyzacji. Ręczne sceny TRV są
osobną, już zaimplementowaną i zaakceptowaną na podglądzie funkcją.

## Cel i decyzje właściciela

Pilot ma sam ograniczać ogrzewanie przy otwartych oknach/drzwiach i przywracać
wcześniejsze nastawy, oszczędzając użytkownikowi ręcznego sterowania. Reguła może
obejmować kilka czujników i kilka termostatów. Uzgodniono:

1. Reakcja następuje po odebraniu zmiany stanu, bez dodatkowego opóźnienia.
2. Temperatura ograniczenia jest ustawiana w regule, domyślnie 8°C.
3. Dowolny otwarty kontakt rozpoczyna ograniczenie; powrót następuje dopiero po
   zamknięciu wszystkich przypisanych kontaktów.
4. Każdy termostat odzyskuje swoją wcześniejszą nastawę, a nie średnią grupy.
5. Ręczna zmiana temperatury podczas otwarcia jest zachowywana po zamknięciu.
6. Po restarcie lub utracie połączenia wykonawca czeka na świeże odczyty wszystkich
   uczestników i wznawia działanie automatycznie, zachowując ręczne zmiany.

Nie dodajemy obecnie reguł ruchu, harmonogramów, skryptów Shelly, chmury ani
uniwersalnego języka warunków. Nie zmieniamy zaakceptowanych kart urządzeń/grup.
Nie publikujemy DEV przed zakończeniem ankiety i walidacją commita wydania.

## Podejście i ograniczenia projektu

Projekt wykonawcy: reguła lokalna w pilocie, korzystająca z istniejącego aktora
`dsr_actions` i workera. Właściciel powierzył dopasowanie do obecnej architektury;
wybór tego sposobu integracji jest decyzją wykonawcy przedstawioną do odbioru.
Przeniesienie logiki do skryptów bramek wymagałoby instalowania i utrzymywania kodu
na innych urządzeniach. Zewnętrzny serwer wymagałby dodatkowego działającego systemu.
Oba warianty rozszerzają uzgodniony zakres; nie są częścią tego projektu.

Pozostają ESP-IDF 5.5.4, LVGL 9.3.0, ST77922 2.1.1, mDNS 1.8.2 i ES3C35P
320×480. Zachowujemy trzy skórki, polski/angielski/niemiecki, obecne partycje,
klucze OTA, prywatne powiązania i wszystkie istniejące edycje robocze.

Reguła działa, gdy pilot jest zasilany i może komunikować się z bramkami. Obecne
`dsr_board_sleep_display()` wyłącza podświetlenie; nie zatrzymuje pętli sieciowej.
Wyłączenie zasilania, niedostępna bramka i brak nowych danych wstrzymują sterowanie.
„Natychmiast” nie oznacza gwarancji czasu od fizycznego otwarcia: obecny transport
odczytuje BLU przez bramkę, kolejka jest szeregowa, BLU jest normalnie odpytywane
co 15 s. Nie zwiększamy bez uzasadnienia częstotliwości RPC ani nie omijamy 429.

## Co istnieje i co trzeba dodać

- `dsr_ui_network.c` jest właścicielem floty, publikuje odczyty i zleca pojedyncze
  zadania. `dsr_fleet` śledzi świeżość, kolejkę, typ oraz tożsamość końcówki.
- `dsr_actions` zamraża tożsamości, porządkuje kroki i rozróżnia ACK, błąd oraz
  niepewność. `DSR_ACTION_TARGET` już uruchamia istniejące `DSR_JOB_TRV`.
- `dsr_http_trv()` wykonuje RPC i późniejszy odczyt; jego komentarz jawnie mówi,
  że lustrzana nastawa bramki może być opóźniona. Sam wynik DSR_OK nie dowodzi,
  że już zobaczono nową nastawę fizycznego TRV.
- Konfiguracja jest zapisywana przez asynchroniczny `dsr_workspace` z CAS i
  dwoma checkpointami. Dotychczasowy schemat 2 nie zawiera reguł.
- Nowe moduły: przenośny model reguł, integracja wykonawcy, mały trwały dziennik
  odzyskiwania oraz edytor/status reguł. Nie powstaje drugi transport RPC.

## Konfiguracja reguły

Dodajemy schemat 3 z tablicą `automations`; maksymalnie 8 reguł. Przykład:

```json
{
  "id": 1,
  "name": "Wietrzenie sypialni",
  "kind": "contact_heating",
  "enabled": true,
  "sensors": [5, 6],
  "targets": [9, 10],
  "target_half": 16
}
```

`target_half` ma zakres 8..60, czyli 4..30°C co 0,5°C; domyślnie 16.
Obie listy mają 1..16 unikalnych istniejących ID kart. Nie zapisują IP, hasła,
klucza BLE ani indeksu w tablicy. Czujnik musi rzeczywiście udostępniać kontakt
otwarcia, a cel musi być zweryfikowanym TRV; nazwa lub wizualny `kind=sensor`
nie nadają takich uprawnień. Utrata możliwości po zapisie blokuje daną regułę.

Decyzje wykonawcy ograniczające niejednoznaczność:

- Termostat może należeć tylko do jednej reguły kontaktowej. Wiele okien dodaje
  się do jej listy kontaktów; nie tworzymy konkurujących reguł różnych temperatur.
- Czujnik może występować w kilku regułach z rozłącznymi celami.
- Cele są jawnymi ID termostatów. Edycja istniejącej grupy nie może samoczynnie
  zmienić urządzeń sterowanych przez regułę.
- Jeżeli bieżąca nastawa jest już niższa niż temperatura reguły, nie podnosimy
  jej przy otwarciu. Efektywna nastawa to minimum bieżącej i ustawionej w regule.

Walidator odrzuca duplikaty kluczy, obce pola, niecałkowite ID/nastawy, puste listy,
przekroczone limity i konflikty celów. Limit całego JSON pozostaje 24576 B.
Zamrażamy osobny walidator dotychczasowego schematu 2, obejmujący ręczne `target`;
schemat 1 również pozostaje zamrożony. Migracja 1/2 → 3 w RAM dodaje pustą tablicę,
zachowuje rewizję i treść istniejących obiektów. Trwałe przejście następuje dopiero
po jawnym zapisie użytkownika. Starszy firmware nie potrafi odczytać reguł schematu 3;
nie przedstawiamy tego jako kompatybilnego eksportu dla wcześniejszych wersji.

Przejście na schemat 3 obejmuje wszystkich odbiorców konfiguracji, w tym
`web/actions.js` (obecnie sprawdza literalne `schema===2`), edytory LCD/WWW,
usuwanie referencji urządzeń oraz migracje/checkpointy. Nie zmieniamy samej stałej
wersji bez aktualizacji tych kontraktów. Podczas aktywnego cyklu blokujemy zapis
zmienionych list kontaktów/celów lub nastawy reguły; formularz wyjaśnia potrzebę
zakończenia cyklu albo jawnego wyłączenia reguły. Nazwa i prezentacja są edytowalne.

## Świeżość i ponowne uruchomienie

Świeży HTTP do bramki nie wystarcza. Bramka przechowuje ostatnio odebraną wartość
BLU. Dla kontaktu potrzebny jest znacznik czasu konkretnej wartości
`BTHomeSensor.GetStatus`, nie dowolnego pakietu baterii/RSSI urządzenia. Dla TRV
potrzebny jest skorelowany, bezpośredni odczyt TRV opisany niżej.
Obecny parser zachowuje czas `BTHomeDevice`, ale nie czas konkretnego kontaktu.
Integracja musi dostarczyć właściwe metadane do modelu i testów; nie można zastąpić
czasu źródłowego kontaktu zegarem zakończenia HTTP. Akceptujemy oficjalne `last_update_ts` i występujące
w odpowiedziach `last_updated_ts`, także z ułamkiem sekundy; brak czasu oznacza
brak dowodu świeżości dla automatyzacji, a nie awarię ręcznego UI.

Decyzja wykonawcy: aktualny odczyt transportowy według istniejącego limitu floty
(BLU 45 s), dla kontaktu poprawny znacznik źródłowy nie starszy niż 120 s i nie
pochodzący z przyszłości. Po starcie/utracie połączenia wszystkie kontakty muszą
dostarczyć znaczniki późniejsze od bariery wznowienia, a każdy TRV musi odpowiedzieć
na bezpośredni odczyt rozpoczęty po tej barierze. Dokładność sekundowa
oznacza ostrożne oczekiwanie na następną sekundę, nie akceptację starego pakietu.
Brak wiarygodnego UTC wstrzymuje regułę do synchronizacji czasu. Skok zegara wstecz
unieważnia dowody świeżości i ustanawia nową barierę. Nie odtwarzamy zegarów
monotonicznych z poprzedniego uruchomienia.

**Dowód świeżej nastawy TRV:** `BluTrv.GetStatus.last_updated_ts` opisuje ostatni
pakiet TRV, a nie świeżość lustrzanej wartości `target_C`. Nie wystarcza ani on,
ani sam `rsv`, ani nowe HTTP do cache `GetRemoteStatus`. Do zebrania nastawy
bazowej, potwierdzenia własnego zapisu, uzgodnienia po restarcie oraz wykrywania
zmiany zewnętrznej wykonujemy istniejącym transportem jawnie dozwolone wywołanie
odczytowe `BluTrv.Call` → `TRV.GetStatus` z `id=0`. Weryfikujemy powiązanie,
skorelowaną odpowiedź, wewnętrzne ID i zakres wartości. Świadkiem jest udany RPC
od rzeczywistego TRV rozpoczęty w bieżącej epoce, nie czas pobrania pamięci bramki.
Po zapisie wymagany jest nowy taki odczyt rozpoczęty po zakończeniu próby zapisu.
Odczyty nie ponawiają `TRV.SetTarget`; POST przenoszący odczytowy RPC nie jest
zapisem temperatury. Nieznana metoda lub brak pełnej odpowiedzi wstrzymują regułę,
bez fallbacku na lustrzany cel. Mechanizm pokrywa Gateway Gen3; inne bramki muszą
potwierdzić ten kontrakt w testach urządzenia, zanim zostaną dopuszczone.

W aktywnym cyklu odczyty bezpośrednie są szeregowe, nie częściej niż dotychczasowy
interwał BLU na cel; nowe świadectwo jest potrzebne przed każdym następnym zapisem
oraz po zmianie epoki. Uwzględniamy dotychczasowy timeout: opóźnienie BLE może dać
niepewny wynik, nie automatyczny sukces. Błąd odczytu można ponowić jako odczyt
z ograniczeniem częstotliwości; nie pociąga za sobą ponowienia nastawy.

BLU Door/Window bez trybu beacon może nie nadawać regularnie niezmienionego stanu.
Po restarcie lub dłuższym braku danych reguła będzie wtedy czekać na nowy raport
każdego kontaktu. UI podaje ten powód oraz możliwość włączenia beacon w istniejących
ustawieniach czujnika. Nie włączamy beacon ani nie zmieniamy ustawień czujnika
samoczynnie. To istotne ograniczenie odbioru świeżych danych, nie błąd pomijany
przez przyjęcie zapamiętanego „zamknięte”. Brak lub sprzeczność raportów obiektów
kontaktu też blokują sterowanie; nie zgadujemy stanu.

Nowe metadane trzeba przenieść bez przypadkowego zwiększania wielkości kolejki.
Obecne asercje `dsr_job_t=560` i `dsr_event_t=1192` pozostają bramką projektu:
wykonawca wybiera oszczędną reprezentację właściwego czasu dla typu końcówki
albo jawnie dokumentuje i uzasadnia zmianę budżetu przed jej przyjęciem.

## Cykl i ochrona ręcznych zmian

| Sytuacja | Działanie |
| --- | --- |
| Wszystkie kontakty zamknięte, brak aktywnego cyklu | Obserwacja, bez wysyłania temperatury |
| Pierwsze otwarcie, komplet świeżych danych | Zapis wcześniejszej nastawy każdego celu i rozpoczęcie cyklu |
| Następny kontakt otwiera się w tym cyklu | Zachowanie tej samej wcześniejszej nastawy, bez ponownego obniżania |
| Zamknięto część kontaktów | Pozostawienie ograniczenia |
| Zamknięto wszystkie kontakty | Przywrócenie tylko celów nadal należących do automatyzacji |
| Wykryto ręczną zmianę celu | Wyłączenie automatycznego sterowania tym celem do końca cyklu |
| Brak odczytu dowolnego uczestnika | Wstrzymanie całej reguły, bez automatycznego podnoszenia temperatury |
| Restart lub powrót łączności | Odczyt dziennika, komplet nowych danych, ponowna ocena bieżącej sytuacji |

Gdy termostaty miały 20°C i 22°C, po cyklu otrzymują odpowiednio 20°C i 22°C.
Nie używamy bieżącej temperatury powietrza jako nastawy do przywrócenia.
Obecny kontrakt sterowania używa 4..30°C co 0,5°C. Jeżeli bazowa nastawa z innej
aplikacji nie mieści się w tym zakresie lub kroku, reguła jej nie zaokrągla:
pozostaje wstrzymana z czytelną przyczyną. Nie zaczynamy cyklu, którego poprzedniej
wartości obecny interfejs nie potrafi wiernie przywrócić.

Ręczny zamiar z LCD/WWW, również z grupy lub ręcznej sceny, oznacza override
przed ewentualnym wysłaniem automatycznego kroku i ma pierwszeństwo. Niewysłany
krok automatyczny jest anulowany. Trwający zapis kończy się własnym, skorelowanym
wynikiem; jawne polecenie użytkownika czeka w dotychczasowej kontrolowanej ścieżce.
Nie wolno zgubić tego zamiaru ani oznaczyć odpowiedzi automatyzacji jako jego ACK.

Inna, świeżo odczytana nastawa po ustaleniu własnego readbacku jest traktowana
konserwatywnie jako zmiana zewnętrzna (pokrętło TRV, inna aplikacja lub harmonogram).
Nie ponawiamy ograniczenia, gdy otworzy się kolejny kontakt w tym samym cyklu.
Po zamknięciu wszystkich kończy się cykl; nowe otwarcie może rozpocząć nowy z nową
nastawą bazową. Opóźniony odczyt sprzed własnego zapisu nie jest ręczną zmianą.

Ograniczenie obserwowalności: samo odczytanie tej samej wartości nie mówi, czy
ktoś ponownie ustawił ją w innej aplikacji. Nie obiecujemy rozpoznania takiego
zdarzenia bez danych o jego źródle. Zamiary z naszego LCD/WWW są rozpoznawane
bezpośrednio, a zmiana wartości zewnętrznej jest wykrywana z raportów.

## Wykonanie i dziennik odzyskiwania

Model reguł nie wykonuje I/O i nie mutuje LVGL. Jedyny właściciel to obecna pętla
sieciowa; HTTP przekazuje definicje przez CAS, a worker zwraca wyniki zadania.
Automatyzacja zgłasza skończony plan nastaw przez prywatne wejście aktora, bez
udawania grupy/sceny użytkownika i bez tworzenia zmiennego JSON sceny w pamięci.
Plany obejmują maksymalnie 16 urządzeń i mają własne ID cyklu oraz intencji.
Dziennik aktywności dostaje jawny typ/źródło „Automatyzacja”; WWW nie może podać
takiego źródła ani skonstruować prywatnego planu w publicznym żądaniu.

Każdy krok zamraża rzeczywiste powiązanie i sprawdza regułę, jej generację,
brak override, aktualność kontaktów/celu oraz tożsamość przed przyjęciem i ponownie
przed każdym zapisem. Ręczny zamiar, zmiana kontaktu, usunięcie/wyłączenie reguły,
zmiana jej list lub przepięcie urządzenia unieważniają niewysłane kroki.
Przeniesienie pokoju/zmiana nazwy nie unieważniają cyklu. Nie wprowadzamy zagnieżdżonych
blokad workspace/aktora ani wywołań LVGL z workera. Dla wielu reguł sprawiedliwy
kursor zapobiega zagłodzeniu; zwykłe ręczne sterowanie i pilny STOP mają pierwszeństwo.

Trwały dziennik zawiera identyfikatory reguł, skróty ich definicji/tożsamości,
wcześniejsze i żądane nastawy, per-cel override i fazę przejścia. Nigdy hasła,
klucze BLE ani kompletne połączenia z credentialami. Nie trafia do eksportu
konfiguracji ani publicznego dziennika aktywności.

Decyzja wykonawcy: dwa naprzemienne, wersjonowane rekordy z numerem sekwencji
i CRC w osobnej przestrzeni NVS `dsr_auto`. Cały rekord ma limit 3072 B, tabela
tożsamości maksymalnie 16 rzeczywistych urządzeń, wspólna dla wszystkich reguł.
Każdy zapis wymaga `nvs_commit`, zamknięcia/ponownego odczytu i zgodności treści.
I/O dziennika jest serializowane w istniejącym zadaniu konfiguracji przez osobny
rodzaj żądania i ACK, poza wątkiem LVGL. Nie zwiększamy `dsr_job_t` o duży blob,
nie dodajemy osobnego zadania RTOS i nie zapisujemy Flash przy każdym odczycie.
Zapis następuje tylko przy zmianie cyklu, stanu wykonania lub override.

Jawne włączenie reguły zapisuje również prywatne przypisanie jej rzeczywistych
uczestników, zanim reguła może sterować. Zmiana powiązania wymaga ponownego
przeglądu i jawnego zapisu reguły; nie zapisujemy automatycznie nowego odbiorcy
pod ponownie użytym ID. Dziennik nie jest opcjonalną historią: jego brak przy
już włączonej regule oznacza niepewność, nie pozwolenie na odtworzenie bazowej
nastawy ze zgadywania. Żądania dziennika nie zwiększają rewizji konfiguracji
i mają osobny stan ACK, żeby nie udawały zakończenia zapisu formularza.

Przed pierwszym zapisem TRV muszą być trwale zapisane bazowa nastawa i intencja.
Błąd tego zapisu blokuje automat i pokazuje komunikat; ręczny tryb urządzenia
pozostaje dostępny, jeśli automat nie przejął jeszcze nastawy. Wynik polecenia i potwierdzony odczyt są osobnymi fazami.
Sam ACK bramki nie przejmuje własności nastawy. Niepewny wynik uruchamia tylko
odczyty i uzgadnianie; nigdy ślepe ponowienie zapisu temperatury.

**Błąd utrwalenia ręcznego override:** dla każdego aktywnego cyklu, od rozpoczęcia
zapisu intencji do trwałego zamknięcia cyklu, lokalny zamiar z LCD/WWW najpierw blokuje automat w RAM,
a następnie musi trwale zapisać override przed przyjęciem ręcznego polecenia
do wykonania. Dotyczy to intencji przygotowanej, oczekującej, wykonywanej,
niepewnej i potwierdzonej; bramka nie czeka na ustanowienie własności po readbacku.
Obejmuje to jawne ustawienie tej samej temperatury. Trwający wcześniejszy zapis
dziennika nie może potwierdzić nowszego override: przed przyjęciem ręcznej komendy
wymagany jest osobny ACK właściwej generacji zawierającej ten override, po wszystkich
starszych zapisach. Nieudany
commit/readback oznacza jawny błąd polecenia (bez przyjęcia, bez fizycznego zapisu
i bez pozornego ACK); UI zachowuje żądaną wartość do ponowienia i pokazuje błąd
w złotej ramce. Automatyczne zapisy pozostają wstrzymane. Nie przyjmujemy ręcznej
komendy z gwarancją ochrony po restarcie, jeśli tej ochrony nie udało się zapisać.
Poza aktywnym cyklem nie dodajemy takiej zależności do ręcznego sterowania.
To granica błędu pamięci, nie dodatkowe potwierdzenie w normalnym użyciu.

Po restarcie nie odtwarzamy kolejek ani starych nonce. Z aktualnymi odczytami:
potwierdzona ograniczona nastawa i zgodny dziennik pozwalają kontynuować cykl;
odmienna nastawa oznacza zachowanie zmiany zewnętrznej. Dla przygotowanej, ale
niepotwierdzonej intencji nie powtarzamy zapisu. Jeżeli świeży raport potwierdza
jej cel, można uzgodnić stan; jeżeli różni się od wartości bazowej i żądanej,
oznaczamy override. Nierozstrzygalny zapis pozostaje widocznie niepewny, bez
automatycznej restauracji. Reguła nie żąda ręcznego wznowienia po zwykłym restarcie,
ale nie fabrykuje utraconej nastawy przy uszkodzeniu obu rekordów.

Uszkodzony/future rekord, błąd sekwencji lub brak miejsca nie prowadzą do formatowania
NVS. Nie korzystamy ze starszego rekordu, gdy nowszy istniejący rekord jest
nierozpoznany/uszkodzony: mógł zawierać nowszy override. Reguły z niepewnym dziennikiem
pozostają wstrzymane z jawną przyczyną. Nie odtwarzamy historii sprzed zmiany definicji
reguły lub tożsamości celu. Wyłączenie/usunięcie reguły anuluje dalsze automatyczne
zapisy; samo w sobie nie wysyła nowej temperatury do urządzenia.

## Interfejs i odbiór

Wejście przez obszar Scen, z przejściem do Automatyzacji. Nie dokładamy przycisku
do zaakceptowanej dolnej nawigacji ani siódmego wiersza do głównych Ustawień.
Formularz korzysta z obecnych elementów kreatora scen/grup: nazwa, wybór kontaktów,
wybór termostatów, nastawa −/+ i podsumowanie z jawnym włączeniem. Długie listy
korzystają z wyszukiwania i przewijanej treści; akcje są poza przewijaną listą.
Definicja działa dopiero po potwierdzonym zapisie, bez próbnych poleceń przy edycji.
Nowa reguła jest domyślnie wyłączona, a użytkownik jawnie włącza ją przy zapisie.

Lista pokazuje: Wyłączona, Oczekiwanie na dane, Aktywna, Ręczna zmiana,
Oczekiwanie na potwierdzenie albo Błąd. Szczegóły podają brakujący czujnik/cel,
wcześniejszą/żądaną/odczytaną nastawę i osobne wyniki celów. Ważne komunikaty
zachowują zaakceptowany złoty kolor i ramki; Cyber Blue zachowuje podwójne
sfazowane obramowania. Grafit i Jasny mają tę samą funkcjonalność.
Panel WWW edytuje ten sam schemat i pokazuje te same stany. Nie dodajemy stałego
panelu diagnostycznego do zaakceptowanych kart urządzeń.

## Walidacja bez powtarzania pełnych zestawów

W trakcie implementacji: konkretna regresja modelu i parsera, następnie wymagane
testy aktora/workera, dziennika i tylko zmienionych ekranów. Istotne przypadki:

- kilka kontaktów, różne bazowe nastawy, już niska temperatura, brak powtórzeń;
- ręczna zmiana przed przyjęciem, w kolejce, podczas I/O i podczas przerwy;
- stare raporty bramki, brak czasu konkretnej wartości, ułamkowe czasy, cofnięcie
  czasu, źródłowy raport TRV opóźniony względem ACK, brak beacon;
- restart/utrata połączenia przy każdej fazie; nowe odczyty wszystkich uczestników;
- rozłączenie po POST, zmiana powiązania, usunięcie/wyłączenie reguły, konflikt CAS;
- ucięty zapis dziennika, błąd commit/readback, nowszy uszkodzony/future rekord,
  brak pamięci i przepełniona kolejka, bez erase i bez ślepego powtórzenia;
- wspólna kolejka z ręcznymi scenami/grupami i pilnym STOP, brak obcych ACK;
- zachowanie reguł i referencji po usuwaniu urządzeń oraz po migracji 1/2 → 3;
- rzeczywiste piksele LVGL 9.3: tylko nowe formularze/statusy i reprezentatywne
  stany PL/EN/DE w trzech skórkach, plus odpowiadający im panel WWW.

Po głównej implementacji: przegląd i kompilacja ES3C35P z audytem pamięci/stacku.
Przed nowym DEV: wymagane regresje, końcowy przegląd UI i CI dla dokładnego commita,
odbiór rzeczywistego sterowania/readback na sprzęcie oraz integralność i OTA.
Nie zastępujemy dowodu sprzętowego hostowym modelem. Obecne lokalne Cygwin nie ma
UBSan; wymagane sanitizery muszą przejść w obsługującym je środowisku przed wydaniem.

## Źródła i granice rozpoznania

Repozytorium: `dsr_ui_network.c`, `dsr_fleet.h/.c`, `dsr_actions*`,
`dsr_discovery_model.c`, `dsr_http.c`, `dsr_workspace.c`, `dsr_config.c`,
`dsr_connection.c`, `dsr_es3c35p.c` oraz istniejące testy aktora/workera/BLU.

Oficjalne materiały sprawdzono 2026-10-23. Espressif MCP mimo żądania 5.5.4 zwrócił
także latest/master; podstawą kontraktu NVS są lokalne nagłówki IDF 5.5.4 oraz
[wersjonowana dokumentacja NVS 5.5.4](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/storage/nvs_flash.html).
Znacznik konkretnej wartości i warianty jego nazwy opisuje
[BTHomeSensor](https://shelly-api-docs.shelly.cloud/gen2/DynamicComponents/BTHome/BTHomeSensor/);
czas ostatniego pakietu urządzenia opisuje
[BTHomeDevice](https://shelly-api-docs.shelly.cloud/gen2/DynamicComponents/BTHome/BTHomeDevice/).
Tryb okresowych raportów opisują
[BLU Door/Window](https://shelly-api-docs.shelly.cloud/docs-ble/Devices/BLU/dw/) oraz
[dokumentacja produktu](https://us.shelly.com/blogs/documentation/blu-door-window).
Lustrzany status TRV i jego ograniczenia opisuje
[BLU Gateway Gen3](https://shelly-api-docs.shelly.cloud/gen2/Devices/Gen3/ShellyBluGwG3/).
Odczytowy RPC TRV jest opisany w
[BLU TRV — RPC commands](https://shelly-api-docs.shelly.cloud/docs-ble/Devices/BLU_ZB/trv/).
To rozpoznanie interfejsów i projekt zmian, nie dowód zachowania fizycznych urządzeń.
