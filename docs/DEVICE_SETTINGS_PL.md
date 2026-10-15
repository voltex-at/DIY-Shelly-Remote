# Ustawienia wybranego urządzenia — #29A, 0.7.5

Gałąź `work/device-settings-context`, na PR #33. Wersja rozwojowa
`0.7.5-device-settings-dev`; osobny Draft PR, bez scalenia do main.
Realizacja istniejącego #29A według kolejności #25. Pełne #29 pozostaje otwarte:
Wykresy i historia wymagają #22; nie tworzymy demonstracyjnych odczytów.

## Co działa w kodzie

- Szczegóły → Ustawienia: własny widok wybranego trwałego ID, niezależny od
  numeru pozycji na liście. Powrót/zakładka Sterowanie prowadzi do tego urządzenia.
- Status rzeczywistego połączenia, zapisane ID i rodzina API Gen1/Gen2+, adres
  skonfigurowanej karty, kanał oraz dostępne i poprawne W/V/A z ostatniego odczytu.
  Pusta próbka nie staje się zerem. Błąd logowania/odłączenie usuwa prezentację
  pomiarów jako bieżących. Nie odgadujemy dokładnego modelu/firmware/generacji
  z nazwy karty ani tekstu reklamy BLE. Gen2+ nie oznacza konkretnej generacji 2.
- Odśwież status: dotychczasowy autoryzowany GET/weryfikacja tożsamości,
  bez SET i bez nowego transportu. Wpis bez aktywnego połączenia ma przycisk
  wyłączony, więc nie może przypadkowo odpytać innego aktywnego Shelly.
- Edytuj kartę: zmiana nazwy i pomieszczenia na pilocie i w Web Panelu;
  nie jest to zmiana nazwy/parametrów w samym Shelly. Nie edytuje hosta,
  kanału, hasła ani typu, które mogłyby rozłączyć właściwy binding.
- Istniejący async zapis/CAS, niezmienny snapshot zlecenia i potwierdzenie
  opublikowanej wersji. Wstecz czeka na zapis; niezapisane zmiany wymagają
  odrzucenia. Konflikt z WWW nie nadpisuje nowszej konfiguracji.
- Usunięte ID daje powrót na listę urządzeń. Zmiana kolejności nie zmienia
  celu edycji. Dotychczasowy edytor całej konfiguracji pozostaje dostępny.

Sterowniki, partycje, sekrety/NVS i API Shelly pozostają bez zmian. Dodatkowe
teksty identyfikacji istnieją tylko w modelu UI; rozmiar BSS oraz rezerwacje
PSRAM/stosu nadal sprawdza rzeczywisty ELF w CI. To nie jest implementacja
wielu aktywnych urządzeń ani zdalnych ustawień konkretnego modelu Shelly.

## Testy programowe i rzeczywiste rendery

`python3 tools/test_ui.py --ubsan-only`: kliknięcia zakładki i przycisków,
odświeżenie/GET, nowe metryki, auth error, brak meteringu, blokada odświeżania
obcego wpisu, lokalna nazwa/pokój, opóźniony/błędny zapis, retry, Web CAS,
trwałe ID przy reorderze, zniknięcie wpisu, PL/EN/DE i 40 cykli edit/Back.
Cała seria obejmuje 1641 kontroli i 74 kadry. Review LVGL ma nadal 2635 kontroli
i 24 kadry; pozostałe regresje uruchamiają `tools/test.py` i `test_config.py`.
CI wykonuje ASan/UBSan, browser oraz pełny profil ES3C35P na ESP-IDF 5.5.4.

`tools/render_review.py --no-run` eksportuje natywne kadry C/LVGL 9.3.0 320×480:
`110-*`–`119-*`: Settings, rzeczywiste mapowanie metryk z kontrolowanego zdarzenia,
edytor, brak bindingu, błąd uwierzytelnienia oraz EN/DE. To syntetyczny provider
TEST / DEMO, bez danych prywatnego domu i bez dowodu fizycznego działania.

## Odbiór na fizycznej ES3C35P

Zapisz exact SHA/wersję paczki i USB log startu bez sekretów. Nie kasuj NVS,
całej Flash ani zdjęć. Stosuj offsety właściwe dla obrazu aplikacji i aktywnego
slotu z instrukcji paczki. Nie zmieniaj działającej instalacji w trakcie testu
0.7.4 tylko po to, by uzyskać status nowej wersji.

1. Potwierdź boot, Wi-Fi, dotyk i dotychczasowy Shelly (np. 1PM Mini Gen3).
   Sprawdź ON/OFF oraz W/V/A przed wejściem do ustawień.
2. Urządzenie → Ustawienia. Porównaj kanał, adres i zapisane ID z własną
   konfiguracją. Model nieodczytany/Gen2+ nie może być sztuczną identyfikacją
   dokładnego modelu. Nie publikuj prywatnych adresów/ID w materiałach review.
3. Odśwież status. Porównaj dostępne W/V/A ze Switch.GetStatus/Shelly;
   powrót do Sterowanie ma wskazywać to samo urządzenie i działać bez restartu.
4. Edytuj kartę → nazwa z PL/DE, wybierz pokój → Zapisz. Poczekaj na
   Zapisano • zsynchronizowano. Wstecz wraca do Ustawień tego samego urządzenia.
   Web Panel ma pokazać tę samą nazwę i pokój; host/kanał/sekrety bez zmian.
5. Wpisz zmianę, Wstecz → Zostań, następnie Wstecz → Odrzuć. Zapisana nazwa
   musi pozostać. Zmień dane przez WWW podczas otwartego formularza LCD;
   zapis LCD ma zgłosić konflikt. Otwórz formularz ponownie i porównaj rewizję.
6. Zmień kolejność definicji przez WWW, również przy otwartej zakładce Settings.
   Edycja oraz powrót nie mogą przejść do innego ID. Wpis bez aktywnego bindingu
   ma wyłączone odświeżanie, bez odpytywania obecnego aktywnego Shelly.
7. Odłącz Shelly od sieci / sprawdź błąd autoryzacji bez resetu jego ustawień.
   Brak odpowiedzi ma być jawny; nie pokazuj starego pomiaru jako aktualnego.
   Po przywróceniu połączenia użyj Odśwież. Sprawdź ponownie ON/OFF.
8. Sprawdź PL/EN/DE, długie nazwy, klawiaturę, przewijanie opisów przy stale
   widocznych przyciskach, kilkadziesiąt wejść/powrotów i BLE scan z Wi-Fi.
9. Zrestartuj pilot. Nazwa/pokój i dotychczasowy binding mają przetrwać;
   powtórz WWW sync oraz ON/OFF. Zachowanie przy OTA sprawdź osobno przy
   zatwierdzonej metodzie aktualizacji, bez formatowania ustawień i zdjęć.

Brak odbioru sprzętowego tej wersji. Native OTA #24, AP #19, wiele transportów
#20 i historia #22/#29B pozostają osobnymi otwartymi wymaganiami. Nie zamykaj #29
na podstawie samego działającego formularza i kompilacji.
