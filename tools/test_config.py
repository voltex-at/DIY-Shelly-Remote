#!/usr/bin/env python3
"""Exercise the actual shared C schema and dual-checkpoint storage on disk."""
import argparse
import base64
import os
import random
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument("--sanitize",action="store_true")
p.add_argument("--ubsan-only",action="store_true")
a=p.parse_args()
(ROOT / "build-ui").mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(dir=ROOT / "build-ui",prefix="config-") as temp:
    directory=Path(temp)
    flags=["-std=c11","-D_POSIX_C_SOURCE=200809L","-Wall","-Wextra","-Werror","-Wpedantic","-g"]
    if a.sanitize: flags += ["-fsanitize=address,undefined","-fno-omit-frame-pointer"]
    elif a.ubsan_only: flags += ["-fsanitize=undefined","-fno-omit-frame-pointer"]
    include=Path("/usr/include/cjson") if Path("/usr/include/cjson/cJSON.h").exists() else ROOT / ".test-deps"
    sources=["components/dsr_config/dsr_config.c","components/dsr_config/dsr_config_store.c","components/dsr_config/dsr_photo_name.c","components/dsr_core/dsr_core.c","tests/test_config.c"]
    subprocess.run([os.environ.get("CC","cc"),*flags,"-I",str(include),"-I",str(ROOT/"components/dsr_config/include"),"-I",str(ROOT/"components/dsr_core/include"),*[str(ROOT/s) for s in sources],"-l:libcjson.so.1","-lm","-o",str(directory/"test_config")],check=True)
    rng=random.Random(7393)
    values=[bytes(16),bytes([255])*16,*[(1<<bit).to_bytes(16,"big") for bit in range(128)],
            *[rng.getrandbits(128).to_bytes(16,"big") for _ in range(128)]]
    vectors=directory/"photo-names.txt"
    vectors.write_text("\n".join(value.hex()+" p"+base64.urlsafe_b64encode(value).decode().rstrip("=")+".rgb" for value in values)+"\n")
    subprocess.run([str(directory/"test_config"),str(directory),str(vectors)],check=True)
