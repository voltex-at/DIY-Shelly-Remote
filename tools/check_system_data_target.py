"""Real ESP32-S3 maintenance objects/own frames; no runtime peak claim."""
import argparse
import json
from pathlib import Path
import subprocess


def require(value, message):
    if not value:
        raise SystemExit(message)


p = argparse.ArgumentParser()
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--elf', type=Path, required=True)
p.add_argument('--sdkconfig', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
units = ['api', 'bonds', 'bonds_model', 'bonds_sdk', 'format', 'json', 'sources', 'sources_model']
units = ['dsr_backup_' + name for name in units]
units += ['dsr_maintenance' + suffix for suffix in ['', '_control', '_crypto', '_grant', '_io', '_model', '_recovery', '_recovery_io', '_stage', '_store']]
units += ['dsr_ui_maintenance']
sdk = set(a.sdkconfig.read_text().splitlines())
require('CONFIG_ESP_MAIN_TASK_STACK_SIZE=6144' in sdk, 'Main stack budget changed')
memory = json.loads((a.build_dir / 'BOOT_MEMORY.json').read_text())
require(memory['ui_stack']['bytes'] == 8192 and memory['lvgl_pool']['bytes'] == 131072,
        'UI stack or PSRAM pool budget changed')
require(memory['internal_bss_bytes'] <= 81920, 'Existing internal BSS budget exceeded')
required = {'dsr_maintenance_recover', 'dsr_backup_api', 'dsr_maintenance_confirm',
            'dsr_maintenance_candidate', 'dsr_ble_maintenance_stop', 'dsr_workspace_cover_request'}
symbols = subprocess.check_output(['xtensa-esp32s3-elf-nm', '--defined-only', str(a.elf)], text=True)
defined = {line.split()[-1] for line in symbols.splitlines() if line.split()}
require(required <= defined, f'Missing retained maintenance symbols: {sorted(required-defined)}')
reports = {}
fixed = 0
for unit in units:
    objects = list(a.build_dir.rglob(unit + '.c.obj'))
    stacks = list(a.build_dir.rglob(unit + '.c.su'))
    require(len(objects) == len(stacks) == 1, f'Missing/duplicate target evidence: {unit}')
    frames = {}
    for line in stacks[0].read_text().splitlines():
        cols = line.split('\t')
        require(len(cols) == 3 and cols[1].isdigit() and cols[2] == 'static', f'Unbounded frame: {line}')
        name = cols[0].rsplit(':', 1)[-1]
        require(name not in frames and int(cols[1]) <= 1024, f'Duplicate/oversized maintenance frame: {line}')
        frames[name] = int(cols[1])
    require(frames, f'Empty stack report: {unit}')
    nm = subprocess.check_output(['xtensa-esp32s3-elf-nm', '-S', '--radix=d', str(objects[0])], text=True)
    state = {v[3]: int(v[1]) for line in nm.splitlines() if len(v := line.split()) == 4 and v[2].lower() in ['b', 'd']}
    fixed += sum(state.values())
    deps = subprocess.check_output(['xtensa-esp32s3-elf-nm', '-u', str(objects[0])], text=True)
    deps = {line.split()[-1] for line in deps.splitlines() if line.split()}
    forbidden = {name for name in deps if name.startswith(('esp_ota_', 'esp_partition_erase', 'nvs_flash_erase'))}
    require(not forbidden, f'Unexpected destructive/OTA dependency: {unit}: {sorted(forbidden)}')
    tasks = {name for name in deps if name.startswith('xTaskCreate')}
    require(tasks <= ({'xTaskCreatePinnedToCore'} if unit == 'dsr_backup_api' else set()), f'Unexpected task owner: {unit}')
    reports[unit] = {'own_frames': frames, 'static_writable_bytes': sum(state.values())}
require(fixed <= 4096, f'Maintenance fixed storage exceeds 4096 B: {fixed}')
a.out.write_text(json.dumps({'objects': reports, 'static_writable_bytes': fixed,
    'maximum_own_frame_bytes': 1024, 'main_stack_bytes': 6144,
    'temporary_https_transport_stack_bytes': 16384,
    'transitive_sdk_stack_or_heap_peak_measured': False,
    'physical_durability_and_reconnection_verified': False}, indent=2) + '\n')
print(f'PASS: {len(units)} real maintenance objects, own frames <=1024 B, fixed state {fixed}/4096 B; existing memory budgets retained')
