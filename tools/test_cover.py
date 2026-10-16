#!/usr/bin/env python3
"""Focused actual HTTP/NVS/fleet and sole-worker regression build; no hardware."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');args=p.parse_args()
flags=['-std=c11','-g','-Wall','-Wextra','-Werror','-Wpedantic','-DDSR_BOARD_ES3C35P=1']
if args.sanitize: flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif args.ubsan_only: flags+=['-fsanitize=undefined','-fno-sanitize-recover=undefined','-fno-omit-frame-pointer']
common=['components/dsr_core/dsr_core.c','components/dsr_platform/dsr_activity.c','components/dsr_platform/dsr_ap_model.c','components/dsr_platform/dsr_wifi_policy.c']
cases=[('platform',['tests/support','.test-deps','components/dsr_core/include','components/dsr_platform/include','components/dsr_config/include'],common+['components/dsr_platform/dsr_http.c','components/dsr_platform/dsr_ap.c','components/dsr_platform/dsr_discovery_model.c','components/dsr_config/dsr_config.c','components/dsr_platform/dsr_connection.c','components/dsr_platform/dsr_fleet.c','components/dsr_platform/dsr_settings.c','tests/test_platform.c'],['-l:libcjson.so.1','-lm']),('worker',['tests/battery','tests/actor','tests/support','components/dsr_core/include','components/dsr_ota/include','components/dsr_platform/include'],common+['components/dsr_platform/dsr_battery.c','components/dsr_platform/dsr_worker.c','tests/test_battery.c'],[])]
with tempfile.TemporaryDirectory(prefix='dsr-cover-') as tmp:
 for name,includes,sources,libs in cases:
  binary=str(Path(tmp)/name)
  subprocess.run([os.environ.get('CC','cc'),*flags,*[x for d in includes for x in ['-I',str(ROOT/d)]],*[str(ROOT/f) for f in sources],*libs,'-o',binary],check=True)
  subprocess.run([binary],check=True)
