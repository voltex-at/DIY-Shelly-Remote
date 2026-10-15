#!/usr/bin/env python3
"""Compile and execute the actual portable C code without ESP-IDF."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--ubsan-only", action="store_true", help="For restricted runtimes without LeakSanitizer support")
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix="dsr-test-") as tmp:
    temp_root = Path(tmp).resolve()
    binary = temp_root / "test_core"
    test_env = dict(os.environ, TMPDIR=str(temp_root))
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-g"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    elif args.ubsan_only:
        flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer"]
    subprocess.run([os.environ.get("CC", "cc"), *flags,
                    "-I", str(ROOT / "components/dsr_core/include"),
                    str(ROOT / "components/dsr_core/dsr_core.c"),
                    str(ROOT / "tests/test_core.c"), "-o", str(binary)], check=True,
                   cwd=temp_root, env=test_env)
    subprocess.run([str(binary)], check=True, cwd=temp_root, env=test_env)
