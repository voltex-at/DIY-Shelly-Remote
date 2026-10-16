#!/usr/bin/env python3
"""Actual bounded run request/definition parser, real cJSON and OOM injection."""
import argparse,os,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
f=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
if Path('/usr/include/cjson/cJSON.h').is_file():f+=['-I','/usr/include/cjson']
if a.sanitize:f+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:f+=['-fsanitize=undefined','-fno-omit-frame-pointer']
with tempfile.TemporaryDirectory(prefix='dsr-actions-parse-') as d:
 b=Path(d)/'test';subprocess.run([os.environ.get('CC','cc'),*f,*[v for path in ['tests/support','components/dsr_platform/include','components/dsr_platform','components/dsr_core/include','components/dsr_config/include','.test-deps'] for v in ['-I',str(R/path)]],*[str(R/p) for p in ['components/dsr_platform/dsr_actions_parse.c','components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/test_actions_parse.c']],'-l:libcjson.so.1','-lm','-o',str(b)],check=True);subprocess.run([str(b)],check=True)
