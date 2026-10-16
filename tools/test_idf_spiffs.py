#!/usr/bin/env python3
"""Run real pinned ESP-IDF SPIFFS wrapper/core on host simulated NOR."""
import argparse
import base64
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--idf-source", type=Path, default=ROOT / ".test-deps/esp-idf")
p.add_argument("--sanitize", action="store_true")
p.add_argument("--ubsan-only", action="store_true")
a = p.parse_args()
idf = a.idf_source.resolve()
spiffs = idf / "components/spiffs"
for folder, expected in [(idf, "735507283d5b2f9fb363a1901172dbd9e847945d"),
                         (spiffs / "spiffs", "0dbb3f71c5f6fae3747a9d935372773762baf852")]:
    actual = subprocess.check_output(["git", "-C", str(folder), "rev-parse", "HEAD"], text=True).strip()
    if actual != expected:
        raise SystemExit(f"Unexpected SPIFFS test dependency revision: {folder}")
    if subprocess.check_output(["git", "-C", str(folder), "status", "--porcelain", "--untracked-files=no"], text=True).strip():
        raise SystemExit(f"Modified SPIFFS test dependency: {folder}")
flags = ["-std=gnu11", "-g", "-O1", "-fno-sanitize-recover=all", "-include", "inttypes.h", "-include", "sys/stat.h",
         "-include", "stdbool.h", "-include", "time.h"]
if a.sanitize:
    flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
elif a.ubsan_only:
    flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer"]
includes = [ROOT / "tests/storage/idf_support", spiffs / "include", spiffs,
            spiffs / "spiffs/src", ROOT / "tests/actor", ROOT / "tests/support",
            ROOT / "components/dsr_platform", ROOT / "components/dsr_platform/include"]
sources = [spiffs / "esp_spiffs.c", spiffs / "spiffs_api.c",
           *sorted((spiffs / "spiffs/src").glob("*.c")),
           ROOT / "components/dsr_platform/dsr_storage.c",
           ROOT / "components/dsr_platform/dsr_config_health.c",
           ROOT / "tests/storage/test_idf_spiffs.c"]
(ROOT / "build-ui").mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(dir=ROOT / "build-ui", prefix="idf-spiffs-") as tmp:
    binary = Path(tmp) / "test_idf_spiffs"
    cc = os.environ.get("CC", "cc")
    objects = []
    for n, source in enumerate(sources):
        obj = Path(tmp) / f"source-{n}.o"
        # Original IDF configuration intentionally uses unaligned index tables.
        # Disable only that upstream check; retain all sanitizers for our code.
        extra = ["-fno-sanitize=alignment"] if source.is_relative_to(spiffs) else ["-Wall", "-Wextra", "-Werror", "-Wpedantic"]
        subprocess.run([cc, *flags, *extra,
                        *[v for folder in includes for v in ["-I", str(folder)]],
                        "-c", str(source), "-o", str(obj)], check=True)
        objects.append(obj)
    subprocess.run([cc, *flags, *map(str, objects), "-o", str(binary)], check=True)
    image = zlib.decompress(base64.b64decode((ROOT / "tests/storage/spiffs-idf-5.5.4.zlib.base64").read_bytes()))
    if hashlib.sha256(image).hexdigest() != "bd0be5201aa20655b2885a39b017070523bb8c44f275d837a200c949179fb544":
        raise SystemExit("Unexpected synthetic upstream-generated image")
    reference = Path(tmp) / "reference.bin"
    reference.write_bytes(image)
    subprocess.run([str(binary), str(reference)], check=True)
