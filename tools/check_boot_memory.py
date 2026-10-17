"""Fail target CI if the real ELF regresses to the 0.7.1 SRAM layout."""
import argparse
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--elf", type=Path, required=True)
parser.add_argument("--sdkconfig", type=Path, required=True)
parser.add_argument("--out", type=Path, required=True)
args = parser.parse_args()
sdk = set(args.sdkconfig.read_text().splitlines())
for option in ["CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY=y",
               "CONFIG_FREERTOS_SUPPORT_STATIC_ALLOCATION=y",
               "CONFIG_LV_MEM_SIZE_KILOBYTES=128", "CONFIG_BT_NIMBLE_ENABLED=y"]:
    assert option in sdk, option
nm = subprocess.check_output(["xtensa-esp32s3-elf-nm", "-S", "--defined-only", str(args.elf)], text=True)
symbols = []
for line in nm.splitlines():
    columns = line.split()
    if len(columns) == 4:
        address, size, kind, name = columns
        symbols.append(dict(name=name,address=int(address,16),bytes=int(size,16),kind=kind))
pool = [s for s in symbols if s["name"].startswith("work_mem_int")]
assert len(pool) == 1 and pool[0]["bytes"] == 128*1024, pool
stack = [s for s in symbols if s["name"] == "dsr_ui_stack"]
tcb = [s for s in symbols if s["name"] == "dsr_ui_tcb"]
assert len(stack) == 1 and stack[0]["bytes"] == 8192, stack
assert len(tcb) == 1, tcb
# Confirm placement against linker-defined section bounds, not guessed ranges.
all_symbols = {line.split()[-1]: int(line.split()[0],16) for line in nm.splitlines() if len(line.split()) in (3,4)}
ext_start, ext_end = all_symbols["_ext_ram_bss_start"], all_symbols["_ext_ram_bss_end"]
bss_start, bss_end = all_symbols["_bss_start"], all_symbols["_bss_end"]
assert ext_start <= pool[0]["address"] and pool[0]["address"]+pool[0]["bytes"] <= ext_end, pool
for symbol in stack+tcb:
    assert bss_start <= symbol["address"] and symbol["address"]+symbol["bytes"] <= bss_end, symbol
record = dict(lvgl_pool=pool[0],ui_stack=stack[0],ui_tcb=tcb[0],
              ext_bss_bytes=ext_end-ext_start,internal_bss_bytes=bss_end-bss_start,
              physical_boot_verified=False)
args.out.write_text(json.dumps(record,indent=2)+"\n")
print("PASS: real ESP32-S3 ELF: 131072-byte LVGL pool in PSRAM; 8192-byte UI stack and TCB in internal BSS")
