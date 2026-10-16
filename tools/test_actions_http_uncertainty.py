#!/usr/bin/env python3
"""Genuine typed HTTP + actor model; no network/radio/device operations."""
import argparse, os, subprocess, tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g','-DDSR_BOARD_ES3C35P=1']
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
inc=['tests/support','.test-deps','components/dsr_core/include','components/dsr_platform/include','components/dsr_config/include']
src=['components/dsr_core/dsr_core.c','components/dsr_platform/dsr_http.c','components/dsr_platform/dsr_ap.c','components/dsr_platform/dsr_ap_model.c','components/dsr_platform/dsr_discovery_model.c','components/dsr_config/dsr_config.c','components/dsr_platform/dsr_connection.c','components/dsr_platform/dsr_fleet.c','components/dsr_platform/dsr_settings.c','components/dsr_platform/dsr_wifi_policy.c','components/dsr_platform/dsr_activity.c','components/dsr_platform/dsr_actions_model.c','tests/test_actions_http_uncertainty.c']
with tempfile.TemporaryDirectory(prefix='dsr-actions-http-uncertainty-') as d:
    b=Path(d)/'test'
    subprocess.run([os.environ.get('CC','cc'),*flags,*[v for path in inc for v in ['-I',str(R/path)]],*[str(R/s) for s in src],'-l:libcjson.so.1','-lm','-o',str(b)],check=True)
    subprocess.run([str(b)],check=True,cwd=d)
