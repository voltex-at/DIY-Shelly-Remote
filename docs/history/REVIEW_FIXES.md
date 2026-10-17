# PR #5: save acknowledgement, photo storage and translations

Source version: `0.5.1-review-fixes`. This draft is stacked on the visual and
LAN PRs; it does not authorize merging them to main. Drivers, Shelly transport,
NVS, OTA slots and the filesystem partition layout are retained.

## Save regression

Previously the editor could submit A, accept further edits B, and describe B
as saved when only A had reached Flash. Both editors now retain submitted A,
verify the published document rather than revision alone, and keep B unsaved
against the acknowledged revision. A failed write retains B; another Save
during the first request preserves its tracking. Concurrent external changes
require reloading rather than silently overwriting the other editor.

The native regression holds the real shared-model commit, changes A to B,
attempts another Save, publishes A, fails a B write, then retries B successfully.
The browser regression delays a real C checkpoint write/readback response while
the user changes the editable name to B, then verifies durable A and later B.
The test delay and radio providers exist only in the host test harness.

## Photo reclamation

Photo cache pointers remain immutable during a boot. After loading a verified
checkpoint, startup refreshes the older checkpoint with that document using
the same write/fsync/readback path. A power cut leaves the previously newest
record available. Cleanup then retains the union of photo IDs in both valid
checkpoints and unlinks only exact `p<32 lowercase hex>.rgb` filenames that
neither references. Present corrupt, inaccessible or future-schema checkpoints
disable deletion. Unknown files and configuration records are untouched.

Save removed/replaced photo references, then restart the remote to reclaim
unused Flash and the 16 immutable cache slots. Uncommitted uploads are also
removed on restart. Upload errors explain this requirement. No live pointer is
freed; no filesystem formatting, SD implementation or OTA partition change is
introduced. Disk tests cover both retained photo references, 16 orphan files,
corruption/future-schema refusal and 32 subsequent cleanup cycles.

## Reproduce host checks

```sh
python3 tools/test.py --sanitize
python3 tools/test_config.py --sanitize
python3 tools/test_ui.py --sanitize
python3 tools/test_ui.py --review --sanitize
python3 tools/render_review.py --no-run
```

The existing CI also runs the actual shipped panel in Chrome and compiles the
ES3C35P firmware plus both diagnostic targets. Compilation and host rendering
do not establish physical Flash, Wi-Fi, Shelly or display operation.

## Physical acceptance — pending

Record source commit, board and Shelly model/firmware; never record passwords.

1. On touch, edit room name A, Save, then change to B immediately. If saving
   finishes before B, B must become unsaved on the next UI tick. If B is entered
   during the write, only A may be acknowledged. Reopen from another paired
   browser and confirm durable A; Save B and reboot to confirm B persists.
2. Repeat in the panel while keeping touch HOME visible. Interrupt connectivity
   during acknowledgement; the browser must retain the draft and must not
   claim all changes saved without a matching durable response.
3. Upload/replace photos, save, reboot, and repeat beyond 16 distinct images.
   Referenced photos must survive. Removed photos must release slots and
   Flash after reboot. Confirm HOME and preview use the remaining images.
4. Test PL/EN/DE in editor, appearance, Wi-Fi and LAN wizard. Check Polish
   diacritics, keyboard symbols and translated pending/failure/unsaved states.
5. Run the original color, touch, brightness and wake checks, and the LAN and
   OTA acceptance procedures in the other guides. Photograph physical HOME.

Hardware power-cut/corruption experiments require a recoverable USB setup and
backed-up configuration. No such result is claimed in this change.
