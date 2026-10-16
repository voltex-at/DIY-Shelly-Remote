#!/usr/bin/env python3
"""Exercise actual LAN task and radio handoff barrier with a deterministic RTOS."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--sanitize", action="store_true")
p.add_argument("--ubsan-only", action="store_true")
a = p.parse_args()
flags = ["-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-Wpedantic"]
if a.sanitize:
    flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
elif a.ubsan_only:
    flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer"]
with tempfile.TemporaryDirectory(prefix="dsr-lan-barrier-") as tmp:
    out = Path(tmp) / "test_lan_barrier"
    folders = ["tests/lan", "tests/actor", "tests/support", "components/dsr_core/include", "components/dsr_platform/include", "components/dsr_config/include", ".test-deps"]
    subprocess.run([os.environ.get("CC", "cc"), *flags, *[x for folder in folders for x in ["-I", str(ROOT / folder)]],
                    str(ROOT / "tests/test_lan_barrier.c"), str(ROOT / "components/dsr_platform/dsr_discovery_model.c"), str(ROOT / "components/dsr_core/dsr_core.c"),
                    "-l:libcjson.so.1", "-o", str(out)], check=True, cwd=tmp)
    subprocess.run([str(out)], check=True, cwd=tmp)
