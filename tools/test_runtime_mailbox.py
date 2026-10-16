#!/usr/bin/env python3
"""Actual workspace runtime with explicit host RTOS/Flash providers."""
import argparse
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--source-root',type=Path,default=ROOT,help='Review actual sources from another checkout')
p.add_argument('--sanitize',action='store_true')
p.add_argument('--ubsan-only',action='store_true')
a=p.parse_args()
source=a.source_root.resolve()
build=ROOT/'build-runtime-mailbox'
build.mkdir(exist_ok=True)
json_include=Path('/usr/include/cjson')
if not (json_include/'cJSON.h').exists():json_include=source/'.test-deps'
flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
includes=[source/path for path in ['tests/actor','tests/support','components/dsr_core/include','components/dsr_platform/include','components/dsr_config/include']]+[json_include]
sources=[source/path for path in ['components/dsr_platform/dsr_workspace.c','components/dsr_platform/dsr_config_health.c','components/dsr_platform/dsr_activity.c','components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c']]+[ROOT/'tests/test_runtime_mailbox.c']
binary=build/'test_runtime_mailbox'
subprocess.run([os.environ.get('CC','cc'),*flags,*[value for folder in includes for value in ['-I',str(folder)]],*map(str,sources),'-l:libcjson.so.1','-lm','-o',str(binary)],check=True,env=dict(os.environ,TMPDIR=str(build)))
subprocess.run([str(binary)],check=True,cwd=build)
