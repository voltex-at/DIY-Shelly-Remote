#!/usr/bin/env python3
"""Compile and execute the actual portable C code without ESP-IDF.

SUITES are plain compile-and-run C tests. RUNNERS are scripts that need more
(endpoint extraction, signed fixtures, fault loops); they get the same
sanitizer switches.
"""
import os
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from hostcc import ROOT, build, cjson_include, forward, parse_args  # noqa: E402

args = parse_args()
JSON = str(cjson_include())
CJSON = ['-l:libcjson.so.1', '-lm']
P = 'components/dsr_platform/'
CORE, PLAT, CONF, OTA = 'components/dsr_core/include', P + 'include', 'components/dsr_config/include', 'components/dsr_ota/include'
ES3 = ['-DDSR_BOARD_ES3C35P=1']
HTTP = ['components/dsr_core/dsr_core.c', P + 'dsr_http.c', P + 'dsr_ap.c', P + 'dsr_ap_model.c', P + 'dsr_discovery_model.c',
        'components/dsr_config/dsr_config.c', P + 'dsr_connection.c', P + 'dsr_fleet.c', P + 'dsr_settings.c',
        P + 'dsr_wifi_policy.c', P + 'dsr_activity.c']
WORKSPACE = [P + 'dsr_workspace.c', P + 'dsr_config_health.c', P + 'dsr_activity.c', 'components/dsr_config/dsr_config.c', 'components/dsr_core/dsr_core.c']

