#!/usr/bin/env python3
"""Actual panel revoke/pair/auth functions; SDK/NVS/HTTP explicitly MODEL."""
import argparse,hashlib,os,re,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');args=p.parse_args()
def function(source,name):
    match=re.search(r'^(?:static )?[^\n;]+\b'+re.escape(name)+r'\([^\n]*\)\n',source,re.M)
    if not match:raise SystemExit(f'Missing actual function {name}')
    start=source.index('{',match.end());depth=0;quoted=None;escaped=False
    for at in range(start,len(source)):
        ch=source[at]
        if quoted:
            if escaped:escaped=False
            elif ch=='\\':escaped=True
            elif ch==quoted:quoted=None
        elif ch in ('"',"'"):quoted=ch
        elif ch=='{':depth+=1
        elif ch=='}':
            depth-=1
            if depth==0:return source[match.start():at+1]+'\n'
    raise SystemExit(f'Unclosed actual function {name}')
panel=(ROOT/'components/dsr_platform/dsr_panel.c').read_text()
# Frozen exact function bytes extracted from reviewed Draft PR47 parent
# 1765187de0ff697c47c6a10df7a08910e177965f:components/dsr_platform/dsr_panel.c.
# API route table is separately frozen from current Cover/history parent
# faa623fca5f7bb7cf63808fca963ccc1b9dee667. Only the exact new actions
# dispatch is normalized, and must remain after the unchanged auth gate.
# No runtime git/history/network dependency: CI can use a depth-one checkout.
auth_hashes={'local_request':'77af2b1f465d425091ef1097e93853d24a341a626624604dad26fc86ce9a2f30',
             'authorized':'effb45bc5a73931fe3a2acae29c646a013e8d8ff9dbe06ee140a4b623bba99c3',
             'api':'0a44b9f8ce8aa1cfecfd30412dfbbe0811cb11bfa2c5db41e32e48d3eaf193e4'}
