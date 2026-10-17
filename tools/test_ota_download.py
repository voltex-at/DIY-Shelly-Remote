#!/usr/bin/env python3
"""Independent signed manifest/app chain; HTTP SDK MODEL, genuine crypto/hash."""
import argparse,base64,copy,hashlib,json,os,struct,subprocess,tempfile
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');p.add_argument('--image',type=Path);p.add_argument('--sha256');args=p.parse_args()
if args.sha256 and not args.image:p.error('--sha256 requires --image')
if args.image:
    image=args.image.read_bytes();digest=hashlib.sha256(image).hexdigest()
    if args.sha256 and digest!=args.sha256.lower():raise SystemExit('Artifact differs from supplied audit SHA256')
    print(f'HOST ARTIFACT FIXTURE ONLY size={len(image)} sha256={digest}; RFC test signer, NOT publisher release trust',flush=True)
else:
    image=bytearray(b'\x5a'*8192);image[:512]=bytes(512);image[0]=0xe9;image[1]=1;image[12]=9;image[29]=1
    image[32:36]=bytes.fromhex('3254cdab');image[80:98]=b'diy_shelly_remote\0';marker=b'DSR-OTA:lcdwiki-es3c35p-n16r8:config-api-1\0';image[600:643]=marker
    assert len(image)==8192
key=Ed25519PrivateKey.from_private_bytes(bytes.fromhex('9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60'))
base=dict(schema_version=2,hardware_id='lcdwiki-es3c35p-n16r8',partition_layout='es3c35p-16m-ota6m-dsrdata3e0000-v1',version='1.2.4',channel='stable',url='https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/app.bin',size=len(image),sha256=hashlib.sha256(image).hexdigest(),key_id='rfc8032-host-only',signature_algorithm='ed25519')
def sign(m):
    m=copy.deepcopy(m);message=b'DSR-OTA-MANIFEST\0\x02'
    for name in ('hardware_id','partition_layout','version','channel','url','key_id','signature_algorithm'):
        value=m[name].encode('ascii');message+=struct.pack('>H',len(value))+value
    message+=struct.pack('>Q',m['size'])+bytes.fromhex(m['sha256']);m['signature']=base64.b64encode(key.sign(message)).decode();return m
valid=sign(base);bad=dict(valid,signature=base64.b64encode(bytes(64)).decode());unknown=sign(dict(base,key_id='unknown-host-key'))
small=sign(dict(base,size=511));bad_url=sign(dict(base,url='https://github.com/other/repo/releases/download/v1/app.bin'))
with tempfile.TemporaryDirectory(prefix='dsr-download-') as tmp:
    tmp=Path(tmp);fixture=tmp/'chain.bin'
    with fixture.open('wb') as f:
        for data in [bytes(image)]+[json.dumps(m,separators=(',',':')).encode('ascii') for m in (valid,bad,unknown,small,bad_url)]:f.write(struct.pack('>I',len(data)));f.write(data)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    elif args.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
    for folder in ['tests/ota_fetch','tests/ota_image','tests/support','components/dsr_ota/include','components/dsr_ota','components/dsr_core/include','components/dsr_crypto/vendor','components/dsr_crypto/vendor/optional','.test-deps']:flags+=['-I',str(ROOT/folder)]
    flags+=['-I','/usr/include/cjson']
    sources=['components/dsr_ota/dsr_ota_fetch.c','components/dsr_ota/dsr_ota_manifest.c','components/dsr_ota/dsr_ota_image.c','components/dsr_core/dsr_core.c','components/dsr_crypto/vendor/monocypher.c','components/dsr_crypto/vendor/optional/monocypher-ed25519.c','tests/ota_image/sha256_adapter.c','tests/test_ota_download.c']
    for trusted in (True,False):
        extra=['-DDSR_DOWNLOAD_HOST_TEST_TRUST=1'] if trusted else [str(ROOT/'components/dsr_ota/dsr_ota_trust.c')]
        binary=tmp/('host-test-trust' if trusted else 'production-empty')
        subprocess.run([os.environ.get('CC','cc'),*flags,*extra,*[str(ROOT/s) for s in sources],'-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=free','-Wl,--wrap=time','-Wl,--wrap=EVP_DigestFinal_ex','-l:libcjson.so.1','-lcrypto','-o',str(binary)],check=True)
        subprocess.run([str(binary),str(fixture)],check=True)
