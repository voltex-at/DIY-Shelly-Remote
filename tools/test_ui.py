#!/usr/bin/env python3
"""Build real pinned LVGL + firmware UI natively, without SDL or CMake."""
import argparse
import tempfile
from concurrent.futures import ThreadPoolExecutor
import hashlib
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--sanitize", action="store_true")
p.add_argument("--ubsan-only", action="store_true")
p.add_argument("--case", choices=["test_shelly_settings_ui","test_actions_editor_ui"], help="Run one focused regression after compiling production UI")
p.add_argument("--review", action="store_true", help="Render the design-review fixture, explicitly labelled DEMO")
p.add_argument("--lvgl-source", type=Path, default=ROOT / ".test-deps/lvgl")
args = p.parse_args()
lvgl = args.lvgl_source.resolve()
revision = subprocess.check_output(["git", "-C", str(lvgl), "rev-parse", "HEAD"], text=True).strip()
if revision != "c033a98afddd65aaafeebea625382a94020fe4a7":
    raise SystemExit("UI validation requires the pinned LVGL v9.3.0 commit")
build = ROOT / "build-ui" / (("review-" if args.review else "") + ("asan" if args.sanitize else "ubsan" if args.ubsan_only else "native"))
build.mkdir(parents=True, exist_ok=True)
screens = build / "screenshots"
screens.mkdir(exist_ok=True)
cc = os.environ.get("CC", "cc")
flags = ["-std=c11", "-O1", "-g", "-DLV_KCONFIG_IGNORE", "-DDSR_BOARD_ES3C35P=1",
         f'-DDSR_TEST_PHOTO_DIR="{ROOT / "tests/ui/photos"}"',
         f'-DLV_CONF_PATH="{ROOT / "tests/ui/lv_conf.h"}"']
if args.review:
    flags += ["-DDSR_UI_REVIEW_DEMO=1"]
if args.sanitize:
    flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
elif args.ubsan_only:
    flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer"]
includes = [lvgl, ROOT / "tests/ui", ROOT / "tests/actions", ROOT / "tests/history", ROOT / "tests/actor", ROOT / "tests/support",
            *[ROOT / f"components/{name}/include" for name in ["dsr_core", "dsr_config", "dsr_ota", "dsr_platform", "dsr_ui", "dsr_board"]],
            ROOT / ".test-deps", Path("/usr/include/cjson"),
            ROOT / "components/dsr_ui"]
flags += [item for folder in includes for item in ["-I", str(folder)]]
sources = list(sorted((lvgl / "src").rglob("*.c")))
sources += [ROOT / name for name in ["tests/ui/test_ui_review.c" if args.review else "tests/ui/test_ui.c", "components/dsr_ui/dsr_ui.c", "components/dsr_ui/dsr_ui_onboarding.c", "components/dsr_ui/dsr_ui_native_ota.c", "components/dsr_ui/dsr_ui_battery.c", "components/dsr_ui/dsr_ui_display.c",
            "components/dsr_ui/dsr_ui_blu.c", "components/dsr_platform/dsr_blu_model.c", "tests/ui/blu_fixture.c", "components/dsr_ui/dsr_ui_device_settings.c", "components/dsr_platform/dsr_device_settings.c", "components/dsr_platform/dsr_device_settings_model.c", "tests/ui/device_settings_fixture.c", "components/dsr_ui/dsr_ui_keyboard.c", "components/dsr_ui/dsr_ui_network.c", "components/dsr_ui/dsr_ui_wifi.c", "components/dsr_ui/dsr_ui_lan.c", "components/dsr_ui/dsr_ui_ap.c", "components/dsr_platform/dsr_ap_model.c", "components/dsr_ui/dsr_ui_actions.c", "components/dsr_ui/dsr_ui_usage.c", "components/dsr_ui/dsr_ui_ble.c", "components/dsr_ui/dsr_ui_ble_rpc.c", "components/dsr_platform/dsr_ble_model.c",
            "components/dsr_platform/dsr_wifi_policy.c"]]
