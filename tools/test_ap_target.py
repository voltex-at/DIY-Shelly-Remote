"""Positive and rejection fixtures for the target auditor; no target ELF claim."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[1]
units = {
    "dsr_ap": ["dsr_ap_execute", "dsr_ap_sensitive", "dsr_ap_cancel", "dsr_ap_prepare"],
    "dsr_ap_model": ["dsr_ap_candidate", "dsr_ap_identity_matches"],
    "dsr_http": ["dsr_http_ap_inspect", "dsr_http_ap_provision", "dsr_http_ap_verify_lan"],
    "dsr_wifi": ["dsr_wifi_temporary_connect", "dsr_wifi_restore_home", "dsr_wifi_connected"],
    "dsr_ui_ap": ["dsr_ui_ap_open", "dsr_ui_ap_result", "dsr_ui_ap_tick"],
    "dsr_lan": ["dsr_lan_suspend", "dsr_lan_resume"],
    "dsr_worker": ["dsr_worker_submit"],
    "dsr_ui_network": ["dsr_network_request_ap", "dsr_network_init"],
}
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
        (folder / "dsr_ap.c.obj").write_text("00000000 00000003 b sensitive_flags\n")
        (folder / "dsr_ui_ap.c.obj").write_text("00000000 00001500 b state\n")
    def check(expected):
        r = subprocess.run([sys.executable, str(ROOT / "tools/check_ap_target.py"), "--build-dir", str(folder), "--out", str(folder / "AP.json")], env=env, capture_output=True, text=True)
        assert (r.returncode == 0) == expected, r.stdout + r.stderr
    fixture();check(True)
    fixture();(folder / "dsr_ui_ap.c.su").write_text("dsr_ui_ap.c:1:1:dsr_ui_ap_tick\t4000\tstatic\n");check(False)
    fixture();(folder / "dsr_worker.c.su").write_text("dsr_worker.c:1:1:dsr_worker_submit\t2064\tstatic\n");check(False)
    fixture();(folder / "dsr_http.c.su").write_text("dsr_http.c:1:1:dsr_http_ap_provision\t128\tdynamic,bounded\n");check(False)
    fixture();(folder / "dsr_wifi.c.su").unlink();check(False)
    fixture();(folder / "dsr_ap_model.c.su").write_text("");check(False)
    fixture();(folder / "dsr_ui_ap.c.obj").write_text("00000000 00002049 b state\n");check(False)
    fixture();memory = json.loads((folder / "BOOT_MEMORY.json").read_text());memory["internal_bss_bytes"] = 81921;(folder / "BOOT_MEMORY.json").write_text(json.dumps(memory));check(False)
print("PASS: AP target auditor 1 positive + 7 rejection fixtures (synthetic reports, not target evidence)")
