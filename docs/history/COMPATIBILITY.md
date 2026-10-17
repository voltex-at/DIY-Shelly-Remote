# Compatibility

LCDWiki ES3C35P now has a documented source-level hardware-test port. The user confirmed boot/display rendering and touch responses with photographs of 0.1.0-bringup; full physical validation is incomplete. No Shelly model has been tested with this project yet.

| Category | Source implementation | Confirmed on hardware |
| --- | --- | --- |
| Gen1 HTTP relay | Selected channel read/explicit ON/OFF | No |
| Gen2+ Switch component | Selected channel read/explicit ON/OFF, optional `apower` | No |
| Authenticated devices | Gen1 Basic / Gen2+ IDF Digest source and fixtures; actual exchange pending | No |
| Dimmer, RGB/RGBW, light | Not implemented | No |
| Cover/roller | Not implemented | No |
| Environmental sensors and energy meters | Not implemented | No |
| Shelly BLU/BTHome | Not implemented | No |
| Gen2+ open-window BLE RPC → Wi-Fi → HTTP LAN relay | DEV B1, two LCD consents; no bonding/TLS-only | Physical acceptance pending; see [scope](BLE_RPC_B1_PL.md) |

Do not translate a protocol implementation into a blanket model support claim. Add verified rows with exact model, hardware revision, firmware version, transport, component/channel, authentication mode, test date and logs. A model's capability can vary with device profile and firmware.
