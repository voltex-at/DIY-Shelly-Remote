#!/usr/bin/env python3
"""Current docs must match the shipped trust anchor and the CMake version.

Fails when a maintained document (docs/*.md, README.md, board README) still
claims an empty/missing production OTA trust store while dsr_ota_trust.c holds
an approved key, names another key id, or when RELEASE_NOTES/README disagree
with PROJECT_VER. docs/history/ is an archive and is not checked."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
trust = (ROOT / 'components/dsr_ota/dsr_ota_trust.c').read_text()
key_id = re.search(r'#define DSR_OTA_KEY_ID "([^"]+)"', trust)
has_key = bool(key_id and re.search(r'production_key\[32\]=\{0x', trust))
version = re.search(r'set\(PROJECT_VER "([^"]+)"\)', (ROOT / 'CMakeLists.txt').read_text()).group(1)
docs = sorted((ROOT / 'docs').glob('*.md')) + [ROOT / 'README.md', ROOT / 'boards/lcdwiki-es3c35p/README.md']
stale = re.compile(r'truststore\s+(?:jest\s+|nadal\s+)?(?:EMPTY|pusty)|trust store remains empty|EMPTY truststore|pusty(?:m)? (?:produkcyjny|publisher) truststore|Production trust EMPTY|production trust EMPTY', re.I)
errors = []
for doc in docs:
    text = doc.read_text()
    for number, line in enumerate(text.splitlines(), 1):
        if has_key and stale.search(line):
            errors.append(f'{doc.relative_to(ROOT)}:{number}: claims an empty trust store, firmware trusts {key_id.group(1)}')
        for other in re.findall(r'voltex-ota-\d{4}-\d+', line):
            if key_id and other != key_id.group(1):
                errors.append(f'{doc.relative_to(ROOT)}:{number}: key id {other} != {key_id.group(1)}')
notes = (ROOT / 'docs/RELEASE_NOTES.md').read_text()
if not notes.startswith(version + ':'):
    errors.append(f'docs/RELEASE_NOTES.md must start with "{version}:"')
if f'`{version}`' not in (ROOT / 'README.md').read_text():
    errors.append(f'README.md does not name the development version {version}')
if errors:
    sys.exit('\n'.join(errors))
print(f'PASS: {len(docs)} maintained docs consistent with trust anchor {key_id.group(1) if key_id else "none"} and version {version}')
