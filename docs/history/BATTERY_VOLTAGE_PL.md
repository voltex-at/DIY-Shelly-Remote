# Napięcie BAT na żądanie — etap #6/#25

Wersja 0.7.8-battery-voltage-dev rozwija istniejący backlog zaakceptowany w
[decyzji właściciela](https://github.com/voltex-at/DIY-Shelly-Remote/issues/25#issuecomment-6206200976).
Priorytetem pozostaje sprzętowy odbiór SPIFFS PR #33 (0.7.7). Ta gałąź nie
zastępuje aktualnie testowanego obrazu. Bez nowej zgody nie wykonywać erase.

## Implementacja

GPIO8, według [schematu producenta](https://www.lcdwiki.com/3.5inch_ESP32-S3_Display),
odbiera BAT przez nominalny dzielnik 100k/100k. Sterownik sprawdza mapowanie
GPIO do ADC1, konfiguruje 12 bitów i tłumienie 12 dB. Odrzuca pierwszą konwersję,
wybiera medianę dziewięciu kolejnych i korzysta z kalibracji curve fitting IDF.
Napięcie BAT to dwukrotność skalibrowanego napięcia ADC. Kalibracja ADC nie
uwzględnia tolerancji dzielnika ani nie potwierdza typu ogniwa.

Pomiar uruchamia przycisk w Ustawienia → Bateria i zasilanie albo wejście przez
ikonę baterii i przycisk pomiaru. Istniejący worker obsługuje żądanie również
bez Wi-Fi i bez workspace. Brak nowego taska, timera, odczytów startowych lub
okresowych; ADC i kalibracja są zwalniane po każdym żądaniu. UI zachowuje
ostatni wynik i jego wiek. Po 30 s nagłówek pokazuje znak zapytania. Błąd ADC,
brak kalibracji i zajęty worker mają osobne stany; nie powstaje zgadywane mV.

Nie ma jeszcze procentów, progu niskiej baterii, czasu pracy, okresowego
monitorowania ani animacji ładowania. Wzrost napięcia nie dowodzi ładowania.
Nie ustalono wiarygodnego sygnału obecności ogniwa, USB ani statusu ładowarki.

## Dowody i odbiór

Host: 3367 kontroli adaptera ADC i rzeczywistego workera, w tym 100 cykli
init/read/delete, błędy, brak kalibracji i działanie offline; natywne C/LVGL
rendery PL/EN/DE, stary wynik, Back podczas żądania i 40 cykli bez ubytku heap.
To nie jest potwierdzenie napięcia na fizycznej płytce. Pełne CI targetu oraz
rzeczywisty odbiór wymagają osobnych dowodów dla SHA tej gałęzi.

Po odbiorze SPIFFS: zapisać typ ogniwa i rewizję PCB, porównać BAT oraz GPIO8
z multimetrem przy bezczynności i aktywnym LCD/Wi-Fi/BLE, ustalić tolerancję,
sprawdzić powtarzalność, pamięć, touch/wake i współpracę RF. Dokumentacja IDF
wskazuje zależność ADC i źródła entropii RNG; brak stałego ADC nie zastępuje
weryfikacji współpracy na urządzeniu. Bez hardware nie zamykać #6/#25.
