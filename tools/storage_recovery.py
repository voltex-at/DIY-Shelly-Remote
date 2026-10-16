#!/usr/bin/env python3
"""Read-only ES3C35P backup and offline SPIFFS evidence/recovery.

No erase, format, write-flash, mount or repair commands. Output is PRIVATE.
Layout reference: ESP-IDF v5.5.4 components/spiffs/spiffsgen.py and
include/spiffs_config.h (little endian, 16-bit IDs, unaligned index tables).
Detection is evidence, not proof that an unknown image can be safely erased.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import zlib

EXPECTED = [
    ("nvs", 1, 2, 0x9000, 0x10000),
    ("otadata", 1, 0, 0x19000, 0x2000),
    ("phy_init", 1, 1, 0x1B000, 0x1000),
    ("ota_0", 0, 0x10, 0x20000, 0x600000),
    ("ota_1", 0, 0x11, 0x620000, 0x600000),
    ("dsrdata", 1, 0x82, 0xC20000, 0x3E0000),
]
DATA_SIZE = 0x3E0000


def sha(data):
    return hashlib.sha256(data).hexdigest()


def partitions(data):
    if len(data) != 4096:
        raise ValueError("Partition table read must contain exactly 4096 bytes")
    entries = []
    md5_checked = False
    for offset in range(0, len(data), 32):
        record = data[offset:offset + 32]
        magic = struct.unpack_from("<H", record)[0]
        if magic == 0xFFFF:
            break
        if magic == 0xEBEB:
            if record[2:16] != b"\xff" * 14 or hashlib.md5(data[:offset]).digest() != record[16:]:
                raise ValueError("Partition table MD5 mismatch")
            md5_checked = True
            break
        if magic != 0x50AA:
            raise ValueError("Unrecognized partition table entry")
        _, kind, subtype, start, size, label, flags = struct.unpack("<HBBII16sI", record)
        if flags:
            raise ValueError("Unexpected partition flags; preserve backup and inspect manually")
        entries.append((label.split(b"\0", 1)[0].decode("ascii"), kind, subtype, start, size))
    if not md5_checked or entries != EXPECTED:
        raise ValueError("Partition table differs from ES3C35P profile; no further reads scheduled")
    return {"profile_matches": True, "md5_verified": True}


def checkpoint(data):
    if len(data) < 16:
        return False
    magic, size, crc, revision = struct.unpack_from("<4I", data)
    if magic != 0x44535231 or not 0 < size <= 24576 or size + 16 != len(data):
        return False
    if zlib.crc32(data[16:]) != crc:
        return False
    try:
        doc = json.loads(data[16:])
        return isinstance(doc, dict) and type(doc.get("revision")) is int and doc["revision"] == revision and type(doc.get("schema")) is int
    except (ValueError, UnicodeError, RecursionError):
        return False


def spiffs_candidates(data, page_size=256, block_size=4096, name_length=32, meta_length=4):
    """Extract only complete indexed files; ambiguous/torn pages are rejected.

    Does not replay a journal, choose between duplicate live headers, rewrite
    lookup pages or claim a successful mount. Files remain offline candidates.
    """
    if page_size not in (256, 512, 1024) or block_size % page_size or len(data) % block_size:
        raise ValueError("Unsupported SPIFFS geometry")
    lookup_pages = math.ceil((block_size // page_size) * 2 / page_size)
    first_offset = 8 + 4 + 1 + name_length + meta_length
    if not 16 <= name_length <= 256 or not 0 <= meta_length <= 64 or first_offset > page_size - 16:
        raise ValueError("Unsupported SPIFFS name/metadata geometry")
    pages = {}
    indexes = defaultdict(list)
    for pos in range(0, len(data), page_size):
        local = (pos % block_size) // page_size
        if local < lookup_pages:
            continue
        obj, span, flags = struct.unpack_from("<HHB", data, pos)
        if obj in (0, 0x8000, 0xFFFF) or flags not in (0xF8, 0xFC):
            continue
        lookup = pos // block_size * block_size + (local - lookup_pages) * 2
        if struct.unpack_from("<H", data, lookup)[0] != obj:
            continue
        p = data[pos:pos + page_size]
        pages[pos // page_size] = (obj, span, flags, p)
        if flags == 0xF8 and obj & 0x8000:
            indexes[(obj, span)].append((pos, p))
    files = []
    for (obj, span), heads in indexes.items():
        if span or len(heads) != 1:
            continue
        pos, head = heads[0]
        size = struct.unpack_from("<I", head, 8)[0]
        if head[12] != 1 or size > 1024 * 1024:
            continue
        name = head[13:13 + name_length].split(b"\0", 1)[0]
        if not name or len(name) >= name_length:
            continue
        needed = math.ceil(size / (page_size - 5))
        first_limit, following_limit = (page_size - first_offset) // 2, (page_size - 8) // 2
        chunks, used = [], set()
        for n in range(needed):
            index_span = 0 if n < first_limit else 1 + (n - first_limit) // following_limit
            index_item = n if not index_span else (n - first_limit) % following_limit
            matches = indexes.get((obj, index_span), [])
            if len(matches) != 1:
                break
            array = first_offset if not index_span else 8
            page = struct.unpack_from("<H", matches[0][1], array + index_item * 2)[0]
            item = pages.get(page)
            if page in used or not item or item[:3] != (obj & 0x7FFF, n, 0xFC):
                break
            used.add(page)
            chunks.append(item[3][5:])
        else:
            content = b"".join(chunks)[:size]
            # IDF VFS/spiffsgen store absolute-looking SPIFFS names.
            # Normalize only this optional prefix, never export arbitrary paths.
            basename = name[1:] if name.startswith(b"/") else name
            known = basename in (b"config0.dat", b"config1.dat")
            label = basename.decode("ascii") if known else "object.bin"
            files.append({"object": obj & 0x7FFF, "header_offset": pos, "file": label,
                          "bytes": size, "sha256": sha(content),
                          "checkpoint_crc_valid": checkpoint(content) if known else None,
                          "_content": content})
    return files


def analyze(data, **geometry):
    if len(data) != DATA_SIZE:
        raise ValueError("dsrdata image must contain exactly 0x3E0000 bytes")
    page, block = geometry.get("page_size", 256), geometry.get("block_size", 4096)
    if page not in (256, 512, 1024) or block != 4096:
        raise ValueError("Unsupported SPIFFS geometry")
    if len(data) % block:
        raise ValueError("Invalid block geometry")
    blocks = len(data) // block
    # Magic occupies the penultimate 16-bit entry in the last lookup page.
    lu_pages = math.ceil((block // page) * 2 / page)
    inferred = []
    matches = 0
    for i in range(blocks):
        observed = struct.unpack_from("<H", data, i * block + lu_pages * page - 4)[0]
        if observed == 0xFFFF:
            continue
        inferred.append(((observed ^ 0x20140529 ^ page) & 0xFFFF) + i)
        matches += observed == ((0x20140529 ^ page ^ (blocks - i)) & 0xFFFF)
    candidates = spiffs_candidates(data, **geometry)
    erased = sum(data[i:i + block] == b"\xff" * block for i in range(0, len(data), block))
    report = {"bytes": len(data), "sha256": sha(data), "all_erased": erased == blocks,
              "erased_blocks": erased, "blocks": blocks, "page_size": page, "block_size": block,
              "matching_length_magic_blocks": matches,
              "possible_formatted_sizes": [{"bytes": length * block, "observations": count}
                                          for length, count in Counter(inferred).most_common(3) if 1 < length <= 4096 and count >= 3],
              "complete_indexed_files": len(candidates),
              "files": [{k: v for k, v in c.items() if not k.startswith("_")} for c in candidates],
              "mount_verified": False, "safe_to_erase": False}
    return report, candidates


def private_write(path, data):
    with path.open("xb") as f:
        os.chmod(path, 0o600)
        f.write(data)
        f.flush()
        os.fsync(f.fileno())


def backup(port, output, baud=460800):
    # Fail closed on unknown CLI versions; never translate a user string into
    # an arbitrary esptool command or shell fragment.
    version = subprocess.run([sys.executable, "-m", "esptool", "version"], capture_output=True, text=True, check=True)
    match = re.search(r"(?:esptool(?:\.py)?\s+v?|^v?)([45])\.\d+", version.stdout, re.M | re.I)
    if not match:
        raise ValueError("Install esptool 4.x or 5.x; unsupported CLI version")
    modern = match[1] == "5"
    output.mkdir(mode=0o700, parents=True, exist_ok=False)
    records = {}
    for n, (name, offset, size) in enumerate([
        ("partition-table.bin", 0x8000, 4096), ("PRIVATE-nvs.bin", 0x9000, 0x10000),
        ("dsrdata.bin", 0xC20000, DATA_SIZE)
    ]):
        samples = []
        for repeat in range(2):
            target = output / (name + f".read{repeat}")
            before = ("default-reset" if modern else "default_reset") if n == 0 and repeat == 0 else ("no-reset" if modern else "no_reset")
            command = [sys.executable, "-m", "esptool", "--chip", "esp32s3", "--port", port, "--baud", str(baud),
                       "--before", before, "--after", "no-reset" if modern else "no_reset",
                       "read-flash" if modern else "read_flash", hex(offset), hex(size), str(target)]
            result = subprocess.run(command, capture_output=True)
            if result.returncode:
                raise ValueError(f"Read failed for {name}; retained partial private files; esptool exit {result.returncode}")
            os.chmod(target, 0o600)
            content = target.read_bytes()
            if len(content) != size:
                raise ValueError(f"Incomplete read: {name}")
            samples.append(content)
        if samples[0] != samples[1]:
            raise ValueError(f"Repeated read differs: {name}; retained both files; stop and inspect USB link")
        if n == 0:
            partitions(samples[0])  # Validate BEFORE scheduling NVS/dsrdata reads.
        private_write(output / name, samples[0])
        for repeat in range(2):
            (output / (name + f".read{repeat}")).unlink()
        records[name] = {"offset": offset, "bytes": size, "sha256": sha(samples[0]), "identical_reads": 2}
    report, _ = analyze((output / "dsrdata.bin").read_bytes())
    private_write(output / "PRIVATE-backup-report.json", (json.dumps({"files": records, "analysis": report}, indent=2) + "\n").encode())
    print("Backup verified by two identical reads. Keep ALL files private. Press EN/reset to restart the remote.")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest="action", required=True)
    b = sub.add_parser("backup")
    b.add_argument("--port", required=True)
    b.add_argument("--out", type=Path, required=True, help="New private directory, must not exist")
    b.add_argument("--baud", type=int, choices=[115200, 460800, 921600], default=460800)
    a = sub.add_parser("analyze")
    a.add_argument("image", type=Path)
    a.add_argument("--out", type=Path, required=True, help="New private directory")
    a.add_argument("--extract", action="store_true", help="Save complete offline file candidates; never writes to ESP32")
    a.add_argument("--page-size", type=int, choices=[256, 512, 1024], default=256)
    a.add_argument("--name-length", type=int, default=32)
    a.add_argument("--meta-length", type=int, default=4)
    args = p.parse_args()
    if args.action == "backup":
        backup(args.port, args.out, args.baud)
    else:
        data = args.image.read_bytes()
        report, files = analyze(data, page_size=args.page_size, name_length=args.name_length, meta_length=args.meta_length)
        args.out.mkdir(mode=0o700, parents=True, exist_ok=False)
        private_write(args.out / "PRIVATE-analysis.json", (json.dumps(report, indent=2) + "\n").encode())
        if args.extract:
            for i, candidate in enumerate(files):
                private_write(args.out / f"candidate-{i:03d}-{candidate['file']}", candidate["_content"])
        print("Offline analysis saved. No Flash writes. A missing candidate is NOT permission to erase.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"Stopped: {error}", file=sys.stderr)
        raise SystemExit(1)
