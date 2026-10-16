#!/usr/bin/env python3
"""Actual schema2 upgrade/store vs frozen pre-upgrade firmware schema1 code."""
import argparse,hashlib,os,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
f=['-std=c11','-D_POSIX_C_SOURCE=200809L','-Wall','-Wextra','-Werror','-Wpedantic','-g']
if Path('/usr/include/cjson/cJSON.h').is_file():f+=['-I','/usr/include/cjson']
if a.sanitize:f+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:f+=['-fsanitize=undefined','-fno-omit-frame-pointer']
old=R/'tests/actions/schema1';inc=R/'.test-deps';core=R/'components/dsr_core/include';config=R/'components/dsr_config/include'
# These are verbatim production files at80366f3, not a rewritten old parser.
expected={'config.c': 'bfeefd660f657ac4bffa15704b03312b63cbde686cae203d0f6e025fb13e363b', 'store.c': '6f175a1879addcfe31c63e648ad8643dea4a553e3d9afd2087fc80be34fb542e', 'dsr_config.h': '983ffbd0c028658c700ba30ca2ab749d29aee657d8c53265de531d47966bc7d8'}
for name,digest in expected.items():
 assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest, "Frozen production schema1 fixture changed: "+name
renames=['default','validate','json_depth','revision','replace','crc32','store_load','store_save','store_collect_photos']
with tempfile.TemporaryDirectory(prefix='dsr-schema2-') as d:
 d=Path(d);cc=os.environ.get('CC','cc');objs=[]
 for name in ['config.c','store.c']:
  obj=d/(name+'.o');subprocess.run([cc,*f,*['-Ddsr_config_'+n+'=old_'+n for n in renames],'-I',str(old),'-I',str(core),'-I',str(inc),'-c',str(old/name),'-o',str(obj)],check=True);objs.append(str(obj))
 b=d/'test';subprocess.run([cc,*f,'-I',str(config),'-I',str(core),'-I',str(inc),*[str(R/s) for s in ['components/dsr_config/dsr_config.c','components/dsr_config/dsr_config_store.c','components/dsr_config/dsr_photo_name.c','components/dsr_core/dsr_core.c','tests/test_actions_schema.c']],*objs,'-Wl,--wrap=fwrite','-l:libcjson.so.1','-lm','-o',str(b)],check=True)
 subprocess.run([str(b),str(d)],check=True)
