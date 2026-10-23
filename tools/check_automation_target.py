#!/usr/bin/env python3
"""Audit actual Xtensa .su frames; this is not a physical stack high-water mark."""
import argparse
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build', type=Path, required=True)
a = p.parse_args()
limits = {
 ('dsr_automation_model.c', 'dsr_auto_configure'): 768,
 ('dsr_automation.c', 'dsr_automation_tick'): 768,
 ('dsr_automation.c', 'dsr_automation_snapshot'): 1024,
 ('dsr_automation_api.c', 'dsr_automation_status_json'): 1024,
 ('dsr_ui_automation.c', 'draw'): 1024,
}
for (source, name), limit in limits.items():
 files = list(a.build.rglob(source + '.su'))
 assert len(files) == 1, (source, files)
 frames = []
 for line in files[0].read_text().splitlines():
  where, size, kind = line.split('\t')
  if where.rsplit(':', 1)[-1].replace('$', '.').split('.')[0] == name:
   assert kind == 'static', line
   frames.append(int(size))
 assert frames, (source, name, 'missing frame')
 assert max(frames) <= limit, (source, name, max(frames), limit)
 print(f'{source}:{name}: {max(frames)} B <= {limit} B')
print('PASS: automation target frame budgets; physical high-water mark still required')
