"""Audit actual target link: real verifier retained, no fixture trust/signing."""
import argparse
import json
import hashlib
from pathlib import Path
import subprocess

p=argparse.ArgumentParser()
p.add_argument("--elf",type=Path,required=True)
p.add_argument("--bin",type=Path,required=True)
p.add_argument("--out",type=Path,required=True)
args=p.parse_args()
trust=Path(__file__).resolve().parents[1]/"components/dsr_ota/dsr_ota_trust.c"
# Owner-approved production anchor (2026-10-17, #24). Any change to the
# provider requires an explicit trust-policy review and a deliberate update
# of this pin, never a silent fixture key.
APPROVED_KEY_ID=b"voltex-ota-2026-1"
APPROVED_KEY=bytes.fromhex("23adf99d68e6ce2315a7b0b2b53c5839d9dd15e39ee748a24ec68841861a47fe")
# Hash canonical repository line endings, including a Windows CRLF checkout.
assert hashlib.sha256(trust.read_bytes().replace(b"\r\n",b"\n")).hexdigest()=="ff62fed25beb9cc2a64a043ee1cd06a8daf46be6c5ca2e5480714289250406ed","unreviewed production trust provider"
nm=subprocess.check_output(["xtensa-esp32s3-elf-nm","-S","--defined-only",str(args.elf)],text=True)
symbols={line.split()[-1]:line.split() for line in nm.splitlines() if len(line.split()) in (3,4)}
# The HTTP endpoint uses the guarded receiver; the compatibility wrapper may
# correctly disappear under target link-time garbage collection.
required=["dsr_ota_manifest_verify","dsr_ota_trusted_key","crypto_ed25519_check","crypto_eddsa_check_equation","dsr_ota_image_create","dsr_ota_image_create_dev_checksum","dsr_ota_image_update","dsr_ota_image_finish","dsr_ota_image_context_size","dsr_ota_upload_receive_guarded","dsr_ota_upload_sha256"]
for name in required:
    assert name in symbols,name
assert "dsr_ota_fetch_manifest" in symbols,"manifest fetch backend not retained"
assert "dsr_ota_fetch_app" in symbols,"app validation chain not retained"
assert "dsr_ota_release_candidate_parse" in symbols,"release candidate parser not retained"
sha512=[name for name in symbols if name.startswith("crypto_sha512") or name=="sha512_compress"]
assert sha512,"SHA512 implementation missing from target"
for name in ["crypto_ed25519_sign","crypto_ed25519_key_pair","crypto_eddsa_sign","crypto_eddsa_key_pair"]:
    assert name not in symbols,name
image=args.bin.read_bytes()
for marker in [b"rfc8032-host-only",b"rfc8032-host-alias",bytes.fromhex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a")]:
    assert marker not in image,"test trust anchor leaked into target"
assert image.count(APPROVED_KEY)==1 and APPROVED_KEY_ID in image,"approved production anchor missing from target"
record={"required_symbols":required,"sha512_symbols":sha512,"test_key_absent":True,"signing_symbols_absent":True,"production_trust_store":{"key_id":APPROVED_KEY_ID.decode(),"public_key":APPROVED_KEY.hex()},"network_or_install_tested":False}
args.out.write_text(json.dumps(record,indent=2)+"\n")
print("PASS: actual target verifier/Ed25519/SHA512 retained; test key and signing absent; only approved production anchor voltex-ota-2026-1 present")
