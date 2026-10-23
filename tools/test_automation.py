#!/usr/bin/env python3
"""Focused contact/heating regressions. Host providers never use a radio."""
import argparse
import subprocess
import tempfile
from pathlib import Path
from hostcc import ROOT, build, parse_args

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--case', choices=['model', 'store', 'http', 'config', 'runtime', 'all'], default='all')
a = parse_args(p)
cases = {
 'model': (['components/dsr_platform/dsr_automation_model.c', 'tests/test_automation_model.c'], [], []),
}
selected = list(cases) if a.case == 'all' else [a.case]
with tempfile.TemporaryDirectory(prefix='dsr-automation-') as tmp:
 for name in selected:
  if name not in cases:
   raise SystemExit(f'{name}: case not implemented yet')
  sources, includes, libs = cases[name]
  binary = Path(tmp) / name
  build(binary, sources, ['components/dsr_core/include', 'components/dsr_platform/include',
        'components/dsr_platform', *includes], libs=libs, args=a)
  subprocess.run([str(binary)], check=True, cwd=ROOT)
