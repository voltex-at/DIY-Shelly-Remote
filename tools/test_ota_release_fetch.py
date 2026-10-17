#!/usr/bin/env python3
"""Real release fetch/parser; SDK transport is explicitly a host MODEL."""
import argparse,hashlib,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true')
p.add_argument('--cjson-source',type=Path,help='Existing pinned IDF cJSON checkout; no downloads')
args=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dsr-release-fetch-') as folder:
    tmp=Path(folder)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    elif args.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
    dependency=['-l:libcjson.so.1']
    if args.cjson_source:
        source=args.cjson_source.resolve();pin='c859b25da02955fef659d658b8f324b5cde87be3'
        if subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()!=pin:raise SystemExit('Unexpected cJSON commit')
        hashes={'cJSON.c':'298581a04a36c0165da4b0aade235c23088cb2faa58651d720ea2f3706ed0b0d','cJSON.h':'25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee'}
        pinned=tmp/'cjson';pinned.mkdir()
        for name,digest in hashes.items():
            data=(source/name).read_bytes()
            if hashlib.sha256(data).hexdigest()!=digest:raise SystemExit(f'Unexpected cJSON hash: {name}')
            (pinned/name).write_bytes(data)
        flags+=['-I',str(pinned)];dependency=[str(pinned/'cJSON.c'),'-lm']
        print(f'Instrumented genuine IDF cJSON {pin}',flush=True)
    else:flags+=['-I','/usr/include/cjson']
    for folder in ['tests/ota_fetch','tests/ota_image','tests/support','components/dsr_ota/include','components/dsr_ota','components/dsr_core/include','components/dsr_crypto/vendor','components/dsr_crypto/vendor/optional','.test-deps']:flags+=['-I',str(ROOT/folder)]
    sources=['components/dsr_ota/dsr_ota_fetch.c','components/dsr_ota/dsr_ota_release_fetch.c','components/dsr_ota/dsr_ota_discovery_fetch.c','components/dsr_ota/dsr_ota_release.c','components/dsr_ota/dsr_ota_manifest.c','components/dsr_ota/dsr_ota_trust.c','components/dsr_ota/dsr_ota_image.c','tests/ota_image/sha256_adapter.c','components/dsr_core/dsr_core.c','components/dsr_crypto/vendor/monocypher.c','components/dsr_crypto/vendor/optional/monocypher-ed25519.c','tests/test_ota_release_fetch.c']
    wraps=['malloc','calloc','free','time','dsr_ota_release_candidate_parse','dsr_ota_discovery_parse']
    binary=tmp/'release-fetch-test'
    subprocess.run([os.environ.get('CC','cc'),*flags,*[str(ROOT/s) for s in sources],*[f'-Wl,--wrap={name}' for name in wraps],*dependency,'-lcrypto','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
