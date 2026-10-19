# Lokalne sceny i grupy

Wersja 0.7.56-dev upraszcza tworzenie bezpośrednio na stronach
**Grupy** i **Sceny**, na LCD i w Web Panelu. Nie wymaga Shelly Cloud.

Grupa służy do wspólnego włączenia lub wyłączenia przekaźników i gniazdek.
Scena uruchamia po dotknięciu zapisane polecenia w ustalonej kolejności.
Pilot musi być włączony i mieć dostęp do urządzeń w lokalnej sieci. Sceny nie
są automatycznymi regułami działającymi w tle ani w chmurze.

W kandydacie po 0.7.56-dev wybierz ikonę domu na dole, a następnie jedną
z głównych zakładek **Pomieszczenia / Grupy / Sceny / Urządzenia**. Listy
pozostają w tym samym układzie; po zapisie lub anulowaniu wrócisz do właściwej
zakładki. Przycisk + na dolnym pasku dodaje wpis odpowiedniego typu.

## Utworzenie grupy

1. Na stronie Grupy wybierz **+ Grupa** (WWW: **+ Nowa grupa**) i wpisz nazwę,
   np. „Lampy na parterze”.
2. Wybierz **Dalej** i zaznacz urządzenia. Listy są widoczne według
   pomieszczeń; wyszukiwanie pomaga znaleźć urządzenie. Licznik pokazuje wybór.
3. Wybierz **Dalej**, sprawdź nazwy i naciśnij **Zapisz grupę**.

## Utworzenie sceny

1. Na stronie Sceny wybierz **+ Scena** (WWW: **+ Nowa scena**) i nazwij ją,
   np. „Dobranoc”.
2. Wybierz **Dalej → + Dodaj polecenie**, wskaż urządzenie, a następnie jawnie
   wybierz polecenie. Dla Switch: Włącz/Wyłącz; dla Cover: Otwórz/Zamknij/Stop.
   Sam wybór urządzenia niczego nie uruchamia i nie dodaje domyślnego Włącz.
   Powtórz dla pozostałych poleceń. Strzałki zmieniają kolejność, Usuń usuwa
   krok, a dotknięcie polecenia pozwala je zmienić lub wskazać inne urządzenie.
3. Wybierz **Dalej**, sprawdź kolejność oraz polecenia i naciśnij **Zapisz scenę**.

Pomieszczenie i zdjęcie są dodatkiem pod **Pokój i zdjęcie (opcjonalnie)** na
pierwszym kroku. Można je całkowicie pominąć. Nazwa ma limit 47 bajtów UTF-8;
polskie znaki zajmują więcej niż jeden bajt. Limit to 8 grup, 8 scen i 8
poleceń w jednej scenie; urządzenia grupy korzystają z istniejącego rejestru.

## Zapis i anulowanie

W formularzu WWW widoczny jest jego własny przycisk zapisu na ostatnim kroku.
Globalna stopka zapisu jest wtedy ukryta. Na małym ekranie znika też podgląd
LCD; wraca po zakończeniu lub anulowaniu edycji.

Zapis wymaga trwałego potwierdzenia pilota i zgodnej rewizji. Błąd pozostawia
zmiany w formularzu do poprawienia lub ponowienia. Późniejsze zmiany nie są
usuwane po potwierdzeniu wcześniejszego zapisu. Anuluj na pierwszym kroku
wymaga potwierdzenia odrzucenia zmienionego wpisu. WWW zachowuje niezależne
zmiany w pozostałych częściach konfiguracji. Podczas zapisu poczekaj na wynik.

Sterowanie pozostaje oddzielnym przyciskiem na karcie. Wyniki dotyczą każdego
urządzenia osobno: część poleceń może się nie udać. Niepewny wynik nie oznacza
potwierdzenia wykonania i nie powoduje automatycznego ponawiania.

## Odbiór na pilocie

Testy automatyczne obejmują produkcyjne LVGL i panel WWW; dostawcy radia i
pamięci w testach hostowych są modelami. Kompilacja targetu i raporty stosu
nie mierzą szczytu SDK ani nie potwierdzają fizycznego wykonania.

- Utwórz grupę dwóch własnych przekaźników bez zdjęcia i pokoju. Sprawdź wybór,
  zapis, ponowne otwarcie i zachowanie po restarcie.
- Utwórz scenę z jawnym Wyłącz i Włącz, zmień kolejność, usuń krok, cofnij się
  z wyboru urządzenia. Sprawdź, że edycja nie uruchamia odbiorników.
- Sprawdź Anuluj → Zostań/Odrzuć oraz zapis w WWW i widoczność na LCD.
- Sprawdź długie nazwy, przewijanie, klawiaturę oraz PL/EN/DE na ekranie 320×480.
- Dopiero na przygotowanych do testu własnych odbiornikach sprawdź przyciski
  uruchomienia, odłączenie jednego urządzenia i oddzielny wynik dla każdego.

Nie wykonywano w tej zmianie operacji na fizycznych przekaźnikach ani roletach.
Wersja 0.7.56-dev nie zmienia plików poprzedniego wydania 0.7.55-dev ani gałęzi main.
