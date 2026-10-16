"""Offline synthetic images; no USB, user dump, secrets or repair in CI."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import zlib

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("recovery", ROOT / "tools/storage_recovery.py")
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)


def table():
    data = b"".join(struct.pack("<HBBII16sI", 0x50AA, kind, sub, start, size, name.encode(), 0)
                    for name, kind, sub, start, size in r.EXPECTED)
    data += b"\xeb\xeb" + b"\xff" * 14 + hashlib.md5(data).digest()
    return data.ljust(4096, b"\xff")


def filesystem(content=b"TEST", filename=b"object", formatted_blocks=None):
    data = bytearray(b"\xff" * r.DATA_SIZE)
    blocks = r.DATA_SIZE // 4096
    for i in range(blocks):
        struct.pack_into("<H", data, i * 4096 + 252, (0x20140529 ^ 256 ^ ((formatted_blocks or blocks) - i)) & 0xFFFF)
    cursor = 1
    def allocate(obj, span, flags):
        nonlocal cursor
        if cursor % 16 == 0:
            cursor += 1
        page = cursor
        cursor += 1
        struct.pack_into("<H", data, page // 16 * 4096 + (page % 16 - 1) * 2, obj)
        struct.pack_into("<HHB", data, page * 256, obj, span, flags)
        return page
    chunks = [content[i:i + 251] for i in range(0, len(content), 251)]
    pages = []
    for n, chunk in enumerate(chunks):
        p = allocate(1, n, 0xFC)
        data[p * 256 + 5:p * 256 + 5 + len(chunk)] = chunk
        pages.append(p)
    first = allocate(0x8001, 0, 0xF8)
    struct.pack_into("<IB", data, first * 256 + 8, len(content), 1)
    data[first * 256 + 13:first * 256 + 49] = filename.ljust(32, b"\0") + b"\0" * 4
    for n, page in enumerate(pages[:103]):
        struct.pack_into("<H", data, first * 256 + 49 + 2 * n, page)
    for span, offset in enumerate(range(103, len(pages), 124), 1):
        index = allocate(0x8001, span, 0xF8)
        for n, page in enumerate(pages[offset:offset + 124]):
            struct.pack_into("<H", data, index * 256 + 8 + 2 * n, page)
    return data, pages, first


class RecoveryTests(unittest.TestCase):
    def test_table_and_md5(self):
        self.assertTrue(r.partitions(table())["md5_verified"])
        for at in [0, 8, 192 + 16]:
            data = bytearray(table())
            data[at] ^= 1
            with self.assertRaises(ValueError):
                r.partitions(data)

    def test_geometry_and_blank(self):
        for image in [b"", b"\xff" * 4096]:
            with self.assertRaises(ValueError):
                r.analyze(image)
        report, files = r.analyze(b"\xff" * r.DATA_SIZE)
        self.assertTrue(report["all_erased"])
        self.assertFalse(report["safe_to_erase"])
        self.assertEqual(files, [])
        with self.assertRaises(ValueError):
            r.analyze(b"\xff" * r.DATA_SIZE, page_size=0)

    def test_indexed_file_and_long_index(self):
        for content in [b"", b"TEST", bytes(range(256)) * 160]:
            data, _, _ = filesystem(content)
            before = r.sha(data)
            report, files = r.analyze(data)
            self.assertEqual(len(files), 1)
            self.assertEqual(files[0]["_content"], content)
            self.assertEqual(before, r.sha(data))
            self.assertFalse(report["mount_verified"])
            self.assertEqual(report["matching_length_magic_blocks"], r.DATA_SIZE // 4096)

    def test_wrong_length_magic_preserved(self):
        data, _, _ = filesystem(formatted_blocks=1024)
        report, _ = r.analyze(data)
        self.assertEqual(report["matching_length_magic_blocks"], 0)
        self.assertEqual(report["possible_formatted_sizes"][0]["bytes"], 0x400000)
        self.assertFalse(report["safe_to_erase"])

    def test_checkpoint_crc(self):
        payload = b'{"schema":1,"revision":7}'
        content = struct.pack("<4I", 0x44535231, len(payload), zlib.crc32(payload), 7) + payload
        data, pages, _ = filesystem(content, b"config0.dat")
        _, files = r.analyze(data)
        self.assertTrue(files[0]["checkpoint_crc_valid"])
        data[pages[0] * 256 + 5 + 8] ^= 1
        _, files = r.analyze(data)
        self.assertFalse(files[0]["checkpoint_crc_valid"])

    def test_torn_wrong_lookup_span_and_duplicate_head(self):
        original, pages, head = filesystem(b"x" * 900)
        for at in [pages[1] * 256, pages[1] * 256 + 2, pages[1] * 256 + 4,
                   head * 256 + 49, pages[1] // 16 * 4096 + (pages[1] % 16 - 1) * 2]:
            data = original.copy()
            data[at] ^= 1
            self.assertEqual(r.spiffs_candidates(data), [])
        data = original.copy()
        duplicate = 10
        data[duplicate * 256:(duplicate + 1) * 256] = data[head * 256:(head + 1) * 256]
        struct.pack_into("<H", data, (duplicate - 1) * 2, 0x8001)
        self.assertEqual(r.spiffs_candidates(data), [])

    def test_backup_only_reads_and_twice(self):
        commands = []
        def run(command, **kwargs):
            if command[-1] == "version":
                return SimpleNamespace(stdout="esptool v5.1.0", returncode=0)
            commands.append(command)
            size = int(command[-2], 16)
            content = table() if size == 4096 else b"\xff" * size
            Path(command[-1]).write_bytes(content)
            return SimpleNamespace(returncode=0)
        with tempfile.TemporaryDirectory() as temp, patch.object(r.subprocess, "run", run):
            target = Path(temp) / "PRIVATE"
            r.backup("TEST_PORT", target)
            self.assertEqual(len(commands), 6)
            self.assertEqual((target / "dsrdata.bin").stat().st_size, r.DATA_SIZE)
            self.assertTrue((target / "PRIVATE-backup-report.json").exists())
            for i, command in enumerate(commands):
                self.assertIn("read-flash", command)
                self.assertIn("default-reset" if i == 0 else "no-reset", command)
                self.assertNotIn("write-flash", command)
                self.assertNotIn("erase-flash", command)
                self.assertEqual(command[command.index("--after") + 1], "no-reset")

    def test_mismatched_table_stops_before_private_reads(self):
        reads = []
        def run(command, **kwargs):
            if command[-1] == "version":
                return SimpleNamespace(stdout="esptool.py v4.9.0", returncode=0)
            reads.append(command)
            Path(command[-1]).write_bytes(b"\0" * 4096)
            return SimpleNamespace(returncode=0)
        with tempfile.TemporaryDirectory() as temp, patch.object(r.subprocess, "run", run):
            with self.assertRaises(ValueError):
                r.backup("TEST_PORT", Path(temp) / "PRIVATE")
            self.assertEqual(len(reads), 2)


if __name__ == "__main__":
    unittest.main()
