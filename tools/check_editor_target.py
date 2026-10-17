"""Real target own-frame evidence for editor acceptance paths, NOT peak stack."""
import argparse
import json
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('--build-dir',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
a=p.parse_args()
budgets={'dsr_ui_workspace_tick':512,'delete_entity':512,
         'delete_entity_confirm':256,'room_leave':128,'dsr_network_room_context':128}
frames={}
reports=[]
for unit in ('dsr_ui_config','dsr_ui_network'):
    files=list(a.build_dir.rglob(unit+'.c.su'))
    if len(files)!=1:raise SystemExit(f'Expected one real {unit} stack report: {files}')
    reports.append(str(files[0]))
    for line in files[0].read_text().splitlines():
        columns=line.split('\t');name=columns[0].rsplit(':',1)[-1]
        base=name.split('.',1)[0]
        if base not in budgets:continue
        if len(columns)!=3 or not columns[1].isdigit() or columns[2]!='static':raise SystemExit(f'Unexpected editor frame: {line}')
        size=int(columns[1])
        if name in frames or size>budgets[base]:raise SystemExit(f'Duplicate/oversize editor own frame: {line}')
        frames[name]=size
missing=set(budgets)-{name.split('.',1)[0] for name in frames}
if missing:raise SystemExit(f'Missing actual editor own frames: {sorted(missing)}')
a.out.write_text(json.dumps({'stack_reports':reports,'own_static_frames':frames,
    'base_function_budgets':budgets,'compiler_split_variants_included':True,
    'configured_ui_stack_bytes':8192,'configuration_evidence':'separate BOOT_MEMORY.json actual ELF audit',
    'transitive_lvgl_cjson_sdk_peak_measured':False,'physical_acceptance_verified':False},indent=2)+'\n')
print('PASS: actual editor/room-context own static frames and compiler variants within budgets; not transitive/physical proof')
