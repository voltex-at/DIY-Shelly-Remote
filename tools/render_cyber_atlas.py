#!/usr/bin/env python3
"""Complete atlas from genuine LVGL captures, with exact-frame provenance.

No UI is drawn here. Captions and page furniture stay outside every 320x480
frame. Only captures tagged by the active production Cyber theme qualify.
"""
import argparse
import hashlib
import html
import json
import math
from pathlib import Path
import re
import subprocess
import textwrap
import zipfile
from PIL import Image, ImageDraw, ImageFont
from reportlab.pdfgen import canvas
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--screens', type=Path, default=ROOT/'build-ui/ubsan/screenshots')
p.add_argument('--out', type=Path, default=ROOT/'build-ui/cyber-atlas')
p.add_argument('--base-ref', default='8a8dffdc7961cd60456a892eb90085ee52a0cfdb', help='Source base for this quality pass')
a = p.parse_args()
a.out.mkdir(parents=True, exist_ok=True)
frames = a.out/'frames'
frames.mkdir(exist_ok=True)
for old in frames.glob('CB-*.png'):
 old.unlink()
groups = [
 ('01 Pulpit i pokoje', r'home|room-detail|rooms$|devices$|24-tiles|dashboard-picker'),
 ('02 Przekazniki', r'relay'),
 ('03 Rolety', r'cover'),
 ('04 BLU i termostat', r'blu-|trv-'),
 ('05 Oswietlenie i sensory', r'dimmer|rgb-|(?:^|-)ht(?:-|$)|usage'),
 ('06 Ustawienia Shelly', r'recovered-(general|settings)|shelly-'),
 ('07 Konfiguracja i edytory', r'editor|room-|device-(name|card|registry|list)|dashboard-|tile-|scene-|group-|registry|storage-'),
 ('08 Siec i dodawanie', r'wifi-|network|manual-|lan-|ap-|ble-|onboard|add-method'),
 ('09 System i OTA', r'.*'),
]
def category(name):
 for label, pattern in groups:
  if re.search(pattern, name): return label

records = []
hashes = {}
for source in sorted(a.screens.glob('atlas-*.ppm')):
 im = Image.open(source).convert('RGB')
 assert im.size == (320, 480), source
 raw_hash = hashlib.sha256(im.tobytes()).hexdigest()
 name = source.stem.removeprefix('atlas-').removeprefix('native-')
 if raw_hash in hashes:
  hashes[raw_hash]['aliases'].append(name)
  continue
 record = {'name':name, 'category':category(name), 'aliases':[], 'rgb_sha256':raw_hash,
           'source':source.name, 'size':[320,480]}
 hashes[raw_hash] = record
 records.append(record)
records.sort(key=lambda r:(r['category'], r['name']))
assert len(records) >= 150, f'Incomplete suite: only {len(records)} frames'
font_path = Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
font = ImageFont.truetype(str(font_path), 14) if font_path.exists() else ImageFont.load_default()
heading_font = ImageFont.truetype(str(font_path), 30) if font_path.exists() else font
if font_path.exists(): pdfmetrics.registerFont(TTFont('Atlas',str(font_path)))
pdf_font = 'Atlas' if font_path.exists() else 'Helvetica'
for i, r in enumerate(records,1):
 r['id'] = f'CB-{i:03d}'
 r['file'] = f"frames/{r['id']}-{r['name']}.png"
 im = Image.open(a.screens/r['source']).convert('RGB')
 im.save(a.out/r['file'])
 assert Image.open(a.out/r['file']).convert('RGB').tobytes()==im.tobytes()
 r['sha256'] = hashlib.sha256((a.out/r['file']).read_bytes()).hexdigest()

