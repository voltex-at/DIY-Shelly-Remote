DIY SHELLY REMOTE — UI REVIEW v1
Gałąź: ui/shelly-smart-control

Są to rzeczywiste klatki LVGL 9.3.0 z kodu projektu, 320 x 480 px.
Nie są to ilustracje koncepcyjne ani zrzuty strony HTML.
Pakiet NIE zawiera firmware do wgrania. Projekt czeka na ocenę wyglądu.

01-home.png — Home / Mój pulpit, karty ulubionych urządzeń.
02-rooms.png — Pomieszczenia.
03-devices.png — Wszystkie urządzenia, wspólny komponent kafelka.
04-room-salon.png — Pomieszczenie Salon i jego urządzenia.
05-device-details-on.png — Szczegóły urządzenia, stan ON i pomiary.
06-device-details-off.png — Ten sam widok w stanie OFF.
07-device-offline.png — Brak łączności, ukryte niepotwierdzone pomiary.
08-live-empty.png — Tryb rzeczywisty bez konfiguracji, bez fikcyjnych danych.
contact_sheet.png — Zestawienie pierwszych sześciu klatek w skali 1:1.

Wszystkie wypełnione widoki mają oznaczenie DEMO na górze i na dole.
Nazwy pomieszczeń, urządzeń i pomiary w nich służą wyłącznie ocenie projektu.
Dane są w tests/ui/test_ui_review.c, którego firmware ESP-IDF nie kompiluje.
Prawdziwy model widoku pobiera stan z istniejącego modułu Shelly; nie wymyśla
pomieszczeń, odczytów napięcia/natężenia ani historii energii.

Odniesienia: aktualnie publikowane oficjalne materiały Shelly, Google Play
oraz instrukcje My Home i Shelly 1PM. Zasoby aplikacji nie są kopiowane:
piktogramy są własnymi rysunkami LVGL, Roboto pochodzi z osobnego projektu
Google Fonts i ma dołączoną licencję Apache 2.0.

Do oceny: układ, proporcje, szare karty, górne zakładki, niebieskie stany,
dolna nawigacja i czytelność. Grupy, sceny, energia i edycja układu nie są
tu rozwijane; ich nieaktywne miejsca nie oznaczają gotowych funkcji.

Po akceptacji projekt zostanie zastosowany w pozostałych ekranach.
Dotychczasowy main, sterowniki LCD/dotyku/podświetlenia i komunikacja Shelly
pozostają zachowane. Punkt odniesienia: reference/0.2.0-local-dev.
