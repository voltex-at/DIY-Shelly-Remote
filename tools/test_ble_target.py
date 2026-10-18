"""Positive and rejection fixtures for the target auditor; no target ELF claim."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[1]
units = {
    "dsr_device_settings": (1024, {"dsr_device_settings_prepare", "dsr_device_settings_execute", "dsr_device_settings_snapshot"}),
    "dsr_device_settings_model": (1024, {"dsr_setting_patch", "dsr_setting_network_patch", "dsr_setting_groups"}),
    "dsr_blu": (1536, {"dsr_blu_start", "dsr_blu_write", "dsr_blu_forget", "dsr_blu_pin", "dsr_blu_snapshot"}),
    "dsr_blu_model": (512, {"dsr_blu_value_parse", "dsr_blu_value_format"}),
    "dsr_ui_blu": (1024, {"dsr_ui_blu_open", "dsr_ui_blu_settings", "dsr_ui_blu_tick"}),
    "dsr_ui_device_settings": (1024, {"dsr_ui_device_settings_open", "dsr_ui_device_settings_tick"}),

    "dsr_ble": (1024, {"dsr_ble_init", "dsr_ble_scan", "dsr_ble_snapshot", "dsr_ble_manager_rpc", "dsr_ble_handoff_request", "dsr_ble_handoff_take"}),
    "dsr_ble_rpc": (1536, {"dsr_ble_rpc_start", "dsr_ble_rpc_confirm", "dsr_ble_rpc_cancel", "dsr_ble_rpc_snapshot"}),
    "dsr_ble_rpc_protocol": (1024, {"dsr_rpc_stream_begin", "dsr_rpc_stream_next", "dsr_rpc_stream_complete", "dsr_rpc_request", "dsr_rpc_response", "dsr_rpc_identity", "dsr_rpc_setup_policy"}),
    "dsr_ble_handoff": (256, {"dsr_ble_handoff_parse"}),
    "dsr_ui_ble_rpc": (1024, {"dsr_ui_ble_rpc_open", "dsr_ui_ble_rpc_tick", "dsr_ui_ble_rpc_result"}),
    "dsr_ui_onboarding": (512, {"dsr_ui_onboarding_open", "dsr_ui_onboarding_tick", "dsr_ui_onboarding_reset", "dsr_ui_onboarding_active", "dsr_onboarding_screen", "dsr_onboarding_footer", "dsr_onboarding_keyboard", "dsr_onboarding_result"}),
    "dsr_http": (2048, {"dsr_http_lan_verify_station", "dsr_http_light", "dsr_http_rgb"}),
    "dsr_worker": (2048, {"dsr_worker_submit"}),
    "dsr_ui_mobile": (1024, {"dsr_mobile_update", "dsr_mobile_show"}),
    "dsr_ui_network": (2048, {"dsr_network_request_station", "dsr_network_init"}),
}
units = {name:list(public) for name,(_,public) in units.items()}
with tempfile.TemporaryDirectory(prefix="dsr-ap-audit-") as temp:
    folder = Path(temp)
    nm = folder / "xtensa-esp32s3-elf-nm"
    nm.write_text("#!" + sys.executable + "\nimport sys\nfrom pathlib import Path\nprint(Path(sys.argv[-1]).read_text())\n")
    nm.chmod(0o755)
    env = dict(os.environ, PATH=str(folder) + os.pathsep + os.environ["PATH"])
    def fixture():
        for file in folder.glob("dsr_*"):
            file.unlink()
        (folder / "BOOT_MEMORY.json").write_text(json.dumps({"internal_bss_bytes": 67000, "ui_stack": {"bytes": 8192}, "lvgl_pool": {"bytes": 131072}}))
        for unit, public in units.items():
            (folder / (unit + ".c.su")).write_text("".join(f"{unit}.c:1:1:{name}\t128\tstatic\n" for name in public))
        (folder / "dsr_ble.c.obj").write_text("00000000 00001000 b state\n")
        (folder / "dsr_ble_rpc.c.obj").write_text("00000000 00001000 b state\n")
        (folder / "dsr_ui_ble_rpc.c.obj").write_text("00000000 00000500 b state\n")
        (folder / "dsr_ui_onboarding.c.obj").write_text("00000000 00000100 b state\n")
        for unit in ["dsr_blu","dsr_device_settings","dsr_ui_blu","dsr_ui_device_settings"]:
            (folder / (unit + ".c.obj")).write_text("00000000 00000008 b state\n")
    def check(expected):
        r = subprocess.run([sys.executable, str(ROOT / "tools/check_ble_target.py"), "--build-dir", str(folder), "--out", str(folder / "BLE.json")], env=env, capture_output=True, text=True)
        assert (r.returncode == 0) == expected, r.stdout + r.stderr
    fixture();check(True)
    fixture();(folder / "dsr_ui_ble_rpc.c.su").write_text("dsr_ui_ble_rpc.c:1:1:dsr_ui_ble_rpc_tick\t4000\tstatic\n");check(False)
    fixture();(folder / "dsr_worker.c.su").write_text("dsr_worker.c:1:1:dsr_worker_submit\t2064\tstatic\n");check(False)
    fixture();(folder / "dsr_http.c.su").write_text("dsr_http.c:1:1:dsr_http_ap_provision\t128\tdynamic,bounded\n");check(False)
    fixture();(folder / "dsr_ble.c.su").unlink();check(False)
    fixture();(folder / "dsr_ble_handoff.c.su").write_text("");check(False)
    fixture();(folder / "dsr_ui_ble_rpc.c.obj").write_text("00000000 00001025 b state\n");check(False)
    fixture();memory = json.loads((folder / "BOOT_MEMORY.json").read_text());memory["internal_bss_bytes"] = 81921;(folder / "BOOT_MEMORY.json").write_text(json.dumps(memory));check(False)
    fixture();(folder / "dsr_ui_onboarding.c.su").write_text("dsr_ui_onboarding.c:1:1:dsr_onboarding_screen\t528\tstatic\n");check(False)
    fixture();(folder / "dsr_ui_onboarding.c.obj").write_text("00000000 00000257 b state\n");check(False)
    fixture();(folder / "dsr_ui_mobile.c.su").write_text("dsr_ui_mobile.c:1:1:dsr_mobile_update\t1040\tstatic\n");check(False)
    fixture();(folder / "dsr_blu.c.su").write_text("dsr_blu.c:1:1:dsr_blu_start\t1537\tstatic\n");check(False)
    fixture();(folder / "dsr_device_settings.c.obj").write_text("00000000 00000129 b state\n");check(False)
    fixture();(folder / "dsr_ui_device_settings.c.su").unlink();check(False)
print("PASS: BLE/Light/onboarding target auditor 1 positive + 13 rejection fixtures (synthetic reports, not target evidence)")
