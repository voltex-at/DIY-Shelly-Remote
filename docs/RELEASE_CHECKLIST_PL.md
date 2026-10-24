# Obowiązkowy odbiór pobierania i aktualizacji przed wydaniem

Dotyczy każdego kolejnego DEV. Wynik CI 5/5, podpis i pobranie na komputerze
są osobnymi dowodami i nie zastępują aktualizacji pilota. Nie oznaczać wydania
jako gotowego, dopóki wymagane punkty nie mają wyniku PASS dla dokładnego
commita i tych samych bajtów aplikacji.

## Kandydat i regresje przed zgodą na wydanie

- [ ] Zapisać commit, tag, wersję źródłową pilota i docelową, profil ES3C35P
  oraz SHA-256 aplikacji. Pełne CI 5/5 musi dotyczyć dokładnie tego commita.
- [ ] Sprawdzić regresje pobierania, parsera HTTP, łańcucha wydanie → podpisany
  manifest → BIN, instalatora, zasilania i restartu. Zachować przypadek CSP
  GitHub dłuższego niż 8 KiB oraz odmowę przekroczenia łącznego limitu 16 KiB.
- [ ] Sprawdzić odrzucanie uszkodzonego podpisu/obrazu, niepełnej odpowiedzi,
  błędnej długości, obcego hosta przekierowania i niewłaściwej wersji/kanału.
  Przerwanie pobierania/anulowanie nie może wybierać nowego slotu startowego.
- [ ] Potwierdzić blokadę rozpoczęcia aktualizacji poniżej 20% baterii oraz
  osobną zgodę LCD na stabilne USB przy nieznanym procencie. Testów brzegowych
  nie wykonywać przez odłączanie zasilania podczas zapisu Flash.

Istniejące zestawy: `tools/test_ota_fetch.py`,
`tools/test_ota_fetch_parser.py`, `tools/test_ota_release_chain.py`,
`tools/test_ota_download.py`, `tools/test_ota_install.py`,
`tools/test_ota_upload.py`, `tools/test_ota_power.py` oraz aktor native OTA
w `tools/test.py`. Podczas poprawek uruchamiać tylko dotknięte moduły;
pełny zestaw raz na końcowym commicie przed wydaniem.

## Rzeczywiste pobieranie z GitHub

- [ ] Pobrać katalog wydań, manifest i cały BIN zwykłymi publicznymi adresami,
  bez tokena, sesji GitHub i `gh release download`. Użyć HTTP/1.1, nagłówków
  żądania i polityki przekierowań zgodnych z downloaderem firmware.
- [ ] Sprawdzić każdą odpowiedź w łańcuchu: status, host, długość i sumę
  nagłówków, CSP, Content-Length/chunked i kodowanie. Odpowiedź musi mieścić
  się w rzeczywistych limitach wersji, z której użytkownik aktualizuje.
  Nie wystarcza HTTP 200 z wcześniejszego odczytu katalogu ani zapytanie HEAD.
- [ ] Sprawdzić rozmiar katalogu/manifestu, pełną długość i SHA-256 BIN,
  produkcyjny podpis Ed25519, sprzęt, partycje, kanał i wersję. Porównać bajty
  z przetestowanym kandydatem i publicznym SHA256SUMS.txt.
- [ ] Sprawdzić pobieranie również downloaderem na prawdziwym pilocie.
  Sukces curl/Pythona potwierdza dostępność serwera, nie zgodność ESP-IDF
  ani możliwość aktualizacji z poprzedniego firmware.

## Rzeczywista aktualizacja pilota

- [ ] Na zatwierdzonym ES3C35P wykonać pełną aktualizację natywną z poprzedniego
  obsługiwanego DEV do kandydata: wykrycie wersji, pobranie manifestu i całego
  BIN, weryfikacja, zapis nieaktywnego slotu i restart.
- [ ] Po restarcie potwierdzić docelową wersję, zdrowy start bez pętli
  restartów/nieoczekiwanego rollbacku oraz zachowanie Wi-Fi, powiązań BLE,
  urządzeń, pomieszczeń, dashboardów, ustawień i zdjęć.
