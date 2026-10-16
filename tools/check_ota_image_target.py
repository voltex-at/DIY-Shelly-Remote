"""Audit the real ESP32-S3 stream helper object, stack frames and heap context.

This covers helper-owned frames only, not the transitive mbedTLS call stack,
FreeRTOS scheduling, runtime heap fragmentation or physical operation.
"""
import argparse
import json
import re
from pathlib import Path
import subprocess


def require(condition, message):
    if not condition:
        raise SystemExit(message)


parser = argparse.ArgumentParser()
parser.add_argument("--build-dir", type=Path, required=True)
parser.add_argument("--elf", type=Path, required=True)
parser.add_argument("--out", type=Path, required=True)
parser.add_argument("--max-context", type=int, default=1024)
parser.add_argument("--max-frame", type=int, default=256)
parser.add_argument("--upload", action="store_true", help="Also audit the private legacy HTTP receiver")
args = parser.parse_args()
require(args.max_context > 0 and args.max_frame > 0, "Invalid memory budget")
objects = sorted(args.build_dir.rglob("dsr_ota_image.c.obj"))
stacks = sorted(args.build_dir.rglob("*dsr_ota_image*.su"))
require(len(objects) == 1, f"Expected one real stream helper object: {objects}")
require(len(stacks) == 1, f"Expected one real stream helper .su report: {stacks}")

# Audit the individual object, rather than the entire firmware, which rightly
# contains existing network, storage and development OTA implementations.
nm = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "-u", str(objects[0])], text=True
)
undefined = sorted({line.split()[-1] for line in nm.splitlines() if line.split()})
allowed = {
    "calloc", "free", "memcpy", "memset", "memcmp",
    "mbedtls_sha256_init", "mbedtls_sha256_free", "mbedtls_sha256_starts",
    "mbedtls_sha256_update", "mbedtls_sha256_finish",
    "__stack_chk_fail", "__stack_chk_guard", "_GLOBAL_OFFSET_TABLE_",
}
unexpected = set(undefined) - allowed
require(not unexpected, f"Unexpected stream helper dependencies: {sorted(unexpected)}")
for name in ["calloc", "free", "mbedtls_sha256_starts", "mbedtls_sha256_update", "mbedtls_sha256_finish"]:
    require(name in undefined, f"Expected real helper dependency is missing: {name}")

frames = {}
for line in stacks[0].read_text().splitlines():
    columns = line.split("\t")
    require(len(columns) == 3, f"Unrecognized stack usage line: {line}")
    function = columns[0].rsplit(":", 1)[-1]
    require(columns[1].isdigit(), f"Non-numeric stack usage: {line}")
    size = int(columns[1])
    require(columns[2] == "static", f"Non-static helper stack frame: {line}")
    require(size <= args.max_frame, f"Helper frame exceeds budget: {line}")
    require(function not in frames, f"Duplicate stack report for {function}")
    frames[function] = size
required = {
    "dsr_ota_image_create", "dsr_ota_image_reset", "dsr_ota_image_update",
    "dsr_ota_image_finish", "dsr_ota_image_destroy", "dsr_ota_image_header_ready",
    "dsr_ota_image_received", "dsr_ota_image_context_size",
}
require(required <= set(frames), f"Missing helper stack reports: {sorted(required - set(frames))}")

# The retained function returns sizeof(the opaque context). Xtensa returns an
# integer in a2; this small constant must be directly materialized by MOVI.
# Fail on an unfamiliar code shape instead of guessing host sizeof or ABI.
disassembly = subprocess.check_output(
    ["xtensa-esp32s3-elf-objdump", "-d", "--disassemble=dsr_ota_image_context_size", str(args.elf)],
    text=True,
)
require("<dsr_ota_image_context_size>:" in disassembly, "Target context-size symbol missing")
values = re.findall(r"\bmovi(?:\.n)?\s+a2,\s*(0x[0-9a-fA-F]+|[0-9]+)\b", disassembly)
require(len(values) == 1, "Cannot establish target context size from MOVI a2")
context_bytes = int(values[0], 16 if values[0].lower().startswith("0x") else 10)
require(512 < context_bytes <= args.max_context, f"Target context outside budget: {context_bytes}")

