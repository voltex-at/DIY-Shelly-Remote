#!/usr/bin/env python3
"""Actual photo codec/loader, genuine SHA, real files and fault-model providers."""
import argparse
import base64
import os
from pathlib import Path
import random
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--ubsan-only", action="store_true")
args = parser.parse_args()
flags = ["-std=c11", "-D_POSIX_C_SOURCE=200809L", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-g"]
if args.sanitize:
    flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
elif args.ubsan_only:
    flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer"]
json_include = Path("/usr/include/cjson")
if not (json_include / "cJSON.h").exists():
    json_include = ROOT / ".test-deps"
if not (json_include / "cJSON.h").exists():
    raise SystemExit("Install libcjson-dev to run photo loader fixtures")
includes = ["tests/restart", "tests/support", "tests/actor", "components/dsr_platform/include",
            "components/dsr_platform", "components/dsr_config/include", "components/dsr_core/include"]
sources = ["components/dsr_platform/dsr_photos.c", "components/dsr_config/dsr_photo_name.c",
           "tests/restart/photo_names.c"]
wraps = ["malloc", "free", "fopen", "fclose", "opendir", "readdir", "closedir"]
with tempfile.TemporaryDirectory(prefix="dsr-photo-names-") as temp:
    directory = Path(temp)
    binary = directory / "photo_names"
    subprocess.run([os.environ.get("CC", "cc"), *flags, "-I", str(json_include),
                    *[value for include in includes for value in ["-I", str(ROOT / include)]],
                    *[str(ROOT / source) for source in sources],
                    *[f"-Wl,--wrap={name}" for name in wraps], "-lcrypto", "-o", str(binary)], check=True)
    rng = random.Random(9321)
    rows, seen = [], set()
    for _ in range(1000):
        value = rng.getrandbits(128).to_bytes(16, "big")
        name = "p" + base64.urlsafe_b64encode(value).decode().rstrip("=") + ".rgb"
        assert name not in seen
        seen.add(name)
        rows.append(f"{value.hex()} {name}")
    alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
    for position in range(1, 23):
        for index, char in enumerate(alphabet):
            name = list("pAAAAAAAAAAAAAAAAAAAAAA.rgb")
            name[position] = char
            name = "".join(name)
            expected = (base64.urlsafe_b64decode(name[1:23] + "==").hex()
                        if position < 22 or index % 16 == 0 else "-")
            rows.append(f"{expected} {name}")
    vectors = directory / "vectors.txt"
    vectors.write_text("\n".join(rows) + "\n")
    subprocess.run([str(binary), "codec", str(vectors)], check=True)
    # Production cache has no reset API: each case must use a fresh process.
    for mode in range(7):
        photos = directory / f"case-{mode}"
        photos.mkdir()
        subprocess.run([str(binary), str(photos), str(mode)], check=True)
