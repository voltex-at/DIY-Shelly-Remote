# DIY Shelly Remote

Unofficial open-source touchscreen remote for Shelly devices on the
**LCDWiki ES3C35P N16R8** (ESP32-S3, 320×480). Fully local: Wi-Fi LAN and
Bluetooth LE, no cloud, no CDN. Not affiliated with or endorsed by Shelly.

Development version: `0.7.74-dev` (continuation on `work/dev-0.7.70`).
There is no Stable release yet; `main` holds an older baseline.

Current inventory and remaining work: [PROJECT_STATUS_PL.md](docs/PROJECT_STATUS_PL.md) ·
[one consolidated issue #74](https://github.com/voltex-at/DIY-Shelly-Remote/issues/74).

## Status

| Area | Code | CI | Physical (ES3C35P) |
| --- | --- | --- | --- |
| Boot, display, touch, backlight | yes | yes | confirmed by owner (0.1.0) |
| Wi-Fi + one Shelly switch ON/OFF | yes | yes | confirmed by owner (0.6.x) |
| Multiple Shelly (up to 16), rooms, editor, Web Panel | yes | yes | in test, defects reported 0.7.23 |
| SPIFFS first format with LCD progress | yes | yes | format passed (0.7.37); resume after reset pending |
| BLE scan and Gen2+ onboarding over BLE | yes | yes | pending |
| Covers | yes | yes | control confirmed (0.7.37) |
| Light/Dimmer, RGBW, H&T, groups/scenes (energy history removed in 0.7.51) | yes | yes | pending |
| Shelly BLU Motion/Door/Window, read through a Shelly gateway on LAN | yes (0.7.42) | yes | pending |
| Battery calibration, estimated SOC and optional voltage-trend charging animation | yes (0.7.67-dev) | native/LVGL; physical pending | pending |
| Shelly Gen2/3/4 settings on LCD; BLU bonding/PIN, GATT settings and gateway registration | yes (since 0.7.55-dev) | yes | pending |
| OTA from the device: catalogue, manifest, signature, version policy | yes | yes | 0.7.53-dev installed successfully (owner, 2026-10-18); rollback pending |

Battery setup: **Settings → Battery and power → Battery calibration**. Cell type, nominal voltage, capacity, 0/50/100% anchors and multimeter correction are stored separately. `~%` and the voltage-trend animation are estimates; they do not confirm charging or battery presence. [Calibration details](docs/BATTERY_CALIBRATION_PL.md).

Details per feature: [docs/FEATURE_MATRIX.md](docs/FEATURE_MATRIX.md).
A green CI run is not a physical confirmation.

## Install

- First installation over USB (erases data): [INSTALL_FACTORY_PL.md](docs/INSTALL_FACTORY_PL.md), image at **0x0**.
- Later updates: from the remote (Settings → Firmware Update) or [INSTALL_UPDATE_PL.md](docs/INSTALL_UPDATE_PL.md).
- Never use Erase All as a default step; see [STORAGE_RECOVERY_PL.md](docs/STORAGE_RECOVERY_PL.md).

## Development

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) and the [docs index](docs/README.md).

```sh
sudo apt-get install libcjson-dev libssl-dev
python3 tools/test.py --sanitize
```

Original project code is [MIT](LICENSE). Third-party components keep their own
licenses, see [NOTICE.md](NOTICE.md). No official Shelly code, graphics or logos
are included.
