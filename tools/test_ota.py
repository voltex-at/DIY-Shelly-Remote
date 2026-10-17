#!/usr/bin/env python3
"""Independent wire/canonical signing oracle; real C parser and Ed25519 backend."""
import argparse
import base64
import copy
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
args = p.parse_args()
# Public RFC 8032 fixture seed, not a publisher credential. Host tool only.
key = Ed25519PrivateKey.from_private_bytes(bytes.fromhex('9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60'))
base = dict(schema_version=2, hardware_id='lcdwiki-es3c35p-n16r8', partition_layout='es3c35p-16m-ota6m-dsrdata3e0000-v1', version='1.2.4', channel='stable', url='https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/app.bin', size=1234, sha256=bytes(range(32)).hex(), key_id='rfc8032-host-only', signature_algorithm='ed25519')
def message(m):
    result = b'DSR-OTA-MANIFEST\0\x02'
    for field in ('hardware_id','partition_layout','version','channel','url','key_id','signature_algorithm'):
        value=m[field].encode('ascii'); result += struct.pack('>H',len(value))+value
    return result+struct.pack('>Q',m['size'])+bytes.fromhex(m['sha256'])
def signed(m):
    m=copy.deepcopy(m); m['signature']=base64.b64encode(key.sign(message(m))).decode(); return m
def wire(m): return json.dumps(m,separators=(',',':')).encode('ascii')
cases=[]
def add(raw, expected, channel=0, slot=0x600000, current='1.2.3'):
    cases.append((raw if isinstance(raw,bytes) else wire(raw),expected,channel,slot,current.encode()))
valid=signed(base); raw=wire(valid)
add(valid,0)
dev=signed(dict(base,channel='dev'));add(dev,0,channel=1)
add(dict(reversed(list(valid.items()))),0)
add(b' \n'+json.dumps(valid,indent=2).encode()+b'\t\r\n',0)
for field,value in [('hardware_id','wrong-board'),('partition_layout','wrong-layout'),('version','1.2.3'),('version','1.2.2'),('version','v1.2.4'),('version','01.2.4'),('version','1.2.4-dev'),('version','4294967296.0.0'),('channel','dev'),('channel','beta'),('url','http://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1/app.bin'),('url','https://github.com.evil/voltex-at/DIY-Shelly-Remote/releases/download/v1/app.bin'),('url',base['url']+'#x'),('url',base['url']+'?x'),('url',base['url'].replace('app.bin','../app.bin')),('url',base['url'].replace('app.bin','%61pp.bin')),('url',base['url']+'/extra')]:
    add(signed(dict(base,**{field:value})),2)
add(valid,2,slot=1233);add(valid,0,slot=1234)
add(valid,2,current='1.2.4');add(valid,2,current='v1.2.3');add(valid,2,current='1.2.3-dev')
for kwargs in [dict(channel=2),dict(slot=0),dict(slot=0x600001)]:add(valid,1,**kwargs)
for field,value in [('version','1.2.5'),('url',base['url'].replace('app.bin','other.bin')),('size',1235),('sha256','10'+base['sha256'][2:]),('key_id','rfc8032-host-alias')]:
    add(dict(valid,**{field:value}),4)
add(signed(dict(base,key_id='unknown-host-key')),3)
add(dict(valid,channel='dev'),4,channel=1)
for field,value in [('hardware_id','wrong-board'),('partition_layout','wrong-layout'),('schema_version',1),('signature_algorithm','ed25519ph')]:
    add(dict(valid,**{field:value}),1 if field in ('schema_version','signature_algorithm') else 2)