record = {
    "object": str(objects[0]),
    "stack_report": str(stacks[0]),
    "target_context_bytes": context_bytes,
    "context_budget_bytes": args.max_context,
    "helper_stack_frames": frames,
    "helper_frame_budget_bytes": args.max_frame,
    "undefined_symbols": undefined,
    "dependency_scope": "direct undefined symbols in the helper object",
    "network_flash_partition_restart_dependencies_absent": True,
    "transitive_mbedtls_stack_peak_measured": False,
    "physical_operation_verified": False,
}
if args.upload:
    upload_objects = sorted(args.build_dir.rglob("dsr_ota_upload.c.obj"))
    upload_stacks = sorted(args.build_dir.rglob("*dsr_ota_upload*.su"))
    require(len(upload_objects) == 1, f"Expected one real upload receiver object: {upload_objects}")
    require(len(upload_stacks) == 1, f"Expected one real upload receiver .su report: {upload_stacks}")
    upload_nm = subprocess.check_output(
        ["xtensa-esp32s3-elf-nm", "-u", str(upload_objects[0])], text=True
    )
    upload_undefined = sorted({line.split()[-1] for line in upload_nm.splitlines() if line.split()})
    upload_required = {
        "malloc", "free", "httpd_req_recv", "esp_ota_begin", "esp_ota_write",
        "esp_ota_end", "esp_ota_abort", "dsr_ota_image_create_dev_checksum",
        "dsr_ota_image_update", "dsr_ota_image_header_ready",
        "dsr_ota_image_finish", "dsr_ota_image_destroy",
    }
    upload_allowed = upload_required | {
        "memcpy", "memset", "__stack_chk_fail", "__stack_chk_guard", "_GLOBAL_OFFSET_TABLE_",
    }
    unexpected = set(upload_undefined) - upload_allowed
    require(not unexpected, f"Unexpected receiver dependency (no boot/partition selection): {sorted(unexpected)}")
    require(upload_required <= set(upload_undefined), f"Receiver dependency missing: {sorted(upload_required - set(upload_undefined))}")
    upload_frames = {}
    for line in upload_stacks[0].read_text().splitlines():
        columns = line.split("\t")
        require(len(columns) == 3, f"Unrecognized receiver stack usage line: {line}")
        function = columns[0].rsplit(":", 1)[-1]
        require(columns[1].isdigit(), f"Non-numeric receiver stack usage: {line}")
        size = int(columns[1])
        require(columns[2] == "static", f"Non-static receiver stack frame: {line}")
        require(size <= args.max_frame, f"Receiver frame exceeds budget: {line}")
        require(function not in upload_frames, f"Duplicate receiver stack report for {function}")
        upload_frames[function] = size
    require({"dsr_ota_upload_receive", "dsr_ota_upload_sha256"} <= set(upload_frames), "Receiver stack reports missing")
    require("dsr_ota_image_create_dev_checksum" in frames, "Explicit development image factory stack report missing")
    target_nm = subprocess.check_output(
        ["xtensa-esp32s3-elf-nm", "--defined-only", str(args.elf)], text=True
    )
    target_symbols = {line.split()[-1] for line in target_nm.splitlines() if line.split()}
    require({"dsr_ota_upload_receive", "dsr_ota_upload_sha256", "dsr_ota_image_create_dev_checksum"} <= target_symbols,
            "Receiver/development factory not retained in real target ELF")
    source = (Path(__file__).resolve().parents[1] / "components/dsr_platform/dsr_ota_upload.c").read_text()
    require(re.search(r"_Static_assert\s*\(\s*sizeof\s*\(\s*struct\s+upload_buffers\s*\)\s*==\s*4608\s*,", source),
            "Exact target-compiled 4608-byte receiver workspace assertion missing")
    record["upload"] = {
        "object": str(upload_objects[0]),
        "stack_report": str(upload_stacks[0]),
        "receiver_stack_frames": upload_frames,
        "receiver_frame_budget_bytes": args.max_frame,
        "heap_workspace_bytes": 4608,
        "heap_workspace_evidence": "target-compiled _Static_assert(sizeof(struct upload_buffers)==4608)",
        "heap_workspace_measured_runtime_or_disassembly": False,
        "undefined_symbols": upload_undefined,
        "dependency_scope": "direct undefined symbols in the receiver object",
        "boot_partition_selection_and_restart_dependencies_absent": True,
        "client_checksum_is_publisher_authentication": False,
        "transitive_sdk_stack_peak_measured": False,
        "physical_upload_verified": False,
    }
args.out.write_text(json.dumps(record, indent=2) + "\n")
print(f"PASS: target stream context {context_bytes} B; helper frames <= {args.max_frame} B; only heap/memory/SHA dependencies; no hardware claim")
if args.upload:
    print(f"PASS: target receiver frames <= {args.max_frame} B; compiled 4608-byte heap workspace assertion; no boot selection/restart dependency; legacy checksum only")
