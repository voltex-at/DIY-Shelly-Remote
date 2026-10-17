"""Shared helpers for the host C test runners (compiler flags, cJSON, build)."""
import argparse
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-Wpedantic', '-g']


def parse_args(parser=None):
    p = parser or argparse.ArgumentParser()
    p.add_argument('--sanitize', action='store_true', help='AddressSanitizer + UndefinedBehaviorSanitizer')
    p.add_argument('--ubsan-only', action='store_true', help='For restricted runtimes without LeakSanitizer support')
    return p.parse_args()


def sanitizer(args):
    if args.sanitize:
        return ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    if args.ubsan_only:
        return ['-fsanitize=undefined', '-fno-omit-frame-pointer']
    return []


def forward(args):
    """Sanitizer switches to pass to a child runner."""
    return ['--sanitize'] if args.sanitize else ['--ubsan-only'] if args.ubsan_only else []


def cjson_include():
    for folder in (Path('/usr/include/cjson'), ROOT / '.test-deps'):
        if (folder / 'cJSON.h').exists():
            return folder
    raise SystemExit('Install libcjson-dev to run host fixtures')


def _path(value):
    path = Path(value)
    return str(path if path.is_absolute() else ROOT / path)


def build(out, sources, includes=(), flags=(), libs=(), args=None, env=None):
    subprocess.run([os.environ.get('CC', 'cc'), *BASE, *(sanitizer(args) if args else []), *flags,
                    *[x for folder in includes for x in ('-I', _path(folder))],
                    *[_path(source) for source in sources], *libs, '-o', str(out)], check=True, env=env)
