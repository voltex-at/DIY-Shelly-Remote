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
