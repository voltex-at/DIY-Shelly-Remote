# Pomieszczenia — 0.7.4, Issue #28

Osobna gałąź `work/rooms-context-editor`, oparta na PR #31. Nie scalono do main.
Wersja rozwojowa `0.7.4-rooms-editor-dev`; kod nadal obsługuje jeden aktywny
przekaźnik. Edycja pokojów nie uruchamia nowego transportu ani parowania BLE.

## Obsługa

Na stronie Pomieszczenia pasek „Dodaj pomieszczenie”, przycisk pustego stanu
i dolny + prowadzą bezpośrednio do formularza pokoju. Ołówek 44×44 na każdej
karcie edytuje jej trwałe ID, a nie numer miejsca na liście. Po otwarciu pokoju
dostępny jest również podpisany przycisk „Edytuj pomieszczenie”. Na innych
stronach + nadal otwiera kreator dodawania urządzenia.

Formularz obejmuje nazwę, jedną z czterech ikon i przełączanie zdjęć już obecnych
w pamięci. Kolejne naciśnięcia wyboru zdjęcia przechodzą przez zapisane fotografie
oraz wariant bez zdjęcia. Pliki dodaje się przez lokalny, uwierzytelniony Web Panel.
Pod formularzem istniejącego pokoju jest „Usuń pomieszczenie”. Podpowiedź o WWW
może wymagać przewinięcia; cele dotykowe mają co najmniej 44 px.

„Zapisz” używa istniejącego schematu 1, wersji CAS oraz zadania trwałego zapisu.
Komunikat sukcesu pojawia się po potwierdzeniu opublikowanej wersji, nie po samym
przyjęciu zlecenia. Późniejsze wpisywanie podczas zapisu nie staje się omyłkowo
zapisane. Wstecz blokuje wyjście podczas zapisu; przy niezapisanych zmianach
wymaga wyboru Zostań/Odrzuć. Przy konflikcie z WWW trzeba wrócić i otworzyć pokój
ponownie. Draft pozostaje dostępny do czasu jawnego odrzucenia.

Usuwanie jest dwustopniowe: potwierdzenie, następnie Zapisz. Powiązania urządzeń
i kart pokojów na dowolnym pulpicie blokują operację. Najpierw jawnie przenieś
urządzenia oraz usuń karty z pulpitów w istniejącym edytorze lub Web Panelu.
Sceny odwołują się do urządzeń; te urządzenia nie są kasowane ani odpinane.
Współdzielone zdjęcia nie są usuwane w czasie pracy LVGL. Dotychczasowe GC
uruchamiane przy starcie chroni obydwa checkpointy konfiguracji i cache zdjęć.

## Powtarzalna weryfikacja programowa

```sh
python3 tools/test.py --ubsan-only
python3 tools/test_config.py --ubsan-only
python3 tools/test_ui.py --ubsan-only
python3 tools/test_ui.py --review --ubsan-only
python3 tools/render_review.py --no-run
```

W CI te same źródła przechodzą ASan/UBSan, test przeglądarkowy, kompilację profilu
ES3C35P oraz kontrolę rzeczywistego ELF: pool LVGL 128 KiB w PSRAM, stos 8192 B
i TCB w RAM wewnętrznym. Dwa sloty OTA, SPIFFS, NVS i sterowniki pozostają bez zmian.
Renderowanie używa przypiętego LVGL 9.3.0, C i RGB565. Kadry `90-*` do `106-*`
w artefakcie `real-lvgl-ui-review-NOT-FIRMWARE` pokazują rzeczywisty formularz,
listę, zapis/błąd/konflikt, zależności i PL/EN/DE. Dane hostowe są TEST / DEMO;
nie są to fotografie działającej wersji na ESP32.

## Test fizycznego pilota — wymagany przed odbiorem

