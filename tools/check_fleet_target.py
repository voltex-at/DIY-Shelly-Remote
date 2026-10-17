"""Audit exact ESP32-S3 multi-device objects, own frames and static memory.

These bounds do not measure SDK/cJSON transitive stack, heap or physical boot.
"""
import argparse
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()


def require(condition, message):
    if not condition:
        raise SystemExit(message)


memory = json.loads((a.build_dir/'BOOT_MEMORY.json').read_text())
require(type(memory['internal_bss_bytes']) is int and 8192 <= memory['internal_bss_bytes'] <= 81920,
        'Internal BSS exceeds 80 KiB multi-device budget; parent was 48952 B')
require(memory['lvgl_pool']['bytes'] == 131072 and memory['ui_stack']['bytes'] == 8192,
        'Existing PSRAM LVGL pool/internal UI stack changed')
units = {
    'dsr_fleet': (512, {'dsr_fleet_request', 'dsr_fleet_light_request', 'dsr_fleet_rgb_request', 'dsr_fleet_ht_refresh', 'dsr_fleet_next', 'dsr_fleet_complete'}),
    'dsr_discovery_model': (512, {'dsr_ht_parse', 'dsr_lan_parse_capabilities'}),
    'dsr_connection': (1024, {'dsr_connections_load', 'dsr_connection_save', 'dsr_connection_publish', 'dsr_connection_remove'}),
    'dsr_runtime_api': (512, {'dsr_runtime_json', 'dsr_output_parse', 'dsr_light_parse', 'dsr_rgb_parse', 'dsr_ht_refresh_parse'}),
    'dsr_workspace': (1024, {'dsr_workspace_output_request', 'dsr_workspace_light_request', 'dsr_workspace_rgb_request', 'dsr_workspace_ht_refresh', 'dsr_workspace_take_output', 'dsr_workspace_runtime_snapshot'}),
    'dsr_ui_network': (2048, {'dsr_network_init'}),
    'dsr_ui_config': (1024, {'dsr_ui_workspace_migrate_status'}),
    'dsr_worker': (2048, {'dsr_worker_submit'}),
}
reports = {}
for unit, (maximum, public) in units.items():
    paths = sorted(a.build_dir.rglob(unit+'.c.su'))
    require(len(paths) == 1, f'Expected exact stack report for {unit}: {paths}')
    frames = {}
    for line in paths[0].read_text().splitlines():
        parts = line.split('\t')
        require(len(parts) == 3 and parts[1].isdigit() and parts[2] == 'static',
                f'Unsupported/unbounded own frame: {line}')
        name, size = parts[0].rsplit(':', 1)[-1], int(parts[1])
        budget = 2048 if unit == 'dsr_ui_config' and name.split('.')[0] == 'object_choice' else maximum
        require(size <= budget, f'{unit} own frame exceeds {budget} B: {line}')
        require(name not in frames, f'Duplicate own-frame record: {unit}:{name}')
        frames[name] = size
    bases = {name.split('.')[0] for name in frames}
    require(public <= bases, f'{unit} required public frames missing: {public-bases}')
    reports[unit] = {'report': str(paths[0]), 'budget_bytes': maximum, 'own_frames': frames}

states = {}
for unit, symbol, maximum in [('dsr_ui_network', 'fleet', 8192),
                             ('dsr_workspace', 'runtime', 4096),
                             ('dsr_workspace', 'outputs', 4096),
                             ('dsr_ui_config', 'migrations', 4096)]:
    objects = sorted(a.build_dir.rglob(unit+'.c.obj'))
    require(len(objects) == 1, f'Expected one {unit} target object')
    table = subprocess.check_output(['xtensa-esp32s3-elf-nm', '-S', '--radix=d', str(objects[0])], text=True)
    rows = [line.split() for line in table.splitlines() if line.split() and line.split()[-1] == symbol]
    require(len(rows) == 1 and len(rows[0]) == 4, f'{unit}:{symbol} target state missing/ambiguous')
    size = int(rows[0][1])
    require(rows[0][2].lower() == 'b' and 0 < size <= maximum,
            f'{unit}:{symbol} exceeds compiled static budget: {size}/{maximum}')
    states[symbol] = {'object': str(objects[0]), 'bytes': size, 'maximum_bytes': maximum}

a.out.write_text(json.dumps({
    'parent_internal_bss_bytes': 48952,
    'actual_internal_bss_bytes': memory['internal_bss_bytes'],
    'maximum_internal_bss_bytes': 81920,
    'actual_delta_from_parent_bytes': memory['internal_bss_bytes']-48952,
    'target_object_static_state': states,
    'own_stack_reports': reports,
    'existing_object_choice_buffer_exception_bytes': 2048,
    'configured_ui_stack_bytes': 8192,
    'lvgl_pool_in_psram_bytes': 131072,
    'runtime_heap_or_transitive_stack_peak_measured': False,
    'physical_boot_or_two_shelly_acceptance_verified': False,
}, indent=2)+'\n')
print('PASS: exact target multi-device own frames/static arrays; internal BSS <=80 KiB; physical boot/heap/SDK peak remain unmeasured')
