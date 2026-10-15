#!/usr/bin/env python3
"""Build real pinned LVGL + firmware UI natively, without SDL or CMake."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--sanitize", action="store_true")
p.add_argument("--ubsan-only", action="store_true")
p.add_argument("--lvgl-source", type=Path, default=ROOT / ".test-deps/lvgl")
args = p.parse_args()
lvgl = args.lvgl_source.resolve()
revision = subprocess.check_output(["git", "-C", str(lvgl), "rev-parse", "HEAD"], text=True).strip()
if revision != "c033a98afddd65aaafeebea625382a94020fe4a7":
    raise SystemExit("UI validation requires the pinned LVGL v9.3.0 commit")
build = ROOT / "build-ui" / ("asan" if args.sanitize else "ubsan" if args.ubsan_only else "native")
build.mkdir(parents=True, exist_ok=True)
screens = build / "screenshots"
screens.mkdir(exist_ok=True)
cc = os.environ.get("CC", "cc")
flags = ["-std=c11", "-O1", "-g", "-DLV_KCONFIG_IGNORE", "-DDSR_BOARD_ES3C35P=1",
         f'-DLV_CONF_PATH="{ROOT / "tests/ui/lv_conf.h"}"']
if args.sanitize:
    flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
elif args.ubsan_only:
    flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer"]
includes = [lvgl, ROOT / "tests/ui", ROOT / "tests/support",
            *[ROOT / f"components/{name}/include" for name in ["dsr_core", "dsr_platform", "dsr_ui", "dsr_board"]],
            ROOT / "components/dsr_ui"]
flags += [item for folder in includes for item in ["-I", str(folder)]]
sources = list(sorted((lvgl / "src").rglob("*.c")))
sources += [ROOT / name for name in ["tests/ui/test_ui.c", "components/dsr_ui/dsr_ui.c",
            "components/dsr_ui/dsr_ui_network.c", "components/dsr_ui/dsr_ui_wifi.c",
            "components/dsr_platform/dsr_wifi_policy.c"]]
config_hash = hashlib.sha256((ROOT / "tests/ui/lv_conf.h").read_bytes() + repr(flags).encode()).hexdigest()
def compile_source(source):
    obj = build / (hashlib.sha256(str(source).encode()).hexdigest()[:20] + ".o")
    stamp = obj.with_suffix(".sha256")
    fingerprint = hashlib.sha256(source.read_bytes() + config_hash.encode()).hexdigest()
    if obj.exists() and stamp.exists() and stamp.read_text() == fingerprint and source.is_relative_to(lvgl):
        return obj
    warnings = [] if source.is_relative_to(lvgl) else ["-Wall", "-Wextra", "-Werror", "-Wpedantic"]
    subprocess.run([cc, *flags, *warnings, "-c", str(source), "-o", str(obj)], check=True,
                   env=dict(os.environ, TMPDIR=str(build)))
    stamp.write_text(fingerprint)
    return obj
with ThreadPoolExecutor(max_workers=6) as pool:
    objects = list(pool.map(compile_source, sources))
binary = build / "test_ui"
subprocess.run([cc, *flags, *map(str, objects), "-lm", "-o", str(binary)], check=True,
               env=dict(os.environ, TMPDIR=str(build)))
subprocess.run([str(binary), str(screens)], check=True, cwd=build)
