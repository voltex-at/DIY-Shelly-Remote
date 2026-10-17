# LCD review frames

Frames are rendered from the real C/LVGL 9.3.0 sources into an RGB565 buffer
(320 × 480), not mock-ups. They are not committed: get them from the CI artifact
`real-lvgl-ui-review-NOT-FIRMWARE` of the exact commit, or locally with
`python3 tools/test_ui.py --review` (output in `build-ui/review-export/`).
`render-manifest.json` lists the branch and the SHA-256 of every PNG.

`before-0.5.1/` and `before-0.6.1/` are the only committed images: fixed
baselines copied into the review ZIP for before/after comparison.
