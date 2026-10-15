#!/usr/bin/env python3
"""Export untouched real LVGL RGB565 render frames as lossless PNG + contact sheet.

No concept art, browser imitation, UI bitmap overlays or external app images.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile
import zlib
import tempfile

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--out", type=Path, default=ROOT / "build-ui/review-export")
p.add_argument("--no-run", action="store_true")
args = p.parse_args()
if not args.no_run:
    subprocess.run(["python3", str(ROOT / "tools/test_ui.py"), "--review"], check=True)
out = args.out.resolve()
out.mkdir(parents=True, exist_ok=True)
def latest_frames(names):
    directories=[ROOT / 'build-ui' / name / 'screenshots' for name in names]
    directories=[d for d in directories if list(d.glob('*.ppm'))]
    assert directories, 'Run the native LVGL tests before exporting'
    return max(directories,key=lambda d:max(p.stat().st_mtime_ns for p in d.glob('*.ppm')))

source = latest_frames(['review-native','review-ubsan','review-asan'])

def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)

def png(path, width, height, rgb):
    assert len(rgb) == width * height * 3
    rows = b"".join(b"\0" + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
    data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    data += chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b"")
    path.write_bytes(data)

frames = {}
for path in sorted(source.glob("*.ppm")):
    magic, size, maximum, rgb = path.read_bytes().split(b"\n", 3)
    assert magic == b"P6" and size == b"320 480" and maximum == b"255"
    assert len(rgb) == 320 * 480 * 3
    frames[path.stem] = rgb
    png(out / (path.stem + ".png"), 320, 480, rgb)
assert len(frames) >= 13
onboarding = latest_frames(['native','ubsan','asan'])
for original, name in [("02-network-list", "20-fixture-wifi-list"), ("03-password-keyboard", "21-fixture-wifi-keyboard"), ("14-add-device-wizard", "22-fixture-add-wizard"), ("15-lan-scanning", "23-fixture-lan-scanning"), ("16-lan-results", "24-fixture-lan-results"), ("17-lan-auth-error", "25-fixture-lan-auth-error"), ("18-lan-components", "26-fixture-lan-components"), ("19-lan-no-wifi", "27-fixture-lan-no-wifi"), ("20-workspace-save-race", "28-fixture-save-a-edited-b")]:
    path = onboarding / (original + ".ppm")
    if path.exists():
        _, size, _, rgb = path.read_bytes().split(b"\n", 3)
        assert size == b"320 480"
        png(out / (name + ".png"), 320, 480, rgb)
for path in sorted(onboarding.glob('*.ppm')):
    if int(path.name.split('-')[0])<30:continue
    _,size,_,rgb=path.read_bytes().split(b'\n',3)
    assert size==b'320 480'
    png(out / (path.stem+'.png'),320,480,rgb)
before=ROOT / 'docs/ui-review/before-0.5.1'
for path in sorted(before.glob('*.png')):
    shutil.copyfile(path,out / ('before-'+path.name))
before_cover=ROOT / 'docs/ui-review/before-0.6.1/11-cover-details.png'
if before_cover.exists():
    shutil.copyfile(before_cover,out / 'before-cover-0.6.1.png')
sheet = bytearray(bytes((235, 237, 240)) * (648 * 1456))
for index, name in enumerate(["10-home-room-photos", "01-home", "04-room-salon", "05-device-details-on", "11-cover-details", "07-device-offline"]):
    frame = frames[name]
    ox, oy = (index % 2) * 328, (index // 2) * 488
    for y in range(480):
        start = ((oy + y) * 648 + ox) * 3
        sheet[start:start + 960] = frame[y * 960:(y + 1) * 960]
png(out / "contact_sheet.png", 648, 1456, bytes(sheet))
manifest = {
    "branch": os.environ.get("GITHUB_HEAD_REF") or subprocess.check_output(["git", "branch", "--show-current"], cwd=ROOT, text=True).strip(), "lvgl": "9.3.0", "display": [320, 480],
    "renderer": "native C project source, LVGL software draw, RGB565 framebuffer", 
    "demo_fixture": "tests/ui/test_ui_review.c (not in firmware)",
    "cover_preview": "Explicit production menu DEMO only; no Cover transport or network commands",
    "onboarding_fixture": "tests/ui/test_ui.c (host radio fixture, not hardware evidence)",
    "frames": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.glob("*.png"))},
}
(out / "render-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
guide = ROOT / "docs/ui-review/README_PL.txt"
if guide.exists():
    shutil.copyfile(guide, out / "README_PL.txt")
archive = out.parent / f"{out.name}.zip"
with tempfile.NamedTemporaryFile(prefix='review-',suffix='.tmp',dir=out.parent,delete=False) as temp:
    temporary=Path(temp.name)
with zipfile.ZipFile(temporary, "w", zipfile.ZIP_DEFLATED) as z:
    for path in sorted(out.iterdir()):
        if path.is_file():
            z.write(path, path.name)
with zipfile.ZipFile(temporary) as z:
    assert z.testzip() is None
os.replace(temporary,archive)
print(f"Exported {len(frames)} native frames, contact_sheet.png, manifest; package: {archive}")
