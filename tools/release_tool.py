#!/usr/bin/env python3
"""Fail-closed gate and immutable publication for one signed GitHub Release.

gate     the tag still points at the built commit, `Firmware checks` passed
         5/5 for exactly that commit, the signing environment of the channel
         requires the owner's approval and admits only release tags, and a
         release opened in the UI is hidden (draft) until it is complete.
publish  uploads the signed set once. Existing assets are never replaced: an
         identical asset is accepted (idempotent re-run), a different one fails.
         The release is made visible only after every digest was read back.

Decision functions are pure and tested by tools/test_release_tool.py; the CLI
only adds `gh api` I/O.
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import time
import tempfile
from pathlib import Path

CI_WORKFLOW = 'ci.yml'
CI_JOBS = {'native', 'ui-review', 'diagnostic (esp32)', 'diagnostic (esp32s3)', 'es3c35p'}
STABLE_ENVIRONMENT = 'stable-release'
# The signing job of each channel runs in this environment (release.yml).
ENVIRONMENTS = {'dev': 'release', 'stable': STABLE_ENVIRONMENT}
# Deployment tag patterns an environment may admit, per channel.
TAG_POLICIES = {'dev': {'dev-v*', 'dev-v*.*.*'}, 'stable': {'v*', 'v*.*.*'}}
TAG = re.compile(r'^(?:v|dev-v)(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$')


class Refused(Exception):
    pass


def channel(tag):
    if not TAG.match(tag or ''):
        raise Refused(f'unsupported tag {tag!r}')
    return 'dev' if tag.startswith('dev-') else 'stable'


# Match the installed firmware's unchanged 16 KiB catalogue limit. A single
# release can overflow it too: full historical release notes are not a summary.
CATALOGUE_MAX = 16384
NOTES_MAX = 320


def compact_notes(repo, tag, text):
    kind = channel(tag)
    version = tag.removeprefix('dev-v').removeprefix('v') + ('-dev' if kind == 'dev' else '')
    title = text.splitlines()[0].strip() if text else ''
    if not title.startswith(version + ':'):
        raise Refused('release notes do not start with the tag version')
    link = f'\n\nPełna lista zmian i ograniczenia:\nhttps://github.com/{repo}/blob/{tag}/docs/RELEASE_NOTES.md'
    budget = NOTES_MAX - len(link.encode('utf-8'))
    if budget <= 0:
        raise Refused('release notes link exceeds native note budget')
    return title.encode('utf-8')[:budget].decode('utf-8', errors='ignore') + link


def catalogue_bytes(tag, release):
    projected = dict(release, draft=False)
    if release.get('draft'):
        # GitHub drafts may expose temporary untagged-* download routes. Only
        # project this known same-repo route to its public tag URL; a published
        # release is always checked byte-for-byte with its actual URLs.
        projected['assets'] = [dict(asset) for asset in release.get('assets', [])]
        prefix = 'https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/'
        for asset in projected['assets']:
            name = asset.get('name', '')
            url = asset.get('browser_download_url', '')
            if not re.fullmatch(r'[A-Za-z0-9_.-]+', name) or '..' in name:
                raise Refused('unsafe draft asset name')
            if re.fullmatch(re.escape(prefix) + r'untagged-[A-Za-z0-9_-]+/' + re.escape(name), url):
                asset['browser_download_url'] = prefix + tag + '/' + name
    payload = [projected] if channel(tag) == 'dev' else projected
    # Escaped Unicode + indentation is conservative compared with the API's
    # UTF-8 response. Do not increase the firmware's allocation or parser caps.
    data = json.dumps(payload, ensure_ascii=True, indent=2).encode('ascii')
    if len(data) > CATALOGUE_MAX:
        raise Refused(f'catalogue {len(data)} bytes exceeds firmware limit {CATALOGUE_MAX}')
    return data


def verify_catalogue(tag, release):
    data = catalogue_bytes(tag, release)
    with tempfile.TemporaryDirectory(prefix='dsr-catalogue-publication-') as tmp:
        path = Path(tmp) / 'release.json'
        path.write_bytes(data)
        subprocess.run([sys.executable, str(Path(__file__).with_name('check_release_catalogue.py')),
                        '--input', str(path), '--channel', channel(tag), '--tag', tag], check=True)


def asset_fingerprint(release):
    keys = ('id', 'name', 'size', 'digest', 'state', 'browser_download_url')
    return sorted(tuple(a.get(k) for k in keys) for a in release.get('assets', []))


def repair_notes_allowed(request, release, notes):
    if request.get('schema') != 1 or request.get('tag') != 'dev-v0.7.53' or request.get('sha') != '4a4d69cb200f341162146c2b90d83135bbd1ea62':
        raise Refused('unsupported one-time DEV description repair')
    if release.get('id') != request.get('release_id') or release.get('tag_name') != request['tag'] or release.get('draft') or not release.get('prerelease'):
        raise Refused('repair target is not the expected published DEV')
    if len(release.get('assets', [])) != len(request.get('assets', {})) or set(a.get('name') for a in release.get('assets', [])) != set(request.get('assets', {})):
        raise Refused('repair asset set changed')
    verify_uploaded(release, request['assets'])
    body = release.get('body') or ''
    if body != notes and hashlib.sha256(body.encode('utf-8')).hexdigest() != request.get('old_body_sha256'):
        raise Refused('release description changed since the repair was prepared')
    return body != notes


def ci_verdict(sha, runs, jobs_by_run):
    """'ok', 'wait' (a run for sha is still active) or raises Refused."""
    own = [r for r in runs if r.get('head_sha') == sha]
    if not own:
        raise Refused(f'no "{CI_WORKFLOW}" run for exact commit {sha}; push the branch and wait for CI')
    for run in sorted(own, key=lambda r: r.get('run_number', 0), reverse=True):
        if run.get('status') != 'completed':
            return 'wait'
        if run.get('conclusion') != 'success':
            continue
        jobs = {j.get('name'): j.get('conclusion') for j in jobs_by_run.get(run.get('id'), [])}
        missing = sorted(CI_JOBS - set(jobs))
        failed = sorted(name for name in CI_JOBS & set(jobs) if jobs[name] != 'success')
        if not missing and not failed:
            return 'ok'
    raise Refused(f'no successful 5/5 "{CI_WORKFLOW}" run for {sha}')


def stable_approval(environment):
    rules = (environment or {}).get('protection_rules') or []
    if not any(rule.get('type') == 'required_reviewers' and rule.get('reviewers') for rule in rules):
        raise Refused(f'Stable needs environment "{STABLE_ENVIRONMENT}" with required reviewers (owner approval)')


def signing_environment(kind, environment, policies, owner):
    """The OTA signing key may only be reachable from an owner-approved
    environment that admits release tags, never an arbitrary branch."""
    name = ENVIRONMENTS[kind]
    rules = (environment or {}).get('protection_rules') or []
    reviewers = [reviewer for rule in rules if rule.get('type') == 'required_reviewers'
                 for reviewer in rule.get('reviewers') or []]
    # GitHub accepts approval from any listed reviewer, so including another
    # user or team would let them release the key without the owner's consent.
    if not owner or not reviewers or any(reviewer.get('type') != 'User' or
            (reviewer.get('reviewer') or {}).get('login', '').lower() != owner.lower()
            for reviewer in reviewers):
        raise Refused(f'environment "{name}" must require approval only from repository owner {owner}')
    if (environment or {}).get('can_admins_bypass') is not False:
        raise Refused(f'environment "{name}" must disable administrator bypass of owner approval')
    policy = (environment or {}).get('deployment_branch_policy') or {}
    if not policy.get('custom_branch_policies') or policy.get('protected_branches'):
        raise Refused(f'environment "{name}" must be limited to custom release tag patterns')
    entries = (policies or {}).get('branch_policies') or []
    if (policies or {}).get('total_count') != len(entries):
        raise Refused(f'environment "{name}" deployment policies are incomplete; every policy must be checked')
    if not entries or any(e.get('type') != 'tag' or e.get('name') not in TAG_POLICIES[kind] for e in entries):
        raise Refused(f'environment "{name}" may admit only {sorted(TAG_POLICIES[kind])} tags')


def tag_unchanged(expected_sha, current_sha):
    if not current_sha or current_sha != expected_sha:
        raise Refused(f'tag moved: built {expected_sha}, tag now {current_sha}')


def plan(tag, release, local):
    """Actions for an existing release (or None) and local {name: sha256}.

    Returns dict(create, hide, upload, publish). Never deletes or replaces."""
    kind = channel(tag)
    if release is None:
        return dict(create=True, hide=False, upload=sorted(local), publish=True)
    if bool(release.get('prerelease')) != (kind == 'dev'):
        raise Refused(f'release {tag} prerelease flag does not match channel {kind}')
    remote = {}
    for asset in release.get('assets') or []:
        name, digest = asset.get('name'), asset.get('digest') or ''
        if name not in local:
            raise Refused(f'release {tag} holds a foreign asset {name!r}')
        if asset.get('state') != 'uploaded' or not digest.startswith('sha256:'):
            raise Refused(f'release {tag} asset {name!r} has no verified sha256 digest')
        remote[name] = digest[7:]
    changed = sorted(name for name in remote if remote[name] != local[name])
    if changed:
        raise Refused(f'release {tag} already has different bytes for {changed}; published assets are immutable')
    upload = sorted(set(local) - set(remote))
    visible = not release.get('draft')
    return dict(create=False, hide=bool(upload) and visible, upload=upload, publish=bool(upload) or not visible)


def verify_uploaded(release, local):
    digests = {a.get('name'): a.get('digest') for a in (release or {}).get('assets') or []}
    wrong = sorted(n for n, sha in local.items() if digests.get(n) != 'sha256:' + sha)
    if wrong:
        raise Refused(f'uploaded digests differ or are missing: {wrong}')


# ---- GitHub I/O (gh api) ----------------------------------------------------
def gh(*args, data=None):
    command = ['gh', 'api', *args]
    if data is not None:
        command += ['--input', '-']
    result = subprocess.run(command, input=json.dumps(data) if data is not None else None, capture_output=True, text=True)
    if result.returncode:
        if 'HTTP 404' in result.stderr:
            return None
        raise Refused(f'gh api {" ".join(args)} failed: {result.stderr.strip()}')
    return json.loads(result.stdout) if result.stdout.strip() else {}


def tag_commit(repo, tag):
    ref = gh(f'repos/{repo}/git/ref/tags/{tag}')
    if not ref:
        return None
    obj = ref['object']
    while obj['type'] == 'tag':
        obj = gh(f'repos/{repo}/git/tags/{obj["sha"]}')['object']
    return obj['sha']


def release_by_tag(repo, tag):
    for page in range(1, 4):
        for release in gh(f'repos/{repo}/releases?per_page=100&page={page}') or []:
            if release.get('tag_name') == tag:
                return release
    return None


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def cmd_gate(a):
    kind = channel(a.tag)
    # Releases start only from an existing tag on the exact built commit. The
    # tag is pushed by the owner after CI 5/5; nothing here creates or moves it.
    tag_unchanged(a.sha, tag_commit(a.repo, a.tag))
    environment = ENVIRONMENTS[kind]
    signing_environment(kind, gh(f'repos/{a.repo}/environments/{environment}'),
                        gh(f'repos/{a.repo}/environments/{environment}/deployment-branch-policies?per_page=30'),
                        a.repo.split('/')[0])
    deadline = time.time() + a.wait_minutes * 60
    while True:
        runs = (gh(f'repos/{a.repo}/actions/workflows/{CI_WORKFLOW}/runs?head_sha={a.sha}&per_page=20') or {}).get('workflow_runs', [])
        jobs = {r['id']: (gh(f'repos/{a.repo}/actions/runs/{r["id"]}/jobs?per_page=50') or {}).get('jobs', [])
                for r in runs if r.get('head_sha') == a.sha and r.get('status') == 'completed' and r.get('conclusion') == 'success'}
        if ci_verdict(a.sha, runs, jobs) == 'ok':
            break
        if time.time() > deadline:
            raise Refused(f'CI for {a.sha} still running after {a.wait_minutes} min')
        time.sleep(60)
    tag_unchanged(a.sha, tag_commit(a.repo, a.tag))
    if kind == 'stable':
        stable_approval(gh(f'repos/{a.repo}/environments/{STABLE_ENVIRONMENT}'))
    release = release_by_tag(a.repo, a.tag)
    if release and not release.get('draft') and not release.get('assets'):
        gh(f'repos/{a.repo}/releases/{release["id"]}', '-X', 'PATCH', data={'draft': True})
        print(f'Release {a.tag} hidden as draft until its signed assets are complete')
    print(f'GATE OK: {a.tag} ({kind}) = {a.sha}, CI 5/5')


def cmd_publish(a):
    kind = channel(a.tag)
    tag_unchanged(a.sha, tag_commit(a.repo, a.tag))
    files = {p.name: p for p in sorted(Path(a.dir).iterdir()) if p.is_file()}
    local = {name: sha256(path) for name, path in files.items()}
    release = release_by_tag(a.repo, a.tag)
    steps = plan(a.tag, release, local)
    if steps['create']:
        notes = compact_notes(a.repo, a.tag, Path(a.notes).read_text())
        release = gh(f'repos/{a.repo}/releases', '-X', 'POST', data={'tag_name': a.tag, 'name': a.tag, 'body': notes,
                                                                    'draft': True, 'prerelease': kind == 'dev'})
    elif steps['hide']:
        release = gh(f'repos/{a.repo}/releases/{release["id"]}', '-X', 'PATCH', data={'draft': True})
    for name in steps['upload']:
        url = f'https://uploads.github.com/repos/{a.repo}/releases/{release["id"]}/assets?name={name}'
        subprocess.run(['gh', 'api', '-X', 'POST', '-H', 'Content-Type: application/octet-stream', url,
                        '--input', str(files[name])], check=True, stdout=subprocess.DEVNULL)
    release = gh(f'repos/{a.repo}/releases/{release["id"]}')
    verify_uploaded(release, local)
    verify_catalogue(a.tag, release)
    tag_unchanged(a.sha, tag_commit(a.repo, a.tag))
    if steps['publish']:
        data = {'draft': False, 'prerelease': kind == 'dev'}
        if kind == 'stable':
            data['make_latest'] = 'true'
        gh(f'repos/{a.repo}/releases/{release["id"]}', '-X', 'PATCH', data=data)
    # Publication changes GitHub's draft URLs. Validate the actual public
    # response as well; never normalize a published URL or leave bad OTA visible.
    try:
        published = gh(f'repos/{a.repo}/releases/{release["id"]}')
        if published.get('draft'):
            raise Refused('release did not become public')
        verify_uploaded(published, local)
        verify_catalogue(a.tag, published)
        tag_unchanged(a.sha, tag_commit(a.repo, a.tag))
    except Exception:
        gh(f'repos/{a.repo}/releases/{release["id"]}', '-X', 'PATCH', data={'draft': True})
        raise
    print(f'PUBLISHED {a.tag}: ' + ', '.join(f'{n} {local[n][:12]}' for n in sorted(local)))


def cmd_repair_notes(a):
    if a.repo != 'voltex-at/DIY-Shelly-Remote':
        raise Refused('repair is restricted to the owner repository')
    request = json.loads(Path(a.request).read_text())
    tag = request.get('tag')
    notes = compact_notes(a.repo, tag, Path('docs/RELEASE_NOTES.md').read_text())
    release = release_by_tag(a.repo, tag)
    if release is None:
        raise Refused('repair target is absent')
    changed = repair_notes_allowed(request, release, notes)
    tag_unchanged(request['sha'], tag_commit(a.repo, tag))
    verify_catalogue(tag, dict(release, body=notes))
    # The only mutation is the description. No draft/channel/tag, assets,
    # signature, application or device writes; nothing is rebuilt or signed.
    if changed:
        gh(f'repos/{a.repo}/releases/{release["id"]}', '-X', 'PATCH', data={'body': notes})
    after = gh(f'repos/{a.repo}/releases/{release["id"]}')
    repair_notes_allowed(request, after, notes)
    if asset_fingerprint(after) != asset_fingerprint(release) or any(after.get(k) != release.get(k) for k in ('id', 'tag_name', 'draft', 'prerelease', 'target_commitish')):
        raise Refused('repair changed fields outside the description')
    if after.get('body') != notes:
        raise Refused('description readback differs')
    tag_unchanged(request['sha'], tag_commit(a.repo, tag))
    verify_catalogue(tag, after)
    print(f'REPAIRED {tag}: description {len(notes.encode("utf-8"))} bytes; assets and tag unchanged')


def main():
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest='command', required=True)
    for name in ('gate', 'publish'):
        s = sub.add_parser(name)
        s.add_argument('--repo', required=True)
        s.add_argument('--tag', required=True)
        s.add_argument('--sha', required=True)
        if name == 'gate':
            s.add_argument('--wait-minutes', type=int, default=45)
        else:
            s.add_argument('--dir', required=True)
            s.add_argument('--notes', required=True)
    repair = sub.add_parser('repair-notes')
    repair.add_argument('--repo', required=True)
    repair.add_argument('--request', required=True)
    a = p.parse_args()
    try:
        {'gate': cmd_gate, 'publish': cmd_publish, 'repair-notes': cmd_repair_notes}[a.command](a)
    except Refused as error:
        print(f'REFUSED: {error}', file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
