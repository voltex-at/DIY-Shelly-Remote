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
# Changing from the reviewed empty provider requires an explicit trust-policy
# review and a deliberate update of this audit, not a silent fixture key.
assert hashlib.sha256(trust.read_bytes()).hexdigest()=="a607f1e4bc9e8a6ac80336637a15d2447abc06f8f11d1bea889d6bb6cebf142b","unreviewed production trust provider"
nm=subprocess.check_output(["xtensa-esp32s3-elf-nm","-S","--defined-only",str(args.elf)],text=True)
symbols={line.split()[-1]:line.split() for line in nm.splitlines() if len(line.split()) in (3,4)}
required=["dsr_ota_manifest_verify","dsr_ota_trusted_key","crypto_ed25519_check","crypto_eddsa_check_equation","dsr_ota_image_create","dsr_ota_image_create_dev_checksum","dsr_ota_image_update","dsr_ota_image_finish","dsr_ota_image_context_size","dsr_ota_upload_receive","dsr_ota_upload_sha256"]
for name in required:
    assert name in symbols,name
sha512=[name for name in symbols if name.startswith("crypto_sha512") or name=="sha512_compress"]
assert sha512,"SHA512 implementation missing from target"
for name in ["crypto_ed25519_sign","crypto_ed25519_key_pair","crypto_eddsa_sign","crypto_eddsa_key_pair"]:
    assert name not in symbols,name
image=args.bin.read_bytes()
for marker in [b"rfc8032-host-only",b"rfc8032-host-alias",bytes.fromhex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a")]:
    assert marker not in image,"test trust anchor leaked into target"
record={"required_symbols":required,"sha512_symbols":sha512,"test_key_absent":True,"signing_symbols_absent":True,"production_trust_store":"empty","network_or_install_tested":False}
args.out.write_text(json.dumps(record,indent=2)+"\n")
print("PASS: actual target verifier/Ed25519/SHA512 retained; test key and signing absent; production trust store empty")
