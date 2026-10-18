#!/usr/bin/env python3
"""Complete one already-signed, unpublished DEV; never replace/sign/upload assets."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
from types import SimpleNamespace
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey
import release_tool as release
from ota_sign_manifest import message, HARDWARE, LAYOUT

REQUEST = dict(schema=1, tag='dev-v0.7.54', source_sha='aaff1d12f4d2aa619b102bae32a04abc03e9156c', artifact_run=39157279620)
EXPECTED = {
 'diy_shelly_remote.bin':'646e877483daec7f601615191578f822e005f6d7474f3dc47f9e1a913ee92c20',
 'DIY-Shelly-Remote-es3c35p-FACTORY-CLEAN-16MB.zip':'33fd15dd9cf0939481ade3a9827d7756184f540ac060892282e1570b2a96f69b',
 'DIY-Shelly-Remote-es3c35p-UPDATE-APP-ONLY.zip':'c8757bb59bb8404076f41ac712a5113519b0a978e2a100df4cb5e1c16ffc9f8e'}
MANIFEST='DIY-Shelly-Remote-es3c35p-manifest.json'
NAMES=set(EXPECTED)|{MANIFEST,'SHA256SUMS.txt'}

def allowed(request, repo, ref):
 if request!=REQUEST or repo!='voltex-at/DIY-Shelly-Remote' or ref!='refs/heads/work/p0-spiffs-ota-brand':
  raise release.Refused('unsupported one-time DEV completion')

def ready(metadata, local):
 if metadata.get('tag_name')!=REQUEST['tag'] or not metadata.get('draft') or not metadata.get('prerelease'):
  raise release.Refused('expected an unpublished DEV with the fixed tag')
 if set(local)!=NAMES or len(metadata.get('assets',[]))!=5 or set(a['name'] for a in metadata['assets'])!=NAMES:
  raise release.Refused('completion asset set changed')
 if any(local.get(name)!=digest for name,digest in EXPECTED.items()):
  raise release.Refused('completion bytes differ from exact-commit CI')
 release.verify_uploaded(metadata,local)
 steps=release.plan(REQUEST['tag'],metadata,local)
 if steps['create'] or steps['hide'] or steps['upload'] or not steps['publish']:
  raise release.Refused('completion must only publish existing identical assets')

def wait_ci(repo, sha):
 deadline=time.monotonic()+45*60
 while True:
  runs=(release.gh(f'repos/{repo}/actions/workflows/ci.yml/runs?head_sha={sha}&per_page=20') or {}).get('workflow_runs',[])
  jobs={r['id']:(release.gh(f'repos/{repo}/actions/runs/{r["id"]}/jobs?per_page=50') or {}).get('jobs',[]) for r in runs if r.get('status')=='completed' and r.get('conclusion')=='success'}
  if runs and release.ci_verdict(sha,runs,jobs)=='ok':return
  if time.monotonic()>deadline:raise release.Refused('completion CI timed out')
  time.sleep(15)

def main():
 p=argparse.ArgumentParser();p.add_argument('--repo',required=True);p.add_argument('--request',required=True);a=p.parse_args()
 allowed(json.loads(Path(a.request).read_text()),a.repo,os.environ.get('GITHUB_REF'))
 wait_ci(a.repo,REQUEST['source_sha']);wait_ci(a.repo,os.environ['GITHUB_SHA'])
 release.tag_unchanged(REQUEST['source_sha'],release.tag_commit(a.repo,REQUEST['tag']))
 metadata=release.release_by_tag(a.repo,REQUEST['tag'])
 if not metadata:raise release.Refused('signed draft not found')
 root=Path('release');local={p.name:release.sha256(p) for p in root.iterdir() if p.is_file()}
 if local!=EXPECTED:raise release.Refused('downloaded unsigned artifact differs from CI')
 # Read back only the two existing signed-publication files, never a secret/key.
 for name in [MANIFEST,'SHA256SUMS.txt']:
  assets=[x for x in metadata.get('assets',[]) if x.get('name')==name]
  if len(assets)!=1 or not 0<assets[0].get('size',0)<=2048:raise release.Refused('invalid signed asset metadata')
  asset=assets[0]
  result=subprocess.run(['gh','api',f'repos/{a.repo}/releases/assets/{asset["id"]}','-H','Accept: application/octet-stream'],capture_output=True,check=True)
  data=result.stdout
  if len(data)!=asset['size'] or 'sha256:'+hashlib.sha256(data).hexdigest()!=asset.get('digest'):raise release.Refused('signed asset readback mismatch')
  (root/name).write_bytes(data)
 local={p.name:release.sha256(p) for p in root.iterdir() if p.is_file()};ready(metadata,local)
 m=json.loads((root/MANIFEST).read_bytes())
 expected=dict(schema_version=2,hardware_id=HARDWARE,partition_layout=LAYOUT,version='0.7.54',channel='dev',
  url=f'https://github.com/{a.repo}/releases/download/{REQUEST["tag"]}/diy_shelly_remote.bin',size=3744304,
  sha256=EXPECTED['diy_shelly_remote.bin'],key_id='voltex-ota-2026-1',signature_algorithm='ed25519')
 if set(m)!=set(expected)|{'signature'} or any(m.get(k)!=v for k,v in expected.items()):raise release.Refused('manifest does not bind expected firmware')
 Ed25519PublicKey.from_public_bytes(bytes.fromhex('23adf99d68e6ce2315a7b0b2b53c5839d9dd15e39ee748a24ec68841861a47fe')).verify(base64.b64decode(m['signature'],validate=True),message(m))
 sums={line.split('  ',1)[1]:line.split('  ',1)[0] for line in (root/'SHA256SUMS.txt').read_text().splitlines()}
 if sums!={n:h for n,h in local.items() if n!='SHA256SUMS.txt'}:raise release.Refused('SHA256SUMS does not match all assets')
 print('Verified existing production signature and all five asset digests; zero signing/uploads')
 print('Draft routes:',json.dumps({x['name']:x['browser_download_url'] for x in metadata['assets']}))
 release.cmd_publish(SimpleNamespace(repo=a.repo,tag=REQUEST['tag'],sha=REQUEST['source_sha'],dir=str(root),notes='docs/RELEASE_NOTES.md'))

if __name__=='__main__':main()
