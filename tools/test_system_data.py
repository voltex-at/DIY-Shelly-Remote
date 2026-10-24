#!/usr/bin/env python3
"""Focused production system-data codecs with synthetic storage/radio providers."""
import argparse
import os
import re
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
case_names = ['format','sources','bonds','admission','stage','recovery','api']
p.add_argument('--case', choices=[*case_names,'control','grant_model','source_model','bonds_model','stage_model','stage_store','stage_crypto','style_gate','ble_owner','workspace_owner','all'], default='all')
p.add_argument('--sanitize', action='store_true')
p.add_argument('--ubsan-only', action='store_true')
a = p.parse_args()
cases = {
 'control': ['components/dsr_platform/dsr_maintenance_control.c','components/dsr_platform/dsr_maintenance_grant.c',
             'components/dsr_platform/dsr_maintenance.c','components/dsr_platform/dsr_activity.c','tests/test_system_data_control.c'],
 'grant_model': ['components/dsr_platform/dsr_maintenance_grant.c','tests/test_system_data_grants.c'],
 'stage': ['components/dsr_platform/dsr_maintenance_stage.c','components/dsr_platform/dsr_maintenance_crypto.c',
           'components/dsr_platform/dsr_maintenance_model.c','components/dsr_platform/dsr_maintenance_store.c','components/dsr_platform/dsr_maintenance.c',
           'components/dsr_platform/dsr_backup_sources_model.c','components/dsr_platform/dsr_backup_bonds_model.c','components/dsr_platform/dsr_backup_json.c',
           'components/dsr_platform/dsr_backup_format.c','components/dsr_platform/dsr_connection.c','components/dsr_platform/dsr_battery_model.c',
           'components/dsr_platform/dsr_wifi_policy.c','components/dsr_platform/dsr_automation_model.c','components/dsr_platform/dsr_automation_config.c',
           'components/dsr_platform/dsr_activity.c','components/dsr_config/dsr_config.c','components/dsr_config/dsr_photo_name.c','components/dsr_core/dsr_core.c',
           'tests/system_data/crypto.c','tests/system_data/gcm.c','tests/system_data/storage.c','tests/system_data/image.c','tests/test_system_data_stage.c'],
 'style_gate': ['components/dsr_platform/dsr_style_hint.c','components/dsr_platform/dsr_activity.c','tests/test_style_hint.c'],
 'stage_crypto': ['components/dsr_platform/dsr_maintenance_crypto.c','components/dsr_platform/dsr_maintenance_model.c',
                  'components/dsr_config/dsr_photo_name.c','tests/system_data/gcm.c','tests/test_system_data_stage_crypto.c'],
 'stage_store': ['components/dsr_platform/dsr_maintenance_store.c','components/dsr_platform/dsr_maintenance_model.c',
                 'components/dsr_config/dsr_photo_name.c','tests/system_data/storage.c','tests/test_system_data_stage_store.c'],
 'stage_model': ['components/dsr_platform/dsr_maintenance_model.c','components/dsr_config/dsr_photo_name.c','tests/test_system_data_stage_model.c'],
 'workspace_owner': ['components/dsr_platform/dsr_workspace.c','components/dsr_platform/dsr_activity.c','components/dsr_platform/dsr_config_health.c',
                     'components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/restart/workspace.c',
                     'tests/support/automation_store_stub.c','tests/support/automation_runtime_stub.c'],
 'ble_owner': ['components/dsr_platform/dsr_ble.c','components/dsr_platform/dsr_ble_model.c','components/dsr_platform/dsr_activity.c',
               'components/dsr_platform/dsr_backup_format.c','components/dsr_platform/dsr_backup_json.c',
               'components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/system_data/crypto.c','tests/test_ble.c'],
 'admission': ['components/dsr_platform/dsr_activity.c','components/dsr_platform/dsr_maintenance.c','tests/test_system_data_admission.c'],
 'sources': ['components/dsr_platform/dsr_backup_sources.c','components/dsr_platform/dsr_backup_sources_model.c',
             'components/dsr_platform/dsr_backup_json.c','components/dsr_platform/dsr_backup_format.c','components/dsr_platform/dsr_backup_bonds_model.c',
             'components/dsr_platform/dsr_connection.c','components/dsr_platform/dsr_settings.c','components/dsr_platform/dsr_style_hint.c',
             'components/dsr_platform/dsr_battery_model.c','components/dsr_platform/dsr_battery_profile.c','components/dsr_platform/dsr_wifi_policy.c',
             'components/dsr_platform/dsr_automation_model.c','components/dsr_platform/dsr_automation_config.c','components/dsr_platform/dsr_automation_store.c',
             'components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/system_data/crypto.c','tests/system_data/storage.c','tests/test_system_data_sources.c'],
 'format': ['components/dsr_platform/dsr_backup_format.c', 'components/dsr_config/dsr_config.c',
            'components/dsr_core/dsr_core.c', 'tests/system_data/crypto.c', 'tests/test_system_data_format.c'],
 'source_model': ['components/dsr_platform/dsr_backup_sources_model.c','components/dsr_platform/dsr_backup_json.c',
                  'components/dsr_platform/dsr_backup_format.c','components/dsr_platform/dsr_connection.c',
                  'components/dsr_platform/dsr_battery_model.c','components/dsr_platform/dsr_wifi_policy.c',
                  'components/dsr_platform/dsr_automation_model.c','components/dsr_platform/dsr_automation_config.c',
                  'components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/system_data/crypto.c',
                  'tests/test_system_data_sources_model.c'],
 'bonds_model': ['components/dsr_platform/dsr_backup_bonds_model.c','components/dsr_platform/dsr_backup_json.c',
                 'components/dsr_platform/dsr_backup_format.c','components/dsr_config/dsr_config.c',
                 'components/dsr_core/dsr_core.c','tests/system_data/crypto.c','tests/test_system_data_bonds_model.c'],
 'bonds': ['components/dsr_platform/dsr_backup_bonds_model.c','components/dsr_platform/dsr_backup_bonds_sdk.c','components/dsr_platform/dsr_backup_bonds.c',
           'components/dsr_platform/dsr_backup_json.c','components/dsr_platform/dsr_backup_format.c',
           'components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/system_data/crypto.c','tests/test_system_data_bonds.c'],
}
selected = case_names if a.case == 'all' else [a.case]
flags = ['-std=c11','-D_POSIX_C_SOURCE=200809L','-Wall','-Wextra','-Werror','-Wpedantic','-g','-ffunction-sections','-fdata-sections']
if a.sanitize:
    flags += ['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:
    flags += ['-fsanitize=undefined','-fno-omit-frame-pointer']
includes = ['tests/system_data','tests/actor','tests/support','components/dsr_platform','components/dsr_platform/include',
            'components/dsr_config/include','components/dsr_core/include','.test-deps','/usr/include/cjson']
with tempfile.TemporaryDirectory(prefix='dsr-system-data-') as temp:
    for case in selected:
        if case not in cases:
            raise SystemExit(f'{case}: implementation/test not present; cannot claim complete integration')
        target = Path(temp) / case
        sdk_includes = []
        if case == 'bonds':
            idf = Path(os.environ.get('IDF_PATH',ROOT / '.local/esp-idf'))
            nimble = idf / 'components/bt/host/nimble/nimble/nimble'
            if not (nimble / 'host/include/host/ble_store.h').is_file():
                raise SystemExit('bonds: IDF_PATH must provide pinned ESP-IDF 5.5.4 headers')
            version_header = (idf / 'components/esp_common/include/esp_idf_version.h').read_text()
            for key,value in [('MAJOR',5),('MINOR',5),('PATCH',4)]:
                if not re.search(rf'^#define ESP_IDF_VERSION_{key}\s+{value}\s*$', version_header, re.M):
                    raise SystemExit('bonds: requires ESP-IDF 5.5.4, refusing another ABI')
            sdk_includes = [v for folder in ['include','host/include','host/src'] for v in ['-I',str(nimble / folder)]]
        subprocess.run([os.environ.get('CC','cc'), *flags,
                        *(['-DDSR_BOARD_ES3C35P=1','-DDSR_TEST_SYSTEM_DATA=1'] if case == 'sources' else []),
                        *(['-DDSR_TEST_REAL_ACTIVITY=1'] if case == 'stage' else []),
                        *(['-DDSR_BOARD_ES3C35P=1'] if case == 'style_gate' else []),
                        *(['-DDSR_TEST_SYSTEM_DATA=1','-I',str(ROOT / 'tests/ble')] if case == 'ble_owner' else []),
                        *(['-DDSR_TEST_SYSTEM_DATA=1','-I',str(ROOT / 'tests/restart')] if case == 'workspace_owner' else []),
                        *[v for folder in includes for v in ['-I',str(ROOT / folder) if not folder.startswith('/') else folder]],
                        *sdk_includes,
                        *[str(ROOT / source) for source in cases[case]],
                        *(['-Wl,--wrap=cJSON_ParseWithLengthOpts'] if case=='format' else []),
                        '-Wl,--gc-sections','-L',str(ROOT / '.test-deps'),'-l:libcjson.so.1','-lcrypto','-lm','-o',str(target)], check=True)
        result = subprocess.run([str(target),'--maintenance-owner' if case in ('ble_owner','workspace_owner','style_gate') else str(ROOT / 'tests/system_data/manifest-v1.json')], text=True, capture_output=True)
        if result.returncode:
            print(result.stderr.strip())
            raise SystemExit(result.returncode)
        if not result.stdout.startswith('PASS:'):
            raise SystemExit(f'{case}: no success marker')
        print(result.stdout.strip())
        if case == 'stage':
            result = subprocess.run([str(target),str(ROOT / 'tests/system_data/manifest-v1.json'),'commit-uncertain'],text=True,capture_output=True)
            if result.returncode or not result.stdout.startswith('PASS:'):
                raise SystemExit(result.stderr.strip() or 'stage commit-uncertain: missing success marker')
            print(result.stdout.strip())
