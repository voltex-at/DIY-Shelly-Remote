#!/usr/bin/env python3
"""Mechanical LVGL conversion of independently licensed Roboto, not app assets.

Input: .test-deps/Roboto-Regular.ttf and Roboto-Medium.ttf from
googlefonts/roboto-2/src/hinted; conversion: lv_font_conv 1.5.3.
"""
from pathlib import Path
import os
import subprocess

root = Path(__file__).resolve().parents[1]
dest = root / "components/dsr_ui/fonts"
dest.mkdir(exist_ok=True)
env = dict(os.environ, npm_config_cache=str(root / ".test-deps/npm-cache"))
symbols = "ĄąĆćĘęŁłŃńÓóŚśŹźŻżÄäÖöÜüß°µ–—•…"
for size, weight in [(11,"Regular"),(12,"Regular"),(14,"Regular"),(16,"Medium"),(18,"Medium"),(28,"Medium")]:
    name = f"dsr_roboto_{size}"
    subprocess.run(["npm", "exec", "--yes", "--package=lv_font_conv@1.5.3", "--", "lv_font_conv",
                    "--font", str(root / f".test-deps/Roboto-{weight}.ttf"),
                    "--size", str(size), "--bpp", "4", "--format", "lvgl", "--no-compress",
                    "--range", "0x20-0x7e", "--symbols", symbols, "--lv-include", "lvgl.h",
                    "--lv-font-name", name, "-o", str(dest / f"{name}.c")], check=True, env=env)
