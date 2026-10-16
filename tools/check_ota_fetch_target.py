"""Audit target manifest/app fetch direct dependencies and own frames.

The heap bound is a target-compiled static assertion, not a runtime measurement.
This does not bound SDK/TLS allocations or transitive stack use, and does not
claim physical network operation, trusted time or a configured publisher key.
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
parser.add_argument("--sdkconfig", type=Path, required=True)
parser.add_argument("--out", type=Path, required=True)
parser.add_argument("--max-frame", type=int, default=512)
args = parser.parse_args()
require(args.max_frame > 0, "Invalid frame budget")
configuration = {}
for line in args.sdkconfig.read_text().splitlines():
    if line.startswith("CONFIG_") and "=" in line:
        key, value = line.split("=", 1)
        require(key not in configuration, f"Duplicate sdkconfig entry: {key}")
        configuration[key] = value
enabled = {
    "CONFIG_MBEDTLS_CERTIFICATE_BUNDLE", "CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL",
    "CONFIG_MBEDTLS_HAVE_TIME", "CONFIG_MBEDTLS_HAVE_TIME_DATE",
    "CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS",
}
for key in sorted(enabled):
    require(configuration.get(key) == "y", f"Required target sdkconfig option missing: {key}=y")
for key in ["CONFIG_ESP_TLS_INSECURE", "CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY"]:
    require(configuration.get(key) != "y", f"Forbidden insecure target sdkconfig option: {key}=y")
objects = sorted(args.build_dir.rglob("dsr_ota_fetch.c.obj"))
stacks = sorted(args.build_dir.rglob("*dsr_ota_fetch*.su"))
require(len(objects) == 1, f"Expected one real fetch object: {objects}")
require(len(stacks) == 1, f"Expected one real fetch stack report: {stacks}")

undefined_text = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "-u", str(objects[0])], text=True
)
undefined = sorted({line.split()[-1] for line in undefined_text.splitlines() if line.split()})
required = {
    "calloc", "free", "esp_timer_get_time", "time", "esp_crt_bundle_attach",
    "dsr_ota_manifest_verify", "esp_http_client_init", "esp_http_client_set_header",
    "esp_http_client_open", "esp_http_client_fetch_headers",
    "esp_http_client_get_status_code", "esp_http_client_get_content_length",
    "esp_http_client_is_chunked_response", "esp_http_client_read",
    "esp_http_client_is_complete_data_received", "esp_http_client_close",
    "esp_http_client_cleanup",
    "dsr_ota_image_create", "dsr_ota_image_update", "dsr_ota_image_finish",
    "dsr_ota_image_destroy",
}
allowed = required | {
    "memcpy", "memset", "strlen", "strncmp", "strcmp", "strcpy", "strchr", "strstr",
    "__stack_chk_fail", "__stack_chk_guard", "_GLOBAL_OFFSET_TABLE_",
    # Compiler arithmetic support on the 32-bit target, not an SDK provider.
    "__udivdi3", "__udivsi3", "__umoddi3",
}
require(not set(undefined) - allowed,
        f"Unexpected fetch direct dependencies: {sorted(set(undefined) - allowed)}")
require(required <= set(undefined),
        f"Missing real fetch dependencies: {sorted(required - set(undefined))}")

frames = {}
for line in stacks[0].read_text().splitlines():
    columns = line.split("\t")
    require(len(columns) == 3, f"Unrecognized stack usage: {line}")
    function = columns[0].rsplit(":", 1)[-1]
    require(columns[1].isdigit(), f"Non-numeric stack usage: {line}")
    size = int(columns[1])
    require(columns[2] == "static", f"Non-static fetch frame: {line}")
    require(size <= args.max_frame, f"Fetch frame exceeds budget: {line}")
    require(function not in frames, f"Duplicate fetch frame: {function}")
    frames[function] = size
public = {"dsr_ota_fetch_manifest", "dsr_ota_fetch_app", "dsr_ota_fetch_metadata_body",
          "dsr_ota_fetch_app_sized", "dsr_ota_fetch_policy_valid"}
require(public <= set(frames), f"Public fetch stack reports missing: {sorted(public - set(frames))}")
require(any(name == "fetch" or name.startswith("fetch.") for name in frames),
        "Private shared transport stack report missing")

defined_text = subprocess.check_output(
    ["xtensa-esp32s3-elf-nm", "--defined-only", str(args.elf)], text=True
)
defined = {line.split()[-1] for line in defined_text.splitlines() if line.split()}
require(public <= defined, f"Fetch not retained in real target ELF: {sorted(public - defined)}")
source = (Path(__file__).resolve().parents[1] / "components/dsr_ota/dsr_ota_fetch.c").read_text()
require(re.search(r"_Static_assert\s*\(\s*sizeof\s*\(\s*fetch_t\s*\)\s*<=\s*8192\s*,", source),
        "Target-compiled fetch workspace bound assertion missing")
require(re.search(r"_Static_assert\s*\(\s*sizeof\s*\(\s*app_chain_t\s*\)\s*<=\s*512\s*,", source),
        "Target-compiled app chain context bound assertion missing")

args.out.write_text(json.dumps({
    "object": str(objects[0]),
    "stack_report": str(stacks[0]),
    "fetch_stack_frames": frames,
    "fetch_frame_budget_bytes": args.max_frame,
    "owned_heap_context_upper_bound_bytes": 8192,
    "app_chain_context_upper_bound_bytes": 512,
    "app_chain_context_evidence": "target-compiled _Static_assert(sizeof(app_chain_t)<=512)",
    "retained_public_functions": sorted(public),
    "own_chain_and_fetch_context_upper_bound_bytes": 8704,
    "image_context_included_in_chain_and_fetch_bound": False,
    "image_context_evidence": "separate check_ota_image_target.py actual target context-size audit",
    "sdkconfig": str(args.sdkconfig),
    "target_tls_required_options": sorted(enabled),
    "target_insecure_tls_options_disabled": True,
    "heap_context_evidence": "target-compiled _Static_assert(sizeof(fetch_t)<=8192)",
    "heap_context_exact_size_or_runtime_measurement": False,
    "undefined_symbols": undefined,
    "dependency_scope": "direct undefined symbols in the fetch object",
    "flash_slot_restart_and_trust_provider_direct_dependencies_absent": True,
    "transitive_sdk_tls_stack_and_heap_measured": False,
    "trusted_clock_or_publisher_key_established": False,
    "physical_network_operation_verified": False,
}, indent=2) + "\n")
print(f"PASS: target manifest/app fetch frames <= {args.max_frame} B; compiled fetch<=8192 B and chain<=512 B; HTTP/verifier/image/timer/time present; no direct Flash/slot/restart/key-provider or development-checksum dependency")
