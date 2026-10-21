# Styl „Jasny”, ekran startowy i tło (0.7.37)

**Źródło:** grafiki właściciela (2026-10-17), 1536×1024, wersja ciemna i jasna,
z logo i bez. `tools/build_brand_images.py` skaluje je do wysokości 480 px i
wycina środek 320 px (logo mieści się w całości). Wynik: wzorce PNG w
`assets/brand/` oraz surowe RGB565 z ditheringiem Floyd–Steinberg w
`components/dsr_ui/brand/`, osadzone w Flash przez `EMBED_FILES`. Cztery obrazy
zajmują 4 × 300 KiB, bez dekodera, kopii w RAM i PSRAM.

- **Ekran startowy:** logo od `dsr_board_init` do zbudowania UI, z numerem
  wersji. Wariant (ciemny lub jasny) wybiera podpowiedź stylu w NVS
  (`dsr_ui/style`). Jest potrzebna, bo konfiguracja w SPIFFS nie jest jeszcze
  zamontowana. Zapis do NVS następuje tylko przy zmianie stylu. Rysuje
  wyłącznie zadanie UI (8 KiB stosu). `app_main` dotyka LVGL tylko przy
  zamkniętej bramce i po `dsr_ui_task_quiesce`.
- **Tło aplikacji:** obraz bez tekstu na każdym ekranie głównym
  (`dsr_sc_background`). Grafit i Cyber Blue używają wariantu ciemnego, Jasny
  jasnego.
- **Dokładnie trzy skórki (od 0.7.68-dev):** Grafitowy (`graphite`, id 0),
  Jasny (`light`, id 2) i Shelly HUD — Cyber Blue (`cyber_blue`, id 3).
  Skórkę Nocny (`midnight`, id 1) wycofano. Zapisana konfiguracja z
  `midnight` przechodzi bezstratnie na `graphite` przy starcie (upgrade
  workspace z odświeżeniem punktu kontrolnego) oraz przy każdym zapisie, także
  z karty Web Panelu otwartej przed aktualizacją. Podpowiedź NVS 1 jest
  czytana jako 0 i nadpisywana raz przy następnym zastosowaniu stylu. Pozostałe
  ustawienia, pokoje, sceny, powiązania i zdjęcia pozostają bez zmian. Id 1 nie
  jest ponownie używany.
- **Styl „Jasny”:** trzecia wartość `theme` w konfiguracji (`light`), dostępna
  na LCD i w Web Panelu. Wszystkie neutralne kolory ciemnego projektu mają
  jasne odpowiedniki w `dsr_sc_color`. Akcent użyty jako tekst jest
  przyciemniany, przyciski dostają obrys. Domyślny motyw LVGL przełącza się
  na jasny.

**Zgodność wstecz:** starszy firmware nie zna wartości `light` i odmówi
wczytania takiej konfiguracji, nie kasując danych. Przed powrotem do starszej
wersji przez USB przełącz styl na Grafit.

**Testy (host, prawdziwe LVGL):** 18 klatek (splash ciemny i jasny; home,
karta, urządzenie, ustawienia, wygląd, kreator, formularz, edytor w stylu
Grafit i Jasny). Sprawdzane jest 155 etykiet pod kątem kontrastu ≥ 4:1
względem tła, a zapis podpowiedzi w NVS wyłącznie przy zmianie. **Na płytce
niesprawdzone:** płynność rysowania tła z Flash przy przewijaniu oraz odbiór
kolorów na panelu ST77922.
