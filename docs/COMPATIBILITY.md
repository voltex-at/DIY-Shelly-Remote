# Compatibility

LCDWiki ES3C35P now has a documented source-level hardware-test port. Its physical validation is pending. No Shelly model has been tested with this project yet.

| Category | Source implementation | Confirmed on hardware |
| --- | --- | --- |
| Gen1 HTTP relay | Selected channel read/explicit ON/OFF | No |
| Gen2+ Switch component | Selected channel read/explicit ON/OFF, optional `apower` | No |
| Authenticated devices | Recognized as authentication required; control not implemented | No |
| Dimmer, RGB/RGBW, light | Not implemented | No |
| Cover/roller | Not implemented | No |
| Environmental sensors and energy meters | Not implemented | No |
| Shelly BLU/BTHome | Not implemented | No |
| Device-specific BLE RPC | Not implemented | No |

Do not translate a protocol implementation into a blanket model support claim. Add verified rows with exact model, hardware revision, firmware version, transport, component/channel, authentication mode, test date and logs. A model's capability can vary with device profile and firmware.
