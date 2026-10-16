# Trwała jasność i wygaszanie — #6/#24

Podetap istniejących globalnych Ustawień. Nie zamyka #6/#24 ani nie zastępuje
pełnego natywnego podpisanego OTA. P0 SPIFFS PR #33 nadal wymaga odbioru płytki.

Ustawienia ekranu zapisują jasność i timeout w istniejącym rekordzie NVS, bez
nowego schematu i bez zależności od SPIFFS. Zapis wykonuje istniejący worker:
odczytuje najnowszy pełny rekord, zmienia wyłącznie dwa pola, zachowuje Wi-Fi
i nie zapisuje defaultów po błędzie odczytu. UI nie przesyła haseł w żądaniu.
Przy braku gotowości, zajętości lub błędzie zapis nie jest deklarowany jako sukces.

Jasność użytkowa ma minimum 10%. Starsze zapisane 0–9% stosowane są jako 10%
bez automatycznej migracji lub zapisu. Timeout 0 oznacza wyłączone automatyczne
wygaszanie. Ręczne wygaszenie i dotyk wybudzający zachowują tłumienie pierwszej
akcji, aby przypadkowo nie wysłać polecenia do urządzenia.

Należy oddzielić potwierdzony zapis NVS od zastosowania ustawienia przez PWM:
błąd sterownika nie jest komunikatem o udanym zastosowaniu. Start nie może
ponownie nadpisać wczytanej jasności stałą 70%.

Odbiór sprzętowy po SPIFFS: zapisać jasność, uruchomić ponownie, sprawdzić
wybudzenie z tą samą jasnością; zweryfikować wybrany timeout, 0=bez autooff,
ręczny Screen off i brak akcji urządzenia przy pierwszym dotyku. Potwierdzić
niezmienione połączenie Wi-Fi po zapisie oraz widoczny błąd zapisu/sterownika.
Testy hostowe i CI nie stanowią fizycznego odbioru tych zachowań.

Hostowe kontrole obejmują latest-record merge, load/save errors, busy i bounds,
start bez nadpisania zapisanej jasności, Back oraz nagłówek podczas pending,
oddzielny błąd PWM, języki PL/EN/DE i 40 cykli bez ubytku pamięci. Build targetu
oraz fizyczne PWM/idle/wake pozostają osobnymi bramkami.
