0.7.51-dev: uproszczenie aplikacji i dokończenie oprawy graficznej. Stable: brak.

Zmiany względem 0.7.50-dev (decyzje właściciela 2026-10-18):
- Usunięte: historia energii 24 h (zapis w tle do SPIFFS, zadania workera, API `/api/v1/history`, wykres w Web Panelu), strona Energia i ekran Aktywność na LCD. Zapis historii współdzielił jedyny wątek z odpytywaniem Shelly i zatrzymywał pamięć Flash przy zapisie. Istniejące pliki historii w SPIFFS nie są kasowane (wymaga osobnej zgody). Numery zadań 17/18 zostają zarezerwowane.
- Zostaje: zegar mocy "Zużycie" na stronie urządzenia (bieżący odczyt, licznik kWh z Shelly).
- Dolny pasek: zamiast Energia - diagnostyka baterii. Grupy i sceny tylko w zakładce Pomieszczenia. Dziennik aktywności pozostaje w Web Panelu.
- Pasek stanu: godzina obok Wi-Fi i baterii (czas środkowoeuropejski CET/CEST, SNTP pool.ntp.org uruchamiany automatycznie po połączeniu Wi-Fi; bez czasu pole jest puste).
- Pasek przewijania po prawej na wszystkich długich listach i formularzach.
- Ikony urządzeń do wyboru jak w aplikacji Shelly (Auto + 20, m.in. gniazdko, wentylator, termostat, TV, bojler, brama, lampy). Pole `icon` zapisywane w konfiguracji tylko przy wyborze innym niż Auto - po wyborze ikony powrót do 0.7.50 lub starszej odrzuci konfigurację, dopóki ikona nie wróci na Auto.
- Dodawanie przez Wi-Fi: karta urządzenia pokazuje nazwę z Shelly lub linię produktu, SKU, generację, IP, MAC lub znalezione kanały i wymóg hasła; lista wyników pokazuje nazwę z Shelly.
- Czytelność: najmniejsza czcionka 11 px zastąpiona 12 px, wartości w kapsułach i podpisy pól 14 px.
- Ikony Material Symbols renderowane przez Chromium (cairosvg niedostępny w środowisku budowania).

0.7.50-dev: szybsze działanie interfejsu i stabilność dużych pulpitów. Stable: brak.

Zmiany względem 0.7.49-dev (bez zmian wyglądu, bootloadera, partycji i trybu Flash):
- Odczyt (poll) bez zmian wartości nie przebudowuje już pulpitu. Wcześniej pulpit z własnym układem był przebudowywany przy każdym odczycie (błąd: pola kafelków nadpisywały model).
- Zmiana wartości jednego urządzenia przebudowuje tylko jego karty (i karty pokoi, które sumują moc), zamiast całego ekranu; przewinięcie zostaje.
- Wspólne style LVGL zamiast kilkunastu lokalnych właściwości na każdym obiekcie: 24 kafelki mieszanych urządzeń zostawiały < 2 KB z puli 128 KB i pierwsze rysowanie zawieszało się w alokacji cienia; teraz zostaje ok. 16 KB (pomiar hosta). Pula i limity bez zmian.
- Wyświetlacz: dwa bufory DMA po 20 linii (ta sama pamięć co jeden 40-liniowy) - rysowanie następnego pasa w trakcie wysyłania poprzedniego.
- Kompilacja firmware płytki z -O2 (było -Os) i memcpy/memset z ROM/newlib dla LVGL. BIN 3,29 MB -> 3,67 MB, 42% slotu wolne.

0.7.49-dev: odświeżenie wyglądu w stylu Shelly Smart Control (etap 1). Stable: brak.

Zmiany względem 0.7.48-dev (wzór: zrzuty aplikacji Shelly od właściciela):
- Wiersze urządzeń: pasek nagłówka, czarne kółko z ikoną, wartości w jednej kapsule z separatorami, jasny okrągły przycisk zasilania w obwódce; włączone urządzenie ma niebieski pierścień i niebieskie obramowanie karty z poświatą. Roleta: kapsuła góra/dół. Jasny motyw ma własne odpowiedniki kolorów.
- Strona urządzenia: zakładka "Zużycie" zamiast "Wykresy" - zegar mocy jak w aplikacji (łuk zielony-żółty-czerwony, podziałka, wskazówka, W lub kW, "Ostatnia aktualizacja", licznik kWh). Odczyt starszy niż 15 s jest pokazywany jako brak odczytu, nigdy jako stara wskazówka.
- Ustawienia > System: usunięty "Podgląd rolety • DEMO".
- Synchronizacja czasu UTC dla historii zostaje na stronie "Energia".

