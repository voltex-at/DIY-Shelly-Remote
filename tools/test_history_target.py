#!/usr/bin/env python3
"""Synthetic rejection tests of the target auditor; no target evidence."""
import json,os,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
public={'dsr_history':['dsr_history_init','dsr_history_record','dsr_history_query','dsr_history_service','dsr_history_clock_execute'],'dsr_history_model':['dsr_history_model_record','dsr_history_model_query','dsr_history_decode','dsr_history_checkpoint'],'dsr_history_store':['dsr_history_store_load','dsr_history_store_append','dsr_history_store_checkpoint'],'dsr_history_api':['dsr_history_json','dsr_history_parse'],'dsr_clock':['dsr_clock_sync'],'dsr_ui_history':['dsr_ui_history_open','dsr_ui_energy_open','dsr_ui_history_tick'],'dsr_worker':['dsr_worker_submit'],'dsr_ui_network':['dsr_network_init']}
with tempfile.TemporaryDirectory(prefix='history-audit-') as d:
 d=Path(d);nm=d/'xtensa-esp32s3-elf-nm';nm.write_text('#!/usr/bin/env python3\nimport os\nprint("00000000 "+os.environ.get("TEST_BSS","0")+" b state")\n');nm.chmod(0o755)
 def setup():
  (d/'BOOT_MEMORY.json').write_text(json.dumps({'internal_bss_bytes':66000,'ui_stack':{'bytes':8192},'lvgl_pool':{'bytes':131072}}))
  for unit,names in public.items():
   (d/(unit+'.c.su')).write_text(''.join(f'a.c:1:1:{n}\t128\tstatic\n' for n in names));(d/(unit+'.c.obj')).touch()
 def run(ok,bss='0'):
  result=subprocess.run(['python3',str(R/'tools/check_history_target.py'),'--build-dir',str(d),'--out',str(d/'result.json')],env=dict(os.environ,PATH=str(d)+os.pathsep+os.environ['PATH'],TEST_BSS=bss),capture_output=True,text=True);assert (result.returncode==0)==ok,result.stdout+result.stderr
 setup();run(True)
 (d/'dsr_history.c.su').write_text('a.c:1:1:dsr_history_init\t1025\tstatic\n');run(False)
 setup();(d/'dsr_clock.c.su').write_text('a.c:1:1:dsr_clock_sync\t32\tdynamic,bounded\n');run(False)
 setup();(d/'dsr_clock.c.su').write_text('');run(False)
 setup();p=d/'dsr_clock.c.su';p.write_text(p.read_text()*2);run(False)
 setup();run(False,'33')
 setup();m=json.loads((d/'BOOT_MEMORY.json').read_text());m['internal_bss_bytes']=81921;(d/'BOOT_MEMORY.json').write_text(json.dumps(m));run(False)
print('PASS: history target auditor 1 positive + 6 rejection fixtures; synthetic reports only')
