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
# No runtime git/history/network dependency: CI can use a depth-one checkout.
auth_hashes={'local_request':'77af2b1f465d425091ef1097e93853d24a341a626624604dad26fc86ce9a2f30',
             'authorized':'effb45bc5a73931fe3a2acae29c646a013e8d8ff9dbe06ee140a4b623bba99c3',
             'api':'43ea42e3153acd75aa5dfb587eed6132a199c6c9f08def5f47a5f1152002ecf8'}
for name,expected in auth_hashes.items():
    digest=hashlib.sha256(function(panel,name).encode()).hexdigest()
    assert digest==expected,f'Unexpected auth/route change: {name}'
    print(f'UNCHANGED AUTH SOURCE {name} SHA256 {digest}',flush=True)
names=['dsr_panel_pairing_snapshot','dsr_panel_pairing_revoke','random_hex','equal','dsr_panel_pair_begin','local_request','authorized','body','pair']
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
