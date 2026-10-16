#!/usr/bin/env python3
"""Actual workspace boot/task upgrade; explicit RTOS and Flash fault providers."""
import argparse,os,re,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
with tempfile.TemporaryDirectory(prefix='dsr-actions-workspace-') as d:
 b=Path(d)/'test';inc=['tests/actor','tests/support','components/dsr_core/include','components/dsr_config/include','components/dsr_platform/include','.test-deps']
 src=['components/dsr_platform/dsr_workspace.c','components/dsr_platform/dsr_config_health.c','components/dsr_platform/dsr_activity.c','components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/test_actions_workspace.c']
 subprocess.run([os.environ.get('CC','cc'),*flags,*[v for s in inc for v in ['-I',str(R/s)]],*[str(R/s) for s in src],'-l:libcjson.so.1','-lm','-o',str(b)],check=True)
 result=subprocess.check_output([str(b),'0'],text=True);print(result,end='');n=int(re.search(r'BOOT_ALLOCATIONS=(\d+)',result)[1])
 for i in range(1,n+1):subprocess.run([str(b),str(i)],check=True,stdout=subprocess.DEVNULL)
 print(f'PASS: {n} actual workspace boot cJSON allocation failures; legacy bytes never written/GCed')
