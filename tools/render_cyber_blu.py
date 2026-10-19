#!/usr/bin/env python3
"""Convert genuine RGB565 LVGL test PPMs to PNGs and comparison boards.
No synthetic widget rendering. The captions identify model data / host tests.
Run after test_cyber_blu_ui, test_blu_ui, and measure_cyber_blu_heap.py.
"""
import argparse
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--screens',type=Path,default=ROOT/'build-ui/native/screenshots');a=p.parse_args()
out=ROOT/'docs/cyber-blue/blu-control';out.mkdir(parents=True,exist_ok=True)
old=out/'before';old.mkdir(exist_ok=True)
for src in a.screens.glob('native-*-blu-*.ppm'):
    if 'baseline' not in src.name:
        im=Image.open(src);assert im.size==(320,480);im.save(out/(src.stem+'.png'))
for i in range(5):
    src=a.screens/f'native-baseline-blu-{i}.ppm'
    if src.exists():Image.open(src).save(old/f'baseline-{i}.png')
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',16)
small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',13)
board=Image.new('RGB',(656,536),'#070d17');d=ImageDraw.Draw(board)
d.text((8,8),'Przed: Cyber Blue / 0.7.58-dev',font=font,fill='#a5bcd5');d.text((336,8),'Po: nowy układ BLU',font=font,fill='#00cfff')
board.paste(Image.open(old/'baseline-0.png'),(0,34));board.paste(Image.open(out/'native-820-blu-window-closed-pl.png'),(336,34))
d.text((8,517),'Rzeczywiste LVGL 9.3, 320×480 • dane testowe, radio modelowane',font=small,fill='#a5bcd5');board.save(out/'comparison-before-after.png')
board=Image.new('RGB',(976,1050),'#070d17');d=ImageDraw.Draw(board)
for i,title in enumerate(['Okno zamknięte','Okno otwarte','Brak danych','Drzwi zamknięte','Drzwi otwarte','Drzwi: brak danych']):
    x=(i%3)*328;y=(i//3)*522;d.text((x+8,y+6),title,font=font,fill='#f2f7ff')
    board.paste(Image.open(next(out.glob(f'native-{820+i}-blu-*.png'))),(x,y+30))
board.save(out/'state-variants.png')
print('Saved',len(list(out.glob('*.png'))),'native PNGs / boards and five baseline frames')
