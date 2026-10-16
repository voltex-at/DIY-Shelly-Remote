#!/usr/bin/env python3
"""Focused genuine HTTP/parser/fleet counterexamples; SDK and Flash MODEL."""
import argparse,hashlib,json,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--source-root',type=Path,default=ROOT);p.add_argument('--fleet-source',type=Path);a=p.parse_args();source=a.source_root.resolve()
parts=['components/dsr_core/dsr_core.c','components/dsr_platform/dsr_http.c','components/dsr_platform/dsr_ap.c','components/dsr_platform/dsr_ap_model.c','components/dsr_platform/dsr_discovery_model.c','components/dsr_config/dsr_config.c','components/dsr_platform/dsr_connection.c','components/dsr_platform/dsr_fleet.c','components/dsr_platform/dsr_settings.c','components/dsr_platform/dsr_wifi_policy.c','components/dsr_platform/dsr_activity.c']
files=[source/s for s in parts]
if a.fleet_source:files[parts.index('components/dsr_platform/dsr_fleet.c')]=a.fleet_source.resolve()
tracked=files+[source/'components/dsr_core/include/dsr_core.h',source/'components/dsr_platform/include/dsr_platform.h',source/'tests/test_platform.c']
before={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in tracked}
build=ROOT/'build-review';build.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='ht-core-',dir=build) as name:
 binary=Path(name)/'test'
 flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g','-DDSR_BOARD_ES3C35P=1','-fsanitize=undefined','-fno-omit-frame-pointer']
 json_include=Path('/usr/include/cjson');json_include=json_include if (json_include/'cJSON.h').exists() else source/'.test-deps'
 inc=['tests/support',str(json_include),'tests','components/dsr_core/include','components/dsr_platform/include','components/dsr_config/include']
 subprocess.run([os.environ.get('CC','cc'),*flags,*[v for d in inc for v in ['-I',str(source/d)]],*[str(f) for f in files],str(ROOT/'tests/test_ht_core_review.c'),'-l:libcjson.so.1','-lm','-o',str(binary)],check=True)
 result=subprocess.run([str(binary)],check=False)
after={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in tracked}
if before!=after:raise SystemExit('Source changed during compilation/execution; repeat against a stable snapshot')
print(json.dumps({'source_sha256':before,'result':result.returncode},indent=2))
raise SystemExit(result.returncode)
