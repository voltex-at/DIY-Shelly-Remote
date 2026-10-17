#!/usr/bin/env python3
"""Convert the owner's 1536x1024 brand artwork into 320x480 LCD assets.

Portrait crop: scale to 480 px height (1536x1024 -> 720x480), keep the centre
320 px, so the centred logo stays whole. Output per image:
  assets/brand/<name>.png                       lossless 320x480 master
  components/dsr_ui/brand/<name>.rgb565         LVGL RGB565 (little-endian),
                                                Floyd-Steinberg dithered
Usage: build_brand_images.py --splash-dark A --background-dark B
                             --splash-light C --background-light D
"""
import argparse
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
W, H = 320, 480


def portrait(path: Path) -> Image.Image:
    image = Image.open(path).convert("RGB")
    scale = H / image.height
    resized = image.resize((round(image.width * scale), H), Image.LANCZOS)
    left = (resized.width - W) // 2
    return resized.crop((left, 0, left + W, H))


def rgb565(image: Image.Image) -> bytes:
    """Error-diffusion to 5/6/5 bits so dark gradients do not band."""
    width, height = image.size
    pixels = [list(map(float, p)) for p in image.getdata()]
    out = bytearray()
    levels = (31, 63, 31)
    for y in range(height):
        for x in range(width):
            p = pixels[y * width + x]
            q = [min(levels[c], max(0, round(p[c] * levels[c] / 255))) for c in range(3)]
            for c in range(3):
                error = p[c] - q[c] * 255 / levels[c]
                for dx, dy, weight in ((1, 0, 7), (-1, 1, 3), (0, 1, 5), (1, 1, 1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < width and ny < height:
                        pixels[ny * width + nx][c] += error * weight / 16
            value = (q[0] << 11) | (q[1] << 5) | q[2]
            out += value.to_bytes(2, "little")
    return bytes(out)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("splash-dark", "background-dark", "splash-light", "background-light"):
        p.add_argument("--" + name, type=Path, required=True)
    a = p.parse_args()
    (ROOT / "assets/brand").mkdir(parents=True, exist_ok=True)
    (ROOT / "components/dsr_ui/brand").mkdir(parents=True, exist_ok=True)
    for name in ("splash-dark", "background-dark", "splash-light", "background-light"):
        image = portrait(getattr(a, name.replace("-", "_")))
        stem = "brand_" + name.replace("-", "_")
        image.save(ROOT / f"assets/brand/{stem}.png", optimize=True)
        data = rgb565(image)
        assert len(data) == W * H * 2
        (ROOT / f"components/dsr_ui/brand/{stem}.rgb565").write_bytes(data)
        print(f"{stem}: {len(data)} bytes")


if __name__ == "__main__":
    main()
