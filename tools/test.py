#!/usr/bin/env python3
"""Compile and execute the actual portable C code without ESP-IDF."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--ubsan-only", action="store_true", help="For restricted runtimes without LeakSanitizer support")
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix="dsr-test-") as tmp:
    temp_root = Path(tmp).resolve()
    binary = temp_root / "test_core"
    test_env = dict(os.environ, TMPDIR=str(temp_root))
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-g"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    elif args.ubsan_only:
        flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer"]
    subprocess.run([os.environ.get("CC", "cc"), *flags,
                    "-I", str(ROOT / "components/dsr_core/include"),
                    str(ROOT / "components/dsr_core/dsr_core.c"),
                    str(ROOT / "tests/test_core.c"), "-o", str(binary)], check=True,
                   cwd=temp_root, env=test_env)
    subprocess.run([str(binary)], check=True, cwd=temp_root, env=test_env)

    json_include = Path("/usr/include/cjson")
    if not (json_include / "cJSON.h").exists():
        json_include = ROOT / ".test-deps"
    if not (json_include / "cJSON.h").exists():
        raise SystemExit("Install libcjson-dev to run platform fixtures")
    platform_binary = temp_root / "test_platform"
    subprocess.run([os.environ.get("CC", "cc"), *flags,
                    "-I", str(ROOT / "tests/support"), "-I", str(json_include),
                    "-I", str(ROOT / "components/dsr_core/include"),
                    "-I", str(ROOT / "components/dsr_platform/include"),
                    "-I", str(ROOT / "components/dsr_config/include"),
                    *[str(ROOT / source) for source in [
                        "components/dsr_core/dsr_core.c", "components/dsr_platform/dsr_http.c",
                        "components/dsr_platform/dsr_discovery_model.c", "components/dsr_config/dsr_config.c",
                        "components/dsr_platform/dsr_connection.c", "components/dsr_platform/dsr_settings.c",
                        "components/dsr_platform/dsr_wifi_policy.c",
                        "tests/test_platform.c"]],
                    "-l:libcjson.so.1", "-lm", "-o", str(platform_binary)],
                   check=True, cwd=temp_root, env=test_env)
    subprocess.run([str(platform_binary)], check=True, cwd=temp_root, env=test_env)
    wifi_binary = temp_root / "test_wifi"
    subprocess.run([os.environ.get("CC", "cc"), *flags,
                    "-I", str(ROOT / "tests/support"),
                    "-I", str(ROOT / "components/dsr_core/include"),
                    "-I", str(ROOT / "components/dsr_platform/include"),
                    str(ROOT / "components/dsr_platform/dsr_wifi.c"),
                    str(ROOT / "components/dsr_platform/dsr_wifi_policy.c"),
                    str(ROOT / "tests/test_wifi.c"), "-o", str(wifi_binary)],
                   check=True, cwd=temp_root, env=test_env)
    subprocess.run([str(wifi_binary)], check=True, cwd=temp_root, env=test_env)
    ble_binary=temp_root / "test_ble"
    subprocess.run([os.environ.get("CC","cc"),*flags,
                    "-I",str(ROOT / "tests/ble"),"-I",str(ROOT / "tests/support"),
                    "-I",str(ROOT / "components/dsr_platform/include"),
                    str(ROOT / "components/dsr_platform/dsr_ble_model.c"),
                    str(ROOT / "components/dsr_platform/dsr_ble.c"),
                    str(ROOT / "tests/test_ble.c"),"-o",str(ble_binary)],check=True,cwd=temp_root,env=test_env)
    subprocess.run([str(ble_binary)],check=True,cwd=temp_root,env=test_env)
    boot_binary = temp_root / "test_boot_memory"
    subprocess.run([os.environ.get("CC", "cc"), *flags,
                    "-I", str(ROOT / "tests/boot"), "-I", str(ROOT / "tests/support"),
                    "-I", str(ROOT / "components/dsr_board"),
                    "-I", str(ROOT / "components/dsr_platform"),
                    "-I", str(ROOT / "components/dsr_platform/include"),
                    str(ROOT / "components/dsr_board/dsr_ui_task.c"),
                    str(ROOT / "components/dsr_platform/dsr_storage.c"),
                    str(ROOT / "components/dsr_platform/dsr_config_health.c"),
                    str(ROOT / "tests/test_boot_memory.c"), "-o", str(boot_binary)],
                   check=True, cwd=temp_root, env=test_env)
    subprocess.run([str(boot_binary)], check=True, cwd=temp_root, env=test_env)
    startup_binary = temp_root / "test_startup"
    subprocess.run([os.environ.get("CC", "cc"), *flags, "-DDSR_BOARD_ES3C35P=1",
                    "-I", str(ROOT / "tests/startup"), "-I", str(ROOT / "tests/boot"),
                    "-I", str(ROOT / "tests/ble"), "-I", str(ROOT / "tests/ui"),
                    "-I", str(ROOT / "tests/support"),
                    "-I", str(ROOT / "components/dsr_core/include"),
                    "-I", str(ROOT / "components/dsr_platform/include"),
                    str(ROOT / "main/app_main.c"), str(ROOT / "components/dsr_platform/dsr_boot.c"),
                    str(ROOT / "components/dsr_platform/dsr_config_health.c"),
                    str(ROOT / "tests/test_startup.c"), "-o", str(startup_binary)],
                   check=True, cwd=temp_root, env=test_env)
    subprocess.run([str(startup_binary)], check=True, cwd=temp_root, env=test_env)
    workspace_binary=temp_root / "test_workspace_start"
    subprocess.run([os.environ.get("CC","cc"),*flags,
                    *[item for directory in ["tests/actor","tests/support","components/dsr_core/include","components/dsr_platform/include","components/dsr_config/include",str(json_include)] for item in ["-I",str(ROOT / directory) if not Path(directory).is_absolute() else directory]],
                    *[str(ROOT / source) for source in ["components/dsr_platform/dsr_workspace.c","components/dsr_platform/dsr_config_health.c","components/dsr_config/dsr_config.c","components/dsr_core/dsr_core.c","tests/test_workspace_start.c"]],
                    "-Wl,--wrap=malloc","-l:libcjson.so.1","-lm","-o",str(workspace_binary)],check=True,cwd=temp_root,env=test_env)
    for scenario in range(7):
        subprocess.run([str(workspace_binary),str(scenario)],check=True,cwd=temp_root,env=test_env)
    subprocess.run(["python3",str(ROOT / "tests/test_storage_recovery.py")],check=True,cwd=temp_root,env=test_env)
