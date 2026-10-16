#!/usr/bin/env python3
"""Run real pinned ESP-IDF SPIFFS wrapper/core on host simulated NOR."""
import argparse
import base64
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--idf-source", type=Path, default=ROOT / ".test-deps/esp-idf")
p.add_argument("--sanitize", action="store_true")
p.add_argument("--ubsan-only", action="store_true")
p.add_argument("--cjson-source", type=Path)
p.add_argument("--factory-image", type=Path, help="Mount this exact preformatted image on simulated NOR")
p.add_argument("--verification-report", type=Path)
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
            ROOT / "components/dsr_platform", ROOT / "components/dsr_platform/include",
            ROOT / "components/dsr_config/include", ROOT / "components/dsr_core/include"]
json_include = a.cjson_source.resolve() if a.cjson_source else Path("/usr/include/cjson")
if not a.cjson_source and not (json_include / "cJSON.h").exists():
    json_include = ROOT / ".test-deps"
if not (json_include / "cJSON.h").exists():
    raise SystemExit("Install libcjson-dev to run actual checkpoint tests")
includes.append(json_include)
sources = [spiffs / "esp_spiffs.c", spiffs / "spiffs_api.c",
           *sorted((spiffs / "spiffs/src").glob("*.c")),
           ROOT / "components/dsr_platform/dsr_storage.c",
           ROOT / "components/dsr_platform/dsr_config_health.c",
           ROOT / "components/dsr_config/dsr_config.c",
           ROOT / "components/dsr_config/dsr_config_store.c",
           ROOT / "components/dsr_config/dsr_photo_name.c",
           ROOT / "components/dsr_core/dsr_core.c",
           ROOT / "tests/storage/test_idf_config.c"]
json_library = ["-l:libcjson.so.1"]
if a.cjson_source:
    for name, digest in {
        "cJSON.c": "298581a04a36c0165da4b0aade235c23088cb2faa58651d720ea2f3706ed0b0d",
        "cJSON.h": "25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee",
    }.items():
        if hashlib.sha256((json_include / name).read_bytes()).hexdigest() != digest:
            raise SystemExit("Unexpected pinned IDF cJSON content")
    sources.append(json_include / "cJSON.c")
    json_library = []
(ROOT / "build-ui").mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(dir=ROOT / "build-ui", prefix="idf-spiffs-") as tmp:
    binary = Path(tmp) / "test_idf_spiffs"
    cc = os.environ.get("CC", "cc")
    objects = []
    for n, source in enumerate(sources):
        obj = Path(tmp) / f"source-{n}.o"
        # Original IDF configuration intentionally uses unaligned index tables.
        # Disable only that upstream check; retain all sanitizers for our code.
        extra = (["-fno-sanitize=alignment"] if source.is_relative_to(spiffs) else [] if source.name == "cJSON.c"
                 else ["-Wall", "-Wextra", "-Werror", "-Wpedantic"])
        if source.name == "dsr_config_store.c":
            # Only libc file calls bridge to the genuine registered IDF VFS.
            # There is no FILE buffering in this host bridge; fsync is real.
            # glibc fortified inline fread bypasses renamed FILE providers.
            # Our bridge handles sizes explicitly and has no libc FILE object.
            extra += ["-U_FORTIFY_SOURCE", "-D_FORTIFY_SOURCE=0"]
            extra += [f"-D{name}=test_{name}" for name in
                      ["fopen", "fread", "fwrite", "fgetc", "fflush", "fsync", "fileno", "fclose"]]
        subprocess.run([cc, *flags, *extra,
                        *[v for folder in includes for v in ["-I", str(folder)]],
                        "-c", str(source), "-o", str(obj)], check=True)
        objects.append(obj)
    subprocess.run([cc, *flags, *map(str, objects), "-Wl,--wrap=SPIFFS_format", *json_library, "-lm", "-o", str(binary)], check=True)
    image = zlib.decompress(base64.b64decode((ROOT / "tests/storage/spiffs-idf-5.5.4.zlib.base64").read_bytes()))
    if hashlib.sha256(image).hexdigest() != "bd0be5201aa20655b2885a39b017070523bb8c44f275d837a200c949179fb544":
        raise SystemExit("Unexpected synthetic upstream-generated image")
    reference = Path(tmp) / "reference.bin"
    reference.write_bytes(image)
    command = [str(binary), str(reference)]
    digest = None
    if a.factory_image:
        factory = a.factory_image.resolve()
        if factory.stat().st_size != 0x3e0000:
            raise SystemExit("Wrong factory SPIFFS length")
        digest = hashlib.sha256(factory.read_bytes()).hexdigest()
        command.append(str(factory))
    completed = subprocess.run(command, check=True, text=True, stdout=subprocess.PIPE)
    print(completed.stdout, end="")
    if a.factory_image and hashlib.sha256(factory.read_bytes()).hexdigest() != digest:
        raise SystemExit("Factory input file changed during simulated mount")
    if a.verification_report:
        if not a.factory_image:
            raise SystemExit("--verification-report requires --factory-image")
        count = re.search(r"(\d+) process-exit cuts", completed.stdout)
        if not count or "PASS: exact FACTORY SPIFFS mounts unchanged" not in completed.stdout:
            raise SystemExit("Missing actual factory mount proof")
        a.verification_report.write_text(json.dumps(dict(
            idf="735507283d5b2f9fb363a1901172dbd9e847945d",
            spiffs="0dbb3f71c5f6fae3747a9d935372773762baf852",
            image_bytes=0x3e0000, image_sha256=digest,
            exact_image_mounted_without_format_or_mutation=True,
            actual_default_and_rooms_checkpoint_save_remount=True,
            actual_gc_process_exit_cuts=int(count.group(1)),
            provider="host simulated NOR and VFS bridge; not physical Flash or RTOS",
        ), indent=2) + "\n")
