#!/usr/bin/env python3
"""Actual history C model/journal/query/clock with model SDK and real files."""
import argparse,os,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
f=['-std=c11','-g','-Wall','-Wextra','-Werror','-Wpedantic','-DDSR_BOARD_ES3C35P=1']
if a.sanitize:f+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
elif a.ubsan_only:f+=['-fsanitize=undefined','-fno-omit-frame-pointer']
inc=['tests/history','tests/support','components/dsr_core/include','components/dsr_config/include','components/dsr_platform/include','components/dsr_platform','.test-deps']
json_include=Path('/usr/include/cjson')
if not (json_include/'cJSON.h').exists():json_include=R/'.test-deps'
if not (json_include/'cJSON.h').exists():raise SystemExit('Install libcjson-dev to run history fixtures')
src=['dsr_history_model.c','dsr_history_store.c','dsr_history.c','dsr_history_api.c','dsr_clock.c','dsr_activity.c']
with tempfile.TemporaryDirectory(prefix='dsr-history-') as d:
 panel=(R/'components/dsr_platform/dsr_panel.c').read_text();start=panel.index('static esp_err_t history_api(httpd_req_t *r)');end=panel.index('/* HTTP only queues',start);(Path(d)/'history_endpoint.inc').write_text(panel[start:end])
 b=Path(d)/'test_history';env=dict(os.environ,TMPDIR=d)
 subprocess.run([os.environ.get('CC','cc'),*f,'-I',d,'-I',str(json_include),*[x for n in inc for x in ['-I',str(R/n)]],*[str(R/'components/dsr_platform'/n) for n in src],str(R/'components/dsr_config/dsr_config.c'),str(R/'components/dsr_core/dsr_core.c'),str(R/'tests/test_history.c'),'-Wl,--wrap=fopen','-Wl,--wrap=fwrite','-Wl,--wrap=fflush','-Wl,--wrap=fsync','-Wl,--wrap=time','-l:libcjson.so.1','-lm','-o',str(b)],check=True,env=env)
 for mode in range(6):subprocess.run([str(b),d,str(mode)],check=True,env=env)