sources += [ROOT / "components/dsr_ui/dsr_ui_style.c", ROOT / "components/dsr_ui/dsr_ui_mobile.c", ROOT / "components/dsr_ui/dsr_ui_room_photos.c", ROOT / "components/dsr_ui/dsr_ui_icons.c", ROOT / "components/dsr_ui/dsr_ui_provision.c"]
sources += [ROOT / "components/dsr_platform/dsr_actions.c", ROOT / "components/dsr_platform/dsr_actions_api.c", ROOT / "components/dsr_platform/dsr_actions_parse.c", ROOT / "components/dsr_platform/dsr_actions_model.c", ROOT / "components/dsr_platform/dsr_activity.c", ROOT / "components/dsr_platform/dsr_fleet.c", ROOT / "tests/ui/native_ota_fixture.c"]
sources += [ROOT / name for name in ["components/dsr_ui/dsr_ui_config.c","components/dsr_ui/dsr_ui_preview.c","components/dsr_config/dsr_config.c","components/dsr_core/dsr_core.c","components/dsr_platform/dsr_discovery_model.c","tests/ui/workspace_fixture.c"]]
sources += sorted((ROOT / "components/dsr_ui/fonts").glob("*.c"))
# Owner brand artwork: same bytes the firmware embeds via EMBED_FILES.
brand_dir = Path(tempfile.mkdtemp(prefix="dsr-brand-"))
for asset in sorted((ROOT / "components/dsr_ui/brand").glob("*.rgb565")):
    symbol = "_binary_" + asset.name.replace(".", "_") + "_start"
    # .incbin keeps compile memory flat (a 300 KB initializer list per image
    # under ASan exhausted the CI-sized runner). Bytes are identical.
    (brand_dir / (asset.stem + ".c")).write_text('__asm__(".section .rodata\\n.global ' + symbol + '\\n.balign 4\\n' + symbol + ':\\n.incbin \\"' + str(asset) + '\\"\\n.previous\\n");\n')
    sources.append(brand_dir / (asset.stem + ".c"))
sources.append(ROOT / "components/dsr_ui/dsr_ui_brand.c")
if not args.review:
    sources += [ROOT / "components/dsr_platform/dsr_config_health.c"]
if args.review:
    sources = [s for s in sources if s.is_relative_to(lvgl) or s.name.startswith("dsr_roboto_") or s.name.startswith("room_") or s.name in ["test_ui_review.c","dsr_ui_style.c","dsr_ui_mobile.c","dsr_ui_room_photos.c","dsr_ui_icons.c","dsr_ui_config.c","dsr_config.c","dsr_core.c","dsr_activity.c"]]
config_hash = hashlib.sha256((ROOT / "tests/ui/lv_conf.h").read_bytes() + repr(flags).encode()).hexdigest()
def compile_source(source):
    obj = build / (hashlib.sha256(str(source).encode()).hexdigest()[:20] + ".o")
    stamp = obj.with_suffix(".sha256")
    fingerprint = hashlib.sha256(source.read_bytes() + config_hash.encode()).hexdigest()
    if obj.exists() and stamp.exists() and stamp.read_text() == fingerprint and source.is_relative_to(lvgl):
        return obj
    warnings = [] if source.is_relative_to(lvgl) else ["-Wall", "-Wextra", "-Werror", "-Wpedantic"]
    subprocess.run([cc, *flags, *warnings, "-c", str(source), "-o", str(obj)], check=True,
                   env=dict(os.environ, TMPDIR=str(build)))
    stamp.write_text(fingerprint)
    return obj
with ThreadPoolExecutor(max_workers=6) as pool:
    objects = list(pool.map(compile_source, sources))
if args.case:
    selected=[obj for source,obj in zip(sources,objects) if source.name!="test_ui.c"]+[compile_source(ROOT/"tests/ui"/(args.case+".c"))]
    binary=build/args.case
    subprocess.run([cc,*flags,*map(str,selected),"-lm","-l:libcjson.so.1","-o",str(binary)],check=True)
    subprocess.run([str(binary),str(screens)],check=True,cwd=build)
    raise SystemExit(0)