Nie kasuj całej Flash. Zachowaj działającą paczkę oraz aktualną konfigurację.
Ustal exact SHA paczki z artefaktu CI i zapisz wersję/hash z USB przy starcie.
Sam `diy_shelly_remote.bin` jest obrazem aplikacji; na układzie partycji z logu
właściciela slot `ota_0` zaczyna się od `0x20000`. Obrazu aplikacji nie zapisuje
się pod `0x0`. Stosuj instrukcję paczki i jej sprawdzone offsety; obraz scalony
jest innym plikiem i może nadpisać ustawienia. Nie wymuszaj awarii sprzętu,
kasowania danych ani rollbacku w ramach poniższego testu pokojów.

1. Potwierdź normalny start, dotyk, jasność i zachowanie Wi-Fi/bindingu Shelly.
2. Przy pustej liście użyj Dodaj, wpisz „Salon”, Zapisz. Poczekaj na
   „Zapisano • zsynchronizowano”; Wstecz ma wrócić do listy bez restartu.
   Jeśli masz własne pokoje, użyj nowego pokoju testowego bez usuwania istniejących.
3. Dotknij ołówka karty. Zmień nazwę na „Salon 2”, ikonę i istniejące zdjęcie.
   Dodaj plik przez Web Panel, jeśli jeszcze nie ma zdjęć. Zapisz i porównaj WWW.
4. Dodaj drugi pokój z nazwą UTF-8, np. „Küche Łazienka”. Pierwszy pokój oraz
   aktywne Shelly muszą pozostać zachowane. Wybierz ponownie brak zdjęcia i zapisz.
5. Wejdź do pokoju i użyj Edytuj pomieszczenie. Wpisz zmianę, Wstecz → Zostań,
   następnie Wstecz → Odrzuć. Zapisana nazwa nie może zmienić się w WWW.
6. Zmień kolejność pokojów w Web Panelu. Edycja ołówkiem i już otwarty pokój
   nadal muszą wskazywać właściwe ID. Usuń nieużywany pokój z WWW podczas
   otwartego formularza na LCD; jego zapis ma zgłosić konflikt, nie odtworzyć wpis.
7. Otwórz formularz na LCD, zmień inny wpis w WWW i zapisz. LCD nie może
   nadpisać nowszej konfiguracji. Wróć z formularza, odrzuć draft i otwórz ponownie.
8. Przypisz istniejące prawdziwe Shelly do pokoju przez WWW/istniejący edytor.
   Dodaj kartę pokoju na pulpit. Usuń pomieszczenie ma pokazać blokadę i liczniki.
   Sprawdź, że scena wskazująca to urządzenie oraz host/kanał/logowanie nie znikają.
9. Przenieś urządzenie do innego pokoju i usuń jego kartę pokojową z pulpitu.
   Anuluj potwierdzenie usunięcia, potem potwierdź je ponownie. Pokój w WWW
   pozostaje do Zapisz; po potwierdzonym zapisie znika z obu interfejsów.
10. Sprawdź nowy pokój na liście kreatora LAN i w wyborze karty pokoju na pulpicie.
    Nie zapisuj nowego bindingu tylko na potrzeby tego sprawdzenia.
11. Przy ośmiu pokojach dziewiąty ma być zablokowany. Sprawdź przewijanie listy
    i formularzy, klawiaturę, PL/EN/DE oraz powtarzane wejścia/wyjścia.
12. Uruchom pilot ponownie. Nazwy/ikony/zdjęcia/przypisania mają przetrwać,
    obydwa interfejsy mają pokazywać tę samą rewizję. Sprawdź ON/OFF dotychczasowego
    urządzenia i kilka skanów BLE równolegle z Wi-Fi. Zapisz błędy/heap z USB.

Zachowanie ustawień przy OTA wymaga osobnej próby zatwierdzoną metodą aktualizacji
z obrazem zgodnym z płytką. Native signed OTA i fault recovery nie są uznane za
sprawdzone przez powyższy test ani przez samo CI. Potwierdzenie właściciela z
16.10 dotyczy startu i odbioru BLE w jego instalacji, bez podanego exact SHA;
nie obejmuje nowego edytora ani całej listy regresji.
