# LCD review frames

When a UI change needs verification, render the actual C/LVGL 9.3.0 sources
at 320×480 using the relevant case in `tools/test_ui.py`. Inspect only the
affected frames during iteration; a full atlas belongs at the end of a major
UI stage. See [DEVELOPMENT.md](../DEVELOPMENT.md).

Generated captures belong under ignored `build-ui/`, not in source commits.
Historical committed baselines, screenshots, ZIPs and test-output reports were
removed on 2026-10-22 at the owner's request. Source fixtures and generators
remain available. A new export must be generated for the exact source being
reviewed and is host evidence, not physical-device acceptance.
