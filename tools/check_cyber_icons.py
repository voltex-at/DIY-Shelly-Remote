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
retained={f"shud_window_{state}_{size}" for state in ("closed","open") for size in (24,48)}
assert used | retained == set(arrays) == set(manifest["assets"]), "Missing icon variant; only original replaced window masks may remain"
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
print(f'PASS: {len(manifest['assets'])} original A8 masks, {manifest["mask_bytes"]} bytes; SHA256/stride, original window sources retained; no PNG/SVG decoding')

extra = json.loads((folder / "extra-manifest.json").read_text())
source = (folder / "shelly_hud_extra.c").read_text()
used = set(re.findall(r"&(shudx_\w+_(?:24|32|48))\b", mapping))
arrays = dict(re.findall(r"static const uint8_t (shudx_\w+)_px\[\d+\] = \{(.*?)\n};", source, re.S))
retained={f"shudx_door_{state}_{size}" for state in ("closed","open") for size in (24,32,48)}
assert used | retained == set(arrays) == set(extra["assets"]), "Missing supplementary mask; original door sources retained"
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
print(f'PASS: {len(extra["svg_sha256"])} supplementary SVGs, {len(extra['assets'])} A8 masks, {extra["mask_bytes"]} Flash bytes; SHA256/stride; existing masks unchanged')

# Dedicated reference-inspired frames: validate every generated source and mask.
blu=json.loads((folder/'blu-manifest.json').read_text())
source=(folder/'shelly_hud_blu.c').read_text()
arrays=dict(re.findall(r"static const uint8_t (shudb_\w+)_px\[\d+\] = \{(.*?)\n};",source,re.S))
assert set(arrays)==set(blu['assets'])
for name,record in blu['assets'].items():
 data=bytes(int(x,16) for x in re.findall(r"0x([0-9a-fA-F]{2})",arrays[name]))
 n=record['size']
 assert len(data)==n*n==record['bytes'] and hashlib.sha256(data).hexdigest()==record['sha256']
 assert 0 in data and max(data)>200
 assert f'.w = {n}, .h = {n}, .stride = {n}' in re.search(r'const lv_image_dsc_t '+name+r' = \{(.*?)\n};',source,re.S).group(1)
for name,sha in blu['svg_sha256'].items():
 assert hashlib.sha256((ROOT/'assets/cyber-blu-icons/svg'/(name+'.svg')).read_bytes()).hexdigest()==sha
assert sum(r['bytes'] for r in blu['assets'].values())==blu['mask_bytes']
print(f"PASS: {len(arrays)} dedicated BLU A8 masks; {blu['mask_bytes']} Flash bytes; retained original assets; no decoder or dynamic icon allocation")
