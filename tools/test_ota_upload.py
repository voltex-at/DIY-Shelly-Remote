#!/usr/bin/env python3
"""Execute the production legacy receiver and extracted endpoint without Flash."""
import argparse
import hashlib
import os
import re
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');args=p.parse_args()
# Byte-for-byte source-invariance gate, NOT runtime HTTP authentication proof.
# Baseline: git show 2a3e7dd:components/dsr_platform/dsr_panel.c; each block
# starts at its static function and ends immediately before the next ^static.
auth_blocks={
    'local_request':'77af2b1f465d425091ef1097e93853d24a341a626624604dad26fc86ce9a2f30',
    'authorized':'effb45bc5a73931fe3a2acae29c646a013e8d8ff9dbe06ee140a4b623bba99c3',
    'api':'7725ecad42a0249d491b33d71fecbcbcc172c210b667a81ff1d62c6ec365b70f',
}
panel_bytes=(ROOT/'components/dsr_platform/dsr_panel.c').read_bytes()
for name,expected in auth_blocks.items():
    start=re.search(rb'^static [^\n]+\b'+name.encode()+rb'\([^\n]*\)\n',panel_bytes,re.M)
    assert start,f'Missing authentication block: {name}'
    end=re.search(rb'^static\b',panel_bytes[start.end():],re.M)
    assert end,f'Missing end of authentication block: {name}'
    block=panel_bytes[start.start():start.end()+end.start()]
    if name=='api':
        # Deliberately approved restart dispatch change ONLY. The complete API
        # block still hashes against the old baseline after this exact rewrite.
        new=b' if(!strcmp(r->uri,"/api/v1/restart")&&r->method==HTTP_POST)return restart(r);\n'
        old=b' if(!strcmp(r->uri,"/api/v1/restart")&&r->method==HTTP_POST){esp_err_t e=json(r,"200 OK","{\\"restarting\\":true}");vTaskDelay(pdMS_TO_TICKS(500));esp_restart();return e;}\n'
        assert block.count(new)==1,'Unexpected restart route; normalization must stay exact'
        block=block.replace(new,old)
        runtime=b' if(!strcmp(r->uri,"/api/v1/devices/runtime")||!strcmp(r->uri,"/api/v1/devices/output"))return runtime_api(r);\n'
        assert block.count(runtime)==1 and block.index(runtime)>block.index(b'if(!authorized(r))'),'Runtime must remain after the unchanged authorization gate'
        block=block.replace(runtime,b'')
    assert hashlib.sha256(block).hexdigest()==expected,f'Authentication source changed from 2a3e7dd: {name}'
print('PASS: local_request/authorized unchanged; full API unchanged after exact approved restart/runtime dispatch normalization vs 2a3e7dd; SOURCE INVARIANCE ONLY, not runtime auth proof',flush=True)
with tempfile.TemporaryDirectory(prefix='dsr-upload-') as tmp:
    tmp=Path(tmp)
    panel=(ROOT/'components/dsr_platform/dsr_panel.c').read_text()
    start=panel.index('static esp_err_t ota(httpd_req_t *r)')
    end=panel.index('static esp_err_t restart',start)
    endpoint=panel[start:end]
    assert 'dsr_ota_upload_receive' in endpoint,'Endpoint not integrated with production receiver'
    (tmp/'ota_endpoint.inc').write_text(endpoint)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    elif args.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
    for folder in [tmp,ROOT/'tests/ota_upload',ROOT/'tests/ota_image',ROOT/'tests/support',ROOT/'components/dsr_platform/include',ROOT/'components/dsr_platform',ROOT/'components/dsr_ota/include']:flags+=['-I',str(folder)]
    sources=['components/dsr_platform/dsr_activity.c','components/dsr_platform/dsr_ota_upload.c','components/dsr_ota/dsr_ota_image.c','tests/ota_image/sha256_adapter.c','tests/test_ota_upload.c']
    binary=tmp/'test-upload'
    subprocess.run([os.environ.get('CC','cc'),*flags,*[str(ROOT/s) for s in sources],'-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=free','-lcrypto','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
