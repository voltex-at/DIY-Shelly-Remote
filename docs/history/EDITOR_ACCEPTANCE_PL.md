# P0: powrót po zapisie i potwierdzane usuwanie — 0.7.21

Zakres istniejących #28/#6, na podstawie [fizycznego wyniku właściciela](https://github.com/voltex-at/DIY-Shelly-Remote/pull/33#issuecomment-6213932010)
z16.10.2026 10:54UTC. Właściciel przywrócił kompletny preformatowany SPIFFS;
edytor i odczyt modelu wróciły. [Dalszy wynik11:02UTC](https://github.com/voltex-at/DIY-Shelly-Remote/pull/33#issuecomment-6214082938)
potwierdza pokoje zachowane po resecie i działający Web Panel na0.7.18.
To NIE dowód formatu całkowicie blank partycji, pełnego TLS/pairing czy zakończenia #28. Nie powtarzać erase,
nie przywracać ponownie dsrdata, nie szukać prywatnego nagrania/zdjęć poza rozmową.

## Trzy naprawy

1. Tylko świeży exact ACK własnego zapisu pozwala automatycznie wrócić. Nadal
   wymagane revision+1 i pełne porównanie published z immutable submitted A.
   Po aktualizacji baseline/revision także cały draft musi być czysty. Zmiany B
   podczas zapisu A, CAS/IO, superseded Web write i snapshot mismatch zostają
   w edytorze. Stary save_confirmed nie jest triggerem nawigacji. Capture target
   i generation ekranu przy accepted submit zapobiega późnemu przeskokowi po
   ręcznej nawigacji/odrysowaniu. Appearance pozostaje na swoim ekranie.
2. Generic dashboard/room ma widoczne od razu48px `Usuń pulpit` / `Usuń
   pomieszczenie` za nazwą, nie ukryte przy y900/y420. Inne pola przesunięte,
   scroll i osobne dolne Wstecz/Zapisz zachowane.
3. Generic delete najpierw pyta `Usunąć "Nazwa"?`. ASCII cudzysłowy są obsługiwane
   przez przypięty font Roboto; brak kwadratów za nieobecne glyphy. Anulowanie nie mutuje draftu.
   Potwierdzenie ponownie sprawdza pending, membership i referencje; zmienia
   tylko draft. Osobne Zapisz→ACK jest jedyną publikacją. Ostatni pulpit nie
   znika, usunięcie aktywnego wybiera istniejący ID. Pokój z devices.room lub
   dowolną kartą room jest chroniony. Sceny odnoszą się do urządzeń, nie mają
   nowego wymyślonego pola room. CAS chroni przed nowszymi powiązaniami WWW.

| Wejście / zapis | Powrót po świeżym ACK i czystym draft |
| --- | --- |
| Generic edycja pulpitu/pomieszczenia | Ta sama lista kolekcji |
| Lista kolekcji | Menu edytora |
| Menu edytora HOME | HOME |
| Registry z Ustawień | Ustawienia |
| Context Add z Rooms | Rooms z nowym opublikowanym modelem |
| Context Edit z Rooms | Rooms |
| Context Edit ze szczegółu pokoju | Ten sam trwały ID, rozwiązany ponownie po odczycie modelu |
| Usunięty pokój szczegółowy | Rooms (fallback po braku ID) |
| Generic potwierdzone usunięcie + Save | Ta sama lista kolekcji |
| Wybór pulpitu startowego | HOME |
| Appearance | Bez auto-wyjścia; dotychczasowe A/B guards |

Bez zachowywania starego indeksu pokoju ani wskaźników LVGL przez przebudowę.
Hide/screen reset czyści wskaźniki modalne. Długa nazwa w generic modal ma
ograniczony dwuliniowy nagłówek z ellipsis i oddzielony opis; przyciski48px.
To nie zmiana formatu danych, SPIFFS/NVS, auth, OTA ani partycji.

## Dowody hostowe

Niezależny recenzent przygotował regresje; root przeczytał i powtórzył genuine
LVGL UBSan3223kontrole/111zrzutów. PL/EN/DE rzeczywiście przełączone, maksymalna
47-znakowa nazwa asertywnie zachowana pod ellipsis, geometria bez overlap,
przyciski48px;40cykli anulowania heap104456→104456. Opóźniony ACK/A-B/IO/CAS/
obcy publish/generation, pending delete, anulowanie, active/last dashboard,
referencje i stable-ID context objęte. Provider workspace jawnie MODEL.
Native/config265 UBSan PASS. Koordynator niezależnie przejrzał produkcję i audit
targetu bez blockerów; checker own-frame smoke2positive/8negative PASS.
Rzeczywiste target frames/CI/artefakt w opisie PR, nie SDK/LVGL/cJSON peak.

## Krótki fizyczny retest — bez erase

Po świadomej instalacji dokładnego deweloperskiego app BIN z SHA podanym w PR,
z zachowaniem obecnej tablicy partycji/NVS/dsrdata:

1. Zanotować app version/SHA oraz boot log. Bez kasowania lub kolejnego
   przywracania obrazu SPIFFS. Odczytać storage/workspace/HTTPS status; nie
   uznawać dostępu do formularza za HTTPS READY.
2. HOME ołówek → Pulpity → edytuj → zmień nazwę → Zapisz. Sprawdzić powrót
   dopiero po udanym zapisie, nową nazwę na liście. Rooms Add `Salon`→Save→Rooms.
3. Otworzyć pokój z listy i ze szczegółu, zmienić nazwę, zapisać: właściwy
   kontekst. Potem restart i ponowny odczyt zachowanego pokoju/nazwy/zdjęcia.
4. Sprawdzić widoczne Usuń, Anuluj bez zmiany, następnie świadome potwierdzenie
   usunięcia nieużywanego testowego wpisu i osobne Save. Ostatni pulpit i pokój
   używany przez urządzenia/karty muszą odmówić; urządzenia/sceny zachowane.
5. Po udostępnieniu HTTPS: pairing/fingerprint i konflikt WWW↔LCD; Wi-Fi,
   LAN/ON-OFF/BLE bez regresji. Błąd zapisu zachowuje draft, nie wyrzuca do HOME.

Nie wykonywano tych operacji na sprzęcie w testach hostowych. Faktyczny blank
format, pełne TLS/pairing i fizyczny dotyk/retencja nowego UI nadal wymagają dowodu.
Prywatne zdjęcia/logi z SSID/MAC/IP/sekretami poza publicznym repo. Brak merge,
auto-merge lub Stable bez osobnej zgody właściciela.
