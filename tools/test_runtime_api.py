#!/usr/bin/env python3
"""Compile the actual authenticated API payload code; HTTP/RTOS are modeled."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--source-root', type=Path, default=ROOT)
p.add_argument('--sanitize', action='store_true')
p.add_argument('--ubsan-only', action='store_true')
a = p.parse_args()
source = a.source_root.resolve()
flags = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-Wpedantic', '-g']
if a.sanitize:
    flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
elif a.ubsan_only:
    flags += ['-fsanitize=undefined', '-fno-omit-frame-pointer']
include = Path('/usr/include/cjson')
if not (include/'cJSON.h').exists():
    include = source/'.test-deps'
(ROOT/'build-ui').mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='runtime-api-', dir=ROOT/'build-ui') as tmp:
    binary = str(Path(tmp)/'test_runtime_api')
    subprocess.run([os.environ.get('CC', 'cc'), *flags,
                    *[arg for d in ['tests/support', 'components/dsr_platform/include', 'components/dsr_platform', 'components/dsr_config/include', 'components/dsr_core/include'] for arg in ['-I', str(source/d)]],
                    '-I', str(include), str(ROOT/'tests/test_runtime_api.c'),
                    *[str(source/f) for f in ['components/dsr_platform/dsr_runtime_api.c', 'components/dsr_config/dsr_config.c', 'components/dsr_core/dsr_core.c']],
                    '-l:libcjson.so.1', '-lm', '-o', binary], check=True,
                   env=dict(os.environ, TMPDIR=tmp))
    subprocess.run([binary], check=True, env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1'))
