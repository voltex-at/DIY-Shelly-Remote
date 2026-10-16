#!/usr/bin/env python3
"""Actual target own frames/BSS. No SDK peak, endurance or device claims."""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
def require(ok,message):
 if not ok:raise SystemExit(message)
m=json.loads((a.build_dir/'BOOT_MEMORY.json').read_text())
require(type(m['internal_bss_bytes']) is int and 8192<=m['internal_bss_bytes']<=81920,'History internal BSS exceeds existing 80 KiB limit')
require(m['ui_stack']['bytes']==8192 and m['lvgl_pool']['bytes']==131072,'History changed existing UI stack or LVGL pool')
units={
 'dsr_history':(1024,{'dsr_history_init','dsr_history_record','dsr_history_query','dsr_history_service','dsr_history_clock_execute'}),
 'dsr_history_model':(512,{'dsr_history_model_record','dsr_history_model_query','dsr_history_decode','dsr_history_checkpoint'}),
 'dsr_history_store':(1024,{'dsr_history_store_load','dsr_history_store_append','dsr_history_store_checkpoint'}),
 'dsr_history_api':(1024,{'dsr_history_json','dsr_history_parse'}),
 'dsr_clock':(512,{'dsr_clock_sync'}),
 'dsr_ui_history':(1024,{'dsr_ui_history_open','dsr_ui_energy_open','dsr_ui_history_tick'}),
 'dsr_worker':(2048,{'dsr_worker_submit'}),
 'dsr_ui_network':(2048,{'dsr_network_init'}),
}
reports={};storage={}
for unit,(cap,required) in units.items():
 paths=list(a.build_dir.rglob(unit+'.c.su'));require(len(paths)==1,f'Missing/duplicate history stack report: {unit}')
 frames={}
 for line in paths[0].read_text().splitlines():
  f=line.split('\t');require(len(f)==3 and f[1].isdigit() and f[2]=='static',f'Unbounded history frame: {line}');name=f[0].rsplit(':',1)[-1];size=int(f[1]);require(name not in frames and size<=cap,f'History {unit} duplicate/frame exceeds {cap}: {line}');frames[name]=size
 require(required<={n.split('.')[0] for n in frames},f'Missing history public frame: {unit}')
 reports[unit]={'maximum_own_frame_bytes':cap,'own_frames':frames}
 if unit in ['dsr_history','dsr_history_model','dsr_history_store','dsr_history_api','dsr_clock','dsr_ui_history']:
  objects=list(a.build_dir.rglob(unit+'.c.obj'));require(len(objects)==1,f'Missing/duplicate history object: {unit}')
  symbols={}
  for line in subprocess.check_output(['xtensa-esp32s3-elf-nm','-S','--radix=d',str(objects[0])],text=True).splitlines():
   f=line.split()
   if len(f)==4 and f[2].lower() in ['b','d']:require(f[1].isdigit() and f[3] not in symbols,f'Invalid history writable storage: {line}');symbols[f[3]]=int(f[1])
  limit=32 if unit in ['dsr_history','dsr_ui_history'] else 8 if unit=='dsr_clock' else 0
  require(sum(symbols.values())<=limit,f'History PSRAM state moved to static writable storage: {unit}')
  storage[unit]={'bytes':sum(symbols.values()),'maximum_bytes':limit,'symbols':symbols}
a.out.write_text(json.dumps({'actual_internal_bss_bytes':m['internal_bss_bytes'],'maximum_internal_bss_bytes':81920,'own_stack_reports':reports,'fixed_storage':storage,'psram_model_max_bytes':196608,'psram_checkpoint_max_bytes':186272,'psram_query_max_bytes':16384,'psram_json_max_bytes':49152,'worker_task_bytes':10240,'sdk_stack_or_heap_peak_measured':False,'physical_flash_powercut_or_endurance_verified':False},indent=2)+'\n')
print('PASS: actual history target own frames/BSS; SDK peak and physical Flash remain unmeasured')