# name, sources, includes, flags, libs, scenario argv lists, extra environment
SUITES = [
    ('runtime_mailbox', WORKSPACE + ['tests/test_runtime_mailbox.c'], ['tests/actor', 'tests/support', CORE, PLAT, CONF, JSON], [], CJSON, [[]], {}),
    ('lan_barrier', ['tests/test_lan_barrier.c', P + 'dsr_discovery_model.c', 'components/dsr_core/dsr_core.c'],
     ['tests/lan', 'tests/actor', 'tests/support', CORE, PLAT, CONF, JSON], ['-O1'], ['-l:libcjson.so.1'], [[]], {}),
    ('core', ['components/dsr_core/dsr_core.c', 'tests/test_core.c'], [CORE], [], [], [[]], {}),
    ('platform', HTTP + ['tests/test_platform.c'], ['tests/support', JSON, CORE, PLAT, CONF], ES3, CJSON, [[]], {}),
    ('wifi', [P + 'dsr_wifi.c', P + 'dsr_wifi_policy.c', P + 'dsr_ap_model.c', 'tests/test_wifi.c'], ['tests/support', CORE, PLAT], [], [], [[]], {}),
    ('ble', [P + 'dsr_ble_model.c', P + 'dsr_ble.c', P + 'dsr_activity.c', 'tests/test_ble.c'], ['tests/ble', 'tests/support', PLAT, CORE], [], [], [[]], {}),
    ('battery', [P + 'dsr_battery.c', P + 'dsr_worker.c', P + 'dsr_activity.c', P + 'dsr_ap_model.c', P + 'dsr_wifi_policy.c', 'components/dsr_core/dsr_core.c', 'tests/test_battery.c'],
     ['tests/battery', 'tests/actor', 'tests/support', CORE, OTA, PLAT], ES3, [], [[]], {}),
    ('system_info', [P + 'dsr_system_info.c', 'tests/test_system_info.c'], ['tests/system_info', 'tests/support', PLAT], [], [], [[]], {}),
    ('panel_permission', [P + 'dsr_panel_permission.c', 'tests/test_panel_permission.c'], ['tests/system_info', 'tests/actor', 'tests/support', P, PLAT], [], [], [[]], {}),
    ('boot_memory', ['components/dsr_board/dsr_ui_task.c', P + 'dsr_storage.c', P + 'dsr_config_health.c', P + 'dsr_activity.c', 'tests/test_boot_memory.c'],
     ['tests/boot', 'tests/support', 'components/dsr_board', P, PLAT], [], [], [[]], {}),
    ('startup', ['main/app_main.c', P + 'dsr_boot.c', P + 'dsr_config_health.c', P + 'dsr_activity.c', 'tests/test_startup.c'],
     ['tests/startup', 'tests/boot', 'tests/ble', 'tests/ui', 'tests/support', CORE, PLAT], ES3, [], [[]], {}),
    ('workspace_start', WORKSPACE + ['tests/test_workspace_start.c'], ['tests/actor', 'tests/support', CORE, PLAT, CONF, JSON],
     ['-Wl,--wrap=malloc'], CJSON, [[str(s)] for s in range(7)], {}),
    ('actions_model', [P + 'dsr_actions_model.c', 'components/dsr_core/dsr_core.c', 'tests/test_actions_model.c'], ['tests/support', PLAT, P, CORE], [], [], [[]], {}),
    ('actions_parse', [P + 'dsr_actions_parse.c', 'components/dsr_config/dsr_config.c', 'components/dsr_core/dsr_core.c', 'tests/test_actions_parse.c'],
     ['tests/support', PLAT, P, CORE, CONF, JSON], [], CJSON, [[]], {}),
    ('actions_worker', [P + 'dsr_worker.c', P + 'dsr_actions.c', P + 'dsr_actions_model.c', P + 'dsr_actions_parse.c', P + 'dsr_activity.c', P + 'dsr_ap_model.c',
                        P + 'dsr_wifi_policy.c', 'components/dsr_config/dsr_config.c', 'components/dsr_core/dsr_core.c', 'tests/test_actions_worker.c'],
     ['tests/battery', 'tests/actions', 'tests/history', 'tests/actor', 'tests/support', CORE, CONF, PLAT, P, OTA, JSON], ES3, CJSON, [[]], {}),
    ('actions_http_uncertainty', HTTP + [P + 'dsr_actions_model.c', 'tests/test_actions_http_uncertainty.c'], ['tests/support', JSON, CORE, PLAT, CONF], ES3, CJSON, [[]], {}),
    ('native_ota_clock', [P + 'dsr_native_ota_clock.c', P + 'dsr_clock.c', 'tests/test_native_ota_clock.c'], ['tests/native_ota', 'tests/support', OTA, PLAT],
     ['-Wl,--wrap=time'], [], [[str(m)] for m in range(6)], {}),
    ('native_ota_actor', [P + 'dsr_native_ota.c', 'tests/test_native_ota_actor.c'], ['tests/native_ota', 'tests/support', 'tests/ui', PLAT, P, CORE, OTA, CONF, JSON], [], [], [[str(m)] for m in range(7)], {}),
    ('runtime_api', [P + 'dsr_runtime_api.c', 'components/dsr_config/dsr_config.c', 'components/dsr_core/dsr_core.c', 'tests/test_runtime_api.c'],
     ['tests/support', PLAT, P, CONF, CORE, JSON], [], CJSON, [[]], {'UBSAN_OPTIONS': 'halt_on_error=1', 'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1'}),
]
# script, forwards sanitizer switches ('all', 'asan' = --sanitize only, None)
RUNNERS = [
    ('test_ap_handoff.py', 'all'), ('test_pairing.py', 'all'), ('../tests/test_storage_recovery.py', None),
    ('test_factory.py', None), ('test_ap_target.py', None), ('test_fleet_target.py', None), ('test_ht_core_review.py', None),
    ('test_target_auditors.py', None), ('test_actions_schema.py', 'all'), ('test_actions_workspace.py', 'all'), ('test_actions_runtime.py', 'all'),
    ('test_native_ota_target.py', None), ('test_ble_target.py', None), ('test_ble_rpc.py', 'all'), ('test_history.py', 'all'),
    ('verify_crypto_vendor.py', None), ('test_release_tool.py', None), ('check_docs.py', None), ('test_ota.py', 'all'), ('test_ota_signer.py', 'asan'), ('test_ota_image.py', 'all'),
    ('test_ota_upload.py', 'all'), ('test_ota_fetch.py', 'all'), ('test_ota_download.py', 'all'), ('test_ota_install.py', 'all'),
    ('test_ota_release.py', 'all'), ('test_ota_discovery.py', 'all'), ('test_ota_release_fetch.py', 'all'),
    ('test_ota_release_chain.py', 'all'), ('test_restart.py', 'all'), ('test_photo_names.py', 'all'),
]

with tempfile.TemporaryDirectory(prefix='dsr-test-') as tmp:
    env = dict(os.environ, TMPDIR=tmp)
    for name, sources, includes, flags, libs, scenarios, extra in SUITES:
        binary = Path(tmp) / f'test_{name}'
        build(binary, sources, includes, flags, libs, args, env)
        for scenario in scenarios:
            subprocess.run([str(binary), *scenario], check=True, cwd=tmp, env=dict(env, **extra))
for script, mode in RUNNERS:
    switches = forward(args) if mode == 'all' else ['--sanitize'] if mode == 'asan' and args.sanitize else []
    subprocess.run(['python3', str((ROOT / 'tools' / script).resolve()), *switches], check=True)
print('OK')
