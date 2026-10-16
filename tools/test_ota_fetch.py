#!/usr/bin/env python3
"""Real bounded fetch/parser/Ed25519, independently signed host HTTP fixtures."""
import argparse,base64,copy,json,os,struct,subprocess,tempfile
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');args=p.parse_args()
key=Ed25519PrivateKey.from_private_bytes(bytes.fromhex('9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60'))
base=dict(schema_version=2,hardware_id='lcdwiki-es3c35p-n16r8',partition_layout='es3c35p-16m-ota6m-dsrdata3e0000-v1',version='1.2.4',channel='stable',url='https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/app.bin',size=1234,sha256=bytes(range(32)).hex(),key_id='rfc8032-host-only',signature_algorithm='ed25519')
def sign(m):
    m=copy.deepcopy(m);message=b'DSR-OTA-MANIFEST\0\x02'
    for name in ('hardware_id','partition_layout','version','channel','url','key_id','signature_algorithm'):
        value=m[name].encode('ascii');message+=struct.pack('>H',len(value))+value
    message+=struct.pack('>Q',m['size'])+bytes.fromhex(m['sha256'])
    m['signature']=base64.b64encode(key.sign(message)).decode();return m
valid=sign(base);bad=dict(valid,sha256='10'+base['sha256'][2:]);unknown=sign(dict(base,key_id='unknown-host-key'))
with tempfile.TemporaryDirectory(prefix='dsr-fetch-') as tmp:
    tmp=Path(tmp);fixture=tmp/'signed.bin'
    with fixture.open('wb') as f:
        for m in (valid,bad,unknown):
            wire=json.dumps(m,separators=(',',':')).encode('ascii');f.write(struct.pack('>I',len(wire)));f.write(wire)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    elif args.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
    for folder in ['tests/ota_fetch','tests/support','components/dsr_ota/include','components/dsr_ota','components/dsr_core/include','components/dsr_crypto/vendor','components/dsr_crypto/vendor/optional','.test-deps']:flags+=['-I',str(ROOT/folder)]
    flags+=['-I','/usr/include/cjson']
    sources=['components/dsr_ota/dsr_ota_fetch.c','components/dsr_ota/dsr_ota_manifest.c','components/dsr_core/dsr_core.c','components/dsr_crypto/vendor/monocypher.c','components/dsr_crypto/vendor/optional/monocypher-ed25519.c','tests/test_ota_fetch.c']
    for trusted in (True,False):
        extra=['-DDSR_FETCH_HOST_TEST_TRUST=1'] if trusted else [str(ROOT/'components/dsr_ota/dsr_ota_trust.c')]
        binary=tmp/('host-test-trust' if trusted else 'production-empty-trust')
        subprocess.run([os.environ.get('CC','cc'),*flags,*extra,*[str(ROOT/s) for s in sources],'-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=free','-Wl,--wrap=time','-l:libcjson.so.1','-o',str(binary)],check=True)
        subprocess.run([str(binary),str(fixture)],check=True)
