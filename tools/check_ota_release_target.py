"""Audit the real target release-candidate parser object and its own frames.

Per-function static frames do not measure recursive/transitive stack peak.
The source's input/node/depth limits are not measurements of cJSON heap peak.
This parser provides candidate routing only, never cryptographic approval.
"""
import argparse
import json
from pathlib import Path
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
require(args.max_frame > 0, "Invalid frame budget")
objects = sorted(args.build_dir.rglob("dsr_ota_release.c.obj"))
stacks = sorted(args.build_dir.rglob("dsr_ota_release.c.su"))
require(len(objects) == 1, f"Expected one real release parser object: {objects}")
require(len(stacks) == 1, f"Expected one real release parser stack report: {stacks}")

undefined_text = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "-u", str(objects[0])], text=True
)
undefined = sorted({line.split()[-1] for line in undefined_text.splitlines() if line.split()})
required = {"cJSON_ParseWithLengthOpts", "cJSON_Delete"}
allowed = required | {
    "cJSON_IsObject", "cJSON_IsArray", "cJSON_IsString", "cJSON_IsNumber",
    "cJSON_IsBool", "cJSON_IsTrue", "cJSON_IsFalse", "cJSON_IsNull",
    "cJSON_GetObjectItemCaseSensitive", "cJSON_GetArraySize", "cJSON_GetArrayItem",
    "memcpy", "memset", "memcmp", "strlen", "strcmp", "strncmp", "strcpy",
    "strchr", "strstr",
    "__stack_chk_fail", "__stack_chk_guard", "_GLOBAL_OFFSET_TABLE_",
    # Narrow compiler arithmetic ABI support on the 32-bit target.
    "__udivdi3", "__umoddi3", "__floatundidf", "__fixunsdfdi", "__fixunsdfsi",
    "__ltdf2", "__gtdf2", "__eqdf2", "__nedf2", "__ledf2", "__gedf2",
}
require(not set(undefined) - allowed,
        f"Unexpected parser dependency (routing-only; no network/crypto/trust/Flash): {sorted(set(undefined) - allowed)}")
require(required <= set(undefined),
        f"Missing real cJSON parser dependencies: {sorted(required - set(undefined))}")

frames = {}
for line in stacks[0].read_text().splitlines():
    columns = line.split("\t")
    require(len(columns) == 3, f"Unrecognized parser stack report: {line}")
    function = columns[0].rsplit(":", 1)[-1]
    require(columns[1].isdigit(), f"Non-numeric parser stack usage: {line}")
    require(columns[2] == "static", f"Non-static parser frame: {line}")
    size = int(columns[1])
    require(size <= args.max_frame, f"Parser frame exceeds budget: {line}")
    require(function not in frames, f"Duplicate parser stack frame: {function}")
    frames[function] = size
public = "dsr_ota_release_candidate_parse"
require(public in frames, "Public release parser stack report missing")
defined_text = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "--defined-only", str(args.elf)], text=True
)
defined = {line.split()[-1] for line in defined_text.splitlines() if line.split()}
require(public in defined, "Public release parser not retained in real target ELF")

args.out.write_text(json.dumps({
    "object": str(objects[0]),
    "stack_report": str(stacks[0]),
    "retained_public_function": public,
    "parser_stack_frames": frames,
    "parser_frame_budget_bytes": args.max_frame,
    "undefined_symbols": undefined,
    "dependency_scope": "direct undefined symbols in the release parser object",
    "network_crypto_trust_flash_slot_restart_direct_dependencies_absent": True,
    "candidate_is_verified_or_owner_approved": False,
    "recursive_and_transitive_stack_peak_measured": False,
    "runtime_cjson_heap_peak_measured": False,
    "physical_operation_verified": False,
}, indent=2) + "\n")
print(f"PASS: target release parser frames <= {args.max_frame} B; retained parser and real cJSON; memory/string/arithmetic dependencies only; candidate routing, no trust/network/Flash claim")
