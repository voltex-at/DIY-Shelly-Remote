# Synthetic SPIFFS reference

spiffs-idf-5.5.4.zlib.base64 is a zlib-compressed, base64 encoded 0x3E0000
image produced by the actual ESP-IDF v5.5.4 spiffsgen.py, default geometry.
Generator: https://github.com/espressif/esp-idf/blob/v5.5.4/components/spiffs/spiffsgen.py
Generator SHA-256: 20a4aaac4a22ca1b926d2163cf05fe20c5968f8160b35706014ac777ff406604
Image SHA-256: bd0be5201aa20655b2885a39b017070523bb8c44f275d837a200c949179fb544

Only synthetic files: /config0.dat (DSR1 CRC record with minimal JSON
{"schema":1,"revision":9}; record integrity only, not a valid full C model)
and /large.bin (bytes(range(256)) repeated 200 times, 51200 bytes).
No owner data, credentials or firmware. This fixed upstream-generated fixture
checks the offline reader independently of our hand-built corrupt-page tests,
including slash-prefixed VFS paths and continuation indexes. Never flash it.

## Original IDF wrapper/core integration

`python3 tools/test_idf_spiffs.py --ubsan-only` compiles the real project's
`dsr_storage.c` with **unmodified** ESP-IDF `esp_spiffs.c`, `spiffs_api.c`,
`spiffs_config.h` and the SPIFFS core. It does not stub the mount/format APIs.
Dependencies must be clean at these exact revisions:

- ESP-IDF v5.5.4: `735507283d5b2f9fb363a1901172dbd9e847945d`.
- Its SPIFFS submodule: `0dbb3f71c5f6fae3747a9d935372773762baf852`.

Prepare them with the three git commands in the native CI job. The dependency
path defaults to `.test-deps/esp-idf`; override with `--idf-source PATH`.
CI uses `--sanitize` for ASan/UBSan/LSan. Sanitizer errors fail the process.
Only upstream source disables the alignment sanitizer: the real IDF config
sets `SPIFFS_ALIGNED_OBJECT_INDEX_TABLES=0`, so its index arrays intentionally
have odd addresses. All sanitizer checks remain enabled for project/test code.

The host shims supply bounded NOR reads, 1-to-0 writes, sector erases, a
single-thread mutex and VFS operation registration. They do not replace the
filesystem. Page/block/name/meta sizes, caches and length magic match IDF
defaults. Directory VFS and file timestamp handling are disabled because this
host does not use newlib's DIR/time ABI; the 4-byte disk metadata remains.
This is not full libc-VFS, ESP32 allocation/scheduling, hardware, or power-cut
coverage.

Scenarios: fully erased 0x3E0000 partition provisions; a real file survives ten
unmount/remount/read cycles with no additional write/erase; a single unknown byte
at either end blocks formatting and preserves all bytes; read and format I/O
failure release the upstream mutex/context and report the correct health phase;
a later retry succeeds. The fixed generator image above also mounts with the
actual core: `/config0.dat` and all 51200 bytes of `/large.bin` are read exactly,
with no writes or erases. Fixtures contain no owner data. These checks do not
establish that an unknown damaged owner filesystem is recoverable.
A mismatched prior 4 MiB length-magic fixture also refuses mount/format and
preserves every byte; its expected partition here remains 0x3E0000.

0.7.7 strengthens this to exactly 992 sector erases on a full blank format,
instead of the former success-only assertion (which allowed two passes).
Every erase must occur with the startup priority temporarily at idle; the
previous priority must be restored on success/error. These are API-policy
checks, not a simulated scheduler or measured watchdog timing.
A synthetic 313-marker prefix must remain byte-identical on refused mount.
An actual upstream format with injected write/erase failure after sector 313
must produce that same prefix, report failure, release resources, restore
priority and preserve the prefix on a subsequent boot/mount. This is I/O fault
injection; the owner's original interruption cause remains unproven.