- [ ] Na szczegółach rolety i karcie Dashboardu sprawdzić Otwórz/Zamknij/Stop:
  szybkie potwierdzenie przyjęcia dotyku, zachowanie ostatniego potwierdzonego
  widoku podczas oczekiwania i płynne odczyty ruchu. Prawidłowe polecenie nie
  może powodować chwilowego komunikatu o braku pomiarów ani resetu grafiki.
  Animacja musi wynikać z potwierdzonego ruchu, nie z samej żądanej pozycji;
  rzeczywistego błędu odczytu nie wolno ukrywać ani ponawiać polecenia ruchu.
  Przy timeout sprawdzić zachowanie grafiki z podpisem „Ostatnia pozycja”,
  widoczny błąd, blokadę ruchu i niezależne STOP. Powrót odczytów ma aktualizować
  istniejący widok; zmiana powiązania nie może odziedziczyć poprzedniej pozycji.
- [ ] Na stronie baterii sprawdzić automatyczny pomiar: zmienia wartości bez
  przygaszania/naciskania „Zmierz napięcie” i bez komunikatu pomiaru ręcznego.
  Dotknięcie przycisku podczas odczytu w tle wykorzystuje ten sam pomiar.
  Zaległy odczyt baterii nie może zostać zlecony przez odrysowanie nagłówka
  ani wyprzedzić oczekującego polecenia rolety, STOP lub grupy/sceny.
- [ ] Sprawdzić na sprzęcie również lokalną aktualizację przez sparowany panel
  WWW: zgoda LCD, przesłanie tego samego BIN, weryfikacja, zakończenie i restart.
  Zapisać wersję początkową tej próby oddzielnie.
- [ ] Po instalacji ponownie sprawdzić aktualizacje z nowego firmware.
  Brak nowszego wydania ma dać właściwy komunikat, nie błąd transportu.
- [ ] Zapisać wynik właściciela/testera, datę, wersje, commit, SHA-256 i krótki
  log bez haseł, tokenów i parametrów CDN. Brak urządzenia lub niewykonana
  próba oznacza NIEZWERYFIKOWANE, nie PASS.

Test sprzętowy wymaga osobnej zgody na konkretną aktualizację. Ta checklista
nie upoważnia do flashowania, kasowania danych ani wymuszania rollbacku.
Znana niezgodność starszego DEV wymaga opisu zakresu i sprawdzonej migracji;
aktualizacja przez WWW nie zalicza nieudanej próby natywnego OTA.

## Kolejność publikacji i dowody

GitHub draft nie udostępnia plików anonimowemu downloaderowi pilota. Jeżeli
test dokładnych adresów wymaga publicznego kandydata, najpierw uzyskać osobną
zgodę właściciela na jego udostępnienie jako **kandydat do odbioru**. Nie
przedstawiać go jako sprawdzonego wydania i nie uznawać samego udostępnienia
za zakończenie publikacji. Ten etap nie jest automatycznie obsługiwany przez
obecny workflow i musi zostać jawnie zaplanowany przed uruchomieniem wydania.

Przed ogłoszeniem gotowego DEV muszą przejść publiczne pobrania i próby
sprzętowe powyżej. Ponownie porównać publiczne pliki z kandydatem. Nie zmieniać
tagu ani zastępować opublikowanych zasobów; poprawka wymaga nowej wersji.
Wyniki i ograniczenia zapisać w issue odbioru. Błąd lub brak wymaganego dowodu
blokuje oznaczenie wydania jako gotowego; CI ani podpis go nie zastępują.

Obecne CI/workflow sprawdzają host, build, podpis i integralność pakietów;
nie wykonują fizycznej aktualizacji pilota. Ta zmiana dodaje wymagania odbioru,
a nie automatyczne potwierdzenie sprzętu. Oddzielne polecenie publikacji
i istniejące zasady ochrony środowiska release nadal obowiązują.
