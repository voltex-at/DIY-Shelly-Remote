#!/usr/bin/env python3
"""Three-skin x PL/EN/DE atlas from genuine LVGL captures (test_skin_atlas_ui).

No UI is drawn here: frames are copied from the real 320x480 captures, and
captions stay outside every frame. Fails when a screen is missing for any
skin/language, so every screen is proven in every remaining skin.
"""
import argparse
import collections
import json
from pathlib import Path
import re
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
THEMES = ('graphite', 'light', 'cyber_blue')
LANGS = ('pl', 'en', 'de')
p = argparse.ArgumentParser()
p.add_argument('--screens', type=Path, default=ROOT / 'build-ui/asan/screenshots')
p.add_argument('--out', type=Path, default=ROOT / 'build-ui/skin-atlas')
a = p.parse_args()
name = re.compile(r'^skin-(graphite|light|cyber_blue)-(pl|en|de)-(\d\d-[a-z0-9-]+)$')
frames = {}
for source in sorted(a.screens.glob('skin-*.ppm')):
    match = name.match(source.stem)
    if not match:
        raise SystemExit(f'Unexpected skin capture name: {source.name}')
    image = Image.open(source).convert('RGB')
    if image.size != (320, 480):
        raise SystemExit(f'{source.name} is not 320x480')
    frames[match.groups()] = image
screens = sorted({screen for _, _, screen in frames})
missing = [f'{t}/{l}/{s}' for s in screens for t in THEMES for l in LANGS if (t, l, s) not in frames]
if not screens or missing:
    raise SystemExit('Skin atlas incomplete: ' + (', '.join(missing[:20]) if missing else 'no captures'))

out = a.out
(out / 'frames').mkdir(parents=True, exist_ok=True)
(out / 'compare').mkdir(exist_ok=True)
for (theme, lang, screen), image in frames.items():
    folder = out / 'frames' / theme / lang
    folder.mkdir(parents=True, exist_ok=True)
    image.save(folder / f'{screen}.png', optimize=True)

# One review sheet per screen: rows = skins, columns = languages, half scale.
W, H, PAD, CAP = 160, 240, 8, 16
for screen in screens:
    sheet = Image.new('RGB', (PAD + len(LANGS) * (W + PAD) + 70, CAP + PAD + len(THEMES) * (H + PAD)), (24, 26, 30))
    draw = ImageDraw.Draw(sheet)
    draw.text((PAD, 2), screen, fill=(230, 230, 230))
    for c, lang in enumerate(LANGS):
        draw.text((70 + PAD + c * (W + PAD), 2), lang.upper(), fill=(200, 200, 200))
    for r, theme in enumerate(THEMES):
        y = CAP + PAD + r * (H + PAD)
        draw.text((PAD, y + H // 2), theme, fill=(200, 200, 200))
        for c, lang in enumerate(LANGS):
            sheet.paste(frames[(theme, lang, screen)].resize((W, H), Image.LANCZOS), (70 + PAD + c * (W + PAD), y))
    sheet.save(out / 'compare' / f'{screen}.png', optimize=True)

audit = a.screens / 'skin-audit.tsv'
rows = [line.split('\t') for line in audit.read_text(encoding='utf-8').splitlines()[1:] if line.strip()] if audit.exists() else []
counts = collections.Counter((row[0].split('-')[0], row[1]) for row in rows)
summary = {'screens': len(screens), 'frames': len(frames), 'skins': list(THEMES), 'languages': list(LANGS),
           'findings': {theme: {kind: counts[(theme, kind)] for kind in sorted({row[1] for row in rows})} for theme in THEMES}}
(out / 'summary.json').write_text(json.dumps(summary, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
if audit.exists():
    (out / 'skin-audit.tsv').write_text(audit.read_text(encoding='utf-8'), encoding='utf-8')
print(f'PASS: skin atlas {len(screens)} screens x {len(THEMES)} skins x {len(LANGS)} languages = {len(frames)} genuine LVGL frames; '
      f'audit findings {dict(counts)}')
