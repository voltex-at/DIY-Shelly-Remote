"""Audit the target single-Release fetch wrapper, not transitive SDK peaks.

The wrapper calls a private HTTP transport; direct absence of HTTP symbols does
not mean absence of network I/O. Context bounds are target-compiled assertions,
not exact sizeof/runtime heap measurements. cJSON/SDK/TLS allocations are extra.
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
require(args.max_frame > 0, "Invalid frame budget")
objects = sorted(args.build_dir.rglob("dsr_ota_release_fetch.c.obj"))
stacks = sorted(args.build_dir.rglob("*dsr_ota_release_fetch*.su"))
require(len(objects) == 1, f"Expected one real release fetch wrapper object: {objects}")
require(len(stacks) == 1, f"Expected one real release fetch wrapper stack report: {stacks}")
undefined_text = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "-u", str(objects[0])], text=True
)
undefined = sorted({line.split()[-1] for line in undefined_text.splitlines() if line.split()})
required = {
    "dsr_ota_release_policy_check", "dsr_ota_release_candidate_parse",
    "dsr_ota_fetch_metadata_body", "time", "esp_timer_get_time", "calloc", "free",
}
allowed = required | {
    "memcpy", "memset", "strcpy", "strcat", "strlen", "stpcpy",
    "__stack_chk_guard", "__stack_chk_fail", "_GLOBAL_OFFSET_TABLE_",
}
require(not set(undefined) - allowed,
        f"Unexpected wrapper direct dependencies (no direct HTTP/crypto/trust/Flash/install): {sorted(set(undefined) - allowed)}")
require(required <= set(undefined),
        f"Required real wrapper dependency missing: {sorted(required - set(undefined))}")
frames = {}
for line in stacks[0].read_text().splitlines():
    columns = line.split("\t")
    require(len(columns) == 3, f"Unrecognized wrapper stack report: {line}")
    name = columns[0].rsplit(":", 1)[-1]
    require(columns[1].isdigit(), f"Non-numeric wrapper stack usage: {line}")
    require(columns[2] == "static", f"Non-static wrapper frame: {line}")
    size = int(columns[1])
    require(size <= args.max_frame, f"Wrapper frame exceeds budget: {line}")
    require(name not in frames, f"Duplicate wrapper stack frame: {name}")
    frames[name] = size
public = "dsr_ota_fetch_release"
require(public in frames, "Public release fetch wrapper stack report missing")
defined_text = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "--defined-only", str(args.elf)], text=True
)
defined = {line.split()[-1] for line in defined_text.splitlines() if line.split()}
require(public in defined, "Public release fetch wrapper not retained in real target ELF")
root = Path(__file__).resolve().parents[1]
source = (root / "components/dsr_ota/dsr_ota_release_fetch.c").read_text()
require(re.search(r"_Static_assert\s*\(\s*sizeof\s*\(\s*release_fetch_t\s*\)\s*<=\s*16640\s*,", source),
        "Target-compiled release body workspace assertion missing")
transport = (root / "components/dsr_ota/dsr_ota_fetch.c").read_text()
require(re.search(r"_Static_assert\s*\(\s*sizeof\s*\(\s*fetch_t\s*\)\s*<=\s*8192\s*,", transport),
        "Target-compiled shared transport workspace assertion missing")
args.out.write_text(json.dumps({
    "object": str(objects[0]),
    "stack_report": str(stacks[0]),
    "retained_public_function": public,
    "wrapper_stack_frames": frames,
    "wrapper_frame_budget_bytes": args.max_frame,
    "release_body_workspace_upper_bound_bytes": 16640,
    "release_body_workspace_evidence": "target-compiled _Static_assert(sizeof(release_fetch_t)<=16640)",
    "shared_transport_context_upper_bound_bytes": 8192,
    "combined_own_transport_phase_upper_bound_bytes": 24832,
    "transport_phase": "release body workspace plus shared fetch context; SDK/TLS/header allocations additional",
    "parser_phase": "release body workspace plus cJSON allocations; shared fetch context already freed",
    "exact_context_size_or_runtime_heap_measurement": False,
    "undefined_symbols": undefined,
    "dependency_scope": "direct undefined symbols in the release fetch wrapper object",
    "direct_http_crypto_trust_flash_install_restart_dependencies_absent": True,
    "network_via_private_metadata_transport": True,
    "sdk_tls_cjson_heap_peak_measured": False,
    "transitive_stack_peak_measured": False,
    "candidate_is_verified_or_owner_approved": False,
    "physical_network_operation_verified": False,
}, indent=2) + "\n")
print(f"PASS: target release fetch wrapper frames <= {args.max_frame} B; compiled own workspace bounds16640+8192=24832 B; required private transport/parser/time dependencies; no direct install/crypto/trust provider")
