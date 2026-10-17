#!/usr/bin/env python3
"""Check exact Monocypher 4.0.3 upstream bytes before building the verifier."""
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "components/dsr_crypto/vendor"
EXPECTED = {
    "monocypher.c": "f1f838cdd483bdebe0df0ff5c5ed60535e496f769c6a2f933ac4c0b114207123",
    "monocypher.h": "fcaf6ed771358bb4f40fba016f6518ae86ec02b1b877d2cc35ad92d3a26fd7b3",
    "optional/monocypher-ed25519.c": "ce0d2f8e32ca8f66398ba5b3456cc74327c3eff14e7b950ce7d57be9025cc453",
    "optional/monocypher-ed25519.h": "3a3035181f991a158d0e1c7567258f0bae8ba0f1f23c5512b4a1db1b3c9730ce",
    "LICENCE.md": "a5781770269d2516e52ba4863f790c10a16da4089a1e81823aee19ff1e9026b0",
}
for name, expected in EXPECTED.items():
    source = VENDOR / name
    if not source.is_file() or hashlib.sha256(source.read_bytes()).hexdigest() != expected:
        raise SystemExit(f"Pinned Monocypher checksum mismatch: {source}")
print("PASS: 5 exact Monocypher 4.0.3 files; pin ab2b16dd619ad5f6979a4fbe69cfa324a6fcc35f, BSD-2-Clause OR CC0-1.0")