for name,expected in auth_hashes.items():
    block=function(panel,name)
    if name=='api':
        maintenance=' if(!strncmp(r->uri,"/api/v1/backup/",15)||!strncmp(r->uri,"/api/v1/maintenance/",20)||!strcmp(r->uri,"/api/v1/reset/request"))return maintenance_api(r);\n'
        assert block.count(maintenance)==1 and block.index(maintenance)>block.index('if(!authorized(r))'),'Maintenance must stay behind unchanged authentication'
        block=block.replace(maintenance,'')
        automation=' if(!strcmp(r->uri,"/api/v1/automations/status"))return automation_api(r);\n'
        assert block.count(automation)==1 and block.index(automation)>block.index('if(!authorized(r))'),'Read-only automation status must stay behind unchanged authentication'
        block=block.replace(automation,'')
        privacy='(dsr_ap_sensitive()||dsr_ble_rpc_sensitive()||dsr_device_settings_sensitive()||dsr_maintenance_sensitive())'
        assert block.count(privacy)==1 and block.index(privacy)>block.index('if(!authorized(r))'), 'Settings preview privacy must remain behind authorization'
        # New credential/PIN screens additionally suppress preview. Normalize
        # precisely this stricter predicate; preserve the frozen auth/route hash.
        block=block.replace(privacy,'(dsr_ap_sensitive()||dsr_ble_rpc_sensitive())')
        actions=' if(!strncmp(r->uri,"/api/v1/actions/",16)||!strcmp(r->uri,"/api/v1/groups/run")||!strcmp(r->uri,"/api/v1/scenes/run"))return actions_api(r);\n'
        assert block.count(actions)==1 and block.index(actions)>block.index('if(!authorized(r))'),'Actions must remain behind unchanged authorization'
        block=block.replace(actions,'')
        runtime=' if(!strcmp(r->uri,"/api/v1/devices/runtime")||!strcmp(r->uri,"/api/v1/devices/output")||!strcmp(r->uri,"/api/v1/devices/cover")||!strcmp(r->uri,"/api/v1/devices/light")||!strcmp(r->uri,"/api/v1/devices/rgb")||!strcmp(r->uri,"/api/v1/devices/refresh"))return runtime_api(r);\n'
        prior=' if(!strcmp(r->uri,"/api/v1/devices/runtime")||!strcmp(r->uri,"/api/v1/devices/output")||!strcmp(r->uri,"/api/v1/devices/cover"))return runtime_api(r);\n'
        assert block.count(runtime)==1 and block.index(runtime)>block.index('if(!authorized(r))'),'Light must remain behind unchanged authorization'
        # Energy history route removed in 0.7.51 (owner): it must stay absent;
        # the frozen parent had it right after the runtime route.
        history=' if(!strncmp(r->uri,"/api/v1/history/",16))return history_api(r);\n'
        assert '/api/v1/history/' not in block,'Removed history route must not return'
        block=block.replace(runtime,prior+history)
        typed='  const cJSON *endpoint_kind=cJSON_GetObjectItemCaseSensitive(input,"endpoint_kind");if(endpoint_kind){if(!cJSON_IsNumber(endpoint_kind)||(endpoint_kind->valuedouble!=DSR_ENDPOINT_SWITCH&&endpoint_kind->valuedouble!=DSR_ENDPOINT_COVER&&endpoint_kind->valuedouble!=DSR_ENDPOINT_LIGHT&&endpoint_kind->valuedouble!=DSR_ENDPOINT_RGB&&endpoint_kind->valuedouble!=DSR_ENDPOINT_HT&&endpoint_kind->valuedouble!=DSR_ENDPOINT_BLU))valid=false;else{connection.device.endpoint_kind=(uint8_t)endpoint_kind->valueint;if(connection.device.endpoint_kind!=DSR_ENDPOINT_SWITCH)connection.device.generation=DSR_GEN2_PLUS;}}\n'
        prior_typed='  const cJSON *endpoint_kind=cJSON_GetObjectItemCaseSensitive(input,"endpoint_kind");if(endpoint_kind){if(!cJSON_IsNumber(endpoint_kind)||(endpoint_kind->valuedouble!=DSR_ENDPOINT_SWITCH&&endpoint_kind->valuedouble!=DSR_ENDPOINT_COVER))valid=false;else{connection.device.endpoint_kind=(uint8_t)endpoint_kind->valueint;if(connection.device.endpoint_kind==DSR_ENDPOINT_COVER)connection.device.generation=DSR_GEN2_PLUS;}}\n'
        assert block.count(typed)==1,'Unexpected typed Light BIND validation change'
        block=block.replace(typed,prior_typed)

    digest=hashlib.sha256(block.encode()).hexdigest()
    assert digest==expected,f'Unexpected auth/route change: {name}'
    print(f'UNCHANGED AUTH SOURCE {name} SHA256 {digest}',flush=True)
names=['dsr_panel_pairing_snapshot','dsr_panel_pairing_revoke','random_hex','equal','dsr_panel_pair_begin','local_request','authorized','body','pair','dsr_panel_backup_session_valid','maintenance_api']
with tempfile.TemporaryDirectory(prefix='dsr-pairing-') as directory:
    tmp=Path(directory);(tmp/'actual_panel.inc').write_text('\n'.join(function(panel,n) for n in names))
    boot=re.search(r'^ nvs_handle_t n;if\(nvs_open\("dsr_panel",NVS_READONLY[^\n]+',panel,re.M)
    assert boot,'Missing original startup NVS read'
    (tmp/'actual_boot_read.inc').write_text(boot.group()+'\n')
    flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    elif args.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
    for folder in [tmp,ROOT/'tests/revoke',ROOT/'components/dsr_platform/include',ROOT/'components/dsr_platform',ROOT/'components/dsr_core/include']:flags+=['-I',str(folder)]
    binary=tmp/'pairing'
    subprocess.run([os.environ.get('CC','cc'),*flags,str(ROOT/'components/dsr_platform/dsr_activity.c'),str(ROOT/'tests/revoke/pairing.c'),'-lcrypto','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
