# UI direction

Reference reviewed: [Shelly Smart Control](https://www.shelly.com/pages/shelly-app) and the [My Home guide](https://kb.shelly.cloud/knowledge-base/my-home-page), 2026-10-15. The official material organizes devices by rooms, presents device states and allows scenes and consumption views. This informs navigation; it does not imply pixel-for-pixel equivalence to any app version. No official artwork or logo will be copied.

The first LVGL UI is implemented for the documented 320x480 native portrait display. It has an original dark Home/Menu shell, clearly labeled hardware-test scope, four-corner touch test, primary-color test, brightness, full backlight off and first-touch wake. Rooms, Devices, Scenes and Energy are navigation shells with explicit empty/future states, not completed control features. No fabricated devices or measurements are shown. Physical rendering has not been validated yet. The table below describes the full intended UI beyond this bring-up milestone.

| View | Required content | Interaction rules |
| --- | --- | --- |
| Home | Favorites, quick actions, scenes, rooms, selected readings | Only real known states; loading state while commands are pending |
| Rooms | Create/rename rooms, membership, room devices | Local persistent configuration; confirm destructive edits |
| Devices | Saved/discovered Wi-Fi and BLE devices, manual IPv4 add, details | Label discovery versus saved records and control versus passive data |
| Device details | Name, room, connectivity, capability-driven controls and metrics | Only expose actual switch/light/cover capabilities |
| Scenes | Local multi-device actions, create/edit/run | Report partial failures per device; no atomicity claims |
| Energy | Available power/consumption, bounded local history | Distinguish instantaneous W from accumulated Wh; show gaps and time validity |
| Settings | Wi-Fi, BLE, display, sleep/wake, configuration and system | UI remains responsive during network operations |
| System / Firmware Update | Current/latest version, notes, check/install/progress/errors | User confirmation; reject incompatible or unauthenticated images |

Use a short navigation bar or menu that preserves room/device context instead of squeezing six tiny buttons into an unknown width. The first UI acceptance test includes long Polish device names and scrolling. Device actions use explicit desired values. A demo data source, if added, must be visibly labeled DEMO and never mixed with live devices.

Sleep/wake must include a fully dark backlight mode where the hardware supports it. The first touch wakes the display without also activating a hidden switch. LED and charge indications require real hardware signals; no heuristic is assumed at this checkpoint.
