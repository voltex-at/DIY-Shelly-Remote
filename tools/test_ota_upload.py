#!/usr/bin/env python3
"""Execute the production legacy receiver and extracted endpoint without Flash."""
import argparse
import hashlib
import os
import re
from pathlib import Path
import subprocess
import tempfile
import sys
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
# Canonical source bytes: Windows checkout line endings do not change C code.
panel_bytes=(ROOT/'components/dsr_platform/dsr_panel.c').read_bytes().replace(b'\r\n',b'\n')
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
        runtime=b' if(!strcmp(r->uri,"/api/v1/devices/runtime")||!strcmp(r->uri,"/api/v1/devices/output")||!strcmp(r->uri,"/api/v1/devices/cover")||!strcmp(r->uri,"/api/v1/devices/light")||!strcmp(r->uri,"/api/v1/devices/rgb")||!strcmp(r->uri,"/api/v1/devices/refresh"))return runtime_api(r);\n'
        assert block.count(runtime)==1 and block.index(runtime)>block.index(b'if(!authorized(r))'),'Runtime must remain after the unchanged authorization gate'
        block=block.replace(runtime,b'')
        # The typed BIND extension validates a public family discriminator;
        # unchanged origin/boot/pairing gates above still own every route.
        typed=b'  const cJSON *endpoint_kind=cJSON_GetObjectItemCaseSensitive(input,"endpoint_kind");if(endpoint_kind){if(!cJSON_IsNumber(endpoint_kind)||(endpoint_kind->valuedouble!=DSR_ENDPOINT_SWITCH&&endpoint_kind->valuedouble!=DSR_ENDPOINT_COVER&&endpoint_kind->valuedouble!=DSR_ENDPOINT_LIGHT&&endpoint_kind->valuedouble!=DSR_ENDPOINT_RGB&&endpoint_kind->valuedouble!=DSR_ENDPOINT_HT&&endpoint_kind->valuedouble!=DSR_ENDPOINT_BLU))valid=false;else{connection.device.endpoint_kind=(uint8_t)endpoint_kind->valueint;if(connection.device.endpoint_kind!=DSR_ENDPOINT_SWITCH)connection.device.generation=DSR_GEN2_PLUS;}}\n'
        assert block.count(typed)==1,'Unexpected typed BIND validation change'
        block=block.replace(typed,b'')
        ap=b' if(!strcmp(r->uri,"/api/v1/ap")||!strcmp(r->uri,"/api/v1/ap/open"))return ap_handoff_api(r);\n'
        assert block.count(ap)==1 and block.index(ap)>block.index(b'if(!authorized(r))'),'AP handoff must remain behind authorization'
        block=block.replace(ap,b'')
        # Energy history (and its route) was removed in 0.7.51.
        assert b'/api/v1/history/' not in block,'Removed history route must not return'
        actions=b' if(!strncmp(r->uri,"/api/v1/actions/",16)||!strcmp(r->uri,"/api/v1/groups/run")||!strcmp(r->uri,"/api/v1/scenes/run"))return actions_api(r);\n'
        assert block.count(actions)==1 and block.index(actions)>block.index(b'if(!authorized(r))'),'Actions must remain behind authorization'
        block=block.replace(actions,b'')
        automation=b' if(!strcmp(r->uri,"/api/v1/automations/status"))return automation_api(r);\n'
        assert block.count(automation)==1 and block.index(automation)>block.index(b'if(!authorized(r))'),'Read-only automation status must remain behind authorization'
        block=block.replace(automation,b'')
        privacy=b'(dsr_ap_sensitive()||dsr_ble_rpc_sensitive()||dsr_device_settings_sensitive())'
        assert block.count(privacy)==1 and block.index(privacy)>block.index(b'if(!authorized(r))'),'Settings preview privacy must remain after authorization'
        block=block.replace(privacy,b'(dsr_ap_sensitive()||dsr_ble_rpc_sensitive())')
        preview=b'esp_timer_get_time()<pair_until||(dsr_ap_sensitive()||dsr_ble_rpc_sensitive())'
        assert block.count(preview)==1,'Unexpected AP preview gate'
        block=block.replace(preview,b'esp_timer_get_time()<pair_until')
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
    for folder in [tmp,ROOT/'tests/ota_upload',ROOT/'tests/ota_image',ROOT/'tests/support',ROOT/'components/dsr_platform/include',ROOT/'components/dsr_platform',ROOT/'components/dsr_ota/include',ROOT/'components/dsr_core/include']:flags+=['-I',str(folder)]
    sources=['components/dsr_platform/dsr_activity.c','components/dsr_platform/dsr_ota_stage.c','components/dsr_platform/dsr_ota_upload.c','components/dsr_ota/dsr_ota_image.c','tests/ota_image/sha256_adapter.c','tests/test_ota_upload.c']
    binary=tmp/'test-upload'
    cc=os.environ.get('CC','cc')
    if sys.platform=='cygwin':
        # PE import --wrap recurses before main when OpenSSL loads. Keep the
        # same fault injection on every allocation in the two production units.
        objects=[]
        for i,source in enumerate(sources):
            obj=tmp/f'unit-{i}.o';extra=[]
            if source.endswith(('dsr_ota_upload.c','dsr_ota_image.c')):
                extra=['-include',str(ROOT/'tests/support/source_allocator_hooks.h')]
            subprocess.run([cc,*flags,'-DDSR_SOURCE_ALLOCATORS',*extra,'-c',str(ROOT/source),'-o',str(obj)],check=True)
            objects.append(str(obj))
        subprocess.run([cc,*flags,*objects,'-lcrypto','-o',str(binary)],check=True)
    else:
        subprocess.run([cc,*flags,*[str(ROOT/s) for s in sources],'-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=free','-lcrypto','-o',str(binary)],check=True)
    for scenario in ([],['uncertain']):
        result=subprocess.run([str(binary),*scenario],capture_output=True,text=True)
        assert result.returncode==0,'Upload executable failed: '+result.stdout+result.stderr
        assert 'actual legacy upload/endpoint checks;' in result.stdout,'Upload executable did not complete its checks: '+result.stderr
        print(result.stdout.strip(),flush=True)
