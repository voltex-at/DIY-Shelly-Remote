#!/usr/bin/env python3
"""Arrange genuine unchanged LVGL pixels for final before/after review.

No UI is drawn here. Captions live outside the original 320x480 frames.
Before captures must come from a separate checkout of the pinned source base.
"""
import argparse
import hashlib
import json
import zipfile
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--screens', type=Path, default=ROOT/'build-ui/ubsan/screenshots')
p.add_argument('--before', type=Path)
p.add_argument('--out', type=Path, default=ROOT/'build-ui/cyber-atlas')
a = p.parse_args()
a.out.mkdir(parents=True, exist_ok=True)
font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 14)
def frame(path):
 im = Image.open(path).convert('RGB')
 assert im.size == (320,480), path
 return im

def board(items, filename, cols):
 image = Image.new('RGB',(cols*340+20,50+((len(items)+cols-1)//cols)*530),'#071323')
 d = ImageDraw.Draw(image)
 d.text((20,12),'LVGL 9.3.0 | 320 x 480 | host fixtures; no hardware acceptance',font=font,fill='#00e5ff')
 for i,(title,path) in enumerate(items):
  x,y=20+i%cols*340,50+i//cols*530
  d.text((x,y),title,font=font,fill='#ecf4ff')
  im=frame(path);image.paste(im,(x,y+28))
  assert image.crop((x,y+28,x+320,y+508)).tobytes()==im.tobytes()
 image.save(a.out/filename)

names=['battery-top-pl','battery-bottom-pl','dashboard-two-rows-pl','delete-confirm-pl','delete-write-error-pl','delete-success-pl','delete-nvs-pending','delete-nvs-retry','delete-nvs-success','header-voltage-pl','header-provider-fixture-7-pl','header-charging-provider-fixture-pl']
items=[(name,a.screens/('atlas-native-final-'+name+'.ppm')) for name in names]
board(items,'Cyber_Blue_Zmiany.png',3)
records=[]
if a.before:
 for name in ['dashboard-pl','battery-voltage-pl','battery-memory-pl','hardware-ready-pl','system-pl','diagnostics-0-top-pl','diagnostics-0-bottom-pl','trv-online-pl','battery-reading-de','diagnostics-0-bottom-de']:
  filename='atlas-native-reference-'+name+'.ppm'
  before,after=a.before/filename,a.screens/filename
  assert before.exists() and after.exists(),filename
  records.append({'name':name,'base_sha':'5d33c2c9a755ba5b5c3692b97d766f6a6e21a04e','before_rgb_sha256':hashlib.sha256(frame(before).tobytes()).hexdigest(),'after_rgb_sha256':hashlib.sha256(frame(after).tobytes()).hexdigest()})
 board([(label,path) for r in records for label,path in [('PRZED / '+r['name'],a.before/('atlas-native-reference-'+r['name']+'.ppm')),('PO / '+r['name'],a.screens/('atlas-native-reference-'+r['name']+'.ppm'))]],'Cyber_Blue_Przed_Po.png',2)
(a.out/'comparison-manifest.json').write_text(json.dumps({'renderer':'untouched production LVGL frames','hardware_acceptance':False,'pairs':records},ensure_ascii=False,indent=2)+'\n')
print(f'Arranged {len(items)} final frames and {len(records)} before/after pairs; source pixels verified.')

with zipfile.ZipFile(a.out.parent/'Cyber_Blue_Atlas.zip','w',zipfile.ZIP_DEFLATED) as archive:
 for path in sorted(a.out.rglob('*')):
  if path.is_file():archive.write(path,path.relative_to(a.out))
