# Nazwy zdjęć zgodne z istniejącym SPIFFS

Test oryginalnego IDF 5.5.4 wykazał, że dotychczasowe `p<32hex>.rgb` ma37
bajtów (38 z ukośnikiem), a zachowane `CONFIG_SPIFFS_OBJ_NAME_LEN=32` obejmuje
również końcowe NUL. Oryginalny VFS odrzucał otwarcie takiego pliku.
Nie wolno rozwiązywać tego przez zmianę geometrii istniejącego SPIFFS.

Zapis używa teraz `p<22base64url>.rgb`:27 bajtów,28 z ukośnikiem,29 z NUL.
To pełne128 bitów dotychczasowego identyfikatora treści, bez skracania hasha.
Workspace i Web API nadal przechowują dokładnie te same32 małe cyfry hex.
Nowy codec jest ścisły i odrzuca niekanoniczne padding bits, inne znaki,
niezgodne długości i ścieżki. Nie zawiera separatorów katalogów.

Loader i bezpieczne zbieranie zdjęć obsługują obie konwencje nazw. Referencja
w dowolnym z dwóch poprawnych checkpointów chroni obie nazwy tego samego ID;
uszkodzony/future checkpoint nadal blokuje cleanup. Loader nie tworzy dwóch
wpisów cache dla obu nazw. Nie zmienia się CRC/checkpoint schema, identyfikator,
hash pikseli, format RGB, partycja, mount, ustawienia użytkownika ani NVS.
Nie przeprowadza się automatycznego rename, konwersji lub formatowania.

Po downgrade starszy firmware nie wyświetli zdjęć zapisanych pod nową,
krótszą nazwą. Ich pliki oraz identyfikatory w workspace pozostają zachowane;
stary cleanup nie rozpoznaje krótkich nazw i ich nie usuwa. Ponowne uruchomienie
wersji obsługującej obie nazwy przywraca ich odczyt. Nie wymaga to migracji
geometrii ani formatowania SPIFFS.

Testy actual config/codecu porównują258 wektorów z niezależnym Python base64,
w tym wszystkie128 pojedynczych bitów oraz losowe128-bit IDs. Pokrywają
padding/traversal/obie nazwy/dual-checkpoint cleanup i corrupt preservation.
Oryginalny IDF/SPIFFS test odtwarza odrzucenie starej nazwy bez mutacji Flash,
a nowa mieści się w zachowanej geometrii; plik kontrolny pod tą nazwą przetrwa
GC i210 process-exit cuts. To test nazwy i trwałości pliku na modelu NOR,
nie kompletny sprzętowy odbiór uploadu/renderowania zdjęć.

`tools/test_photo_names.py` dodaje 1000 deterministycznych ID z niezależną
wyrocznią Python, wszystkie pozycje alfabetu i zapis/odczyt w tym samym
buforze. Siedem osobnych procesów wykonuje rzeczywisty loader na prawdziwych
plikach RGB i SHA-256: obie nazwy jednego ID, uszkodzenie każdej z nazw,
OOM pierwszej/obu prób z odzyskaniem, nadmiarowy bajt oraz 16 różnych ID
z dwiema nazwami każdy. Kolejność katalogu, błędy alokacji i dostawcy SDK
są modelowane; asercje sprawdzają również zamknięcie plików/katalogu,
zwolnienie odrzuconych buforów i dokładnie jeden wpis cache na ID.