wrong_key=Ed25519PrivateKey.from_private_bytes(bytes.fromhex('00'*32))
add(dict(valid,signature=base64.b64encode(wrong_key.sign(message(base))).decode()),4)
add(dict(valid,signature=base64.b64encode(bytes(64)).decode()),4)
sig=bytearray(base64.b64decode(valid['signature']));sig[0]^=1;add(dict(valid,signature=base64.b64encode(sig).decode()),4)
sig=base64.b64decode(valid['signature']);order=2**252+27742317777372353535851937790883648493
malleable=sig[:32]+(int.from_bytes(sig[32:],'little')+order).to_bytes(32,'little');add(dict(valid,signature=base64.b64encode(malleable).decode()),4)
for signature in ['',valid['signature'][:-1],valid['signature']+'=',valid['signature'][:-2]+'AA','!'+valid['signature'][1:]]:add(dict(valid,signature=signature),1)
alphabet='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
last=alphabet.index(valid['signature'][85]);add(dict(valid,signature=valid['signature'][:85]+alphabet[last|1]+'=='),1)
for field,value in [('schema_version',1),('size',0),('size',0x600001),('sha256',base['sha256'].upper()),('sha256','g'*64),('sha256','0'*63),('key_id','Bad-Key'),('signature_algorithm','ed25519ph'),('size','1234'),('version',1234)]:add(dict(valid,**{field:value}),1)
for field in valid:
    missing=dict(valid);del missing[field];add(missing,1)
    pair=json.dumps(field)+':'+json.dumps(valid[field]);add(raw[:-1]+b','+pair.encode()+b'}',1)
add(dict(valid,unknown='x'),1)
for number in [b'1.0',b'1e3',b'1E3',b'-0',b'-1',b'+1',b'01234',b'4294967296',b'99999999999',b'9007199254740993']:
    add(raw.replace(b'"size":1234',b'"size":'+number),1)
for number in [b'2.0',b'2e0',b'02']:
    add(raw.replace(b'"schema_version":2',b'"schema_version":'+number),1)
for field,value in [('size',None),('size',True),('size',[]),('size',{}),('version',None),('version',['1.2.4'])]:add(dict(valid,**{field:value}),1)
for payload in [b'',b'{}',b'[]',raw+b'{}',raw+b'x',raw+b'\0',raw+b'\0junk',b'\0'+raw,raw[:-1],raw+b' '*2049,raw.replace(b'"version"',b'"vers\\u0069on"'),raw.replace(b'1.2.4"',b'1.2.4\\u0000evil"',1),raw.replace(b'"version"',b'"version\\u0000evil"'),raw.replace(b'"stable"',b'"sta\x00ble"'),raw.replace(b'"stable"',b'"st\xc3\xa4ble"')]:add(payload,1)

with tempfile.TemporaryDirectory(prefix='dsr-ota-') as tmp:
    tmp=Path(tmp);fixtures=tmp/'manifests.bin'
    with fixtures.open('wb') as f:
        f.write(struct.pack('>I',len(cases)))
        for payload,expected,channel,slot,current in cases:
            f.write(struct.pack('>IIIII',expected,channel,slot,len(current),len(payload)));f.write(current);f.write(payload)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
    if args.sanitize: flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    elif args.ubsan_only: flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
    includes=['components/dsr_ota/include','components/dsr_ota','components/dsr_core/include','components/dsr_crypto/vendor','components/dsr_crypto/vendor/optional','.test-deps']
    flags += [x for folder in includes for x in ['-I',str(ROOT/folder)]]+['-I','/usr/include/cjson']
    sources=['components/dsr_ota/dsr_ota_manifest.c','components/dsr_core/dsr_core.c','components/dsr_crypto/vendor/monocypher.c','components/dsr_crypto/vendor/optional/monocypher-ed25519.c','tests/test_ota_manifest.c']
    kat=tmp/'test-ed25519-rfc'
    subprocess.run([os.environ.get('CC','cc'),*flags,'-ffunction-sections','-fdata-sections',
                    str(ROOT/'components/dsr_crypto/vendor/monocypher.c'),
                    str(ROOT/'components/dsr_crypto/vendor/optional/monocypher-ed25519.c'),
                    str(ROOT/'tests/test_ed25519.c'),'-Wl,--gc-sections','-o',str(kat)],check=True)
    subprocess.run([str(kat)],check=True)
    for trusted in (True,False):
        binary=tmp/('test-trust' if trusted else 'production-empty')
        extra=['-DDSR_OTA_HOST_TEST_TRUST=1'] if trusted else [str(ROOT/'components/dsr_ota/dsr_ota_trust.c')]
        subprocess.run([os.environ.get('CC','cc'),*flags,*extra,*[str(ROOT/s) for s in sources],'-l:libcjson.so.1','-o',str(binary)],check=True)
        subprocess.run([str(binary),str(fixtures)],check=True)
