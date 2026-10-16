# Host-only signed manifest fixtures

Run `python3 tools/test_ota.py --ubsan-only` (or `--sanitize` where ASan and
LeakSanitizer are available). Python's `cryptography` independently constructs
the documented signed bytes and produces genuine Ed25519 signatures using a
public RFC 8032 fixture seed. No production signing key is generated or stored.

The C fixture executes the production parser, policy and vendored Ed25519
implementation. Two separately linked binaries exercise a host-only trusted
fixture provider and the unchanged, empty production trust store. The fixture
provider is defined only in this test source behind `DSR_OTA_HOST_TEST_TRUST`;
neither the test source nor the Python signing tool belongs to an IDF component.

Cases cover independently signed stable/dev manifests, field ordering and
whitespace, compatibility/version/channel/URL/size policy, every signed field,
unknown keys, wrong signatures/keys, scalar malleability, strict base64,
duplicates/missing/unknown JSON members, embedded NUL/escapes/non-ASCII,
ambiguous numbers, trailing input, zero output on failure and every cJSON
allocation failure. These tests perform no networking, image installation,
Flash writes or physical rollback validation. Vendored cryptographic vector
tests are a separate check; this fixture tests the complete manifest boundary.
