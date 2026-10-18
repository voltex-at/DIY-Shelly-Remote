#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile
from hostcc import ROOT,build,cjson_include,parse_args
args=parse_args()
with tempfile.TemporaryDirectory(prefix='dsr-settings-') as temporary:
    binary=Path(temporary)/'settings'
    build(binary,['tests/test_device_settings.c','components/dsr_platform/dsr_device_settings_model.c',
                  'components/dsr_platform/dsr_device_settings.c','components/dsr_platform/dsr_discovery_model.c',
                  'components/dsr_core/dsr_core.c'],
          ['tests/actor','tests/support',str(cjson_include()),'components/dsr_core/include','components/dsr_platform/include'],
          libs=['-l:libcjson.so.1','-lm'],args=args)
    subprocess.run([str(binary)],check=True,cwd=temporary)
