DIY SHELLY REMOTE — HOME v3 / konfiguracja — 320 × 480

To rzeczywiste rendery kodu LVGL 9.3.0, nie ilustracje ani makieta HTML.
Gałąź: work/config-web-panel, wersja 0.4.0-config-dev, draft PR #5.
Nie scalono zmian z main. Oczekujemy oceny nowego HOME.

01-home.png: światło, czujnik otwarcia i roleta. Szerokie karty, duże
okrągłe ikony po lewej, nazwa/status pośrodku, jasne sterowanie po prawej,
ciemne paski W/V, baterii/temperatury i pozycji. Dane oznaczono DEMO.
Pozostałe pliki pokazują wspólne komponenty, pomieszczenia w dwóch kolumnach,
szczegóły, offline, pusty tryb rzeczywisty i centrowanie ikon.

Porównanie ze wzorcem użytkownika (Shelly Smart Control 3.77.26):
- Zachowane: ciemna paleta, grafitowe zaokrąglone karty, niebieski akcent,
  aktywna druga zakładka, pasek edycji pulpitu i sześć ikon dolnej nawigacji.
- Pomieszczenia pozostają w dwóch kolumnach, ze zdjęciem i ciemnym polem danych.
- Urządzenia mają pełną szerokość, zgodnie z korektą użytkownika. Nie są
  domyślnie układane jak dwukolumnowe karty pomieszczeń ze zrzutu telefonu.
- Zamiast logo/zasobów Shelly użyto własnych ikon i niezależnej czcionki Roboto.
- Ekran 320×480 wymaga skróconych nagłówków i przewijanej listy. Brak danych
  nie jest zastępowany zerami; offline ma własny czytelny status.

Kod firmware nie zawiera danych demonstracyjnych. Aktualny transport obsługuje
jedno rzeczywiste połączenie relay/switch. Roleta, czujnik i sceny mają model
oraz UI, ale nowe protokoły i wykonawca scen nie są jeszcze wdrożone.

Dodano wspólny model i edytory dotykowy/Web, trwały zapis i podgląd LVGL.
Web Panel wymaga parowania na ekranie i HTTPS. Lokalny instalator OTA wymaga
osobnego zezwolenia dotykiem; to wersja rozwojowa bez podpisów wydawcy.
Instrukcja i dokładne granice: docs/CONFIG_WEB_PL.md w repozytorium.
Nie potwierdzono jeszcze tej wersji na fizycznym ES3C35P.
