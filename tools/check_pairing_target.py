"""Audit the two pairing APIs in the real ESP32-S3 panel object and ELF.

The surrounding panel object intentionally also implements HTTP/OTA. Dependency
checks below inspect ONLY relocation sections belonging to the new APIs. Frames
are their own static frames, not NVS/FreeRTOS/transitive peaks or durability proof.
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
objects = sorted(args.build_dir.rglob("dsr_panel.c.obj"))
stacks = sorted(args.build_dir.rglob("*dsr_panel*.su"))
require(len(objects) == 1, f"Expected one real panel object: {objects}")
require(len(stacks) == 1, f"Expected one real panel stack report: {stacks}")
apis = {"dsr_panel_pairing_snapshot", "dsr_panel_pairing_revoke"}
defined = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "--defined-only", str(args.elf)], text=True)
symbols = {line.split()[-1] for line in defined.splitlines() if line.split()}
require(apis <= symbols, f"Missing retained pairing APIs: {sorted(apis - symbols)}")
frames = {}
for line in stacks[0].read_text().splitlines():
    columns = line.split("\t")
    name = columns[0].rsplit(":", 1)[-1]
    if name not in apis:
        continue
    require(len(columns) == 3 and columns[1].isdigit() and columns[2] == "static",
            f"Unrecognized/non-static pairing frame: {line}")
    require(name not in frames, f"Duplicate pairing frame: {name}")
    frames[name] = int(columns[1])
    require(frames[name] <= args.max_frame, f"Pairing own frame exceeds budget: {line}")
require(apis == set(frames), "Missing pairing public stack reports")

# Xtensa calls use function-local literal pools: both .text and .literal
# relocations must be inspected. Whole-object undefined symbols would conflate
# these APIs with pre-existing panel HTTP/OTA/TLS paths.
relocations = subprocess.check_output(
    ["xtensa-esp32s3-elf-objdump", "-r", str(objects[0])], text=True)
sections = {}
current = None
for line in relocations.splitlines():
    match = re.fullmatch(r"RELOCATION RECORDS FOR \[(.+)\]:", line)
    if match:
        current = match.group(1)
        require(current not in sections, f"Duplicate relocation section: {current}")
        sections[current] = set()
        continue
    fields = line.split()
    if current and len(fields) == 3 and re.fullmatch(r"[0-9a-fA-F]+", fields[0]):
        require(fields[1].startswith("R_XTENSA_"), f"Unexpected target relocation: {line}")
        sections[current].add(re.split(r"[+-]0x[0-9a-fA-F]+$", fields[2])[0])

common = {"dsr_boot_ready", "xQueueSemaphoreTake", "xQueueGenericSend",
          "memset", "__stack_chk_guard", "__stack_chk_fail", "_GLOBAL_OFFSET_TABLE_"}
state = {"lock", "panel_ready", "has_token", "revocation_unconfirmed",
         "token_hash", "pairing", "pair_until", "ota_until", "attempts"}
nvs = {"nvs_open", "nvs_erase_key", "nvs_commit", "nvs_get_blob", "nvs_close"}
required = {
    "dsr_panel_pairing_snapshot": {"dsr_boot_ready", "xQueueSemaphoreTake", "xQueueGenericSend"},
    "dsr_panel_pairing_revoke": {"dsr_boot_ready", "xQueueSemaphoreTake", "xQueueGenericSend",
                               "dsr_activity_begin", "dsr_activity_end"} | nvs,
}
records = {}
for api in sorted(apis):
    names = [".text." + api, ".literal." + api]
    require(all(name in sections and sections[name] for name in names),
            f"Missing actual text/literal relocation evidence for {api}")
    deps = set().union(*(sections[name] for name in names))
    allowed = common | state | {".bss." + item for item in state} | set(names)
    if api.endswith("revoke"):
        allowed |= nvs | {"dsr_activity_begin", "dsr_activity_end"}
    # Literal strings are compiler-local data, not additional callable SDK APIs.
    unexpected = {dep for dep in deps if dep not in allowed and
                  not dep.startswith((".rodata.", ".LC"))}
    require(not unexpected, f"Unexpected {api} direct dependencies: {sorted(unexpected)}")
    require(required[api] <= deps,
            f"Required {api} dependencies missing: {sorted(required[api] - deps)}")
    records[api] = {"own_static_frame_bytes": frames[api],
                    "relocation_sections": names, "direct_relocations": sorted(deps)}
args.out.write_text(json.dumps({
    "object": str(objects[0]), "stack_report": str(stacks[0]),
    "retained_public_symbols": sorted(apis), "functions": records,
    "own_frame_budget_bytes": args.max_frame,
    "dependency_scope": "only new API text and literal relocation sections, not whole panel object",
    "snapshot_direct_nvs_dependencies_absent": True,
    "revoke_direct_storage_dependencies": sorted(nvs),
    "new_task_or_heap_allocation_direct_dependencies_absent": True,
    "direct_network_flash_erase_namespace_wipe_restart_dependencies_absent": True,
    "new_static_state_source_bytes": "one bool; no target sizeof/placement claim",
    "transitive_sdk_stack_heap_peak_or_physical_durability_measured": False,
}, indent=2) + "\n")
print(f"PASS: real retained pairing APIs, own frames<={args.max_frame} B, function-local target relocation audit; no transitive/physical durability claim")