0.7.48-dev: termostat BLU TRV w stylu Shelly Smart Control. Stable: brak.

Zmiany względem 0.7.47-dev (wzór: zrzuty aplikacji Shelly od właściciela):
- Ekran termostatu: rząd wartości (temperatura, cel, zawór, bateria), łuk 4-30 °C od niebieskiego do czerwonego z gałką na celu, aktualna temperatura i duży cel w środku, okrągłe -/+.
- -/+ wysyłają cel 1,2 s po ostatnim dotknięciu (bez przycisku "Ustaw"); odczyt co 15 s jak w aplikacji (bez przycisku odświeżania).
- Lista urządzeń: wiersz TRV z pastylką "- cel +" po prawej.
- Poprawka: przy TRV offline lista mogła pokazać cel jako "lx".

0.7.47-dev: termostat BLU TRV, stabilny odczyt czujników BLU, wbudowane zdjęcia pomieszczeń. Stable: brak.

Zmiany względem 0.7.46-dev:
- Czujniki BLU: wartości nie znikają co 15 s. Odczyt uznawany za aktualny przez 45 s (bramka jest odpytywana co 15 s, jeden odczyt to kilka zapytań); odczyt w tle nie zasłania wartości.
- Termostat BLU TRV (przez bramkę): temperatura przy głowicy, cel, zawór, bateria; ustawianie celu 4-30 °C co 0,5 °C przyciskami -/+ i "Ustaw" (BluTrv.Call TRV.SetTarget). Pilot pokazuje to, co raportuje bramka; niepewny zapis nie jest powtarzany. Web Panel pokazuje termostat (bez ustawiania).
- Pomieszczenia: 8 wbudowanych zdjęć (Salon, Kuchnia, Sypialnia, Łazienka, Przedpokój, Piwnica, Strych, Rozdzielnia) do wyboru w edytorze; własne zdjęcia z Web Panelu działają jak dotąd.
- Poprawka: zapis z treścią spoza pola żądania nie wyłączał już ponowienia logowania digest (dotyczyło tylko nowej ścieżki TRV; provisioning AP bez zmian).

0.7.46-dev: czujniki BLU w kreatorze pod nazwą z bramki albo modelem. Stable: brak.

Zmiany względem 0.7.45-dev:
- Lista kanałów bramki: nazwa czujnika ustawiona na bramce (jeśli jest), inaczej model i końcówka MAC, np. "BLU Door/Window 0b11", "BLU TRV b567". Ta sama etykieta staje się domyślną nazwą urządzenia.
- Znane modele: BLU Door/Window (model 2), BLU TRV (model 8), potwierdzone na prawdziwej bramce; pozostałe jako "Czujnik BLU" + MAC, dopóki nie potwierdzimy numerów.
- Nazwy z aplikacji Shelly są w chmurze, nie na bramce; nazwę widoczną dla pilota ustawia się w panelu WWW bramki.

0.7.45-dev: bramka BLU Gateway Gen3 rozpoznaje czujniki, automatyczny restart po aktualizacji. Stable: brak.

Zmiany względem 0.7.44-dev (dev-v0.7.44 = kod 0.7.43):
- Czujniki BLU na bramce: pilot czyta listę z Shelly.GetComponents (stronami), bo Shelly.GetStatus bramki BLU Gateway Gen3 ich nie podaje. Dotyczy tylko urządzeń Shelly z Bluetooth.
- BLU TRV z dwiema temperaturami: pokazywana pierwsza (idx 0).
- Po zainstalowaniu aktualizacji z pilota restart następuje sam po 10 s; "Później" go wstrzymuje, "Uruchom ponownie" od razu.

0.7.43-dev: naprawa instalacji OTA z pilota (kod 12/11: limit czasu pobierania obrazu). Stable: brak.

Zmiany względem 0.7.42-dev:
- Pobieranie obrazu aplikacji miało ten sam łączny limit 30 s co manifest; 3,3 MB z zapisem Flash nie mieściło się. Teraz: przerwanie po 30 s bez postępu albo po 10 min łącznie.
- Partycja docelowa jest kasowana sektor po sektorze w trakcie zapisu (zamiast całej z góry w środku pobierania).
- Podpis, SHA-256 i rozmiar weryfikowane jak dotąd.
- Uwaga: wersje do 0.7.42 mają ten błąd, więc na 0.7.43 trzeba przejść raz przez USB (0x20000) lub Web Panel.

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
