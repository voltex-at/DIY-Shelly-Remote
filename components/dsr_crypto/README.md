# Pinned Monocypher Ed25519 dependency

Upstream: https://github.com/LoupVaillant/Monocypher

Version **4.0.3**, exact commit
`ab2b16dd619ad5f6979a4fbe69cfa324a6fcc35f` (upstream tag `4.0.3`).
The five files under `vendor/` are byte-for-byte upstream files. Source paths
are `src/monocypher.c`, `src/monocypher.h`,
`src/optional/monocypher-ed25519.c`, `src/optional/monocypher-ed25519.h`,
and `LICENCE.md`; only the `src/` prefix was removed locally.

License: **BSD-2-Clause OR CC0-1.0**, retained in each source and in
`vendor/LICENCE.md`. Version 4.0.3 includes upstream's signature timing fix;
do not replace it with 4.0.2. No production signing key or trust anchor is
provided by this component.

Use `crypto_ed25519_check(signature64, public_key32, message, length)` from
`monocypher-ed25519.h`: zero means valid, minus one means invalid. This is
RFC 8032 Ed25519 with SHA-512. Core `crypto_eddsa_check` uses BLAKE2b, and the
`_ph_check` API is Ed25519ph; neither matches the manifest algorithm.
The caller must validate fixed input lengths before invoking this API.

Verification requires no allocation, RNG, radio, Flash write, task or global
initialization. Function/data sections permit unused vendor routines to be
removed by the firmware linker. Exact target code size and peak stack remain
target-build/hardware evidence; source size is not the linked footprint.

Upstream verification checks S < L and uses the cofactored verification
equation, allowing noncanonical A/R point encodings. Do not claim libsodium's
strict point-rejection semantics. Trust anchors must be independently reviewed
valid Ed25519 public keys, never keys supplied by a manifest. Empty production
trust stores must reject all manifests; RFC keys belong only to tests.

The host test uses RFC 8032 section 7.1 vectors 1–3, tampering, message
truncation/extension and S+L rejection. It neither generates keys nor signs.

SHA-256 checksums:

```
f1f838cdd483bdebe0df0ff5c5ed60535e496f769c6a2f933ac4c0b114207123  vendor/monocypher.c
fcaf6ed771358bb4f40fba016f6518ae86ec02b1b877d2cc35ad92d3a26fd7b3  vendor/monocypher.h
ce0d2f8e32ca8f66398ba5b3456cc74327c3eff14e7b950ce7d57be9025cc453  vendor/optional/monocypher-ed25519.c
3a3035181f991a158d0e1c7567258f0bae8ba0f1f23c5512b4a1db1b3c9730ce  vendor/optional/monocypher-ed25519.h
a5781770269d2516e52ba4863f790c10a16da4089a1e81823aee19ff1e9026b0  vendor/LICENCE.md
```

The Git blob IDs also match the pinned upstream tree, in the same order:
`c7c5450e84e4dffda44514d9f918162f1830de56`,
`cf635e88e4f5851e1f58a9e2b3e85b8029a73861`,
`4fdd07641e1c4d54a348e95d0ab05c28f84654a6`,
`d7aa004123e1d38b891fdabd4171fbf9957242ae`,
`9d69ad72ec113a743264d34dd86a5c04d0c42d35`.
