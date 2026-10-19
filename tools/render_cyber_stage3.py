#!/usr/bin/env python3
"""Export untouched native LVGL frames and Stage 3 before/after boards.

Run tools/test_ui.py first. Pillow is only used for the labeled contact sheets.
The 320x480 frames themselves are lossless copies of the RGB565 render output.
"""
from pathlib import Path
import hashlib
import io
import json
import zipfile
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
source=ROOT/'build-ui/native/screenshots'
folder=ROOT/'docs/cyber-blue'
target=folder/'stage3'
target.mkdir(exist_ok=True)
before=ROOT/'output/stage3-before'
if not before.exists():before=target/'before'
def save(image,path):
    buffer=io.BytesIO();image.save(buffer,format='PNG',optimize=True)
    data=buffer.getvalue();path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    assert path.read_bytes()==data
    with Image.open(path) as check:check.verify()
def frame(name,dest):
    path=source/(name+'.ppm')
    with Image.open(path) as image:
        assert image.size==(320,480)
        rgb=image.convert('RGB');save(rgb,dest)
        assert Image.open(dest).tobytes()==rgb.tobytes()

# Existing gallery keeps its familiar paths; no synthetic UI image is added.
for path in sorted(source.glob('6[0-2][0-9]-*.ppm')):
    frame(path.stem,folder/(path.stem+'.png'))
chosen=['630-cyber-dashboard-editor-pl','631-cyber-room-editor-pl','632-cyber-room-keyboard-pl',
        '633-cyber-memory-rows-pl','634-cyber-room-photo-pl','635-cyber-room-delete-confirm-pl',
        '636-cyber-device-name-keyboard-pl','637-cyber-scene-keyboard-pl','638-cyber-onboarding-keyboard-pl',
        '640-cyber-blu-offline','641-cyber-blu-gateway-error','642-cyber-blu-no-data','643-cyber-home-long-name',
        '650-cyber-supplementary-icons','651-cyber-scene-review-pl','652-cyber-group-review-pl',
        '661-cyber-wifi-password-pl','662-cyber-wifi-password-error-de','663-cyber-ota-error-de','664-cyber-ota-boot-warning-de',
        '632-cyber-room-keyboard-de','633-cyber-memory-rows-de','634-cyber-room-photo-de']
for name in chosen:frame(name,target/(name+'.png'))

font_path='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
font=ImageFont.truetype(font_path,14)
pairs=[('Pulpit','600-cyber-home','600-cyber-home'),('Roleta','602-cyber-cover','602-cyber-cover'),
       ('Czujnik BLU','603-cyber-blu-closed','603-cyber-blu-closed'),('Bateria','607-cyber-diagnostics','607-cyber-diagnostics'),
       ('OTA','606-cyber-firmware-error','606-cyber-firmware-error'),('Wi-Fi','617-cyber-wifi-networks','617-cyber-wifi-networks'),
       ('Edycja pokoju','610-cyber-room-editor','610-cyber-room-editor'),('Edycja pulpitu','630-cyber-dashboard-editor','630-cyber-dashboard-editor-pl'),
       ('Klawiatura / formularz','612-cyber-keyboard','612-cyber-keyboard')]
for index in range(3):
    sheet=Image.new('RGB',(3*664,522),'#07101c');draw=ImageDraw.Draw(sheet)
    for col,(title,old,new) in enumerate(pairs[index*3:index*3+3]):
        x=col*664;draw.text((x+8,4),title,fill='#f2f7ff',font=font)
        draw.text((x+8,23),'PRZED',fill='#a5bcd5',font=font);draw.text((x+336,23),'PO',fill='#00e5ff',font=font)
        old_path=before/(old+'.png');before_dest=target/'before'/(old+'.png')
        before_dest.parent.mkdir(exist_ok=True);before_dest.write_bytes(old_path.read_bytes())
        sheet.paste(Image.open(old_path),(x+8,42));sheet.paste(Image.open(source/(new+'.ppm')),(x+336,42))
    save(sheet,target/f'before-after-{index+1}.png')

overview_names=['600-cyber-home','602-cyber-cover','603-cyber-blu-closed','607-cyber-diagnostics',
                '633-cyber-memory-rows-pl','606-cyber-firmware-error','617-cyber-wifi-networks','634-cyber-room-photo-pl',
                '630-cyber-dashboard-editor-pl','632-cyber-room-keyboard-pl','618-cyber-wifi-password','619-cyber-firmware-progress']
sheet=Image.new('RGB',(4*332,3*510),'#07101c');draw=ImageDraw.Draw(sheet)
for i,name in enumerate(overview_names):
    x=(i%4)*332+6;y=(i//4)*510;draw.text((x,y+4),name.split('-cyber-',1)[1],fill='#a5bcd5',font=font)
    sheet.paste(Image.open(source/(name+'.ppm')),(x,y+27))
save(sheet,target/'overview.png')

baseline=ROOT/'output/stage3-legacy-baseline.json'
legacy=json.loads(baseline.read_text()) if baseline.exists() else json.loads((target/'legacy-frames.json').read_text())['sha256']
assert all((source/n).exists() and hashlib.sha256((source/n).read_bytes()).hexdigest()==sha for n,sha in legacy.items()),'Legacy render changed'
(target/'legacy-frames.json').write_text(json.dumps({'base_commit':'ca805ab23d07780dc34e99e7e8b013350aa031ea','unchanged':len(legacy),'sha256':legacy},indent=2)+'\n')
records={}
archive=ROOT/'output/DIY-Shelly-Remote-Stage3-LVGL-320x480.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for path in sorted(source.glob('*.ppm')):
        if not(path.name.startswith('6') or path.name.startswith('738') or path.name.startswith('739')):continue
        number=int(path.name.split('-')[0])
        if 630<=number<=639 and not path.stem.endswith(('-pl','-en','-de')):continue
        with Image.open(path) as image:
            assert image.size==(320,480);buf=io.BytesIO();image.save(buf,format='PNG');data=buf.getvalue()
        name=path.stem+'.png';z.writestr(name,data);records[name]=hashlib.sha256(data).hexdigest()
    manifest={'lvgl':'9.3.0','display':[320,480],'renderer':'Production C + native LVGL software renderer / RGB565','backend':'Radio, ADC, storage, updater models; no physical ESP32 test','frames':records}
    z.writestr('render-manifest.json',json.dumps(manifest,indent=2)+'\n')
    for p in target.glob('before-after-*.png'):z.write(p,p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
(target/'render-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(f'Exported {len(records)} native LVGL frames, 9 before/after pairs, overview; {len(legacy)} unchanged legacy frames; {archive}')
