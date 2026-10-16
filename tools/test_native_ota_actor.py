#!/usr/bin/env python3
"""Real synchronized actor, explicit chain/installer/worker/clock MODEL."""
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
for folder in ['tests/support', 'tests/ui', 'components/dsr_platform/include',
               'components/dsr_platform', 'components/dsr_core/include',
               'components/dsr_ota/include', 'components/dsr_config/include']:
    flags += ['-I', str(source / folder)]
json_include = Path('/usr/include/cjson')
if not (json_include / 'cJSON.h').exists():
    json_include = source / '.test-deps'
flags += ['-I', str(json_include)]
with tempfile.TemporaryDirectory(prefix='dsr-native-actor-') as temporary:
    binary = Path(temporary) / 'actor'
    subprocess.run([os.environ.get('CC', 'cc'), *flags,
                    str(source / 'components/dsr_platform/dsr_native_ota.c'),
                    str(ROOT / 'tests/test_native_ota_actor.c'), '-o', str(binary)],
                   check=True)
    for mode in range(7):
        subprocess.run([str(binary), str(mode)], check=True)
