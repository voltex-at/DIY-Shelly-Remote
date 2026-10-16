#!/usr/bin/env python3
"""Actual actor/sole worker integration; explicit HTTP/SDK/RTOS providers."""
import argparse,os,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g','-DDSR_BOARD_ES3C35P=1']
if Path('/usr/include/cjson/cJSON.h').is_file():flags+=['-I','/usr/include/cjson']
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
with tempfile.TemporaryDirectory(prefix='dsr-actions-worker-') as d:
 inc=['tests/battery','tests/actions','tests/history','tests/actor','tests/support','components/dsr_core/include','components/dsr_config/include','components/dsr_platform/include','components/dsr_platform','components/dsr_ota/include','.test-deps'];b=Path(d)/'test'
 src=['components/dsr_platform/dsr_worker.c','components/dsr_platform/dsr_actions.c','components/dsr_platform/dsr_actions_model.c','components/dsr_platform/dsr_actions_parse.c','components/dsr_platform/dsr_activity.c','components/dsr_platform/dsr_ap_model.c','components/dsr_platform/dsr_wifi_policy.c','components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/test_actions_worker.c']
 subprocess.run([os.environ.get('CC','cc'),*flags,*[v for p in inc for v in ['-I',str(R/p)]],*[str(R/s) for s in src],'-l:libcjson.so.1','-lm','-o',str(b)],check=True);subprocess.run([str(b)],check=True)
