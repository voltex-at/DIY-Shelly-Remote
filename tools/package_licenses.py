#!/usr/bin/env python3
"""Retain actual SDK/component license files alongside binary test artifacts."""
import argparse
from pathlib import Path
import shutil

parser = argparse.ArgumentParser()
parser.add_argument("--idf", type=Path, required=True)
parser.add_argument("--managed", type=Path, required=True)
parser.add_argument("--out", type=Path, required=True)
args = parser.parse_args()
for required in [args.idf / "LICENSE", args.idf / "components/json/cJSON/LICENSE",
                 args.idf / "components/lwip/lwip/COPYING"]:
    if not required.is_file():
        raise SystemExit(f"Required dependency notice missing: {required}")
count = 0
for name, root in [("ESP-IDF", args.idf / "components"), ("managed", args.managed)]:
    for source in sorted(root.rglob("*")):
        if not source.is_file() or source.is_symlink():
            continue
        if not source.name.lower().startswith(("license", "licence", "copying", "notice")):
            continue
        dest = args.out / name / source.relative_to(root)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, dest)
        count += 1
args.out.mkdir(parents=True, exist_ok=True)
shutil.copyfile(args.idf / "LICENSE", args.out / "ESP-IDF-LICENSE")
(args.out / "README.txt").write_text(
    "License/notice files from the pinned ESP-IDF and managed-component checkouts.\n"
    "This intentionally includes notices for SDK components not linked into this image.\n"
    "It does not imply all SDK components share the project's MIT license.\n")
print(f"Packaged {count + 1} dependency license/notice files")
