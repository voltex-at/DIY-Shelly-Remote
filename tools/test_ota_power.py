"""Focused private OTA worker handoff; no device or Flash."""
import os,subprocess,tempfile
from pathlib import Path
from hostcc import ROOT,build,parse_args
args=parse_args()
with tempfile.TemporaryDirectory(prefix='dsr-power-') as tmp:
    binary=Path(tmp)/'test-power'
    build(binary,['components/dsr_platform/dsr_ota_power.c','components/dsr_platform/dsr_activity.c','tests/test_ota_power.c'],
          ['tests/actor','tests/support','components/dsr_platform/include','components/dsr_platform','components/dsr_core/include'],
          ['-pthread'],[],args)
    result=subprocess.run([str(binary)],check=True,capture_output=True,text=True)
    assert 'OTA power handoff checks;' in result.stdout,result.stderr
    print(result.stdout.strip())
