"""Audit actual ESP32-S3 AP objects; this is not an SDK stack/heap peak test."""
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
        "AP internal BSS exceeds existing 80 KiB fleet budget")
require(memory["ui_stack"]["bytes"] == 8192 and memory["lvgl_pool"]["bytes"] == 131072,
        "AP changed the existing UI stack/LVGL pool")
units = {
    "dsr_ap": (1024, {"dsr_ap_execute", "dsr_ap_sensitive", "dsr_ap_cancel", "dsr_ap_prepare"}),
    "dsr_ap_model": (256, {"dsr_ap_candidate", "dsr_ap_identity_matches"}),
    "dsr_http": (2048, {"dsr_http_ap_inspect", "dsr_http_ap_provision", "dsr_http_ap_verify_lan"}),
    "dsr_wifi": (1024, {"dsr_wifi_temporary_connect", "dsr_wifi_restore_home", "dsr_wifi_connected"}),
    "dsr_ui_ap": (1024, {"dsr_ui_ap_open", "dsr_ui_ap_result", "dsr_ui_ap_tick"}),
    "dsr_lan": (4096, {"dsr_lan_suspend", "dsr_lan_resume"}),
    "dsr_worker": (2048, {"dsr_worker_submit"}),
    "dsr_ui_network": (2048, {"dsr_network_request_ap", "dsr_network_init"}),
}
reports = {}
for unit, (maximum, public) in units.items():
    paths = sorted(a.build_dir.rglob(unit + ".c.su"))
    require(len(paths) == 1, f"Expected one AP stack report: {unit}")
    frames = {}
    for line in paths[0].read_text().splitlines():
        fields = line.split("\t")
        require(len(fields) == 3 and fields[1].isdigit() and fields[2] == "static",
                f"Unbounded/unrecognized AP own frame: {line}")
        name, size = fields[0].rsplit(":", 1)[-1], int(fields[1])
        require(size <= maximum, f"AP {unit} own frame exceeds {maximum} B: {line}")
        require(name not in frames, f"Duplicate AP own frame: {unit}:{name}")
        frames[name] = size
    require(public <= {name.split(".")[0] for name in frames}, f"AP public frames missing: {unit}")
    reports[unit] = {"maximum_own_frame_bytes": maximum, "own_frames": frames}
states = {}
for unit, maximum in [("dsr_ap", 64), ("dsr_ui_ap", 2048)]:
    objects = sorted(a.build_dir.rglob(unit + ".c.obj"))
    require(len(objects) == 1, f"Expected one AP object: {unit}")
    table = subprocess.check_output(["xtensa-esp32s3-elf-nm", "-S", "--radix=d", str(objects[0])], text=True)
    symbols = {}
    for line in table.splitlines():
        fields = line.split()
        if len(fields) == 4 and fields[2].lower() == "b":
            require(fields[1].isdigit() and fields[3] not in symbols, f"Invalid AP BSS symbol: {line}")
            symbols[fields[3]] = int(fields[1])
    total = sum(symbols.values())
    require(0 < total <= maximum, f"AP fixed storage exceeds {maximum} B: {unit}={total}")
    states[unit] = {"bytes": total, "maximum_bytes": maximum, "symbols": symbols}
a.out.write_text(json.dumps({
    "actual_internal_bss_bytes": memory["internal_bss_bytes"],
    "multi_device_parent_internal_bss_bytes": 64992,
    "actual_delta_from_multi_device_parent_bytes": memory["internal_bss_bytes"] - 64992,
    "maximum_internal_bss_bytes": 81920,
    "own_stack_reports": reports,
    "fixed_storage": states,
    "configured_ui_stack_bytes": 8192,
    "configured_worker_stack_bytes": 10240,
    "sdk_stack_or_heap_peak_measured": False,
    "physical_ap_or_home_restore_verified": False,
}, indent=2) + "\n")
print("PASS: actual AP target own frames and fixed storage; SDK peak and physical RF remain unmeasured")
