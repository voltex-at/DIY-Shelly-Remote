"""Audit actual ESP32-S3 BLE RPC objects; this is not an SDK stack/heap peak test."""
import argparse
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument("--build-dir", type=Path, required=True)
p.add_argument("--out", type=Path, required=True)
a = p.parse_args()

def require(ok, message):
    if not ok:
        raise SystemExit(message)

memory = json.loads((a.build_dir / "BOOT_MEMORY.json").read_text())
require(type(memory["internal_bss_bytes"]) is int and 8192 <= memory["internal_bss_bytes"] <= 81920,
        "BLE RPC internal BSS exceeds existing 80 KiB fleet budget")
require(memory["ui_stack"]["bytes"] == 8192 and memory["lvgl_pool"]["bytes"] == 131072,
        "BLE RPC changed the existing UI stack/LVGL pool")
units = {
    "dsr_ble": (1024, {"dsr_ble_init", "dsr_ble_scan", "dsr_ble_snapshot", "dsr_ble_manager_rpc", "dsr_ble_handoff_request", "dsr_ble_handoff_take"}),
    "dsr_ble_rpc": (1536, {"dsr_ble_rpc_start", "dsr_ble_rpc_confirm", "dsr_ble_rpc_cancel", "dsr_ble_rpc_snapshot"}),
    "dsr_ble_rpc_protocol": (1024, {"dsr_rpc_stream_begin", "dsr_rpc_stream_next", "dsr_rpc_stream_complete", "dsr_rpc_request", "dsr_rpc_response", "dsr_rpc_identity", "dsr_rpc_setup_policy"}),
    "dsr_ble_handoff": (256, {"dsr_ble_handoff_parse"}),
    "dsr_ui_ble_rpc": (1024, {"dsr_ui_ble_rpc_open", "dsr_ui_ble_rpc_tick", "dsr_ui_ble_rpc_result"}),
    "dsr_ui_onboarding": (512, {"dsr_ui_onboarding_open", "dsr_ui_onboarding_tick", "dsr_ui_onboarding_reset", "dsr_ui_onboarding_active", "dsr_onboarding_screen", "dsr_onboarding_footer", "dsr_onboarding_keyboard", "dsr_onboarding_result"}),
    "dsr_http": (2048, {"dsr_http_lan_verify_station"}),
    "dsr_worker": (2048, {"dsr_worker_submit"}),
    "dsr_ui_network": (2048, {"dsr_network_request_station", "dsr_network_init"}),
}
reports = {}
for unit, (maximum, public) in units.items():
    paths = sorted(a.build_dir.rglob(unit + ".c.su"))
    require(len(paths) == 1, f"Expected one BLE RPC stack report: {unit}")
    frames = {}
    for line in paths[0].read_text().splitlines():
        fields = line.split("\t")
        require(len(fields) == 3 and fields[1].isdigit() and fields[2] == "static",
                f"Unbounded/unrecognized BLE RPC own frame: {line}")
        name, size = fields[0].rsplit(":", 1)[-1], int(fields[1])
        require(size <= maximum, f"BLE RPC {unit} own frame exceeds {maximum} B: {line}")
        require(name not in frames, f"Duplicate BLE RPC own frame: {unit}:{name}")
        frames[name] = size
    require(public <= {name.split(".")[0] for name in frames}, f"BLE RPC public frames missing: {unit}")
    reports[unit] = {"maximum_own_frame_bytes": maximum, "own_frames": frames}
states = {}
for unit, maximum in [("dsr_ble", 2048), ("dsr_ble_rpc", 2048), ("dsr_ui_ble_rpc", 1024), ("dsr_ui_onboarding", 256)]:
    objects = sorted(a.build_dir.rglob(unit + ".c.obj"))
    require(len(objects) == 1, f"Expected one BLE RPC object: {unit}")
    table = subprocess.check_output(["xtensa-esp32s3-elf-nm", "-S", "--radix=d", str(objects[0])], text=True)
    symbols = {}
    for line in table.splitlines():
        fields = line.split()
        if len(fields) == 4 and fields[2].lower() == "b":
            require(fields[1].isdigit() and fields[3] not in symbols, f"Invalid BLE RPC BSS symbol: {line}")
            symbols[fields[3]] = int(fields[1])
    total = sum(symbols.values())
    require(0 < total <= maximum, f"BLE RPC fixed storage exceeds {maximum} B: {unit}={total}")
    states[unit] = {"bytes": total, "maximum_bytes": maximum, "symbols": symbols}
a.out.write_text(json.dumps({
    "actual_internal_bss_bytes": memory["internal_bss_bytes"],
    "ap_parent_internal_bss_bytes": 66552,
    "actual_delta_from_ap_parent_bytes": memory["internal_bss_bytes"] - 66552,
    "maximum_internal_bss_bytes": 81920,
    "own_stack_reports": reports,
    "fixed_storage": states,
    "configured_ui_stack_bytes": 8192,
    "configured_worker_stack_bytes": 10240,
    "sdk_stack_or_heap_peak_measured": False,
    "physical_gatt_provisioning_or_home_lan_verified": False,
}, indent=2) + "\n")
print("PASS: actual BLE RPC target own frames and fixed storage; SDK peak and physical RF remain unmeasured")
