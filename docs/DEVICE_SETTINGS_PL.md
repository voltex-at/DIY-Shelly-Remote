# Ustawienia wybranego Shelly — integracja #29A

Wersja 0.7.27 integruje istniejący ekran i formularz z PR34 z aktualną bazą
multi-device/AP PR53. Zakładka Ustawienia działa w szczegółach urządzenia.
Pokazuje rzeczywistą zapisaną tożsamość tylko pasującego prywatnego endpointu,
adres i kanał karty, status oraz dostępne potwierdzone pomiary. Nie deklaruje
zmiany konfiguracji samego Shelly. Wykresy wymagają osobnej historii #22.

Odśwież status wskazuje trwałe ID wybranego endpointu. Niepowiązana definicja
nie odświeża innego Shelly. Edytuj kartę udostępnia wyłącznie nazwę i pokój.
Nie zmienia adresu/kanału ani danych logowania. Wspólny CAS oraz dokładny trwały
ACK chronią równoległe edycje WWW; błąd zapisu zachowuje formularz. Dopiero
potwierdzenie własnej wersji wraca do ustawień tego samego trwałego ID.

Regresja prawdziwego LVGL obejmuje dwa endpointy, IO/retry, CAS, reorder/delete
z WWW, niepowiązaną definicję, PL/EN/DE i 40 wejść/wyjść z edytora. Testy
używają jawnych modeli workspace/workera. Lokalny executor jest niedostępny;
dowodem wykonania testów jest wynik CI konkretnego commitu, a nie ta instrukcja.

Odbiór sprzętowy bez kasowania danych: otwórz Ustawienia B przy obecnym A;
odśwież B; zmień jego nazwę/pokój i sprawdź zachowanie A, powrót po zapisie,
pamięć po restarcie oraz anulowanie edycji. Zmień kolejność przez WWW i sprawdź,
że ustawienia nadal wskazują B. P0 SPIFFS #28 pozostaje otwarte: nie wykonywać
nowego erase/formatu ani wgrywania obrazu danych jako części tej poprawki.
