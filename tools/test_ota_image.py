#!/usr/bin/env python3
"""Build actual streaming validator with genuine OpenSSL-backed SDK SHA shim."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--sanitize',action='store_true')
p.add_argument('--ubsan-only',action='store_true')
p.add_argument('--image',type=Path,help='Read-only app BIN artifact fixture; does not authenticate its publisher')
p.add_argument('--sha256',help='Optional known artifact-audit SHA256; checked before the image fixture runs')
args=p.parse_args()
if args.sha256 and not args.image:p.error('--sha256 requires --image')
image=bytearray(b'\x5a'*1024);image[:512]=bytes(512)
image[0]=0xe9;image[1]=1;image[12]=9;image[29]=1
image[32:36]=bytes.fromhex('3254cdab');image[48:54]=b'1.2.4\0';image[80:98]=b'diy_shelly_remote\0'
marker=b'DSR-OTA:lcdwiki-es3c35p-n16r8:config-api-1\0';image[600:600+len(marker)]=marker
assert len(image)==1024 and len(b'diy_shelly_remote\0')==18 and image[97]==0
with tempfile.TemporaryDirectory(prefix='dsr-ota-image-') as tmp:
    tmp=Path(tmp);digest=tmp/'python-sha256.bin';digest.write_bytes(hashlib.sha256(image).digest());binary=tmp/'test-image'
    flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    elif args.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
    flags += [x for folder in ['tests/ota_image','components/dsr_ota/include'] for x in ['-I',str(ROOT/folder)]]
    sources=['components/dsr_ota/dsr_ota_image.c','tests/ota_image/sha256_adapter.c','tests/test_ota_image.c']
    subprocess.run([os.environ.get('CC','cc'),*flags,*[str(ROOT/s) for s in sources],'-Wl,--wrap=calloc','-lcrypto','-o',str(binary)],check=True)
    subprocess.run([str(binary),str(digest)],check=True)
    if args.image:
        artifact=args.image.resolve();actual=hashlib.sha256(artifact.read_bytes()).hexdigest()
        if args.sha256 and actual!=args.sha256.lower():raise SystemExit('Artifact SHA256 differs from supplied audit digest')
        print(f'ARTIFACT FIXTURE ONLY sha256={actual}; no signed metadata or installation authorization',flush=True)
        digest.write_bytes(bytes.fromhex(actual));binary=tmp/'test-image-file'
        sources=['components/dsr_ota/dsr_ota_image.c','tests/ota_image/sha256_adapter.c','tests/test_ota_image_file.c']
        subprocess.run([os.environ.get('CC','cc'),*flags,*[str(ROOT/s) for s in sources],'-lcrypto','-o',str(binary)],check=True)
        subprocess.run([str(binary),str(artifact),str(digest)],check=True)
