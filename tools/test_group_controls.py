#!/usr/bin/env python3
"""Focused group admission/typed-dispatch regressions; no radio or full suite."""
import argparse
import subprocess
import tempfile
from pathlib import Path
from hostcc import build, cjson_include

p = argparse.ArgumentParser()
p.add_argument('--case', choices=['parse', 'model', 'runtime', 'schema', 'worker'], action='append')
p.add_argument('--sanitize', action='store_true')
p.add_argument('--ubsan-only', action='store_true')
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
platform = 'components/dsr_platform/'
includes = ['tests/support', platform + 'include', platform, 'components/dsr_core/include',
            'components/dsr_config/include', str(cjson_include())]
with tempfile.TemporaryDirectory(prefix='dsr-groups-') as temp:
    for case in a.case or ['parse', 'model', 'runtime']:
        if case in ('runtime', 'schema'):
            switches = ['--sanitize'] if a.sanitize else ['--ubsan-only'] if a.ubsan_only else []
            subprocess.run(['python3', str(root / ('tools/test_actions_' + case + '.py')), *switches], check=True)
            continue
        sources = [platform + 'dsr_actions_' + case + '.c', 'components/dsr_core/dsr_core.c',
                   'tests/test_actions_' + case + '.c']
        inc = includes[:]
        flags = []
        if case == 'parse':
            sources += ['components/dsr_config/dsr_config.c']
        if case == 'worker':
            sources = [platform + name + '.c' for name in ['dsr_worker', 'dsr_actions', 'dsr_actions_model',
                       'dsr_actions_parse', 'dsr_activity', 'dsr_ap_model', 'dsr_wifi_policy']]
            sources += ['components/dsr_config/dsr_config.c', 'components/dsr_core/dsr_core.c', 'tests/test_actions_worker.c']
            inc = ['tests/battery', 'tests/actions', 'tests/history', 'tests/actor', *inc, 'components/dsr_ota/include']
            flags += ['-DDSR_BOARD_ES3C35P=1']
        binary = Path(temp) / case
        build(binary, sources, inc, flags, ['-l:libcjson.so.1', '-lm'], a)
        subprocess.run([str(binary)], check=True)
