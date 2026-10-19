#!/usr/bin/env python3
"""Read-only comparison against immutable dev-v0.7.58: same fixture, compiler,
LVGL pool and cache warmup. Links the baseline mobile+mapper instead of current
objects. Does not claim ESP32 heap/PSRAM peaks; no radio or device writes.
Run from a checkout with the local dev-v0.7.58 tag and native UI dependencies.
"""
import hashlib,os,runpy,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];os.chdir(root);saved=subprocess.run;link=[]
def run(args,*a,**kw):
 if isinstance(args,list) and '-o' in args and '-c' not in args and str(args[0])=='cc':link[:]=args
 return saved(args,*a,**kw)
subprocess.run=run;os.environ['DSR_TEST_BLU_MEMORY']='1';sys.argv=['tools/test_ui.py','--case','test_cyber_blu_ui','--jobs','1']
try:runpy.run_path('tools/test_ui.py',run_name='__main__')
except SystemExit as e:assert not e.code
subprocess.run=saved
assert link
first=next(i for i,v in enumerate(link) if str(v).endswith('.o'));flags=link[1:first]
old=root/'output/blu-layout/old-objects';old.mkdir(parents=True,exist_ok=True)
for name in ('dsr_ui_mobile.c','dsr_ui_cyber_icons.c'):
 source='components/dsr_ui/'+name
 p=old/name;p.write_bytes(subprocess.check_output(['git','show','dev-v0.7.58:'+source]))
 obj=old/(name+'.o');subprocess.run(['cc',*flags,'-c',str(p),'-o',str(obj)],check=True)
 current=root/'build-ui/native'/(hashlib.sha256(str(root/source).encode()).hexdigest()[:20]+'.o')
 assert str(current) in link
 link[link.index(str(current))]=str(obj)
link[link.index('-o')+1]=str(old/'before');subprocess.run(link,check=True)
print('BEFORE (0.7.58 mobile + mapper; same LVGL/pool/compiler/fixture/caches):',flush=True)
subprocess.run([str(old/'before'),str(root/'build-ui/native/screenshots')],check=True)
