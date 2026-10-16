#!/usr/bin/env python3
"""Synthetic rejection tests of the target auditor; no target evidence."""
import json,os,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
public={'dsr_actions':['dsr_actions_init','dsr_actions_request','dsr_actions_prepare','dsr_actions_worker_allowed','dsr_actions_complete'],'dsr_actions_model':['dsr_actions_model_begin','dsr_actions_model_worker_allowed','dsr_actions_model_complete'],'dsr_actions_parse':['dsr_actions_request_parse','dsr_actions_plan_build'],'dsr_actions_api':['dsr_actions_status_json','dsr_actions_log_json','dsr_actions_cancel_parse','dsr_actions_log_parse'],'dsr_ui_actions':['dsr_ui_actions_open','dsr_ui_actions_execute','dsr_ui_actions_tick'],'dsr_http':['dsr_http_job_guard','dsr_http_job_write_hook','dsr_http_set_output','dsr_http_cover'],'dsr_worker':['dsr_worker_submit','dsr_worker_stop_request','dsr_worker_action_stop_request'],'dsr_ui_network':['dsr_network_init']}
with tempfile.TemporaryDirectory(prefix='actions-audit-') as d:
 d=Path(d);nm=d/'xtensa-esp32s3-elf-nm';nm.write_text('#!/usr/bin/env python3\nimport os\nprint("00000000 "+os.environ.get("TEST_BSS","0")+" b state")\n');nm.chmod(0o755)
 def setup():
  (d/'BOOT_MEMORY.json').write_text(json.dumps({'internal_bss_bytes':66000,'ui_stack':{'bytes':8192},'lvgl_pool':{'bytes':131072}}))
  for unit,names in public.items():
   (d/(unit+'.c.su')).write_text(''.join(f'a.c:1:1:{n}\t128\tstatic\n' for n in names));(d/(unit+'.c.obj')).touch()
 def run(ok,bss='0'):
  result=subprocess.run(['python3',str(R/'tools/check_actions_target.py'),'--build-dir',str(d),'--out',str(d/'result.json')],env=dict(os.environ,PATH=str(d)+os.pathsep+os.environ['PATH'],TEST_BSS=bss),capture_output=True,text=True);assert (result.returncode==0)==ok,result.stdout+result.stderr
 setup();run(True)
 (d/'dsr_actions.c.su').write_text('a.c:1:1:dsr_actions_init\t1025\tstatic\n');run(False)
 setup();(d/'dsr_actions_parse.c.su').write_text('a.c:1:1:dsr_actions_request_parse\t32\tdynamic,bounded\n');run(False)
 setup();(d/'dsr_actions_parse.c.su').write_text('');run(False)
 setup();p=d/'dsr_actions_parse.c.su';p.write_text(p.read_text()*2);run(False)
 setup();run(False,'65')
 setup();m=json.loads((d/'BOOT_MEMORY.json').read_text());m['internal_bss_bytes']=81921;(d/'BOOT_MEMORY.json').write_text(json.dumps(m));run(False)
print('PASS: actions target auditor 1 positive + 6 rejection fixtures; synthetic reports only')
