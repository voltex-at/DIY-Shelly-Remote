#!/usr/bin/env python3
"""Actual signed stream installer; software crypto genuine, SDK/Flash MODEL."""
import argparse
import base64
import copy
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--sanitize', action='store_true')
p.add_argument('--ubsan-only', action='store_true')
a = p.parse_args()
image = bytearray(b'\x5a' * 8192)
image[:512] = bytes(512)
image[0] = 0xe9
image[1] = image[29] = 1
image[12] = 9
image[32:36] = bytes.fromhex('3254cdab')
image[80:98] = b'diy_shelly_remote\0'
image[600:643] = b'DSR-OTA:lcdwiki-es3c35p-n16r8:config-api-1\0'
# Existing RFC8032 HOST fixture only. This source is never part of IDF builds.
key = Ed25519PrivateKey.from_private_bytes(bytes.fromhex('9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60'))
base = dict(schema_version=2, hardware_id='lcdwiki-es3c35p-n16r8', partition_layout='es3c35p-16m-ota6m-dsrdata3e0000-v1', version='1.2.4', channel='stable', url='https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/app.bin', size=len(image), sha256=hashlib.sha256(image).hexdigest(), key_id='rfc8032-host-only', signature_algorithm='ed25519')
def sign(value):
    value = copy.deepcopy(value)
    message = b'DSR-OTA-MANIFEST\0\x02'
    for field in ('hardware_id', 'partition_layout', 'version', 'channel', 'url', 'key_id', 'signature_algorithm'):
        encoded = value[field].encode('ascii')
        message += struct.pack('>H', len(encoded)) + encoded
    message += struct.pack('>Q', value['size']) + bytes.fromhex(value['sha256'])
    value['signature'] = base64.b64encode(key.sign(message)).decode()
    return value
valid = sign(base)
manifests = [valid, dict(valid, signature=base64.b64encode(bytes(64)).decode()), sign(dict(base, key_id='unknown-host-key')), sign(dict(base, size=511)), sign(dict(base, url='https://github.com/other/repo/releases/download/v1/app.bin'))]
with tempfile.TemporaryDirectory(prefix='dsr-install-') as temporary:
    temporary = Path(temporary)
    fixture = temporary / 'signed-fixture.bin'
    with fixture.open('wb') as out:
        for data in [bytes(image)] + [json.dumps(value, separators=(',', ':')).encode('ascii') for value in manifests]:
            out.write(struct.pack('>I', len(data)))
            out.write(data)
    flags = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-Wpedantic', '-g']
    if a.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    elif a.ubsan_only:
        flags += ['-fsanitize=undefined', '-fno-omit-frame-pointer']
    for folder in ['tests/ota_upload', 'tests/ota_fetch', 'tests/ota_image', 'tests/support', 'components/dsr_platform/include', 'components/dsr_platform', 'components/dsr_ota/include', 'components/dsr_ota', 'components/dsr_core/include', 'components/dsr_crypto/vendor', 'components/dsr_crypto/vendor/optional', '.test-deps']:
        flags += ['-I', str(ROOT / folder)]
    flags += ['-I', '/usr/include/cjson']
    stage_binary = temporary / 'stage-race'
    subprocess.run([os.environ.get('CC', 'cc'), *flags, str(ROOT / 'components/dsr_platform/dsr_ota_stage.c'), str(ROOT / 'components/dsr_platform/dsr_activity.c'), str(ROOT / 'tests/test_ota_stage.c'), '-pthread', '-o', str(stage_binary)], check=True)
    subprocess.run([str(stage_binary)], check=True)
    sources = ['components/dsr_platform/dsr_ota_install.c', 'components/dsr_platform/dsr_ota_stage.c', 'components/dsr_platform/dsr_activity.c', 'components/dsr_ota/dsr_ota_fetch.c', 'components/dsr_ota/dsr_ota_manifest.c', 'components/dsr_ota/dsr_ota_image.c', 'components/dsr_core/dsr_core.c', 'components/dsr_crypto/vendor/monocypher.c', 'components/dsr_crypto/vendor/optional/monocypher-ed25519.c', 'tests/ota_image/sha256_adapter.c', 'tests/test_ota_install.c']
    for trusted in (True, False):
        extra = ['-DDSR_DOWNLOAD_HOST_TEST_TRUST=1'] if trusted else [str(ROOT / 'components/dsr_ota/dsr_ota_trust.c')]
        binary = temporary / ('host-fixture' if trusted else 'production-empty')
        subprocess.run([os.environ.get('CC', 'cc'), *flags, *extra, *[str(ROOT / source) for source in sources], '-Wl,--wrap=malloc', '-Wl,--wrap=calloc', '-Wl,--wrap=free', '-Wl,--wrap=time', '-Wl,--wrap=EVP_DigestFinal_ex', '-l:libcjson.so.1', '-lcrypto', '-o', str(binary)], check=True)
        for mode in range(8 if trusted else 1):
            subprocess.run([str(binary), str(fixture), str(mode)], check=True)