# One board containing every distinct capture, at native pixel dimensions.
cols, stride, rowh = 8, 340, 532
board = Image.new('RGB',(cols*stride+20,100+math.ceil(len(records)/cols)*rowh),'#071323')
d = ImageDraw.Draw(board)
d.text((20,14),'CYBER BLUE PREMIUM / COMPLETE LVGL ATLAS',font=heading_font,fill='#00e5ff')
d.text((20,58),f'{len(records)} frames | LVGL 9.3.0 | 320 x 480 | host fixtures, not hardware acceptance',font=font,fill='#b4cce3')
for i,r in enumerate(records):
 x,y=20+(i%cols)*stride,100+(i//cols)*rowh
 d.text((x,y),r['id']+' | '+r['category'][3:],font=font,fill='#00e5ff')
 for n,line in enumerate(textwrap.wrap(r['name'],width=40)[:2]):d.text((x,y+18+n*15),line,font=font,fill='#ecf4ff')
 board.paste(Image.open(a.out/r['file']),(x,y+48))
board.save(a.out/'Cyber_Blue_Wszystkie_Ekrany.png')

# Readable multipage board, with the same IDs as the large overview.
pdf = canvas.Canvas(str(a.out/'Cyber_Blue_Atlas.pdf'),pagesize=(1360,1664))
for start in range(0,len(records),12):
 pdf.setFillColorRGB(.028,.074,.137);pdf.rect(0,0,1360,1664,fill=1,stroke=0)
 pdf.setFont(pdf_font,23);pdf.setFillColorRGB(0,.9,1);pdf.drawString(24,1628,'Cyber Blue Premium - kompletny atlas LVGL')
 pdf.setFont(pdf_font,11);pdf.setFillColorRGB(.75,.83,.92)
 pdf.drawString(24,1605,f'{len(records)} klatek | 320 x 480 | Dane testowe hosta. Bez odbioru na ESP32. | {start//12+1}/{math.ceil(len(records)/12)}')
 for j,r in enumerate(records[start:start+12]):
  x,y=24+(j%4)*334,1570-(j//4)*518
  pdf.setFillColorRGB(0,.9,1);pdf.setFont(pdf_font,10);pdf.drawString(x,y,r['id']+' | '+r['category'][3:])
  pdf.setFillColorRGB(.93,.96,1);pdf.setFont(pdf_font,8)
  for n,line in enumerate(textwrap.wrap(r['name'],width=55)[:2]):pdf.drawString(x,y-13-n*10,line)
  pdf.drawImage(ImageReader(str(a.out/r['file'])),x,y-507,width=320,height=480)
 pdf.showPage()
pdf.save()

cards=[]
for r in records:
 caption=html.escape(r['id']+' | '+r['name'])
 cards.append(f'<figure data-search="{html.escape((r["category"]+" "+r["name"]+" "+" ".join(r["aliases"])).lower())}"><figcaption>{caption}</figcaption><a href="{r["file"]}"><img loading="lazy" width="320" height="480" src="{r["file"]}" alt="{caption}"></a><small>{html.escape(r["category"])}</small></figure>')
page='''<!doctype html><html lang="pl"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Cyber Blue - wszystkie ekrany</title><style>body{background:#071323;color:#f2f7ff;font:15px system-ui;margin:24px}h1{color:#00e5ff}header{position:sticky;top:0;background:#071323ee;padding:12px;z-index:1}input{padding:12px;width:min(90%,600px);background:#132c48;border:1px solid #00cfff;color:white;border-radius:8px}main{display:grid;grid-template-columns:repeat(auto-fill,minmax(320px,1fr));gap:24px}figure{margin:0;max-width:320px}figcaption{height:3em;overflow-wrap:anywhere;font-size:12px}img{display:block}small{color:#a9c2dc}</style><header><h1>Cyber Blue Premium - wszystkie ekrany</h1><p>COUNT rzeczywistych klatek LVGL 9.3.0, 320 × 480. Dane testowe hosta; bez testu na ESP32.</p><input id="filter" placeholder="Szukaj ekranu, np. BLE, roleta, keyboard, pl..."><span id="count"></span></header><main>CARDS</main><script>const f=document.querySelector('#filter'),all=[...document.querySelectorAll('figure')];function update(){let n=0;for(const x of all){x.hidden=!x.dataset.search.includes(f.value.toLowerCase());if(!x.hidden)n++}document.querySelector('#count').textContent=' '+n+' / '+all.length}f.addEventListener('input',update);update()</script></html>'''.replace('COUNT',str(len(records))).replace('CARDS','\n'.join(cards))
(a.out/'index.html').write_text(page)
manifest={'renderer':'production LVGL 9.3.0 software draw, RGB565; untouched pixels',
 'base_sha':a.base_ref,
 'source_sha':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
 'source_dirty':bool(subprocess.check_output(['git','status','--porcelain','--untracked-files=no'],cwd=ROOT,text=True)),
 'ui_tree_sha':subprocess.check_output(['git','rev-parse','HEAD:components/dsr_ui'],cwd=ROOT,text=True).strip(),
 'hardware_acceptance':False,'distinct_frames':len(records),'captures':sum(1+len(r['aliases']) for r in records),'frames':records}
(a.out/'render-manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n')
with zipfile.ZipFile(a.out.parent/'Cyber_Blue_Atlas.zip','w',zipfile.ZIP_DEFLATED) as z:
 for path in sorted(a.out.rglob('*')):
  if path.is_file():z.write(path,path.relative_to(a.out))
print(f'Exported {len(records)} distinct frames from {manifest["captures"]} captures; complete PNG/PDF/HTML/ZIP.')
