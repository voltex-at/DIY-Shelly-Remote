"""Audit the target Release-to-signed-app validation/discard bridge.

Only direct dependencies and own static frames are inspected. The context
bound is a compiled assertion, not an exact sizeof or runtime/transitive peak.
Network/verification occur through required genuine helpers, never here.
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
objects = sorted(args.build_dir.rglob("dsr_ota_release_chain.c.obj"))
stacks = sorted(args.build_dir.rglob("*dsr_ota_release_chain*.su"))
require(len(objects) == 1, f"Expected one real release bridge object: {objects}")
require(len(stacks) == 1, f"Expected one real release bridge stack report: {stacks}")
undefined_text = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "-u", str(objects[0])], text=True
)
undefined = sorted({line.split()[-1] for line in undefined_text.splitlines() if line.split()})
required = {
    "dsr_ota_fetch_release", "dsr_ota_fetch_policy_valid",
    "dsr_ota_fetch_app_sized", "calloc", "free",
}
allowed = required | {
    "dsr_ota_release_policy_check", "memcpy", "memset",
    "__stack_chk_guard", "__stack_chk_fail", "_GLOBAL_OFFSET_TABLE_",
}
require(not set(undefined) - allowed,
        f"Unexpected bridge dependency (no direct network/crypto/trust/Flash/install): {sorted(set(undefined) - allowed)}")
require(required <= set(undefined),
        f"Required genuine bridge dependency missing: {sorted(required - set(undefined))}")
frames = {}
for line in stacks[0].read_text().splitlines():
    columns = line.split("\t")
    require(len(columns) == 3, f"Unrecognized bridge stack report: {line}")
    name = columns[0].rsplit(":", 1)[-1]
    require(columns[1].isdigit(), f"Non-numeric bridge stack usage: {line}")
    require(columns[2] == "static", f"Non-static bridge frame: {line}")
    size = int(columns[1])
    require(size <= args.max_frame, f"Bridge frame exceeds budget: {line}")
    require(name not in frames, f"Duplicate bridge stack frame: {name}")
    frames[name] = size
public = "dsr_ota_fetch_release_app"
require(public in frames, "Public release bridge stack report missing")
defined_text = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "--defined-only", str(args.elf)], text=True
)
defined = {line.split()[-1] for line in defined_text.splitlines() if line.split()}
require(public in defined, "Public release bridge not retained in real target ELF")
source = (Path(__file__).resolve().parents[1] / "components/dsr_ota/dsr_ota_release_chain.c").read_text()
require(re.search(r"_Static_assert\s*\(\s*sizeof\s*\(\s*release_chain_t\s*\)\s*<=\s*1024\s*,", source),
        "Target-compiled bridge context bound assertion missing")
args.out.write_text(json.dumps({
    "object": str(objects[0]),
    "stack_report": str(stacks[0]),
    "retained_public_function": public,
    "bridge_stack_frames": frames,
    "bridge_frame_budget_bytes": args.max_frame,
    "bridge_heap_context_upper_bound_bytes": 1024,
    "bridge_heap_context_evidence": "target-compiled _Static_assert(sizeof(release_chain_t)<=1024)",
    "exact_context_size_or_runtime_heap_measurement": False,
    "undefined_symbols": undefined,
    "dependency_scope": "direct undefined symbols in the release bridge object",
    "direct_network_crypto_trust_flash_install_restart_dependencies_absent": True,
    "genuine_network_verification_via_required_helpers": True,
    "sdk_tls_cjson_heap_and_transitive_stack_peak_measured": False,
    "approved_release_or_installation_proven": False,
    "physical_network_operation_verified": False,
}, indent=2) + "\n")
print(f"PASS: target release bridge frames <= {args.max_frame} B; compiled context<=1024 B; genuine release fetch/policy/sized app dependencies; no direct network/crypto/trust/Flash/install")
