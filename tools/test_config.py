#!/usr/bin/env python3
"""Exercise the actual shared C schema and dual-checkpoint storage on disk."""
import argparse
import os
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
    sources=["components/dsr_config/dsr_config.c","components/dsr_config/dsr_config_store.c","components/dsr_core/dsr_core.c","tests/test_config.c"]
    subprocess.run([os.environ.get("CC","cc"),*flags,"-I",str(include),"-I",str(ROOT/"components/dsr_config/include"),"-I",str(ROOT/"components/dsr_core/include"),*[str(ROOT/s) for s in sources],"-l:libcjson.so.1","-lm","-o",str(directory/"test_config")],check=True)
    subprocess.run([str(directory/"test_config"),str(directory)],check=True)
