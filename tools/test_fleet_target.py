"""Exercise the real target auditor with synthetic reports; no ELF proof."""
import ast,json,os,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'tools/check_fleet_target.py').read_text()
units=ast.literal_eval(next(n.value for n in ast.parse(source).body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='units' for t in n.targets)))
assert {'dsr_fleet_ht_refresh'}<=units['dsr_fleet'][1]
assert {'dsr_ht_refresh_parse'}<=units['dsr_runtime_api'][1]
assert {'dsr_workspace_ht_refresh'}<=units['dsr_workspace'][1]
assert {'dsr_ht_parse'}<=units['dsr_discovery_model'][1] and units['dsr_discovery_model'][0]==512
with tempfile.TemporaryDirectory(prefix='fleet-target-') as temp:
 folder=Path(temp);nm=folder/'xtensa-esp32s3-elf-nm';nm.write_text('#!'+sys.executable+'\nimport sys\nfrom pathlib import Path\nprint(Path(sys.argv[-1]).read_text())\n');nm.chmod(0o755)
 env=dict(os.environ,PATH=str(folder)+os.pathsep+os.environ['PATH'])
 def fixture():
  for f in folder.glob('dsr_*'):f.unlink()
  (folder/'BOOT_MEMORY.json').write_text(json.dumps({'internal_bss_bytes':70000,'ui_stack':{'bytes':8192},'lvgl_pool':{'bytes':131072}}))
  for name,(cap,public) in units.items():(folder/(name+'.c.su')).write_text(''.join(f'{name}.c:1:1:{fn}\t128\tstatic\n' for fn in public))
  (folder/'dsr_ui_network.c.obj').write_text('00000000 00008080 b fleet\n')
  (folder/'dsr_workspace.c.obj').write_text('00000000 00003072 b runtime\n00003072 00002560 b outputs\n')
  (folder/'dsr_ui_config.c.obj').write_text('00000000 00002944 b migrations\n')
 def check(expected):
  r=subprocess.run([sys.executable,str(ROOT/'tools/check_fleet_target.py'),'--build-dir',str(folder),'--out',str(folder/'FLEET.json')],env=env,capture_output=True,text=True)
  assert (r.returncode==0)==expected,r.stdout+r.stderr
 fixture();check(True)
 for unit,fn in [('dsr_fleet','dsr_fleet_ht_refresh'),('dsr_runtime_api','dsr_ht_refresh_parse'),('dsr_workspace','dsr_workspace_ht_refresh'),('dsr_discovery_model','dsr_ht_parse')]:
  fixture();p=folder/(unit+'.c.su');p.write_text(''.join(line for line in p.read_text().splitlines(True) if ':'+fn+'\t' not in line));check(False)
 for unit in ['dsr_fleet','dsr_runtime_api','dsr_workspace','dsr_discovery_model']:
  fixture();p=folder/(unit+'.c.su');p.write_text(p.read_text()+f'{unit}.c:1:1:extra\t{units[unit][0]+16}\tstatic\n');check(False)
 fixture();(folder/'dsr_discovery_model.c.su').unlink();check(False)
 fixture();(folder/'dsr_ui_network.c.obj').write_text('00000000 00008193 b fleet\n');check(False)
 fixture();p=folder/'BOOT_MEMORY.json';v=json.loads(p.read_text());v['internal_bss_bytes']=81921;p.write_text(json.dumps(v));check(False)
print('PASS: FLEET/H&T target auditor1positive+11rejections; synthetic reports, no target/heap/physical proof')
