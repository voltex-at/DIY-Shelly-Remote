# Scena za roletą

`garden-dusk.png` jest teksturą widoku zewnętrznego. Interfejs i roleta są
rysowane przez produkcyjny LVGL; tekstura nie zawiera elementów UI.

Źródło powstało w trybie `generate` za pomocą ImageGen, bez obrazu wejściowego.
Końcowy prompt:

> Use case: photorealistic-natural. Asset type: outdoor scenery texture behind a moving roller shutter in a 320x480 embedded touchscreen; only the scenery is wanted. Create a realistic straight-on view out toward a quiet private garden at blue hour: a stone terrace in the lower third, low garden wall and shrubs, tall trees left and right, distant dark blue twilight sky, two subtle warm garden path lights near the ground. Crisp recognizable large forms that remain legible when reduced to about 180x160 pixels. Natural depth, calm muted navy/teal greens, restrained warm light. Composition: square image, eye level, centered unobstructed outdoor view. No window, no window frame, no roller shutter, no interior, no interface, no border, no writing, no logos, no people. This is a background asset, not an app mockup.

Zapis źródłowy: PNG 384 × 384. `python3 tools/build_cyber_scene.py` przygotowuje
`components/dsr_ui/brand/cyber_garden_dusk.rgb565`: 152 × 128, little-endian,
**38 912 B Flash**. Hash źródła i danych zapisuje `garden-manifest.json`.
Firmware odczytuje RGB565 bez dekodera PNG/SVG i bez drugiej kopii pikseli.
Rama, prowadnice, kaseta, pancerz i listwa dolna są rysowane osobno.
