#!/usr/bin/env python3
"""Actual target own frames/BSS. No SDK peak, endurance or device claims."""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
def require(ok,message):
 if not ok:raise SystemExit(message)
m=json.loads((a.build_dir/'BOOT_MEMORY.json').read_text())
require(type(m['internal_bss_bytes']) is int and 8192<=m['internal_bss_bytes']<=81920,'Actions internal BSS exceeds existing 80 KiB limit')
require(m['ui_stack']['bytes']==8192 and m['lvgl_pool']['bytes']==131072,'Actions changed existing UI stack or LVGL pool')
units={
 'dsr_actions':(1024,{'dsr_actions_init','dsr_actions_request','dsr_actions_prepare','dsr_actions_worker_allowed','dsr_actions_complete'}),
 'dsr_actions_model':(512,{'dsr_actions_model_begin','dsr_actions_model_worker_allowed','dsr_actions_model_complete'}),
 'dsr_actions_parse':(512,{'dsr_actions_request_parse','dsr_actions_plan_build'}),
 'dsr_actions_api':(1024,{'dsr_actions_status_json','dsr_actions_log_json','dsr_actions_cancel_parse','dsr_actions_log_parse'}),
 'dsr_ui_actions':(1024,{'dsr_ui_actions_open','dsr_ui_actions_execute','dsr_ui_actions_tick'}),
 'dsr_http':(2048,{'dsr_http_job_guard','dsr_http_job_write_hook','dsr_http_set_output','dsr_http_cover'}),
 'dsr_worker':(2048,{'dsr_worker_submit','dsr_worker_stop_request','dsr_worker_action_stop_request'}),
 'dsr_ui_network':(2048,{'dsr_network_init'}),
}
reports={};storage={}
for unit,(cap,required) in units.items():
 paths=list(a.build_dir.rglob(unit+'.c.su'));require(len(paths)==1,f'Missing/duplicate actions stack report: {unit}')
 frames={}
 for line in paths[0].read_text().splitlines():
  f=line.split('\t');require(len(f)==3 and f[1].isdigit() and f[2]=='static',f'Unbounded actions frame: {line}');name=f[0].rsplit(':',1)[-1];size=int(f[1]);require(name not in frames and size<=cap,f'Actions {unit} duplicate/frame exceeds {cap}: {line}');frames[name]=size
 require(required<={n.split('.')[0] for n in frames},f'Missing actions public frame: {unit}')
 reports[unit]={'maximum_own_frame_bytes':cap,'own_frames':frames}
 if unit!='dsr_ui_network':
  objects=list(a.build_dir.rglob(unit+'.c.obj'));require(len(objects)==1,f'Missing/duplicate actions object: {unit}')
  symbols={}
  for line in subprocess.check_output(['xtensa-esp32s3-elf-nm','-S','--radix=d',str(objects[0])],text=True).splitlines():
   f=line.split()
   if len(f)==4 and f[2].lower() in ['b','d']:require(f[1].isdigit() and f[3] not in symbols,f'Invalid actions writable storage: {line}');symbols[f[3]]=int(f[1])
  limit=1024 if unit=='dsr_worker' else 64 if unit=='dsr_ui_actions' else 16 if unit in ['dsr_actions','dsr_http'] else 0
  require(sum(symbols.values())<=limit,f'Actions state moved to static writable storage: {unit}')
  storage[unit]={'bytes':sum(symbols.values()),'maximum_bytes':limit,'symbols':symbols}
a.out.write_text(json.dumps({'actual_internal_bss_bytes':m['internal_bss_bytes'],'maximum_internal_bss_bytes':81920,'own_stack_reports':reports,'fixed_storage':storage,'psram_context_max_bytes':12288,'actor_steps':16,'activity_entries':128,'activity_entry_bytes':32,'public_snapshot_max_bytes':2048,'worker_task_bytes':10240,'sdk_stack_or_heap_peak_measured':False,'physical_multi_device_execution_verified':False},indent=2)+'\n')
print('PASS: actual actions target own frames/BSS; SDK peak and physical Flash remain unmeasured')
