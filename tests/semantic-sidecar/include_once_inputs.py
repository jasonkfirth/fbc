"""Project: FreeBASIC semantic sidecar tests
File: include_once_inputs.py
Purpose: Verify caller protection on actual entered include occurrences.
Responsibilities: Flags, skipped bodies, backends and ordinary-output parity.
This file intentionally does NOT infer once requests from source tokens.
"""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from sidecar import Model

root = Path(__file__).resolve().parents[2]
stage = root / 'out/semantic-lint-migration'
compiler, reader = stage / 'fbc_next.exe', stage / 'array-storage-reader.exe'
providers = [compiler, reader, Path(__file__).resolve()]
pins = {str(path): hashlib.sha256(path.read_bytes()).hexdigest().upper() for path in providers}
cases = []
with tempfile.TemporaryDirectory(prefix='fblint-include-once-', dir='E:/') as temporary:
    working = Path(temporary)
    environment = os.environ.copy()
    environment['TMP'] = environment['TEMP'] = str(working)
    source, artifact, emitted = working / 'policy.bas', working / 'model.sem', working / 'emitted.out'
    header = working / 'selected.bi'
    header.write_text("' Include input with no declarations.\n")
    for backend, target in [('gas64', 'win64'), ('gas', 'win32'), ('gcc', 'win64')]:
        for name, text, expected, outcomes in [
            ('normal', '#Include "selected.bi"\n', ['0'], ['opened']),
            ('once', '#Include Once "selected.bi"\n', ['1'], ['opened']),
            ('repeat-normal', '#Include "selected.bi"\n#Include "selected.bi"\n', ['0', '0'], ['opened', 'opened']),
            ('repeat-once', '#Include Once "selected.bi"\n#Include Once "selected.bi"\n', ['1'], ['opened', 'include-once']),
            ('once-then-normal', '#Include Once "selected.bi"\n#Include "selected.bi"\n', ['1', '0'], ['opened', 'opened']),
            ('inactive-once', '#If 0\n#Include Once "selected.bi"\n#EndIf\n#Include "selected.bi"\n', ['0'], ['opened']),
        ]:
            source.write_text(text + 'Print 1\n')
            emitted = working / ('emitted.c' if backend == 'gcc' else 'emitted.asm')
            base = [str(compiler), '-prefix', r'C:\FreeBASIC', '-r', '-gen', backend, '-target', target, '-o', str(emitted)]
            ordinary = subprocess.run(base + [str(source)], capture_output=True, env=environment, timeout=40)
            assert ordinary.returncode == 0, (backend, name, ordinary.stdout, ordinary.stderr)
            baseline = emitted.read_bytes()
            observed = subprocess.run(base + ['-semantic-model', str(artifact), str(source)], capture_output=True, env=environment, timeout=40)
            assert ordinary.returncode == observed.returncode == 0, (name, ordinary.stdout, observed.stdout, observed.stderr)
            assert (ordinary.stdout, ordinary.stderr) == (observed.stdout, observed.stderr), name
            assert emitted.read_bytes() == baseline, (backend, name, 'emission changed')
            model = Model.read(artifact)
            assert model.capabilities[1]['source-include-once-inputs'] == 'available'
            included = [str(identity) for identity, row in model.source_contexts.items() if row[5] == 'include']
            policies = {row[3].split(':')[1]: row[4].split('\t')[0] for row in model.records['K'] if row[3].startswith('header-policy:')}
            assert [policies[identity] for identity in included] == expected, (backend, name, policies, expected)
            assert [row[5] for row in model.records['INC']] == outcomes, (backend, name)
            consumed = subprocess.run([str(reader), str(artifact), 'storage-valid', '0', '0'], capture_output=True, timeout=40)
            assert consumed.returncode == 0, (backend, name, consumed.stdout, consumed.stderr)
            cases.append({'backend': backend, 'case': name, 'flags': expected, 'ordinary_output_parity': True, 'production_reader': True})
        print(backend, 'caller once facts passed.', flush=True)
assert not working.exists()
assert not [path for path, expected in pins.items() if hashlib.sha256(Path(path).read_bytes()).hexdigest().upper() != expected]
(stage / 'include-once-producer-results.json').write_text(json.dumps({'passed': True, 'cases': cases, 'provider_sha256': pins, 'scratch_removed': True}, indent=2) + '\n')
print('Caller include once checks passed:', len(cases), 'cases.')
# end of include_once_inputs.py
