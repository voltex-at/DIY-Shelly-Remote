# Matryca funkcji — 0.7.70-dev

Aktualny kod: `work/dev-0.7.70`, wydanie `dev-v0.7.70` (`d43da439`).
Pełna lista zleceń, źródeł i niespójności: [PROJECT_STATUS_PL.md](PROJECT_STATUS_PL.md).
Dalsza koordynacja: [issue #74](https://github.com/voltex-at/DIY-Shelly-Remote/issues/74).

„Kod” oznacza implementację; CI sprawdza modele hosta i kompilację. Odbiór na
ES3C35P dotyczy konkretnej wersji i nie przechodzi automatycznie na nowy layout.

| Obszar | Kod / zakres | Potwierdzenie fizyczne |
| --- | --- | --- |
| Start, LCD, dotyk i podświetlenie | Tak | Historycznie 0.1.0; nowy UI do odbioru |
| Wi-Fi i pojedynczy Shelly ON/OFF | Tak | Historycznie 0.6.x |
| Wiele Shelly, trwałe ID/kanały i usuwanie | Tak, do 16 urządzeń | W starszych wersjach zgłoszone błędy; aktualny retest do wykonania |
| Pomieszczenia, dashboard i edytory LCD/WWW | Tak; UI przebudowany 0.7.70 | Aktualny układ i trwałość do odbioru |
| Trzy skórki i PL/EN/DE | Cyber Blue, Grafit, Jasny; migracja Midnight | Aktualne ekrany/kontrast/dotyk do odbioru |
| Rolety, pozycja i pilny STOP | Tak; animacja/gest w trzech skórkach | Sterowanie potwierdzone 0.7.37; aktualny gest/STOP do odbioru |
| Dimmer, RGB/RGBW, H&T | Tak; H&T read-only; nowe widoki Cyber 0.7.70 | Do odbioru |
| BLU Motion/Door/Window i TRV przez bramkę | Tak, wspierane klasyczne modele | Do odbioru na aktualnym gateway/RF |
| LAN / Wi-Fi AP / BLE onboarding | Tak; dwie zgody AP, kontrola tożsamości i restore LAN | LAN historycznie potwierdzony; aktualny AP/BLE do odbioru |
| Ustawienia Shelly i BLU bonding/PIN/GATT | Tak, capability-driven Gen2/3/4 i klasyczne BLU | Do odbioru |
| Ręczne grupy i sceny | Switch grupy, Switch/Cover sceny; LCD/WWW | Wykonanie na kilku urządzeniach do odbioru |
| Aktywność | Backend 128 wpisów RAM i API; widok LCD niepotwierdzony | Zaległość/zakres w #74 |
| Historia energii i wykresy | Usunięte decyzją właściciela w 0.7.51 | Nie przywracać na podstawie starych issues |
| Bateria | Kalibracja BAT; szacowany ~SOC i animacja trendu napięcia | Multimetr, ładowanie i brak baterii do odbioru |
| Web Panel i osobny język przeglądarki | Tak; pairing i zgody pozostają | Pełny aktualny przebieg do odbioru |
| SPIFFS i wznowienie formatowania | Tak, postęp/diagnostyka | Formatowanie 0.7.37 potwierdzone; wznowienie po zaniku zasilania do odbioru |
| Restart z LCD | Od 0.7.69 lokalne potwierdzenie, niezależna rezerwacja i RTC watchdog; WWW/OTA nadal wyłączne | Zajęte BLE/Wi-Fi i czas resetu do pomiaru |
| OTA z LCD i WWW | Podpis Ed25519, TLS/model/wersja/kanał/hash, dwa sloty | Instalacja 0.7.53 potwierdzona; aktualne przerwanie/rollback do odbioru |
| Wydanie DEV | Jeden build na tag, CI 5/5, środowisko właściciela, kontrola publicznych plików | Nie zastępuje testu urządzenia |
| Eksport/backup i reset fabryczny | Niewdrożone | Zaległość System w #74 |

Poza dostarczonym zakresem: Gen1, Midnight jako skórka, automatyczne/chmurowe
reguły, nowe rodziny BLU/Zigbee/Matter oraz pełny onboarding konta Shelly Cloud.
Kalibracja baterii nie zmienia braku wiarygodnego sygnału obecności/ładowarki.
NVS prototypu pozostaje nieszyfrowane; zmiana wymaga osobnej analizy migracji.

Nie wykonywano nowego testu sprzętowego podczas porządkowania. Stare rendery
i raporty wyników usunięto; testy źródłowe i generatory pozostają w repozytorium.
