# Shelly HUD — czytelne okna i drzwi

Oryginalne konturowe wektory przygotowane na podstawie przekazanych referencji
Window / Door. Plansze referencyjne nie są używane jako maski ani tło UI.

Siedem stanów ma SVG oraz transparentne PNG/A8 24, 32, 48 i 80 px.
Dodatkowe ikony lx, RSSI i czasu mają rozmiar 24 px. Maska przechowuje wyłącznie
pokrycie alfa; cyan/złoty/szaroniebieski nadaje LVGL w czasie rysowania.

Generator: `python3 tools/build_cyber_blu_icons.py` (CairoSVG, Pillow).
Manifest z SHA-256: `components/dsr_ui/cyber_icons/blu-manifest.json`.

`window-tilted` jest zasobem do przyszłego, jawnie obsługiwanego stanu. Obecny
transport BLU go nie przekazuje i ekran nie wnioskuje uchylenia z OPEN/RSSI/lx.
Oryginalne zasoby poprzednich etapów pozostają zachowane w ich katalogach.