binary = build / "test_ui"
subprocess.run([cc, *flags, *map(str, objects), "-lm", "-l:libcjson.so.1", "-o", str(binary)], check=True,
               env=dict(os.environ, TMPDIR=str(build)))
subprocess.run([str(binary), str(screens)], check=True, cwd=build)

# Independent fresh processes model reboot boundaries using the same genuine
# UI/fleet objects; no production reset hook or simulated physical acceptance.
if not args.review:
    boot_source=ROOT / "tests/ui/test_bind_boot.c"
    boot_object=compile_source(boot_source)
    boot_objects=[obj for source,obj in zip(sources,objects) if source.name!="test_ui.c"]+[boot_object]
    boot_binary=build / "test_bind_boot"
    subprocess.run([cc,*flags,*map(str,boot_objects),"-lm","-l:libcjson.so.1","-o",str(boot_binary)],check=True,env=dict(os.environ,TMPDIR=str(build)))
    for mode in range(4):
        subprocess.run([str(boot_binary),str(screens),str(mode)],check=True,cwd=build)
    cover_object=compile_source(ROOT / "tests/ui/test_cover_ui.c")
    cover_objects=[obj for source,obj in zip(sources,objects) if source.name!="test_ui.c"]+[cover_object]
    cover_binary=build / "test_cover_ui"
    subprocess.run([cc,*flags,*map(str,cover_objects),"-lm","-l:libcjson.so.1","-o",str(cover_binary)],check=True,env=dict(os.environ,TMPDIR=str(build)))
    subprocess.run([str(cover_binary),str(screens)],check=True,cwd=build)
    late_bind_object=compile_source(ROOT / "tests/ui/test_onboarding_late_bind.c")
    late_bind_objects=[obj for source,obj in zip(sources,objects) if source.name!="test_ui.c"]+[late_bind_object]
    late_bind_binary=build / "test_onboarding_late_bind"
    subprocess.run([cc,*flags,*map(str,late_bind_objects),"-lm","-l:libcjson.so.1","-o",str(late_bind_binary)],check=True,env=dict(os.environ,TMPDIR=str(build)))
    subprocess.run([str(late_bind_binary),str(screens)],check=True,cwd=build)
    light_object=compile_source(ROOT / "tests/ui/test_light_ui.c")
    light_objects=[obj for source,obj in zip(sources,objects) if source.name!="test_ui.c"]+[light_object]
    light_binary=build / "test_light_ui"
    subprocess.run([cc,*flags,*map(str,light_objects),"-lm","-l:libcjson.so.1","-o",str(light_binary)],check=True,env=dict(os.environ,TMPDIR=str(build)))
    subprocess.run([str(light_binary),str(screens)],check=True,cwd=build)

if not args.review:
    rgb_object=compile_source(ROOT / "tests/ui/test_rgb_ui.c")
    rgb_objects=[obj for source,obj in zip(sources,objects) if source.name!="test_ui.c"]+[rgb_object]
    rgb_binary=build / "test_rgb_ui"
    subprocess.run([cc,*flags,*map(str,rgb_objects),"-lm","-l:libcjson.so.1","-o",str(rgb_binary)],check=True,env=dict(os.environ,TMPDIR=str(build)))
    subprocess.run([str(rgb_binary),str(screens)],check=True,cwd=build)

if not args.review:
    # Each of these includes test_ui.c and replaces only its main().
    for name in ["test_ht_ui","test_blu_ui","test_provision_ui","test_style_ui","test_editor_layout_ui","test_shelly_settings_ui","test_actions_editor_ui"]:
        extra=[obj for source,obj in zip(sources,objects) if source.name!="test_ui.c"]+[compile_source(ROOT / "tests/ui" / (name+".c"))]
        binary=build / name
        subprocess.run([cc,*flags,*map(str,extra),"-lm","-l:libcjson.so.1","-o",str(binary)],check=True,env=dict(os.environ,TMPDIR=str(build)))
        subprocess.run([str(binary),str(screens)],check=True,cwd=build)
