0.7.41-dev: pasek stanu w nagłówku (godzina, bateria z animacją i szacunkiem procentu). Stable: brak.

Zmiany względem 0.7.40-dev:
- Ustawienia urządzenia: model i wersja firmware Shelly (Gen2+) z odpowiedzi `/shelly`, którą pilot i tak pobiera przy każdym odświeżeniu (bez dodatkowych zapytań). Wartości informacyjne: nie wpływają na sterowanie ani na wynik odświeżenia; brak odczytu pokazuje „nieodczytany". Uptime i siła sygnału Wi-Fi Shelly nie są jeszcze pobierane.
- Grupy i Sceny: gdy lista jest pusta, ekran wskazuje, gdzie je utworzyć, i ma przycisk „Otwórz Panel WWW". Tworzenie grup i scen na samym LCD nie jest zaimplementowane.
- Godzina HH:MM pod ikoną Wi-Fi. Czas jest synchronizowany automatycznie po połączeniu z Wi-Fi (jedno zapytanie SNTP do pool.ntp.org, ponawiane co 5 min przy błędzie); do czasu synchronizacji widać `--:--`. Strefa domyślna: Europa/Wiedeń (CET/CEST), zmiana w buildzie przez `-DDSR_TIMEZONE_POSIX`.
- Bateria: ikona z animowanym wypełnieniem i procentem oznaczonym `~`. To SZACUNEK z napięcia BAT dla pojedynczego ogniwa Li-ion, nie pomiar stanu naładowania. Pomiar napięcia co 20 s; starszy niż 60 s pokazuje `?`. Poniżej 10% wypełnienie pulsuje. Nie ma animacji ładowania, bo płytka nie daje wiarygodnego sygnału ładowania.

Poprzednio, 0.7.40-dev: naprawa OTA z pilota (rekordy TLS 16 KiB, przekierowanie CDN GitHub), katalog Dev stronami, kody błędów OTA. Stable: brak.

Zmiany względem 0.7.37/0.7.38:
- TLS: bufor odbioru 16 KiB w PSRAM; github.com wysyła ~6 KB nagłówków w jednym rekordzie, 4 KiB powodowało błąd I/O (kod 16/4 w 0.7.39).
- OTA z pilota: akceptuje przekierowanie na objects.githubusercontent.com (przyczyna FAIL w 0.7.37); Dev czyta katalog po jednym wydaniu na stronę; LCD pokazuje kod błędu, log USB linię "native_ota".
- BLE: rozpoznaje Shelly Gen2+ bez UUID RPC w reklamie; więcej informacji na liście skanu.
- Pierwsze formatowanie i wznowienie przerwanego formatowania SPIFFS z postępem na LCD.
- Ekran startowy z logo, tło aplikacji, nowy styl "Jasny".
- Nowe instalacje startują po angielsku; nazwy pomieszczeń i urządzeń nie są tłumaczone.
- Edytor pomieszczenia mieści się bez ucinania tekstu (PL/EN/DE).

Instalacja:
- Pierwsza instalacja USB: DIY-Shelly-Remote-es3c35p-FACTORY-CLEAN-16MB.zip od 0x0 (kasuje dane).
- 0.7.39 lub starsza: raz wgraj diy_shelly_remote.bin przez Web Panel (Zezwól na OTA) lub USB app-only; potem Ustawienia > Firmware Update > kanał Dev.

Znane ograniczenia: brak wydania Stable; test fizyczny OTA/rollback, BLE i wielu Shelly w toku.
