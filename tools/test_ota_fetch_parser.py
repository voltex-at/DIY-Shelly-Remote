#!/usr/bin/env python3
"""Raw HTTP regression with external, byte-pinned IDF parser; no network fetch.

Real parser/fetch/manifest/Ed25519; esp_http_client assembly/cache/transport are
explicit host models. Neither TLS certificates nor physical ESP32 are tested.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

ROOT = Path(__file__).resolve().parents[1]
PIN = "735507283d5b2f9fb363a1901172dbd9e847945d"
HASHES = {
    "http_parser.c": "c164edcb06099b026e7ed853ed4d25536e83b4463d5e79921b28621e16588455",
    "http_parser.h": "21fcfe834198349472a878e0334dd74b8571fafc70c6dd701169f525dd463041",
}
p = argparse.ArgumentParser()
p.add_argument("--idf", type=Path, required=True)
p.add_argument("--sanitize", action="store_true")
p.add_argument("--ubsan-only", action="store_true")
args = p.parse_args()
manifest = dict(schema_version=2, hardware_id="lcdwiki-es3c35p-n16r8",
    partition_layout="es3c35p-16m-ota6m-dsrdata3e0000-v1", version="1.2.4",
    channel="stable", url="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/app.bin",
    size=1234, sha256=bytes(range(32)).hex(), key_id="rfc8032-host-only",
    signature_algorithm="ed25519")
message = b"DSR-OTA-MANIFEST\0\x02"
for field in ("hardware_id", "partition_layout", "version", "channel", "url", "key_id", "signature_algorithm"):
    value = manifest[field].encode("ascii")
    message += struct.pack(">H", len(value)) + value
message += struct.pack(">Q", manifest["size"]) + bytes.fromhex(manifest["sha256"])
key = Ed25519PrivateKey.from_private_bytes(bytes.fromhex(
    "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60"))
manifest["signature"] = base64.b64encode(key.sign(message)).decode("ascii")
with tempfile.TemporaryDirectory(prefix="dsr-raw-http-") as directory:
    tmp = Path(directory)
    # Read committed blobs, never potentially edited external working files.
    for name, expected in HASHES.items():
        data = subprocess.check_output(["git", "-C", str(args.idf), "show",
                                       f"{PIN}:components/http_parser/{name}"])
        if hashlib.sha256(data).hexdigest() != expected:
            raise SystemExit(f"Pinned IDF parser hash mismatch: {name}")
        (tmp / name).write_bytes(data)
    fixture = tmp / "manifest.json"
    fixture.write_bytes(json.dumps(manifest, separators=(",", ":")).encode("ascii"))
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-g"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    elif args.ubsan_only:
        flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer"]
    for folder in ["tests/ota_fetch", "tests/support", "components/dsr_ota/include",
                   "components/dsr_ota", "components/dsr_core/include",
                   "components/dsr_crypto/vendor", "components/dsr_crypto/vendor/optional", ".test-deps"]:
        flags += ["-I", str(ROOT / folder)]
    flags += ["-I", str(tmp), "-I", "/usr/include/cjson"]
    sources = ["components/dsr_ota/dsr_ota_fetch.c", "components/dsr_ota/dsr_ota_manifest.c",
               "components/dsr_core/dsr_core.c", "components/dsr_crypto/vendor/monocypher.c",
               "components/dsr_crypto/vendor/optional/monocypher-ed25519.c",
               "tests/test_ota_fetch_parser.c"]
    # Upstream parser's intentional state-machine fallthroughs are unchanged.
    parser_object = tmp / "http_parser.o"
    subprocess.run([os.environ.get("CC", "cc"), *flags, "-Wno-implicit-fallthrough",
                    "-c", str(tmp / "http_parser.c"), "-o", str(parser_object)], check=True)
    for trusted in (True, False):
        extra = ["-DDSR_FETCH_HOST_TEST_TRUST=1"] if trusted else [str(ROOT / "components/dsr_ota/dsr_ota_trust.c")]
        binary = tmp / ("host-test-trust" if trusted else "production-empty-trust")
        subprocess.run([os.environ.get("CC", "cc"), *flags, *extra,
                        *[str(ROOT / s) for s in sources], str(parser_object),
                        "-Wl,--wrap=time", "-l:libcjson.so.1", "-o", str(binary)], check=True)
        subprocess.run([str(binary), str(fixture)], check=True)
print(f"PASS: original external ESP-IDF parser {PIN}; both SHA256 pins verified; no vendored SDK or network fetch")
