# GitHub and releases

Repository: https://github.com/voltex-at/DIY-Shelly-Remote

Foundation sources were published through PR #1 and merged after successful native and ESP-IDF diagnostic checks. Larger changes continue through focused branches and pull requests. Never store tokens, passwords or private signing keys in Git.

## Foundation CI

`ci.yml` runs portable tests and ESP-IDF v5.5.4 compile coverage for ESP32/ESP32-S3. Diagnostic targets do not support either physical board by implication. Artifacts are diagnostic ELF/map files only. Check real CI results before marking this foundation target-compiled.

## Firmware release gate (future)

No automatic release workflow is enabled in the foundation. Add one only with a confirmed board profile and verified signing method. It must build a specific commit with pinned/resolved dependencies, run tests, bind the hardware ID, calculate image size/digest, sign the manifest/image, preserve third-party notices and create a **draft/prerelease** for human hardware validation.

Production assets must distinguish the USB first-install image/layout from the OTA app-only image. Attach board/MCU/revision, version, checksums, signatures, source commit, build instructions, release notes and tested-model matrix. Never upload diagnostic builds as installable firmware. Never promote a release to stable automatically before physical validation.

Keep signing private keys in an appropriate external signing service or protected release secrets, never in source. Embed only trusted public verification keys in the released firmware. Signing/key rotation and repository authenticity need a documented review before OTA is enabled.
