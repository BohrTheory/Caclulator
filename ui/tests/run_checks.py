#!/usr/bin/env python3
"""Retain exact local client build/unit commands, outcomes and source identities."""
import hashlib
import json
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
evidence = root / 'ui/evidence'
evidence.mkdir(exist_ok=True)
env = dict(os.environ, XMAKE_GLOBALDIR=str(root / 'build/xmake-global'))
commands = [
    ['git', 'rev-parse', 'HEAD'],
    ['clang++', '--version'],
    ['xmake', '--version'],
    ['pkg-config', '--modversion', 'Qt6Widgets'],
    ['python3', 'tools/generate_abi.py', '--check'],
    ['xmake', 'f', '-m', 'debug', '--app=n', '--qt_units=n', '-y'],
    ['xmake', 'build', '-r', '-v', 'client_units'],
    ['xmake', 'run', 'client_units'],
    ['xmake', 'test', '-v', 'client_units'],
    ['git', 'diff', '--check'],
]
records = []
for index, command in enumerate(commands):
    result = subprocess.run(command, cwd=root, env=env, text=True, capture_output=True)
    log = 'command-%02d.txt' % index
    (evidence / log).write_text(result.stdout + '\nSTDERR:\n' + result.stderr)
    records.append(dict(argv=command, cwd=str(root), exit_code=result.returncode,
                        stdout=result.stdout, stderr=result.stderr, log='ui/evidence/' + log,
                        environment={'XMAKE_GLOBALDIR': env['XMAKE_GLOBALDIR']}))
    print(command, 'exit', result.returncode, flush=True)
source_paths = sorted(p for p in (root / 'ui').rglob('*') if p.is_file() and 'evidence' not in p.parts)
source_paths += [root / 'xmake.lua', root / '.gitignore']
report = dict(agent=os.environ.get('PAPERCLIP_AGENT_ID'), task=os.environ.get('PAPERCLIP_TASK_ID'),
    run=os.environ.get('PAPERCLIP_RUN_ID'),
    architecture='67100ba0ce1d7aaf4fcafadb9101465b78d19829',
    spec='752d0961c8aacb2e79addeb6d1e26a7ea820d734',
    historical_base='3ef202581d4e784d0f9da508767577888a4fc272',
    integrated_base='22d6b90c1e1baa16d5de40a0c235847d998cfb66', records=records,
    source_sha256={str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths},
    scope='Author client units, scripted evaluator and FFI doubles. Host strtod bit transport oracle. No real Rust, Qt runtime, external GUI, Linux, CI, package or independent review result.')
(evidence / 'verification.json').write_text(json.dumps(report,indent=2)+'\n')
# Qt discovery failure is an expected recorded capability limitation, not a passed build.
raise SystemExit(any(row['exit_code'] for i,row in enumerate(records) if i != 3))
