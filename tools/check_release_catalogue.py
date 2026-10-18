#!/usr/bin/env python3
"""Read-only host validation of a real API response with the firmware C parser."""
import argparse
import subprocess
import tempfile
from pathlib import Path
from hostcc import ROOT, build, cjson_include

p = argparse.ArgumentParser()
p.add_argument('--input', type=Path, required=True)
p.add_argument('--channel', choices=('dev', 'stable'), required=True)
p.add_argument('--tag', required=True)
a = p.parse_args()
with tempfile.TemporaryDirectory(prefix='dsr-catalogue-') as tmp:
    binary = Path(tmp) / 'check'
    build(binary, ['components/dsr_ota/dsr_ota_release.c', 'tools/check_release_catalogue.c'],
          ['components/dsr_ota/include', str(cjson_include())], libs=['-l:libcjson.so.1'])
    subprocess.run([str(binary), str(a.input.resolve()), a.channel, a.tag], check=True)
