"""Synthetic positive and negative fixtures for the native OTA target auditor."""
import json,os,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
units={
 'dsr_native_ota':['dsr_native_ota_init','dsr_native_ota_prepare','dsr_native_ota_execute','dsr_native_ota_snapshot','dsr_native_ota_cancel'],
 'dsr_native_ota_clock':['dsr_native_ota_clock'],
 'dsr_ota_install':['dsr_ota_install','dsr_ota_install_ready'],
 'dsr_ota_stage':['dsr_ota_stage_acquire','dsr_ota_stage_commit','dsr_ota_stage_uncertain','dsr_ota_stage_release'],
 'dsr_ota_discovery_fetch':['dsr_ota_discovery_fetch'],
 'dsr_ui_native_ota':['dsr_ui_native_ota_open','dsr_ui_native_ota_tick','dsr_ui_native_ota_hide'],
 'dsr_worker':['dsr_worker_submit'],'dsr_ui_network':['dsr_network_request_ota']}
with tempfile.TemporaryDirectory(prefix='dsr-native-budget-') as name:
 folder=Path(name);nm=folder/'xtensa-esp32s3-elf-nm';nm.write_text('#!'+sys.executable+'\nimport sys\nfrom pathlib import Path\nprint(Path(sys.argv[-1]).read_text())\n');nm.chmod(0o755)
 env=dict(os.environ,PATH=str(folder)+os.pathsep+os.environ['PATH'])
 def fixture():
  for path in folder.glob('dsr_*'):path.unlink()
  (folder/'BOOT_MEMORY.json').write_text(json.dumps({'internal_bss_bytes':71000,'ui_stack':{'bytes':8192},'lvgl_pool':{'bytes':131072}}))
  (folder/'target.elf').write_text(''.join('0000 T '+name+'\n' for names in units.values() for name in names))
  for unit,names in units.items():(folder/(unit+'.c.su')).write_text(''.join(unit+'.c:1:1:'+name+'\t128\tstatic\n' for name in names))
  for unit in ['dsr_native_ota','dsr_ui_native_ota','dsr_ota_stage']:(folder/(unit+'.c.obj')).write_text('00000000 00000016 b state\n')
 def check(expected):
  result=subprocess.run([sys.executable,str(ROOT/'tools/check_native_ota_target.py'),'--build-dir',str(folder),'--elf',str(folder/'target.elf'),'--out',str(folder/'NATIVE_OTA.json')],env=env,capture_output=True,text=True)
  assert (result.returncode==0)==expected,result.stdout+result.stderr
 fixture();check(True)
 fixture();(folder/'target.elf').write_text('');check(False)
 fixture();(folder/'dsr_ota_install.c.su').write_text('x:1:1:dsr_ota_install\t2048\tstatic\n');check(False)
 fixture();(folder/'dsr_worker.c.su').write_text('x:1:1:dsr_worker_submit\t2064\tstatic\n');check(False)
 fixture();(folder/'dsr_native_ota_clock.c.su').write_text('x:1:1:dsr_native_ota_clock\t128\tdynamic,bounded\n');check(False)
 fixture();(folder/'dsr_ui_native_ota.c.su').unlink();check(False)
 fixture();(folder/'dsr_native_ota.c.obj').write_text('0000 00002305 b state\n');check(False)
 fixture();memory=json.loads((folder/'BOOT_MEMORY.json').read_text());memory['internal_bss_bytes']=81921;(folder/'BOOT_MEMORY.json').write_text(json.dumps(memory));check(False)
print('PASS: native OTA target auditor1positive+7rejections; synthetic reports, no target proof')
