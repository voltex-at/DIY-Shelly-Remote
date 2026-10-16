#!/usr/bin/env python3
"""Genuine atomic admission and restart backend; scheduler/shutdown MODEL."""
import argparse,os,re,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');args=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dsr-restart-') as directory:
    tmp=Path(directory);flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g','-pthread']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    elif args.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
    flags+=['-I',str(tmp)]
    for folder in ['tests/restart','tests/support','components/dsr_platform/include','components/dsr_platform']:flags+=['-I',str(ROOT/folder)]
    suites=[('activity',['tests/test_restart.c'],['basic','fault','pending-fault','writer-race','restart-race','writers']),
            ('backend',['components/dsr_platform/dsr_restart.c','tests/restart/backend.c'],['before-start','create-fail','boot','busy','notify-fail','boot-lost','spurious','once','race'])]
    for name,sources,cases in suites:
        binary=tmp/name
        subprocess.run([os.environ.get('CC','cc'),*flags,str(ROOT/'components/dsr_platform/dsr_activity.c'),*[str(ROOT/s) for s in sources],'-o',str(binary)],check=True)
        # Fresh process per scenario preserves the production no-reset contract.
        for case in cases:subprocess.run([str(binary),case],check=True)
    panel=(ROOT/'components/dsr_platform/dsr_panel.c').read_text()
    for name in ['pair','restart']:
        start=panel.index(f'static esp_err_t {name}(httpd_req_t *r)')
        end=re.search(r'^static\b',panel[start+1:],re.M)
        assert end
        (tmp/f'{name}_endpoint.inc').write_text(panel[start:start+1+end.start()])
    binary=tmp/'panel'
    subprocess.run([os.environ.get('CC','cc'),*flags,str(ROOT/'components/dsr_platform/dsr_activity.c'),str(ROOT/'tests/restart/panel.c'),'-lcrypto','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    writer_flags=[*flags]
    for folder in ['tests/actor','components/dsr_config/include','components/dsr_core/include']:writer_flags+=['-I',str(ROOT/folder)]
    json_include=Path('/usr/include/cjson')
    if not (json_include/'cJSON.h').exists():json_include=ROOT/'.test-deps'
    if not (json_include/'cJSON.h').exists():raise SystemExit('Install libcjson-dev to run restart workspace fixtures')
    writer_flags+=['-I',str(json_include)]
    binary=tmp/'workspace'
    sources=['components/dsr_platform/dsr_activity.c','components/dsr_platform/dsr_workspace.c','components/dsr_platform/dsr_config_health.c','components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/restart/workspace.c']
    subprocess.run([os.environ.get('CC','cc'),*writer_flags,*[str(ROOT/s) for s in sources],'-l:libcjson.so.1','-lm','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    binary=tmp/'photos'
    sources=['components/dsr_platform/dsr_activity.c','components/dsr_platform/dsr_photos.c','components/dsr_config/dsr_photo_name.c','tests/restart/photos.c']
    wraps=['malloc','fopen','fwrite','fflush','fileno','fsync','fclose']
    subprocess.run([os.environ.get('CC','cc'),*writer_flags,*[str(ROOT/s) for s in sources],*[f'-Wl,--wrap={name}' for name in wraps],'-lcrypto','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
