#!/usr/bin/env python3
"""Exercise real BLU GATT/bond lifecycle; NimBLE and RF are explicit fixtures."""
from pathlib import Path
import subprocess
import tempfile
from hostcc import ROOT, build, parse_args
args = parse_args()
with tempfile.TemporaryDirectory(prefix='dsr-blu-') as temporary:
    binary = Path(temporary) / 'blu'
    build(binary, ['tests/test_blu.c','components/dsr_platform/dsr_blu.c',
                   'components/dsr_platform/dsr_blu_model.c','components/dsr_platform/dsr_activity.c'],
          ['tests/blu','tests/ble_rpc','tests/actor','tests/support','components/dsr_core/include','components/dsr_platform/include'],
          libs=['-lm'],args=args)
    subprocess.run([str(binary)],check=True,cwd=temporary)
