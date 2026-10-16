#!/usr/bin/env python3
"""Actual actor/cache/shared activity API; explicit platform fixtures, no radio."""
import argparse,os,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
f=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
if a.sanitize:f+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:f+=['-fsanitize=undefined','-fno-omit-frame-pointer']
with tempfile.TemporaryDirectory(prefix='dsr-actions-runtime-') as d:
 panel=(R/'components/dsr_platform/dsr_panel.c').read_text();start=panel.index('static esp_err_t actions_api(');end=panel.index('static esp_err_t history_api(',start);(Path(d)/'actions_endpoint.inc').write_text(panel[start:end])
 inc=['tests/actions','tests/history','tests/actor','tests/support','components/dsr_core/include','components/dsr_config/include','components/dsr_platform/include','components/dsr_platform','.test-deps'];b=Path(d)/'test'
 src=['components/dsr_platform/dsr_actions.c','components/dsr_platform/dsr_actions_model.c','components/dsr_platform/dsr_actions_parse.c','components/dsr_platform/dsr_actions_api.c','components/dsr_platform/dsr_activity.c','components/dsr_config/dsr_config.c','components/dsr_core/dsr_core.c','tests/test_actions_runtime.c']
 subprocess.run([os.environ.get('CC','cc'),*f,'-I',d,*[v for p in inc for v in ['-I',str(R/p)]],*[str(R/s) for s in src],'-Wl,--wrap=time','-l:libcjson.so.1','-lm','-o',str(b)],check=True);subprocess.run([str(b)],check=True)
