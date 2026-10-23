#!/usr/bin/env python3
"""Focused production system-data codecs with synthetic storage/radio providers."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
case_names = ['format','sources','bonds','admission','stage','recovery','api']
p.add_argument('--case', choices=[*case_names,'all'], default='all')
p.add_argument('--sanitize', action='store_true')
p.add_argument('--ubsan-only', action='store_true')
a = p.parse_args()
cases = {
 'format': ['components/dsr_platform/dsr_backup_format.c', 'components/dsr_config/dsr_config.c',
            'components/dsr_core/dsr_core.c', 'tests/system_data/crypto.c', 'tests/test_system_data_format.c'],
}
selected = case_names if a.case == 'all' else [a.case]
flags = ['-std=c11','-D_POSIX_C_SOURCE=200809L','-Wall','-Wextra','-Werror','-Wpedantic','-g']
if a.sanitize:
    flags += ['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:
    flags += ['-fsanitize=undefined','-fno-omit-frame-pointer']
includes = ['tests/system_data','tests/support','components/dsr_platform','components/dsr_platform/include',
            'components/dsr_config/include','components/dsr_core/include','.test-deps','/usr/include/cjson']
with tempfile.TemporaryDirectory(prefix='dsr-system-data-') as temp:
    for case in selected:
        if case not in cases:
            raise SystemExit(f'{case}: implementation/test not present; cannot claim complete integration')
        target = Path(temp) / case
        subprocess.run([os.environ.get('CC','cc'), *flags,
                        *[v for folder in includes for v in ['-I',str(ROOT / folder) if not folder.startswith('/') else folder]],
                        *[str(ROOT / source) for source in cases[case]],
                        *(['-Wl,--wrap=cJSON_ParseWithLengthOpts'] if case=='format' else []),
                        '-L',str(ROOT / '.test-deps'),'-l:libcjson.so.1','-lcrypto','-lm','-o',str(target)], check=True)
        result = subprocess.run([str(target),str(ROOT / 'tests/system_data/manifest-v1.json')], text=True, capture_output=True)
        if result.returncode:
            print(result.stderr.strip())
            raise SystemExit(result.returncode)
        if not result.stdout.startswith('PASS:'):
            raise SystemExit(f'{case}: no success marker')
        print(result.stdout.strip())
