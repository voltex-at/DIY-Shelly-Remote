#!/usr/bin/env python3
"""Generate the Ed25519 OTA publisher key pair (run ONCE, offline).

Writes the PRIVATE key to a 0600 file outside the repository and prints the
PUBLIC key as a C initializer for components/dsr_ota/dsr_ota_trust.c.
The private key must never be committed. Store it as the GitHub Actions
secret DSR_OTA_SIGNING_KEY (base64 of the 32-byte seed) and keep an offline
backup: losing it means future releases cannot be installed via device OTA.
"""
import argparse
import base64
import os
import re
from pathlib import Path
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--key-id", required=True, help="lowercase id, e.g. voltex-ota-2026-1")
p.add_argument("--out", type=Path, required=True, help="private key file (outside the repo)")
a = p.parse_args()
if not re.fullmatch(r"[a-z0-9_-]{1,64}", a.key_id):
    raise SystemExit("key id must match [a-z0-9_-]{1,64}")
out = a.out.resolve()
if ROOT in out.parents:
    raise SystemExit("Refusing to write the private key inside the repository")
if out.exists():
    raise SystemExit("Refusing to overwrite an existing key file")
key = Ed25519PrivateKey.generate()
seed = key.private_bytes(serialization.Encoding.Raw, serialization.PrivateFormat.Raw, serialization.NoEncryption())
public = key.public_key().public_bytes(serialization.Encoding.Raw, serialization.PublicFormat.Raw)
fd = os.open(out, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
with os.fdopen(fd, "w") as f:
    f.write(f"key_id={a.key_id}\nprivate_seed_base64={base64.b64encode(seed).decode()}\n")
print(f"PRIVATE key written to {out} (0600). GitHub secret DSR_OTA_SIGNING_KEY = the private_seed_base64 value.")
print(f"key_id: {a.key_id}")
print("public_hex:", public.hex())
print("C: {" + ",".join(f"0x{b:02x}" for b in public) + "}")
