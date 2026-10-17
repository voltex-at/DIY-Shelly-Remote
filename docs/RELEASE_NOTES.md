0.7.42-dev: czujniki Shelly BLU (Motion, Door, Window) odczytywane przez bramkę Shelly w sieci LAN. Stable: brak.

Zmiany względem 0.7.40-dev (dev-v0.7.41 zawiera kod 0.7.40, bez BLU):
- Shelly BLU: Dodaj urządzenie > Skanuj sieć Wi-Fi > wybierz Shelly, który już odbiera czujnik (np. Plus/Pro/Gen3 lub BLU Gateway Gen3) > kanał "Czujnik BLU N". Pilot odpytuje bramkę co 15 s (BTHomeDevice/BTHomeSensor), tylko odczyt: ruch, otwarcie, jasność, bateria, temperatura, wilgotność, RSSI.
- Tożsamość czujnika to identyfikator bramki + MAC czujnika; inny czujnik na tym samym slocie bramki jest odrzucany.
- Rejestr połączeń: bez czujnika BLU zostaje schemat 6 (zgodny z 0.7.40, także przy automatycznym rollbacku). Po dodaniu czujnika BLU zapisywany jest schemat 7, którego 0.7.40 nie odczyta (odmowa bez kasowania danych); przed ręcznym powrotem do 0.7.40 usuń czujniki BLU.
- Ekran skanu BLE wskazuje, że czujnik BLU dodaje się przez bramkę.
- Web Panel: karta BLU w stanie rzeczywistym.

0.7.40-dev: naprawa OTA z pilota (rekordy TLS 16 KiB, przekierowanie CDN GitHub), katalog Dev stronami, kody błędów OTA. Stable: brak.

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
