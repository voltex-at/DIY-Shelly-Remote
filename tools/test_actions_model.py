#!/usr/bin/env python3
"""Actual bounded action actor/log state machine, no simulated I/O success."""
import argparse,os,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
f=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
if a.sanitize:f+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:f+=['-fsanitize=undefined','-fno-omit-frame-pointer']
with tempfile.TemporaryDirectory(prefix='dsr-actions-model-') as d:
 b=Path(d)/'test';subprocess.run([os.environ.get('CC','cc'),*f,*[v for path in ['tests/support','components/dsr_platform/include','components/dsr_platform','components/dsr_core/include'] for v in ['-I',str(R/path)]],'-I',str(R/'components/dsr_core/include'),str(R/'components/dsr_platform/dsr_actions_model.c'),str(R/'components/dsr_core/dsr_core.c'),str(R/'tests/test_actions_model.c'),'-o',str(b)],check=True);subprocess.run([str(b)],check=True)
