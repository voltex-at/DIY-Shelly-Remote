#!/usr/bin/env python3
"""Decision tests for tools/release_tool.py: CI gate, moved tag, Stable approval,
immutable assets and duplicate publication. No network, no GitHub writes."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import release_tool as t  # noqa: E402

checks = 0


def ok(value):
    global checks
    checks += 1
    assert value


def refused(function, *args):
    global checks
    checks += 1
    try:
        function(*args)
    except t.Refused:
        return
    raise AssertionError(f'{function.__name__}{args} was not refused')


SHA, OTHER = 'a' * 40, 'b' * 40
JOBS = [{'name': n, 'conclusion': 'success'} for n in t.CI_JOBS]


def run(id_, sha=SHA, status='completed', conclusion='success', number=1):
    return {'id': id_, 'head_sha': sha, 'status': status, 'conclusion': conclusion, 'run_number': number}


# Tag grammar / channel
ok(t.channel('v1.2.3') == 'stable' and t.channel('dev-v0.7.39') == 'dev')
for bad in ['v1.2', 'dev-v01.2.3', 'v1.2.3-rc1', 'release-1', '', None]:
    refused(t.channel, bad)

# One requested DEV version; wrong ref, tag, CMake, extra/missing fields refuse.
cmake = 'set(PROJECT_VER "0.7.54-dev")'
t.dev_request_allowed('dev-v0.7.54', dict(t.DEV_REQUEST), t.DEV_REQUEST['branch'], cmake)
for tag, ref, text in [('v0.7.54', t.DEV_REQUEST['branch'], cmake),
                       ('dev-v0.7.55', t.DEV_REQUEST['branch'], cmake),
                       ('dev-v0.7.54', 'refs/heads/main', cmake),
                       ('dev-v0.7.54', t.DEV_REQUEST['branch'], 'set(PROJECT_VER "0.7.52-dev")'),
                       ('dev-v0.7.54', t.DEV_REQUEST['branch'], '')]:
    refused(t.dev_request_allowed, tag, dict(t.DEV_REQUEST), ref, text)
for request in [{}, dict(t.DEV_REQUEST, extra=True), dict(t.DEV_REQUEST, schema=2)]:
    refused(t.dev_request_allowed, 'dev-v0.7.54', request, t.DEV_REQUEST['branch'], cmake)

# CI gate: exact SHA, 5/5 jobs, newest successful run wins, active run waits
ok(t.ci_verdict(SHA, [run(1)], {1: JOBS}) == 'ok')
refused(t.ci_verdict, SHA, [], {})                                        # missing CI
refused(t.ci_verdict, SHA, [run(1, sha=OTHER)], {1: JOBS})                # stale: other commit only
refused(t.ci_verdict, SHA, [run(1, conclusion='failure')], {1: JOBS})     # failed run
refused(t.ci_verdict, SHA, [run(1)], {1: JOBS[:4]})                       # a job missing
refused(t.ci_verdict, SHA, [run(1)], {1: JOBS[:4] + [{'name': JOBS[4]['name'], 'conclusion': 'skipped'}]})
ok(t.ci_verdict(SHA, [run(2, status='in_progress', number=2), run(1)], {1: JOBS}) == 'wait')
ok(t.ci_verdict(SHA, [run(2, conclusion='failure', number=2), run(1)], {1: JOBS}) == 'ok')

# Moved tag
t.tag_unchanged(SHA, SHA)
refused(t.tag_unchanged, SHA, OTHER)
refused(t.tag_unchanged, SHA, None)

# Stable approval environment
t.stable_approval({'protection_rules': [{'type': 'required_reviewers', 'reviewers': [{'type': 'User'}]}]})
for env in [None, {}, {'protection_rules': []}, {'protection_rules': [{'type': 'wait_timer', 'wait_timer': 5}]},
            {'protection_rules': [{'type': 'required_reviewers', 'reviewers': []}]}]:
    refused(t.stable_approval, env)

# Publication plan
LOCAL = {'diy_shelly_remote.bin': '1' * 64, 'DIY-Shelly-Remote-es3c35p-manifest.json': '2' * 64, 'SHA256SUMS.txt': '3' * 64}


def asset(name, sha=None, state='uploaded'):
    return {'name': name, 'state': state, 'digest': 'sha256:' + (sha or LOCAL[name])}


def release(assets=(), draft=False, prerelease=True):
    return {'id': 7, 'draft': draft, 'prerelease': prerelease, 'assets': list(assets)}


p = t.plan('dev-v0.7.39', None, LOCAL)                                    # pushed tag, no release yet
ok(p['create'] and p['publish'] and not p['hide'] and p['upload'] == sorted(LOCAL))
p = t.plan('dev-v0.7.39', release(), LOCAL)                               # published empty in the UI
ok(not p['create'] and p['hide'] and p['publish'] and p['upload'] == sorted(LOCAL))
p = t.plan('dev-v0.7.39', release(draft=True), LOCAL)                     # hidden by the gate
ok(not p['hide'] and p['publish'] and p['upload'] == sorted(LOCAL))
p = t.plan('dev-v0.7.39', release([asset(n) for n in LOCAL]), LOCAL)      # duplicate event / re-run, same bytes
ok(p == dict(create=False, hide=False, upload=[], publish=False))
p = t.plan('dev-v0.7.39', release([asset('diy_shelly_remote.bin')], draft=True), LOCAL)  # interrupted upload resumes
ok(p['upload'] == sorted(set(LOCAL) - {'diy_shelly_remote.bin'}) and p['publish'])
refused(t.plan, 'dev-v0.7.39', release([asset('diy_shelly_remote.bin', 'f' * 64)]), LOCAL)  # rebuild with other bytes
refused(t.plan, 'dev-v0.7.39', release([asset('evil.bin', 'f' * 64)]), LOCAL)               # foreign asset
refused(t.plan, 'dev-v0.7.39', release([asset('diy_shelly_remote.bin', state='starter')]), LOCAL)
refused(t.plan, 'dev-v0.7.39', release([{'name': 'diy_shelly_remote.bin', 'state': 'uploaded'}]), LOCAL)
refused(t.plan, 'dev-v0.7.39', release(prerelease=False), LOCAL)         # dev tag as Stable release
refused(t.plan, 'v0.8.0', release(prerelease=True), LOCAL)               # Stable tag as prerelease

# Read-back verification before the release becomes visible
t.verify_uploaded(release([asset(n) for n in LOCAL]), LOCAL)
refused(t.verify_uploaded, release([asset(n) for n in list(LOCAL)[:2]]), LOCAL)
refused(t.verify_uploaded, release([asset(n, 'e' * 64) for n in LOCAL]), LOCAL)

# CLI ordering with a fake GitHub transport: failed exact CI cannot create a
# tag; passed CI precedes tag creation and immutable readback. No network I/O.
from argparse import Namespace
from tempfile import TemporaryDirectory
from unittest.mock import patch
import json
import os
from pathlib import Path
with TemporaryDirectory(prefix='dsr-dev-request-') as tmp:
    request = Path(tmp) / 'request.json'
    request.write_text(json.dumps(t.DEV_REQUEST))
    args = Namespace(repo='fixture/repo', tag='dev-v0.7.54', sha=SHA, dev_request=str(request), wait_minutes=1)
    for conclusion in ('failure', 'success'):
        events = []
        state = {'tag': None}
        def fake_gh(url, *verb, data=None):
            events.append(url)
            if '/git/ref/tags/' in url:
                return {'object': {'type': 'commit', 'sha': state['tag']}} if state['tag'] else None
            if '/actions/workflows/' in url:
                return {'workflow_runs': [run(1, conclusion=conclusion)]}
            if '/actions/runs/' in url:
                return {'jobs': JOBS}
            if url.endswith('/git/refs'):
                ok(data == {'ref': 'refs/tags/dev-v0.7.54', 'sha': SHA})
                state['tag'] = SHA
                return {}
            if '/releases?' in url:
                return []
            raise AssertionError(url)
        source_read = Path.read_text
        def request_read(path, *args, **kwargs):
            return 'set(PROJECT_VER "0.7.54-dev")' if str(path) == 'CMakeLists.txt' else source_read(path, *args, **kwargs)
        with patch.object(t, 'gh', fake_gh), patch.dict(os.environ, GITHUB_REF=t.DEV_REQUEST['branch']), patch.object(Path, 'read_text', request_read):
            if conclusion == 'failure':
                refused(t.cmd_gate, args)
                ok(state['tag'] is None and not any(x.endswith('/git/refs') for x in events))
            else:
                t.cmd_gate(args)
                ok(state['tag'] == SHA)
                ok(next(i for i, x in enumerate(events) if '/actions/runs/' in x) < events.index('repos/fixture/repo/git/refs'))

# A page with one release is still bounded by body + the API/asset envelope.
# Full historical notes reproduced 5/7/200/1 on the owner's 0.7.52 device.
import copy
repo = 'voltex-at/DIY-Shelly-Remote'
tag = 'dev-v0.7.53'
notes_text = '0.7.53-dev: porządek w P0, nazwy Shelly i kanały. Stable: brak.\n\n' + 'history 0.7.52\n' * 1000
short = t.compact_notes(repo, tag, notes_text)
ok(len(short.encode('utf-8')) <= 320 and 'history 0.7.52' not in short)
ok('/blob/dev-v0.7.53/docs/RELEASE_NOTES.md' in short)
wide = t.compact_notes(repo, tag, '0.7.53-dev: ' + 'ąü' * 500)
ok(len(wide.encode('utf-8')) <= 320 and '\ufffd' not in wide)
refused(t.compact_notes, repo, tag, '')
refused(t.compact_notes, repo, tag, '0.7.52-dev: wrong source')

names = ['DIY-Shelly-Remote-es3c35p-manifest.json', 'diy_shelly_remote.bin',
         'DIY-Shelly-Remote-es3c35p-FACTORY-CLEAN-16MB.zip',
         'DIY-Shelly-Remote-es3c35p-UPDATE-APP-ONLY.zip', 'SHA256SUMS.txt']
api_release = dict(id=416596352, tag_name=tag, draft=False, prerelease=True,
                   url=f'https://api.github.com/repos/{repo}/releases/416596352',
                   body=notes_text, assets=[dict(id=i+1, name=n, state='uploaded', size=511,
                       digest='sha256:' + str(i+1)*64, download_count=0,
                       browser_download_url=f'https://github.com/{repo}/releases/download/{tag}/{n}') for i,n in enumerate(names)],
                   api_metadata='x'*9500)
refused(t.catalogue_bytes, tag, api_release)
compact = dict(api_release, body=short)
ok(len(t.catalogue_bytes(tag, compact)) <= 16384)
t.verify_catalogue(tag, compact)  # Actual firmware C parser, same allocation caps.
stable = copy.deepcopy(compact);stable.update(tag_name='v0.7.53', prerelease=False)
for item in stable['assets']:
    item['browser_download_url'] = item['browser_download_url'].replace('/dev-v0.7.53/', '/v0.7.53/')
t.verify_catalogue('v0.7.53', stable)

request_data = dict(schema=1, tag=tag, release_id=api_release['id'], sha='4a4d69cb200f341162146c2b90d83135bbd1ea62',
                    old_body_sha256=t.hashlib.sha256(notes_text.encode()).hexdigest(),
                    assets={x['name']:x['digest'][7:] for x in api_release['assets']})
ok(t.repair_notes_allowed(request_data, api_release, short))
ok(not t.repair_notes_allowed(request_data, compact, short))  # Safe duplicate run.
for delta in [dict(draft=True),dict(prerelease=False),dict(id=12),dict(body='concurrent owner edit')]:
    refused(t.repair_notes_allowed, request_data, dict(api_release, **delta), short)
for key, value in [('schema',2),('tag','v0.7.53'),('sha','b'*40)]:
    refused(t.repair_notes_allowed, dict(request_data, **{key:value}), api_release, short)
changed = copy.deepcopy(api_release);changed['assets'][0]['digest']='sha256:'+'0'*64
refused(t.repair_notes_allowed, request_data, changed, short)
changed = copy.deepcopy(api_release);changed['assets'][0]['download_count']=50
ok(t.asset_fingerprint(changed)==t.asset_fingerprint(api_release))
changed['assets'][0]['size']+=1
ok(t.asset_fingerprint(changed)!=t.asset_fingerprint(api_release))

# The repair CLI may PATCH only body, after preflight. Its tag and all asset
# identities/bytes must survive readback; GitHub I/O is a deterministic model.
with TemporaryDirectory(prefix='dsr-repair-test-') as tmp:
    request_path = Path(tmp) / 'request.json'
    request_path.write_text(json.dumps(request_data))
    args = Namespace(repo=repo, request=str(request_path))
    state = {'release':copy.deepcopy(api_release)};events=[]
    def repair_gh(url, *verb, data=None):
        if '/git/ref/tags/' in url:
            return {'object':{'type':'commit','sha':request_data['sha']}}
        if '/releases?' in url:
            return [copy.deepcopy(state['release'])]
        if url.endswith('/releases/416596352'):
            if verb:
                ok(verb==('-X','PATCH') and data=={'body':short})
                ok(events==['preflight'])
                events.append('patch');state['release']['body']=data['body']
                state['release']['assets'][0]['download_count']+=1
            return copy.deepcopy(state['release'])
        raise AssertionError(url)
    original_read = Path.read_text
    def repair_read(path,*args,**kwargs):
        return notes_text if str(path)=='docs/RELEASE_NOTES.md' else original_read(path,*args,**kwargs)
    def check_projected(tag, release):
        t.catalogue_bytes(tag,release)
        events.append('preflight' if not events else 'readback')
    with patch.object(t,'gh',repair_gh), patch.object(t,'verify_catalogue',check_projected), patch.object(Path,'read_text',repair_read):
        t.cmd_repair_notes(args)
    ok(events==['preflight','patch','readback'])
    ok(t.asset_fingerprint(state['release'])==t.asset_fingerprint(api_release))

print(f'PASS: {checks} release gate/publication decisions; missing/failed/stale CI, moved tag, Stable approval, '
      'immutable assets and duplicate events refused or idempotent; no GitHub I/O')
