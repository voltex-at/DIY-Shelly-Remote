"""Audit compiled AP visit frames, not a wired controller or radio proof."""
import argparse
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--elf', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
reports = list(a.build_dir.rglob('dsr_wifi.c.su'))
if len(reports) != 1:
    raise SystemExit(f'Expected one actual Wi-Fi stack report: {reports}')
budgets = {'dsr_wifi_visit_ap': 256, 'dsr_wifi_ap_cancelled': 128,
           'start_station': 768, 'stop_station': 256, 'wait_connection': 128}
frames = {}
for line in reports[0].read_text().splitlines():
    columns = line.split('\t')
    name = columns[0].rsplit(':', 1)[-1]
    base = name.split('.', 1)[0]
    if base not in budgets:
        continue
    if len(columns) != 3 or not columns[1].isdigit() or columns[2] != 'static':
        raise SystemExit(f'Unexpected AP visit frame: {line}')
    if name in frames or int(columns[1]) > budgets[base]:
        raise SystemExit(f'Duplicate/oversize AP visit frame: {line}')
    frames[name] = int(columns[1])
required = {'dsr_wifi_visit_ap', 'dsr_wifi_ap_cancelled'}
if required - {name.split('.', 1)[0] for name in frames}:
    raise SystemExit('Actual public AP visit compile evidence missing')
symbols = subprocess.check_output(
    ['xtensa-esp32s3-elf-nm', '--defined-only', str(a.elf)], text=True)
retained = sorted(name for name in required if any(
    line.split()[-1] == name for line in symbols.splitlines() if line.split()))
a.out.write_text(json.dumps({
    'stack_report': str(reports[0]), 'own_static_frames': frames,
    'base_function_budgets': budgets, 'required_public_apis_compiled': sorted(required),
    'public_apis_retained_in_linked_elf': retained,
    'forced_linker_retention_added': False, 'production_controller_wired': False,
    'callback_sdk_transitive_stack_peak_measured': False,
    'physical_radio_restore_or_provisioning_verified': False,
}, indent=2) + '\n')
print('PASS: compiled AP visit own frames within budgets; controller NOT wired; no radio/provisioning proof')
