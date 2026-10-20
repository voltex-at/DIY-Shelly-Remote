#!/usr/bin/env python3
"""Convert the project garden texture to decoder-free native RGB565 in Flash."""
from pathlib import Path
import hashlib
import json
import struct
from PIL import Image, ImageOps

ROOT = Path(__file__).resolve().parents[1]
source = ROOT/'assets/cyber-scenes/garden-dusk.png'
image = ImageOps.fit(Image.open(source).convert('RGB'), (152,128), method=Image.Resampling.LANCZOS)
pixels = image.tobytes()
raw = b''.join(struct.pack('<H', ((r>>3)<<11)|((g>>2)<<5)|(b>>3)) for r,g,b in zip(pixels[0::3],pixels[1::3],pixels[2::3]))
assert len(raw)==38912
out = ROOT/'components/dsr_ui/brand/cyber_garden_dusk.rgb565'
out.write_bytes(raw)
(source.parent/'garden-manifest.json').write_text(json.dumps({'source':source.name,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'size':[152,128],'format':'little-endian RGB565, direct Flash image, no decoder/cache copy','bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()},indent=2)+'\n')
print('Built garden texture:',len(raw),'Flash bytes; no decoded pixel buffer')
