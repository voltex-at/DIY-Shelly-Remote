#!/usr/bin/env python3
"""Release signer vs. the REAL firmware verifier and REAL production trust provider.

Always: ephemeral keys are rejected (unknown id -> UNTRUSTED_KEY, approved id
with a foreign key -> BAD_SIGNATURE) and the signer refuses mismatched input.
With DSR_OTA_SIGNING_KEY set (owner machine / release job): the genuine key
produces a manifest the firmware accepts, and tampering is rejected.
"""
import argparse, base64, hashlib, os, struct, subprocess, sys, tempfile
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import ota_sign_manifest as signer  # noqa: E402
APPROVED_ID, APPROVED_HEX = "voltex-ota-2026-1", "23adf99d68e6ce2315a7b0b2b53c5839d9dd15e39ee748a24ec68841861a47fe"
OK, INVALID, POLICY, UNTRUSTED, BAD = range(5)
p = argparse.ArgumentParser(); p.add_argument("--sanitize", action="store_true"); p.add_argument("--cjson-source", type=Path); a = p.parse_args()

def image(version: str) -> bytes:
    """Minimal structurally valid ESP32-S3 app image with a descriptor."""
    desc = struct.pack("<I", 0xabcd5432) + bytes(12) + version.encode().ljust(32, b"\0") + bytes(256 - 48)
    header = bytes([0xe9, 1, 2, 0x2f]) + bytes(8) + struct.pack("<H", 9) + bytes(9) + bytes([1])
    data = header + struct.pack("<II", 0x3c000020, len(desc)) + desc
    checksum = 0xef
    for b in desc: checksum ^= b
    end = (len(data) + 16) // 16 * 16 - 1
    data += bytes(end - len(data)) + bytes([checksum])
    return data + hashlib.sha256(data).digest()

checks = 0
with tempfile.TemporaryDirectory(prefix="dsr-signer-") as tmp:
    tmp = Path(tmp); binary = tmp / "verify"
    cj = a.cjson_source.resolve() if a.cjson_source else None
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-g"] + (["-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if a.sanitize else [])
    inc = ["components/dsr_ota/include", "components/dsr_ota", "components/dsr_core/include", "components/dsr_crypto/vendor", "components/dsr_crypto/vendor/optional"]
    srcs = ["components/dsr_ota/dsr_ota_manifest.c", "components/dsr_ota/dsr_ota_trust.c", "components/dsr_core/dsr_core.c",
            "components/dsr_crypto/vendor/monocypher.c", "components/dsr_crypto/vendor/optional/monocypher-ed25519.c", "tests/test_ota_signer.c"]
    extra = ["-I", str(cj), str(cj / "cJSON.c")] if cj else ["-I", "/usr/include/cjson", "-l:libcjson.so.1"]
    subprocess.run([os.environ.get("CC", "cc"), *flags, *[x for i in inc for x in ("-I", str(ROOT / i))], *[str(ROOT / s) for s in srcs], *extra, "-o", str(binary)], check=True)

    def verify(wire, expected, current="0.7.36", channel=0, fields=()):
        global checks
        path = tmp / "m.json"; path.write_bytes(wire)
        subprocess.run([str(binary), str(path), str(expected), current, str(channel), *map(str, fields)], check=True); checks += 1

    app = image("0.7.37")
    ephemeral = Ed25519PrivateKey.generate()
    _, wire = signer.build(app, "0.7.37", "stable", "v0.7.37", "diy_shelly_remote.bin", "someone-else", ephemeral)
    verify(wire, UNTRUSTED)
    _, wire = signer.build(app, "0.7.37", "stable", "v0.7.37", "diy_shelly_remote.bin", APPROVED_ID, ephemeral)
    verify(wire, BAD)
    for bad in [("0.7.38",), ("0.7",)]:
        try: signer.build(app, bad[0], "stable", "v0.7.37", "diy_shelly_remote.bin", APPROVED_ID, ephemeral)
        except SystemExit: checks += 1
        else: raise AssertionError("signer accepted a version not embedded in the app")
    try: signer.build(app + b"x", "0.7.37", "stable", "v0.7.37", "diy_shelly_remote.bin", APPROVED_ID, ephemeral)
    except SystemExit: checks += 1
    else: raise AssertionError("signer accepted a corrupt image")
    genuine = "skipped (no DSR_OTA_SIGNING_KEY)"
    seed = os.environ.get("DSR_OTA_SIGNING_KEY")
    if seed:
        key = Ed25519PrivateKey.from_private_bytes(base64.b64decode(seed))
        assert key.public_key().public_bytes_raw().hex() == APPROVED_HEX, "secret does not match the firmware anchor"
        for channel, ch in (("stable", 0), ("dev", 1)):
            m, wire = signer.build(app, "0.7.37", channel, "v0.7.37", "diy_shelly_remote.bin", APPROVED_ID, key)
            verify(wire, OK, channel=ch, fields=(m["version"], m["size"], m["sha256"]))
            verify(wire, POLICY, channel=1 - ch)              # wrong channel
            verify(wire, POLICY, current="0.7.37", channel=ch)  # not newer than installed
            verify(wire.replace(b'"size":%d' % m["size"], b'"size":%d' % (m["size"] + 1)), BAD, channel=ch)
        genuine = "genuine production key accepted, tampering/channel/downgrade rejected"
print(f"PASS: {checks} OTA signer checks against real verifier + production trust; {genuine}")
