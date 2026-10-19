# Supplementary Cyber Blue icons

Seventeen editable outline SVGs interpreted from the supplied missing-icon
reference sheets. They follow the existing pack's 24-unit view box, 1.9-unit
rounded stroke, transparent interior and cyan color. These supplemental
sources use the project's MIT license; the original imported pack retains
its separate CC0 notice and unchanged mask bytes.

`svg/` contains the source files, and `png/24`, `png/32`, `png/48` contain
transparent previews. Firmware compiles only the const A8 masks and descriptors
in `components/dsr_ui/cyber_icons/shelly_hud_extra.*`. SVG and PNG decoding are
not enabled. LVGL applies the current color at draw time; no glow is baked in.

Rebuild with build-only CairoSVG 2.8.2 and Pillow 13.0.0:

```sh
python3 -m pip install --target .test-deps/svg-runtime cairosvg==2.8.2 Pillow==13.0.0
python3 tools/build_cyber_extra_icons.py
python3 tools/check_cyber_icons.py
```

The generator emits only A8 variants referenced by the Cyber mapper, records
source and coverage SHA256 hashes, and never writes the legacy atlas, fonts,
brand artwork or original imported A8 pack. The 51 supplementary masks occupy
66,368 bytes of Flash, plus const descriptors. No alpha buffer is copied to RAM.

Mappings use existing `DSR_ICON_*` roles. Stored room/device icon indices and
the 20-choice device icon selector are unchanged. The extra door states and
thermostatic valve are used by the existing sensor and TRV paths.
