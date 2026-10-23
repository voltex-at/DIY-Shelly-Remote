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
 'store': (['components/dsr_platform/dsr_automation_store.c', 'components/dsr_platform/dsr_automation_model.c',
            'tests/automation/nvs_fixture.c', 'tests/test_automation_store.c'], ['tests/support'], []),
 'store_queue': (['components/dsr_platform/dsr_workspace.c', 'components/dsr_platform/dsr_config_health.c',
                  'components/dsr_platform/dsr_activity.c', 'components/dsr_config/dsr_config.c',
                  'components/dsr_core/dsr_core.c', 'tests/test_automation_queue.c'],
                  ['tests/actor', 'tests/support', 'components/dsr_config/include', '.test-deps', '/usr/include/cjson'],
                  ['-l:libcjson.so.1', '-lm']),
 'http': (['components/dsr_core/dsr_core.c', 'components/dsr_platform/dsr_device_settings_model.c',
           'components/dsr_platform/dsr_http.c', 'components/dsr_platform/dsr_ap.c',
           'components/dsr_platform/dsr_ap_model.c', 'components/dsr_platform/dsr_discovery_model.c',
           'components/dsr_config/dsr_config.c', 'components/dsr_platform/dsr_connection.c',
           'components/dsr_platform/dsr_fleet.c', 'components/dsr_platform/dsr_settings.c',
           'components/dsr_platform/dsr_wifi_policy.c', 'components/dsr_platform/dsr_activity.c',
           'tests/test_automation_http.c'],
           ['tests/support', 'components/dsr_config/include', '.test-deps', '/usr/include/cjson'],
           ['-l:libcjson.so.1', '-lm']),
}
selected = list(cases) if a.case == 'all' else ['store', 'store_queue'] if a.case == 'store' else [a.case]
with tempfile.TemporaryDirectory(prefix='dsr-automation-') as tmp:
 for name in selected:
  if name not in cases:
   raise SystemExit(f'{name}: case not implemented yet')
  sources, includes, libs = cases[name]
  binary = Path(tmp) / name
  build(binary, sources, ['components/dsr_core/include', 'components/dsr_platform/include',
        'components/dsr_platform', *includes], libs=libs, args=a)
  subprocess.run([str(binary)], check=True, cwd=ROOT)
