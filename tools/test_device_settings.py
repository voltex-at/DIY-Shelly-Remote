#!/usr/bin/env python3
from pathlib import Path
import argparse
import subprocess
import tempfile
from hostcc import ROOT,build,cjson_include,parse_args
p=argparse.ArgumentParser();p.add_argument('--case',choices=['device','privacy','all'],default='all');args=parse_args(p)
with tempfile.TemporaryDirectory(prefix='dsr-settings-') as temporary:
    binary=Path(temporary)/'settings'
    if args.case in ('device','all'):
      build(binary,['tests/test_device_settings.c','components/dsr_platform/dsr_device_settings_model.c',
                  'components/dsr_platform/dsr_device_settings.c','components/dsr_platform/dsr_discovery_model.c',
                  'components/dsr_core/dsr_core.c'],
          ['tests/actor','tests/support',str(cjson_include()),'components/dsr_core/include','components/dsr_platform/include'],
          libs=['-l:libcjson.so.1','-lm'],args=args)
      subprocess.run([str(binary)],check=True,cwd=temporary)

with tempfile.TemporaryDirectory(prefix='dsr-preview-privacy-') as temporary:
    folder=Path(temporary)
    source=(ROOT/'components/dsr_platform/dsr_panel.c').read_text()
    start=source.index('bool dsr_panel_preview_requested(void)')
    end=source.index('static esp_err_t json(',start)
    (folder/'preview.inc').write_text(source[start:end])
    start=source.index(' if(!strcmp(r->uri,"/api/v1/preview.raw")')
    end=source.index(' if(!strncmp(r->uri,"/api/v1/photos/"',start)
    (folder/'preview_api.inc').write_text('static int preview_get(httpd_req_t *r) {\n'+source[start:end]+'return -1;\n}\n')
    binary=folder/'preview'
    if args.case in ('privacy','all'):
      build(binary,['tests/test_preview_privacy.c'],[str(folder),'tests/actor','tests/support'],args=args)
      subprocess.run([str(binary)],check=True,cwd=folder)
