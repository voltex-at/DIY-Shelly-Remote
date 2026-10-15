# UI direction

Reference reviewed: [Shelly Smart Control](https://www.shelly.com/pages/shelly-app) and the [My Home guide](https://kb.shelly.cloud/knowledge-base/my-home-page), 2026-10-15. The official material organizes devices by rooms, presents device states and allows scenes and consumption views. This informs navigation; it does not imply pixel-for-pixel equivalence to any app version. No official artwork or logo will be copied.

The current 320x480 portrait LVGL UI has Home (one saved switch, network/state, explicit ON/OFF, Refresh, optional actual `apower`), Menu, on-device Wi-Fi entry with Save & restart, and manual Shelly IP/channel/name/login/password forms. Keyboard Ready/Cancel hides the keyboard; the forms scroll. Password fields are masked and byte lengths checked. A bounded worker handles network/NVS writes and returns events to the single LVGL owner; no background task manipulates widgets. View pointers are cleared before objects are deleted.

The previous bring-up UI was photographed working on the user's board, including touch responses. New forms and network/control behavior still need physical validation. Hardware corner/color tests and brightness/off/wake tools remain in Menu. Rooms, scenes, multiple devices, energy history, BLE and OTA installation are not yet available. The table below describes the broader intended UI.

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
