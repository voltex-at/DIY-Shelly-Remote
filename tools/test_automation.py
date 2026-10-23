#!/usr/bin/env python3
"""Focused contact/heating regressions. Host providers never use a radio."""
import argparse
import subprocess
import tempfile
from pathlib import Path
from hostcc import ROOT, build, parse_args

p = argparse.ArgumentParser(description=__doc__)
RUNTIME_SCENARIOS = ['happy', 'manual', 'manual-failure', 'manual-retry', 'reboot-own',
 'reboot-prepared', 'reboot-baseline', 'reboot-restore', 'corrupt', 'rebind', 'disable', 'active-edit', 'stale', 'manual-read-wait', 'reopen-before-restore', 'reopen-after-restore', 'rate-limit']
p.add_argument('--case', choices=['model', 'store', 'http', 'config', 'runtime', 'api', 'all'], default='all')
p.add_argument('--runtime-module', choices=['actor', 'native'], help='Run one controller in the runtime case')
p.add_argument('--scenario', nargs='+', choices=RUNTIME_SCENARIOS, help='Run selected runtime scenarios without repeating controller cases')
a = parse_args(p)
cases = {
 'model': (['components/dsr_platform/dsr_automation_model.c', 'tests/test_automation_model.c'], [], []),
 'store': (['components/dsr_platform/dsr_automation_store.c', 'components/dsr_platform/dsr_automation_model.c',
            'tests/automation/nvs_fixture.c', 'tests/test_automation_store.c'], ['tests/support'], []),
 'store_queue': (['components/dsr_platform/dsr_workspace.c', 'components/dsr_platform/dsr_config_health.c',
                  'components/dsr_platform/dsr_activity.c', 'components/dsr_config/dsr_config.c',
                  'components/dsr_core/dsr_core.c', 'tests/test_automation_queue.c', 'tests/support/automation_runtime_stub.c'],
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
 'config': (['components/dsr_config/dsr_config.c', 'components/dsr_core/dsr_core.c',
             'components/dsr_platform/dsr_automation_config.c', 'tests/test_automation_config.c'],
             ['components/dsr_config/include', '.test-deps', '/usr/include/cjson'], ['-l:libcjson.so.1', '-lm']),
 'runtime_actor': (['components/dsr_platform/dsr_worker.c', 'components/dsr_platform/dsr_actions.c',
                    'components/dsr_platform/dsr_actions_model.c', 'components/dsr_platform/dsr_actions_parse.c',
                    'components/dsr_platform/dsr_activity.c', 'components/dsr_platform/dsr_ap_model.c',
                    'components/dsr_platform/dsr_wifi_policy.c', 'components/dsr_config/dsr_config.c',
                    'components/dsr_core/dsr_core.c', 'tests/test_automation_actor.c'],
                    ['tests/battery', 'tests/actions', 'tests/history', 'tests/actor', 'tests/support',
                     'components/dsr_config/include', 'components/dsr_ota/include', '.test-deps', '/usr/include/cjson'],
                    ['-l:libcjson.so.1', '-lm']),
}
cases['runtime_native'] = (['tests/test_automation_native.c'], ['tests/history', 'tests/actor', 'tests/support'], [])
cases['api'] = (['components/dsr_platform/dsr_automation_api.c','tests/test_automation_api.c'], ['tests/support','components/dsr_config/include','.test-deps','/usr/include/cjson'], ['-l:libcjson.so.1','-lm'])
cases['runtime'] = (cases['runtime_actor'][0][:-1] + [
 'components/dsr_platform/dsr_automation.c', 'components/dsr_platform/dsr_automation_model.c',
 'components/dsr_platform/dsr_automation_store.c', 'components/dsr_platform/dsr_automation_config.c',
 'components/dsr_platform/dsr_workspace.c', 'components/dsr_platform/dsr_config_health.c',
 'components/dsr_platform/dsr_fleet.c', 'tests/automation/nvs_fixture.c', 'tests/test_automation_runtime.c'],
 cases['runtime_actor'][1], cases['runtime_actor'][2])
selected = list(cases) if a.case == 'all' else ['store', 'store_queue'] if a.case == 'store' else ['runtime_actor', 'runtime_native', 'runtime'] if a.case == 'runtime' else [a.case]
if a.scenario:
 if a.case!='runtime':p.error('--scenario requires --case runtime')
 selected=['runtime']
if a.runtime_module:
 if a.case!='runtime' or a.scenario:p.error('--runtime-module requires --case runtime without --scenario')
 selected=['runtime_'+a.runtime_module]
with tempfile.TemporaryDirectory(prefix='dsr-automation-') as tmp:
 for name in selected:
  if name not in cases:
   raise SystemExit(f'{name}: case not implemented yet')
  sources, includes, libs = cases[name]
  binary = Path(tmp) / name
  if name=='api':
   panel=(ROOT/'components/dsr_platform/dsr_panel.c').read_text()
   start=panel.index('static esp_err_t automation_api(');end=panel.index('static esp_err_t api(',start)
   Path(tmp,'automation_endpoint.inc').write_text(panel[start:end])
   end=panel.index(' if(!strcmp(r->uri,"/api/v1/ap")',end)
   start=panel.index('static esp_err_t api(')
   Path(tmp,'automation_route.inc').write_text(panel[start:end]+' return json(r,"404 Not Found","{}");\n}\n')
  build(binary, sources, [tmp,'components/dsr_core/include', 'components/dsr_platform/include',
        'components/dsr_platform', *includes], flags=(['-DDSR_BOARD_ES3C35P=1'] if name.startswith('runtime') else []) +
        (['-Wl,--wrap=time'] if name=='runtime' else []), libs=libs, args=a)
  scenarios = (a.scenario if a.scenario else RUNTIME_SCENARIOS) if name=='runtime' else [None]
  for scenario in scenarios:
   subprocess.run([str(binary), *([scenario] if scenario else [])], check=True, cwd=ROOT)
