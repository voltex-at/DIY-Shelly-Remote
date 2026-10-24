# Responsywność i kandydat 0.7.77-dev — 2026-10-25

Właściciel zlecił poprawę pozostałych ekranów i publikację DEV, po której
sam sprawdzi płytkę. To kandydat do odbioru sprzętowego, nie potwierdzenie
czasów reakcji na ES3C35P. Baza: `dev-v0.7.76` / `8557e72`.

## Odtworzone źródła zbędnej pracy

| Wyzwalacz | Poprzednie zachowanie | Zmiana |
| --- | --- | --- |
| Otwarcie Zużycia, ustawień Shelly lub konfiguracji BLU | Budowa pełnego Dashboardu przed natychmiastowym zastąpieniem go docelową stroną | Przygotowanie modelu bez budowy niewidocznego Dashboardu |
| Pomieszczenia z dolnej nawigacji ekranów systemowych | Dashboard, potem Pomieszczenia | Bezpośrednio Pomieszczenia |
| Wiek sygnału BLU w szczegółach | Usuwanie całego widoku | Aktualizacja istniejącej etykiety; nagłówek Wi-Fi, stan, powiązanie i skórka nadal wymuszają właściwe odświeżenie |
| Pomiary grup przekaźników/dimmerów/RGB | Odtworzenie wszystkich kontrolek | Aktualizacja istniejących pól dla samych zmian liczbowych; zmiana stanu/możliwości/tożsamości zachowuje pełną ścieżkę |
| Zużycie co 5 s, bez nowych danych | Usunięcie i narysowanie całego wskaźnika | Zmienia się tylko wiek odczytu; przejście świeże/nieaktualne i prawdziwe dane nadal aktualizują widok |
| Rzeczywista zmiana Zużycia podczas dotyku | Usunięcie przycisku spod palca | Odroczenie przebudowy do puszczenia palca |
| Wynik grupy/sceny co 1 s | Odtworzenie ekranu również po zakończeniu | Odświeżenie przy zmianie rzeczywistego wyniku |
| Postęp i odliczanie OTA co poll | Ponowne przypisanie identycznych tekstów | Przypisanie dopiero po zmianie wyświetlanej wartości |

Regresje odtworzyły usuwanie elementów, pośrednie budowy Dashboardu oraz
rysowanie niezmienionego procentu OTA. Dodatkowy przypadek zmiany Wi-Fi
wykrył potrzebę zachowania pełnego odświeżenia nagłówka przy szybkiej
aktualizacji wieku sygnału. Poprawiono tę bramkę przed wydaniem.

Nie zmieniono priorytetów FreeRTOS, parametrów dotyku, SPI, buforów ekranu,
LVGL 9.3.0 ani ESP-IDF 5.5.4. Przegląd nie wykazał podstaw do takich zmian.
Wywołania RPC nadal wykonuje worker, a obiekty LVGL mają jednego właściciela.

## Zachowane wcześniejsze poprawki

- Odczyt Cover po niepewnej komendzie dostaje jedną natychmiastową okazję
  przed rutynowym tłem. Odczyty ruchu pomijają dodatkowe I/O nazwy; logi SDK
  nie zawierają hostów, haseł, nagłówków ani treści odpowiedzi.
- Restart OTA poza własną blokadą poll, bez ponownego odliczania 5 s;
  przyjęte operacje i oczekująca intencja TRV kończą się przed restartem.
- Paski pamięci wskazują zajętość/szczyt stosu, tekst podaje wolną pamięć.
- Pomieszczenia zachowują zdjęcia i zaakceptowane ramki; odstęp między
  rzędami wynosi 8 px od wyższej ramki w parze. Odświeżanie nie usuwa zdjęć.
- Karty BLU nie są odtwarzane z powodu samego wieku sygnału na Dashboardzie,
  liście Urządzeń i w pomieszczeniu. Pozycja Cover pozostaje polem aktywnym.

Szczegóły tych zmian i granice diagnozy transportu:
[raport po 0.7.76](RESPONSIVENESS_AUDIT_0_7_76_PL.md).

## Dowody i warunki publikacji

Podczas zmian uruchamiano tylko dotknięte przypadki. Po serii przeszły
`test_priority_ui`, `test_screen_work_ui`, `test_native_ota_ui`,
`test_ota_restart_admission_ui`, `test_shelly_settings_ui` i `test_blu_ui`.
Przypadki obejmują PL/EN/DE i trzy skórki tam, gdzie dotyczą wyglądu;
radio, storage, FreeRTOS i restart w testach hosta są modelowane.
Niezależny przegląd źródła nie wskazał dalszych konkretnych usterek.

Pierwszy przebieg CI wykrył brak adaptera `dsr_http_refresh` w fixture
runtime automatyzacji po dodaniu ścieżki telemetrycznej Cover do workera.
Uzupełniono wyłącznie fixture: nieoczekiwane wywołanie Cover przerywa ten
test kontakt/TRV. Wszystkie 19 powiązanych scenariuszy runtime przeszło
lokalnie; wymagane sanitizery pozostają w CI Linux.

Dokładny commit kandydata musi otrzymać pełne CI 5/5 przed utworzeniem tagu.
Chroniony workflow buduje i podpisuje niezmienne zasoby. Przed przekazaniem
linku właścicielowi sprawdzane są anonimowe pobrania katalogu, manifestu,
całego BIN, podpis Ed25519, SHA-256, pochodzenie pakietów i ich zgodność.
Wyniki tych zewnętrznych etapów zapisujemy w issue odbioru, bez zmiany
przetestowanego commita. Procedura: [checklista](RELEASE_CHECKLIST_PL.md).

Pozostaje odbiór właściciela: OTA 0.7.76→0.7.77, fizyczny restart i dane po
nim, płynność/dotyk wszystkich stron oraz diagnoza rzeczywistego timeoutu
Cover. Nikt nie flashował, nie resetował ani nie kasował danych urządzenia
w ramach lokalnych testów. Publiczny DEV nie jest oznaczany jako wydanie
sprawdzone na sprzęcie; main i Stable pozostają poza zakresem.
