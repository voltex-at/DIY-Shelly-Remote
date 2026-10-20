#!/usr/bin/env python3
"""Export unmodified native LVGL pixels; gallery captions stay outside frames.

Run tools/test_ui.py --case test_cyber_recovered_ui first. No HTML or drawing
of UI elements here. All device/transport/clock values are host fixtures.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--screens', type=Path, default=ROOT/'build-ui/ubsan/screenshots')
p.add_argument('--out', type=Path, default=ROOT/'docs/cyber-blue/recovered')
args = p.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
records = []
for source in sorted(args.screens.glob('native-recovered-*.ppm')):
    image = Image.open(source).convert('RGB')
    assert image.size == (320, 480)
    path = args.out/(source.stem.removeprefix('native-recovered-')+'.png')
    image.save(path)
    assert Image.open(path).tobytes() == image.tobytes()
    records.append({'file': path.name, 'size': [320, 480], 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'rgb_sha256': hashlib.sha256(image.tobytes()).hexdigest()})
assert len(records) >= 72
names = ['relay-off-pl','relay-no-response-pl','settings-pl','trv-pl','general-pl','general-keyboard-pl']
gallery = Image.new('RGB', (3*320+32, 2*510+40), '#080f19')
draw = ImageDraw.Draw(gallery)
for i, name in enumerate(names):
    x, y = 8+(i%3)*328, 8+(i//3)*510
    draw.text((x, y), name+' / LVGL fixture', fill='#00cfff')
    gallery.paste(Image.open(args.out/(name+'.png')), (x, y+22))
gallery.save(args.out/'gallery.png')
(args.out/'render-manifest.json').write_text(json.dumps({'renderer':'production LVGL 9.3.0; RGB565; 320x480; host fixtures, not physical acceptance','files':records}, indent=2)+'\n')
print(f'Exported {len(records)} exact native frames and gallery to {args.out}')
