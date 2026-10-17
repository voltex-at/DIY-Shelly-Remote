#!/usr/bin/env python3
"""Compile genuine RPC framing/policy with real cJSON, no radio emulation claim."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--sanitize", action="store_true")
p.add_argument("--ubsan-only", action="store_true")
a = p.parse_args()
json_include = Path("/usr/include/cjson")
if not (json_include / "cJSON.h").exists():
    json_include = ROOT / ".test-deps"
if not (json_include / "cJSON.h").exists():
    raise SystemExit("Install libcjson-dev to run BLE RPC fixtures")
flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-g"]
if a.sanitize:
    flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
elif a.ubsan_only:
    flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer"]
with tempfile.TemporaryDirectory(prefix="dsr-ble-rpc-") as tmp:
    binary = Path(tmp) / "test_ble_rpc"
    folders = ["tests/support", str(json_include), "components/dsr_core/include", "components/dsr_platform/include", "components/dsr_config/include"]
    sources = ["tests/test_ble_rpc.c", "components/dsr_platform/dsr_ble_handoff.c", "components/dsr_platform/dsr_ble_rpc_protocol.c", "components/dsr_platform/dsr_discovery_model.c", "components/dsr_platform/dsr_wifi_policy.c", "components/dsr_config/dsr_config.c", "components/dsr_core/dsr_core.c"]
    subprocess.run([os.environ.get("CC", "cc"), *flags, *[x for folder in folders for x in ["-I", str(ROOT / folder)]], *[str(ROOT / source) for source in sources], "-l:libcjson.so.1", "-lm", "-o", str(binary)], check=True, cwd=tmp)
    subprocess.run([str(binary)], check=True, cwd=tmp)
    driver = Path(tmp) / "test_ble_rpc_driver"
    folders = ["tests/ble_rpc", "tests/actor", *folders]
    sources = ["tests/test_ble_rpc_driver.c", "components/dsr_platform/dsr_ble_rpc.c", "components/dsr_platform/dsr_ble_rpc_protocol.c", "components/dsr_platform/dsr_discovery_model.c", "components/dsr_platform/dsr_wifi_policy.c", "components/dsr_config/dsr_config.c", "components/dsr_core/dsr_core.c", "components/dsr_platform/dsr_activity.c"]
    subprocess.run([os.environ.get("CC", "cc"), *flags, *[x for folder in folders for x in ["-I", str(ROOT / folder)]], *[str(ROOT / source) for source in sources], "-Wl,--wrap=calloc", "-Wl,--wrap=free", "-l:libcjson.so.1", "-lm", "-o", str(driver)], check=True, cwd=tmp)
    subprocess.run([str(driver)], check=True, cwd=tmp)

    panel=(ROOT / "components/dsr_platform/dsr_panel.c").read_text()
    start=panel.index("static esp_err_t ble_open_api(httpd_req_t *r)")
    end=panel.index("static esp_err_t ble_api(httpd_req_t *r)",start)
    (Path(tmp)/"ble_open_endpoint.inc").write_text(panel[start:end])
    handler=Path(tmp)/"handler"
    subprocess.run([os.environ.get("CC","cc"),*flags,"-I",tmp,*[x for folder in folders for x in ["-I",str(ROOT/folder)]],str(ROOT/"tests/ble_rpc/handlers.c"),str(ROOT/"components/dsr_platform/dsr_ble_handoff.c"),"-l:libcjson.so.1","-lm","-o",str(handler)],check=True,cwd=tmp)
    subprocess.run([str(handler)],check=True,cwd=tmp)
