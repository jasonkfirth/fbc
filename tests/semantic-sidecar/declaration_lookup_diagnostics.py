"""Project: FreeBASIC compiler observations
File: declaration_lookup_diagnostics.py
Purpose: Qualify native failed lookups and suffix/AS grammar conflicts.
Responsibilities: Exact diagnostics, independent readers and observer parity.
This file intentionally does NOT infer types or bindings from console text.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from diagnostics import Diagnostics

parser = argparse.ArgumentParser()
parser.add_argument('--compiler', required=True, type=Path)
parser.add_argument('--reader', required=True, type=Path)
parser.add_argument('--reader-mutations', required=True, type=Path)
parser.add_argument('--receipt', type=Path)
args = parser.parse_args()
providers = [args.compiler.resolve(), args.reader.resolve(), Path(__file__).resolve(),
             Path(__file__).resolve().with_name('diagnostics.py'), args.reader_mutations.resolve()]
pins = {str(path): hashlib.sha256(path.read_bytes()).hexdigest().upper() for path in providers}
cases = []
with tempfile.TemporaryDirectory(prefix='fbc-declaration-diagnostics-', dir='E:/') as temporary:
    working = Path(temporary).resolve()
    artifacts = []
    inputs = [
        ('suffix-name-first', '#Lang "fblite"\nDim value$ As Integer\n', 'declaration-suffix-as-conflict', 2, 4, 17),
        ('suffix-type-first', '#Lang "fblite"\nDim As Integer value$\n', 'declaration-suffix-as-conflict', 2, 15, 17),
        ('unresolved-variable', 'Print missingValue\n', '', 1, 6, 42),
        ('unresolved-remapped', '#Line 900\nPrint missingValue\n', '', 2, 6, 42),
        ('macro-unresolved', '#Define READ_MISSING Print missingValue\nREAD_MISSING\n', '', 2, 0, 42),
        ('macro-suffix', '#Lang "fblite"\n#Define DECLARE_VALUE Dim value$ As Integer\nDECLARE_VALUE\n', 'declaration-suffix-as-conflict', 3, 0, 17),
        ('accepted-ordinary', 'Dim value As Integer\nPrint value\n', None, 0, 0, 0),
        ('accepted-suffixed', '#Lang "fblite"\nDim value$\nPrint value$\n', None, 0, 0, 0),
    ]
    for backend, target in [('gas64', 'win64'), ('gas', 'win32'), ('gcc', 'win64')]:
        for name, text, kind, line, column, code in inputs:
            source = working / 'case.bas'
            source.write_text(text)
            output = working / ('case.c' if backend == 'gcc' else 'case.asm')
            artifact = working / (backend + '-' + name + '.fbcdia')
            command = [str(args.compiler.resolve()), '-prefix', r'C:\FreeBASIC', '-target', target,
                       '-gen', backend, '-r', '-o', str(output), str(source)]
            plain = subprocess.run(command, cwd=working, capture_output=True, timeout=60)
            plain_bytes = output.read_bytes() if output.exists() and plain.returncode == 0 else None
            origin_flags = ['-semantic-model', str(working / 'macro.sem')] if name.startswith('macro-') else []
            observed = subprocess.run(command[:-1] + origin_flags + ['-semantic-diagnostics', str(artifact), str(source)],
                                      cwd=working, capture_output=True, timeout=60)
            assert (observed.returncode, observed.stdout, observed.stderr) == (plain.returncode, plain.stdout, plain.stderr), (backend, name, observed.stdout, plain.stdout)
            if plain_bytes is not None:
                assert output.read_bytes() == plain_bytes, (backend, name, 'emitted code changed')
            model = Diagnostics.read(artifact, observed.returncode)
            assert model.suffix_as_modules == set(range(1, len(model.modules) + 1))
            if kind is None:
                assert observed.returncode == 0 and not model.records, (backend, name, model.records)
            else:
                selected = [row for row in model.records if row[5] == kind and int(row[4]) == code]
                assert [(int(row[12]), int(row[13])) for row in selected] == [(line, column)], (backend, name, selected)
                assert observed.returncode != 0
            native = subprocess.run([str(args.reader.resolve()), str(artifact), str(source), 'valid',
                                     str(observed.returncode), 'available', 'available', 'available', 'available', 'available'],
                                    capture_output=True, timeout=30)
            assert native.returncode == 0, (backend, name, native.stdout, native.stderr)
            # Reader mutations use the retained protocol bytes; the source
            # itself is not consulted by either diagnostic transport reader.
            artifacts.append(artifact)
            cases.append({'backend': backend, 'case': name, 'accepted': observed.returncode == 0})
            print(backend, name, 'passed.', flush=True)
    mutation = subprocess.run([sys.executable, str(args.reader_mutations.resolve()), '--reader', str(args.reader.resolve()),
                               '--reference-reader', str(Path(__file__).resolve().with_name('diagnostics.py'))]
                              + [argument for path in artifacts for argument in ('--artifact', str(path))],
                              capture_output=True, text=True, timeout=120)
    assert mutation.returncode == 0, (mutation.stdout, mutation.stderr)
    print(mutation.stdout.strip(), flush=True)
assert not working.exists()
assert not [path for path, expected in pins.items() if hashlib.sha256(Path(path).read_bytes()).hexdigest().upper() != expected]
if args.receipt:
    args.receipt.write_text(json.dumps({'passed': True, 'cases': cases, 'provider_sha256': pins,
                                     'reader_mutations': mutation.stdout.strip(), 'scratch_removed': True}, indent=2) + '\n')
print('Declaration diagnostic producer checks passed:', len(cases), 'cases.')
# end of declaration_lookup_diagnostics.py
