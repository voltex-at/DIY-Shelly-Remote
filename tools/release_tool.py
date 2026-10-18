#!/usr/bin/env python3
"""Fail-closed gate and immutable publication for one signed GitHub Release.

gate     the tag still points at the built commit, `Firmware checks` passed
         5/5 for exactly that commit, Stable has an owner-approval environment,
         and a release opened in the UI is hidden (draft) until it is complete.
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
from pathlib import Path

CI_WORKFLOW = 'ci.yml'
CI_JOBS = {'native', 'ui-review', 'diagnostic (esp32)', 'diagnostic (esp32s3)', 'es3c35p'}
STABLE_ENVIRONMENT = 'stable-release'
TAG = re.compile(r'^(?:v|dev-v)(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$')


class Refused(Exception):
    pass


def channel(tag):
    if not TAG.match(tag or ''):
        raise Refused(f'unsupported tag {tag!r}')
    return 'dev' if tag.startswith('dev-') else 'stable'


# One owner-requested DEV publication (2026-10-18), not a general branch release.
# It cannot select another tag, Stable, branch or CMake version.
DEV_REQUEST = {'schema': 1, 'tag': 'dev-v0.7.53', 'version': '0.7.53-dev',
               'branch': 'refs/heads/work/codex-p0-review'}


def dev_request_allowed(tag, request, ref, cmake):
    if request != DEV_REQUEST or tag != DEV_REQUEST['tag'] or ref != DEV_REQUEST['branch']:
        raise Refused('DEV request does not match the one authorized version and branch')
    version = re.search(r'set\(PROJECT_VER "([^"]+)"\)', cmake)
    if not version or version.group(1) != DEV_REQUEST['version']:
        raise Refused('DEV request version does not match PROJECT_VER')


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
    requested = bool(a.dev_request)
    current = tag_commit(a.repo, a.tag)
    if requested:
        dev_request_allowed(a.tag, json.loads(Path(a.dev_request).read_text()), os.environ.get('GITHUB_REF'),
                            Path('CMakeLists.txt').read_text())
        if current:
            tag_unchanged(a.sha, current)
    else:
        tag_unchanged(a.sha, current)
    deadline = time.time() + a.wait_minutes * 60
    while True:
        runs = (gh(f'repos/{a.repo}/actions/workflows/{CI_WORKFLOW}/runs?head_sha={a.sha}&per_page=20') or {}).get('workflow_runs', [])
        jobs = {r['id']: (gh(f'repos/{a.repo}/actions/runs/{r["id"]}/jobs?per_page=50') or {}).get('jobs', [])
                for r in runs if r.get('head_sha') == a.sha and r.get('status') == 'completed' and r.get('conclusion') == 'success'}
        # The push starts both workflows: allow the CI run a short discovery window.
        if requested and not runs and time.time() < deadline:
            time.sleep(10)
            continue
        if ci_verdict(a.sha, runs, jobs) == 'ok':
            break
        if time.time() > deadline:
            raise Refused(f'CI for {a.sha} still running after {a.wait_minutes} min')
        time.sleep(60)
    if requested and not current:
        # Only create the immutable tag AFTER exact-commit CI 5/5. A concurrent
        # creation fails closed; no forced update or tag/asset replacement.
        gh(f'repos/{a.repo}/git/refs', '-X', 'POST', data={'ref': 'refs/tags/' + a.tag, 'sha': a.sha})
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
        notes = Path(a.notes).read_text()
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
    tag_unchanged(a.sha, tag_commit(a.repo, a.tag))
    if steps['publish']:
        data = {'draft': False, 'prerelease': kind == 'dev'}
        if kind == 'stable':
            data['make_latest'] = 'true'
        gh(f'repos/{a.repo}/releases/{release["id"]}', '-X', 'PATCH', data=data)
    print(f'PUBLISHED {a.tag}: ' + ', '.join(f'{n} {local[n][:12]}' for n in sorted(local)))


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
            s.add_argument('--dev-request', help='one authorized DEV request; exact commit CI remains mandatory')
        else:
            s.add_argument('--dir', required=True)
            s.add_argument('--notes', required=True)
    a = p.parse_args()
    try:
        (cmd_gate if a.command == 'gate' else cmd_publish)(a)
    except Refused as error:
        print(f'REFUSED: {error}', file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
