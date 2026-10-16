"""Audit actual target native updater retention, own frames and fixed storage.
SDK TLS/Flash/transitive stack or heap peaks are not measured by this audit.
"""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--elf',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
def require(ok,message):
 if not ok:raise SystemExit(message)
memory=json.loads((a.build_dir/'BOOT_MEMORY.json').read_text())
require(8192<=memory['internal_bss_bytes']<=81920,'Native OTA internal BSS exceeds80KiB')
require(memory['ui_stack']['bytes']==8192 and memory['lvgl_pool']['bytes']==131072,'Native OTA grew existing UI stack/LVGL pool')
units={
 'dsr_native_ota':(1024,{'dsr_native_ota_init','dsr_native_ota_prepare','dsr_native_ota_execute','dsr_native_ota_snapshot','dsr_native_ota_cancel'}),
 'dsr_native_ota_clock':(512,{'dsr_native_ota_clock'}),
 'dsr_ota_install':(1024,{'dsr_ota_install','dsr_ota_install_ready'}),
 'dsr_ota_stage':(256,{'dsr_ota_stage_acquire','dsr_ota_stage_commit','dsr_ota_stage_uncertain','dsr_ota_stage_release'}),
 'dsr_ota_discovery_fetch':(512,{'dsr_ota_discovery_fetch'}),
 'dsr_ui_native_ota':(1024,{'dsr_ui_native_ota_open','dsr_ui_native_ota_tick','dsr_ui_native_ota_hide'}),
 'dsr_worker':(2048,{'dsr_worker_submit'}),
 'dsr_ui_network':(2048,{'dsr_network_request_ota'}),
}
defined={line.split()[-1] for line in subprocess.check_output(['xtensa-esp32s3-elf-nm','--defined-only',str(a.elf)],text=True).splitlines() if line.split()}
reports={};retained=[]
for unit,(limit,public) in units.items():
 paths=list(a.build_dir.rglob(unit+'.c.su'));require(len(paths)==1,'Expected one stack report: '+unit);frames={}
 for line in paths[0].read_text().splitlines():
  fields=line.split('\t');require(len(fields)==3 and fields[1].isdigit() and fields[2]=='static','Unbounded/unrecognized frame: '+line)
  name=fields[0].rsplit(':',1)[-1];size=int(fields[1]);require(size<=limit and name not in frames,'Frame/duplicate exceeds policy: '+line);frames[name]=size
 require(public<={name.split('.')[0] for name in frames},'Missing updater stack function: '+unit)
 require(public<=defined,'Updater function absent from linked ELF: '+unit);retained+=sorted(public)
 reports[unit]={'maximum_bytes':limit,'own_frames':frames}
states={}
for unit,limit in [('dsr_native_ota',2304),('dsr_ui_native_ota',1280),('dsr_ota_stage',64)]:
 paths=list(a.build_dir.rglob(unit+'.c.obj'));require(len(paths)==1,'Expected one object: '+unit)
 symbols={}
 for line in subprocess.check_output(['xtensa-esp32s3-elf-nm','-S','--radix=d',str(paths[0])],text=True).splitlines():
  fields=line.split()
  if len(fields)==4 and fields[2].lower()=='b':
   require(fields[1].isdigit() and fields[3] not in symbols,'Invalid static symbol: '+line);symbols[fields[3]]=int(fields[1])
 total=sum(symbols.values());require(0<total<=limit,'Updater fixed storage exceeds budget: '+unit);states[unit]={'bytes':total,'maximum_bytes':limit,'symbols':symbols}
a.out.write_text(json.dumps({'retained_real_caller_functions':retained,'own_frames':reports,'fixed_storage':states,'actual_internal_bss_bytes':memory['internal_bss_bytes'],'maximum_internal_bss_bytes':81920,'configured_worker_stack_bytes':10240,'configured_ui_stack_bytes':8192,'lvgl_psram_pool_bytes':131072,'sdk_transitive_stack_or_heap_peak_measured':False,'approved_publisher_anchor_or_release':False,'physical_install_or_rollback_verified':False},indent=2)+'\n')
print('PASS: real native updater ELF retained, static frames/fixed state and80KiB BSS; no job/event or stack/pool growth')
