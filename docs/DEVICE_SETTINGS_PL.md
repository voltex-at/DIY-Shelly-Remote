# Ustawienia wybranego urządzenia — integracja #29A

Port istniejącego [PR #34](https://github.com/voltex-at/DIY-Shelly-Remote/pull/34),
źródło `693d869f54c147f16a90591f9d1bd90ace11a7f8`, na bazę PR53 `267c32f`.
Zachowuje jego widok i lokalny formularz, uzgadniając je z aktualnym rejestrem
wielu urządzeń, naprawionym CRUD i dokładnym trwałym ACK. Bez zmiany wersji
projektu, schematu workspace/NVS, partycji, transportu i migracji bindingów.

## Rzeczywiste zachowanie

- `Sterowanie → Ustawienia` otwiera wybrany trwały ID karty. Zakładka Sterowanie
  i Wstecz wracają do tego urządzenia. Reorder urządzeń/pokoi zachowuje cel;
  usunięty ID daje bezpieczny powrót, nigdy podmianę na inny indeks.
- Status, ostatnie dostępne W/V/A, wiek próbki oraz zapisane ID i rodzina API
  pochodzą z przypisanego rzeczywistego endpointu. Gen2+ nie oznacza konkretnej
  generacji 2. Dokładny model/firmware nie są odgadywane z nazwy karty/BLE.
  Brak pola, zero, offline i brak obsługiwanych pomiarów pozostają rozróżnione.
- `Odśwież status` rozwiązuje ID ponownie względem aktualnej wspólnej konfiguracji
  i istniejącego prywatnego bindingu. Używa wyłącznie obecnego identity/GET,
  zachowując błędy auth/rate i polecenia pozostałych Shelly. Powtórzenie dołącza
  do aktualnego odczytu. Nie tworzy SET ani osobnego transportu/zadania.
- Obca karta z tym samym hostem/kanałem, zmieniony locator i karta bez bindingu
  mają niedostępne odświeżanie. Nie odczytują aktualnie podłączonego innego Shelly.
  Dawny jednodevicowy ekran bez dostępnego workspace zachowuje odczyt jedynego
  rzeczywistego bindingu; ID0 nie wybiera między wieloma endpointami.
- `Edytuj kartę` zmienia tylko lokalną nazwę i pomieszczenie, wspólne z WWW.
  Host, kanał, typ i sekrety nie są polami tego formularza. To nie zmiana
  konfiguracji w samym Shelly; RPC settings pozostają osobnym zakresem #23.
- Zapis używa obecnego niezmiennego snapshotu i CAS. Dopiero dokładny trwały ACK
  czystego draftu z tej samej generacji ekranu wraca do Ustawień tego samego ID.
  Późniejsza edycja B, IO i konflikt pozostają w formularzu. Pending blokuje
  przedwczesny powrót; niezapisana zmiana wymaga jawnego odrzucenia.

Sam port #29A nie tworzy historii ani nie zamyka całego #29. Kolejny pion
#22 jest opisany w [ISSUE22_HISTORY_PL.md](ISSUE22_HISTORY_PL.md), z oddzielnymi
bramkami P0/target/physical.

## Dowody programowe i granice

`tools/test_ui.py --ubsan-only` wykonuje faktyczny LVGL/C: pointery zakładek,
selected GET i niezależny błąd drugiego urządzenia, offline/no metering, lokalny
zapis/pokój, UTF-8/47 bajtów, klawiatura, deferred IO/retry, Save A/edit B,
Web CAS, reorder i usunięty ID, ponowne użycie hosta oraz zmianę locatora,
PL/EN/DE, kontekst pokoju i 40 cykli edytuj/Wstecz bez utraty warmed LVGL heap.
Pozostają wszystkie istniejące testy CRUD, fleet/migracji, ACK i AP.
Lokalny wynik UBSan: **4086 kontroli /137 kadrów**; osobny review **2635 /24**.
Warmed heap formularza **97432 → 97432 B** po 40 cyklach. Hostowy model mobilny
ma 6184 B (to nie target BSS); model jest ograniczony compile-time do 8192 B.
Pełne `tools/test.py --ubsan-only` i 265 testów config przechodzą na tej bazie.

Kadry `230-*`–`240-*` są prawdziwymi renderami 320×480 z badge TEST / DEMO
i syntetycznymi tożsamościami. SDK/radio/worker/workspace to jawne modele.
Host nie dowodzi retencji fizycznej Flash, działania radia ani faktycznego
sterowania Shelly. CI exact-head, target BSS/stosy i test sprzętowy są osobnymi
bramkami. Bez main/Stable, erase lub operacji na urządzeniu właściciela.

## Odbiór sprzętowy po zatwierdzeniu paczki

1. Dwa istniejące Shelly: ON/OFF/readback i odrębne W/V/A przed oraz po wejściu
   do Ustawień. Odśwież A przy offline/błędzie B; B nie może zmienić tożsamości.
2. A → Ustawienia → Edytuj kartę → nazwa/pokój → Zapisz. Powrót po ACK do A,
   zgodność WWW i retencja po jawnym restarcie. Binding, kanał i B zachowane.
3. Pending/błąd zapisu, niezapisana zmiana i Anuluj/Odrzuć, równoległa edycja WWW,
   reorder urządzeń/pokoi, brak bindingu i długie nazwy PL/EN/DE.
4. Wi-Fi/BLE coexistence, polling, dotyk i kilkadziesiąt wejść/powrotów. Zachować
   exact SHA/wersję i log pamięci bez sekretów. Brak takiego odbioru tej wersji.
