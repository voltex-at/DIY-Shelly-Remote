#!/usr/bin/env python3
"""Create the signed v2 OTA manifest for one GitHub Release (docs/OTA_MANIFEST_PL.md).

The private key is read ONLY from an environment variable (default
DSR_OTA_SIGNING_KEY = base64 of the 32-byte Ed25519 seed); it is never taken
from a command line argument or written anywhere. The app image is validated
(ESP32-S3 image, appended SHA, descriptor) and its embedded PROJECT_VER base
must equal --version, so an installed release never offers itself again.
"""
import argparse
import base64
import hashlib
import json
import os
import re
import struct
import sys
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey, Ed25519PublicKey

sys.path.insert(0, str(Path(__file__).resolve().parent))
from package_factory import validate_app  # noqa: E402  (same image checks as FACTORY/UPDATE)

HARDWARE = "lcdwiki-es3c35p-n16r8"
LAYOUT = "es3c35p-16m-ota6m-dsrdata3e0000-v1"
REPO = "https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/"
SLOT = 0x600000
SEGMENT = re.compile(r"[A-Za-z0-9_.-]+")


def message(m):
    """Exact signed bytes; must match dsr_ota_manifest.c and tools/test_ota.py."""
    out = b"DSR-OTA-MANIFEST\0\x02"
    for field in ("hardware_id", "partition_layout", "version", "channel", "url", "key_id", "signature_algorithm"):
        value = m[field].encode("ascii")
        out += struct.pack(">H", len(value)) + value
    return out + struct.pack(">Q", m["size"]) + bytes.fromhex(m["sha256"])


def build(app: bytes, version: str, channel: str, tag: str, asset: str, key_id: str, private: Ed25519PrivateKey):
    if not re.fullmatch(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version):
        raise SystemExit("version must be canonical X.Y.Z")
    if channel not in ("stable", "dev"):
        raise SystemExit("channel must be stable or dev")
    if not (SEGMENT.fullmatch(tag) and SEGMENT.fullmatch(asset)) or ".." in tag + asset:
        raise SystemExit("tag/asset must be single URL segments")
    if not re.fullmatch(r"[a-z0-9_-]{1,64}", key_id):
        raise SystemExit("bad key id")
    if not 0 < len(app) <= SLOT:
        raise SystemExit("app image does not fit one OTA slot")
    try:
        embedded = validate_app(app)
    except ValueError as error:
        raise SystemExit(f"invalid app image: {error}")
    base = re.match(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(?:-|$)", embedded)
    if not base or base.group(0).rstrip("-") != version:
        raise SystemExit(f"app PROJECT_VER {embedded!r} does not start with release version {version}")
    m = dict(schema_version=2, hardware_id=HARDWARE, partition_layout=LAYOUT, version=version, channel=channel,
             url=REPO + tag + "/" + asset, size=len(app), sha256=hashlib.sha256(app).hexdigest(),
             key_id=key_id, signature_algorithm="ed25519")
    if len(m["url"]) > 256:
        raise SystemExit("url too long")
    m["signature"] = base64.b64encode(private.sign(message(m))).decode("ascii")
    wire = json.dumps(m, separators=(",", ":")).encode("ascii")
    if len(wire) > 2048:
        raise SystemExit("manifest exceeds 2048 bytes")
    return m, wire


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--app", type=Path, required=True)
    p.add_argument("--version", required=True)
    p.add_argument("--channel", choices=["stable", "dev"], required=True)
    p.add_argument("--tag", required=True)
    p.add_argument("--asset", default="diy_shelly_remote.bin")
    p.add_argument("--key-id", required=True)
    p.add_argument("--key-env", default="DSR_OTA_SIGNING_KEY")
    p.add_argument("--public-hex", help="expected public key; refuse to sign with any other key")
    p.add_argument("--out", type=Path, required=True)
    a = p.parse_args()
    encoded = os.environ.get(a.key_env, "").strip()
    try:
        seed = base64.b64decode(encoded, validate=True)
    except Exception:
        seed = b""
    if len(seed) != 32:
        raise SystemExit(f"{a.key_env} must hold base64 of a 32-byte Ed25519 seed")
    private = Ed25519PrivateKey.from_private_bytes(seed)
    public = private.public_key().public_bytes_raw()
    if a.public_hex and public.hex() != a.public_hex.lower():
        raise SystemExit("signing key does not match the trusted public key")
    m, wire = build(a.app.read_bytes(), a.version, a.channel, a.tag, a.asset, a.key_id, private)
    Ed25519PublicKey.from_public_bytes(public).verify(base64.b64decode(m["signature"]), message(m))
    a.out.write_bytes(wire)
    print(f"PASS: signed {a.channel} manifest {m['version']} size={m['size']} sha256={m['sha256']} key={a.key_id}")


if __name__ == "__main__":
    main()
