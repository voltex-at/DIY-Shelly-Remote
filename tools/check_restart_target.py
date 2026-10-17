"""Audit target restart/activity objects, configured memory and own frames.

Stack/context constants are compiled source assertions, not runtime peaks.
FreeRTOS TCB/allocator overhead, shutdown-handler frames and transitive SDK
stack are additional. No physical restart/data-retention claim is made.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess


def require(condition, message):
    if not condition:
        raise SystemExit(message)


parser = argparse.ArgumentParser()
parser.add_argument("--build-dir", type=Path, required=True)
parser.add_argument("--elf", type=Path, required=True)
parser.add_argument("--out", type=Path, required=True)
parser.add_argument("--max-frame", type=int, default=256)
args = parser.parse_args()
require(args.max_frame > 0, "Invalid own-frame budget")
required = {
    "dsr_restart": {
        "dsr_boot_ready", "dsr_activity_restart_reserve", "dsr_activity_restart_cancel",
        "dsr_activity_restart_dispatch", "esp_restart", "xTaskCreatePinnedToCore",
        "xTaskGenericNotify", "ulTaskGenericNotifyTake", "vTaskDelay",
    },
    "dsr_activity": set(),
}
public = {
    "dsr_restart": {"dsr_restart_start", "dsr_restart_request"},
    "dsr_activity": {"dsr_activity_begin", "dsr_activity_end",
                     "dsr_activity_restart_reserve", "dsr_activity_restart_cancel", "dsr_activity_restart_dispatch"},
}
defined_text = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "--defined-only", str(args.elf)], text=True
)
defined = {line.split()[-1] for line in defined_text.splitlines() if line.split()}
records = {}
for unit in ("dsr_restart", "dsr_activity"):
    objects = sorted(args.build_dir.rglob(unit + ".c.obj"))
    stacks = sorted(args.build_dir.rglob("*" + unit + "*.su"))
    require(len(objects) == 1, f"Expected one real {unit} object: {objects}")
    require(len(stacks) == 1, f"Expected one real {unit} stack report: {stacks}")
    nm = subprocess.check_output(["xtensa-esp32s3-elf-nm", "-u", str(objects[0])], text=True)
    undefined = sorted({line.split()[-1] for line in nm.splitlines() if line.split()})
    allowed = required[unit] | {"__stack_chk_guard", "__stack_chk_fail", "_GLOBAL_OFFSET_TABLE_"}
    # IDF's ESP32-S3+PSRAM workaround can route 32-bit atomics through its
    # wrappers. No generic, 64-bit or arbitrary SDK fallback is admitted.
    allowed |= {"__atomic_compare_exchange_4", "__atomic_load_4", "__atomic_store_4"}
    if unit == "dsr_restart":
        allowed |= {"vTaskDelete"}  # SDK noreturn can remove the host-only fallback.
    require(not set(undefined) - allowed,
            f"Unexpected {unit} dependencies (no storage/erase/network/new shutdown hook/timer callback): {sorted(set(undefined) - allowed)}")
    require(required[unit] <= set(undefined),
            f"Required {unit} target dependencies missing: {sorted(required[unit] - set(undefined))}")
    frames = {}
    for line in stacks[0].read_text().splitlines():
        columns = line.split("\t")
        require(len(columns) == 3, f"Unrecognized {unit} stack report: {line}")
        name = columns[0].rsplit(":", 1)[-1]
        require(columns[1].isdigit(), f"Non-numeric {unit} frame: {line}")
        require(columns[2] == "static", f"Non-static {unit} frame: {line}")
        size = int(columns[1])
        require(size <= args.max_frame, f"{unit} own frame exceeds budget: {line}")
        require(name not in frames, f"Duplicate {unit} frame: {name}")
        frames[name] = size
    require(public[unit] <= set(frames), f"Missing {unit} public stack reports: {sorted(public[unit] - set(frames))}")
    require(public[unit] <= defined, f"Missing retained {unit} target symbols: {sorted(public[unit] - defined)}")
    if unit == "dsr_restart":
        require(any(name == "dispatch_task" or name.startswith("dispatch_task.") for name in frames),
                "Deferred dispatcher stack report missing")
    records[unit] = {"object": str(objects[0]), "stack_report": str(stacks[0]),
                     "own_stack_frames": frames, "undefined_symbols": undefined}
root = Path(__file__).resolve().parents[1]
source = (root / "components/dsr_platform/dsr_restart.c").read_text()
require(re.search(r"_Static_assert\s*\(\s*sizeof\s*\(\s*restart_dispatch_t\s*\)\s*<=\s*16\s*,", source),
        "Compiled restart static-context bound missing")
require(re.search(r"#define\s+RESTART_STACK_BYTES\s+4096u?\b", source) and
        re.search(r"_Static_assert\s*\(\s*RESTART_STACK_BYTES\s*==\s*4096\s*,", source) and
        re.search(r'xTaskCreate\s*\(\s*dispatch_task\s*,\s*"dsr_restart"\s*,\s*RESTART_STACK_BYTES\s*,\s*NULL\s*,\s*1\s*,', source),
        "Compiled task stack/priority configuration missing")
activity = (root / "components/dsr_platform/dsr_activity.c").read_text()
require(re.search(r"_Static_assert\s*\(\s*sizeof\s*\(\s*_Atomic\s+uint32_t\s*\)\s*==\s*4\s*,", activity),
        "Compiled single 32-bit atomic admission word assertion missing")
# Exact target symbols, not host sizeof or a similarly named state in another
# translation unit. Address bounds are SOC_DRAM_LOW/HIGH from pinned IDF5.5.4
# components/soc/esp32s3/include/soc/soc.h (0x3fc88000..0x3fd00000).
symbol_table = subprocess.check_output(
    ["xtensa-esp32s3-elf-objdump", "-t", str(args.elf)], text=True
)
state_records = {}
for symbol, maximum, exact in [("activity_state", 4, True), ("restart_dispatcher", 16, False)]:
    lines = [line.split() for line in symbol_table.splitlines() if line.split() and line.split()[-1] == symbol]
    require(len(lines) == 1, f"Expected unique retained target state symbol: {symbol}")
    fields = lines[0]
    require(len(fields) >= 5 and fields[-3] == ".dram0.bss", f"State is not internal DRAM BSS: {symbol}")
    address, size = int(fields[0], 16), int(fields[-2], 16)
    require((size == maximum if exact else 0 < size <= maximum), f"Target state size outside bound: {symbol}={size}")
    require(0x3FC88000 <= address and address + size <= 0x3FD00000 and address % 4 == 0,
            f"State outside aligned ESP32-S3 internal DRAM: {symbol}")
    state_records[symbol] = {"address": hex(address), "size_bytes": size, "section": ".dram0.bss"}
args.out.write_text(json.dumps({
    "objects": records,
    "own_frame_budget_bytes": args.max_frame,
    "dispatcher_static_context_upper_bound_bytes": 16,
    "dispatcher_static_evidence": "target-compiled sizeof(restart_dispatch_t)<=16 assertion",
    "task_stack_configured_bytes": 4096,
    "task_priority_configured": 1,
    "task_stack_evidence": "target-compiled RESTART_STACK_BYTES==4096 passed to xTaskCreate",
    "single_32bit_atomic_activity_assertion_compiled": True,
    "idf_32bit_atomic_wrappers_permitted": True,
    "native_lock_free_or_wait_free_claim": False,
    "actual_target_internal_bss_state": state_records,
    "dependency_scope": "direct undefined symbols in individual restart/activity objects",
    "storage_erase_network_and_timer_dispatch_direct_dependencies_absent": True,
    "tcb_allocator_shutdown_and_transitive_stack_peak_measured": False,
    "physical_restart_or_data_retention_verified": False,
}, indent=2) + "\n")
print(f"PASS: real target restart/activity frames<={args.max_frame} B; actual aligned internalBSS state; compiled dispatcher<=16 B/task stack4096 B priority1; 32-bit atomic gate with IDF wrappers permitted; no lock-free/peak claim")
