# PR #33 acceptance checkpoint — 2026-10-16

Scope remains frozen. This follow-up adds verification and records current
evidence; it changes no firmware source, version, board settings or partitions.
The owner is testing **0.7.6 / source ecdac4ee0d3332a4ee1a4ad4f0d3df664b1d4818**.
Keep that build identity when reporting the physical result, even after this
test-only PR update. Do not infer an app binary hash from a source commit or
from the boot log's ELF hash.

## Evidence independently checked

- [Run 38831522391](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38831522391)
  identifies the exact source above. All five jobs succeeded: ES3C35P, ESP32 and
  ESP32-S3 diagnostics, native ASan/UBSan, real LVGL/browser review. Native job
  logs confirm the storage/startup scenarios and nine offline recovery tests.
- Downloaded firmware artifact `12017900282` passed ZIP CRC and its published
  SHA-256 `da8b2e94b43444567b2b386ccbdcb639f59c5ff0cf3e085d8a1f743b50bec1dd`.
  BUILD_COMMIT identifies source ecdac4e / tested merge fefff790. The app is
  ESP32-S3, version `0.7.6-storage-diagnostics-dev`, 1658224 B, SHA-256
  `1e8b0d52f1aeaa778073c5902e1bd382cb60e461d10048086693baad0553975e`;
  embedded ELF hash `e6ac16e78d26a1a89e294da9f3d7b935973b6a540ab7ceb4d26dd6013893fe71`.
  The merged 1789296 B image has SHA-256
  `fc51e09422bdd8a72ee8f622c239af22df67cc903c3376828ce3641e58df5fc6`;
  its app bytes at 0x20000 match exactly. Flasher offsets are 0x0/0x8000/
  0x19000/0x20000. BOOT_MEMORY records 131072 B LVGL pool in PSRAM and 8192 B
  UI stack in internal RAM, with no physical-boot claim.
- Local repeat with UBSan: core 94, platform 847, Wi-Fi 222, BLE 2138,
  boot/storage 15948, startup 163, workspace 106 checks across seven processes,
  config 265, and all nine offline tests. Real LVGL: 1627 checks / 73 frames;
  review: 2635 / 24. Room edit/back at eight rooms: warmed free 66872 -> 66872 B.
  The local sandbox cannot run LeakSanitizer; full sanitizer evidence is CI.
- New [original IDF/SPIFFS host integration](../tests/storage/README.md) passes
  locally with UBSan (upstream unaligned-index exception documented there).
  It exercises actual mount/format on simulated NOR, file persistence over ten
  remounts, refusal to format unknown bytes, I/O failures and the independent
  IDF-generator image. CI now repeats it with ASan/UBSan/LSan.

The original IDF 5.5.4 `esp_spiffs_format()` supports an unmounted partition:
it creates temporary filesystem structures and frees them after formatting.
The project's blank-only call sequence is therefore supported and passes this
integration test. This is not a diagnosis of the owner's unknown raw bytes or
a substitute for running the exact build on ESP32-S3.

## Physical acceptance still open

The owner's 0.7.4 log establishes SPIFFS_ERR_NOT_A_FS and the unavailable
workspace/HTTPS chain, not the origin of the incompatible bytes. The owner has
since authorized **one full Flash erase on their own board** and is testing a
clean 0.7.6 installation. This supersedes waiting for a private backup as a
prerequisite for that particular authorized clean test; it does not authorize
automatic erasure, repeated erase attempts, another board, main merge or Stable.

Required result: version plus exact app/ELF identity and redacted boot log;
SPIFFS ready, workspace ready and HTTPS state; HOME pencil opens the editor;
Rooms Add -> Salon -> Save -> reset -> Salon retained; second room, edit/delete
and reference protection; Web pairing/HTTPS/CAS; Wi-Fi/LAN/ON-OFF/BLE regression.
The [existing room](ROOM_EDITOR_PL.md) and [storage](STORAGE_RECOVERY_PL.md)
procedures supply the detailed cases. Keep all private videos, RF/network
identifiers, pairing codes and Flash dumps out of the public repository.

If fully erased Flash still fails to provision, preserve that new log and
diagnose the first-provisioning path. Do not repeat erase in a loop. Host NOR
does not establish actual USB writes, partition contents, power stability,
FreeRTOS/heap behavior, RF coexistence, physical touch or TLS behavior.

## Existing queue and dependencies

| Work | Current gate / next evidence |
| --- | --- |
| #28 / PR #33 storage and contextual Rooms | Physical result above; both review threads stay open until verified |
| PR #31 startup recovery | Automated forced-failure coverage passes; physical failure/OTA rollback still required |
| #25 feature audit | Matrix updated with exact CI and this checkpoint; global quality gate remains open |
| #29A / PR #34 settings, #20 multi-device | Existing work preserved; dependent hardware/feature progression blocked by #28 |
| #19 AP, #18 further BLE, #22 history, #23 adapters, #24 signed native OTA, #32 wizard | Already open backlog; separate bounded changes and their model/security/physical gates remain required |

No issue is closed, no PR approved/merged and no Stable release is authorized
by this checkpoint. The current test-only follow-up needs its own CI result;
the earlier green run applies to ecdac4e, not automatically to a later commit.
