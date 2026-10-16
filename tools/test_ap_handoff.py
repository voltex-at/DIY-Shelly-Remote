#!/usr/bin/env python3
"""Shipped AP opening handler; no radio, HTTP-authentication or Flash proof."""
import argparse,os,re,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
panel=(ROOT/'components/dsr_platform/dsr_panel.c').read_text()
start=panel.index('static esp_err_t ap_handoff_api(httpd_req_t *r)')
end=re.search(r'^static\b',panel[start+1:],re.M);assert end
with tempfile.TemporaryDirectory(prefix='dsr-ap-handler-') as folder:
 folder=Path(folder);(folder/'ap_handoff_endpoint.inc').write_text(panel[start:start+1+end.start()])
 flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 elif a.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
 json_include=Path('/usr/include/cjson')
 if not (json_include/'cJSON.h').exists():json_include=ROOT/'.test-deps'
 includes=[folder,json_include]+[ROOT/d for d in ['tests/support','components/dsr_platform/include','components/dsr_core/include','components/dsr_config/include']]
 subprocess.run([os.environ.get('CC','cc'),*flags,*[x for d in includes for x in ['-I',str(d)]],str(ROOT/'tests/ap/handlers.c'),'-o',str(folder/'handler')],check=True)
 subprocess.run([str(folder/'handler')],check=True)
