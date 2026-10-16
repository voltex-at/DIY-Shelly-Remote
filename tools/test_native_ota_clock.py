#!/usr/bin/env python3
import argparse,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dsr-native-clock-') as folder:
 flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 elif a.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
 includes=['tests/native_ota','tests/support','components/dsr_ota/include','components/dsr_platform/include']
 binary=Path(folder)/'test'
 subprocess.run([os.environ.get('CC','cc'),*flags,*[v for f in includes for v in ['-I',str(ROOT/f)]],str(ROOT/'components/dsr_platform/dsr_native_ota_clock.c'),str(ROOT/'components/dsr_platform/dsr_clock.c'),str(ROOT/'tests/test_native_ota_clock.c'),'-Wl,--wrap=time','-o',str(binary)],check=True)
 for mode in range(6):subprocess.run([str(binary),str(mode)],check=True)
