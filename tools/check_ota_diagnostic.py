"""Diagnostic images must not carry a supported-board upload marker."""
import argparse
from pathlib import Path
import re

p=argparse.ArgumentParser()
p.add_argument('--bin',type=Path,required=True)
args=p.parse_args()
header=(Path(__file__).resolve().parents[1]/'components/dsr_ota/include/dsr_ota_image.h').read_text()
matches=re.findall(r'^#define DSR_OTA_IMAGE_BOARD_MARKER "([^"\\]+)"$',header,re.M)
assert len(matches)==1,'Unrecognized marker definition: review diagnostic policy'
marker=matches[0].encode('ascii')+b'\0'
assert marker not in args.bin.read_bytes(),'Supported-board upload marker leaked into diagnostic BIN'
print('PASS: actual diagnostic app BIN has no supported-board upload marker')
