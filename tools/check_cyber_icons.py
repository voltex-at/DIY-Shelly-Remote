#!/usr/bin/env python3
"""Verify imported A8 byte/stride integrity and that no unused variants ship."""
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
folder = ROOT / "components/dsr_ui/cyber_icons"
manifest = json.loads((folder / "manifest.json").read_text())
source = (folder / "shelly_hud_icons.c").read_text()
mapping = (folder.parent / "dsr_ui_cyber_icons.c").read_text()
used = set(re.findall(r"&(shud_\w+_(?:24|32|48))\b", mapping))
arrays = dict(re.findall(r"static const uint8_t (shud_\w+)_px\[\d+\] = \{(.*?)\n};", source, re.S))
assert used == set(arrays) == set(manifest["assets"]), "Missing or unused icon variant"
for name, record in manifest["assets"].items():
    data = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", arrays[name]))
    n = record["size"]
    assert len(data) == n * n == record["bytes"], name
    assert hashlib.sha256(data).hexdigest() == record["sha256"], name
    descriptor = re.search(r"const lv_image_dsc_t " + name + r" = \{(.*?)\n};", source, re.S).group(1)
    assert ".cf = LV_COLOR_FORMAT_A8" in descriptor, name
    assert f".w = {n}, .h = {n}, .stride = {n}" in descriptor, name
    assert f".data = {name}_px" in descriptor, name
assert sum(r["bytes"] for r in manifest["assets"].values()) == manifest["mask_bytes"]
print(f'PASS: {len(used)} exact A8 masks, {manifest["mask_bytes"]} bytes; SHA256/stride, referenced variants only; no PNG/SVG decoding')

extra = json.loads((folder / "extra-manifest.json").read_text())
source = (folder / "shelly_hud_extra.c").read_text()
used = set(re.findall(r"&(shudx_\w+_(?:24|32|48))\b", mapping))
arrays = dict(re.findall(r"static const uint8_t (shudx_\w+)_px\[\d+\] = \{(.*?)\n};", source, re.S))
assert used == set(arrays) == set(extra["assets"]), "Missing or unused supplementary mask"
for name, record in extra["assets"].items():
    data = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", arrays[name]))
    n = record["size"]
    assert len(data) == n*n == record["bytes"], name
    assert hashlib.sha256(data).hexdigest() == record["sha256"], name
    descriptor = re.search(r"const lv_image_dsc_t " + name + r" = \{(.*?)\n};", source, re.S).group(1)
    assert ".cf = LV_COLOR_FORMAT_A8" in descriptor and f".w = {n}, .h = {n}, .stride = {n}" in descriptor, name
    assert f".data = {name}_px" in descriptor, name
for name, sha in extra["svg_sha256"].items():
    assert hashlib.sha256((ROOT / "assets/cyber-icons/svg" / (name + ".svg")).read_bytes()).hexdigest() == sha
assert sum(r["bytes"] for r in extra["assets"].values()) == extra["mask_bytes"]
print(f'PASS: {len(extra["svg_sha256"])} supplementary SVGs, {len(used)} A8 masks, {extra["mask_bytes"]} Flash bytes; SHA256/stride; existing masks unchanged')
