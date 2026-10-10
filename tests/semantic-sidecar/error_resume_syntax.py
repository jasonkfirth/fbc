"""Project: FreeBASIC semantic diagnostics tests
File: error_resume_syntax.py
Purpose: Verify parser-selected ON ERROR RESUME NEXT rejection context.
Responsibilities: Native outcomes, unchanged diagnostics and independent readers.
This file intentionally does NOT accept recovered AST facts or execute fixtures.
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
parser.add_argument('--receipt', type=Path)
args = parser.parse_args()
providers = [args.compiler.resolve(), args.reader.resolve(), Path(__file__).resolve(),
             Path(__file__).with_name('diagnostics.py')]
pins = {str(path): hashlib.sha256(path.read_bytes()).hexdigest().upper() for path in providers}
cases = []
with tempfile.TemporaryDirectory(prefix='fblint-error-resume-producer-', dir='E:/') as temporary:
    working = Path(temporary).resolve()
    source, diagnostic = working / 'probe.bas', working / 'probe.dia'
    for name, text, expected_line in [
        ('module', 'On Error Resume Next\n', 1),
        ('module-local', 'On Local Error Resume Next\n', 1),
        ('routine-local', 'Sub Sample()\nOn Local Error Resume Next\nEnd Sub\n', 2),
        ('continued', 'On Error _\nResume Next\n', 1),
        ('colon-statement', 'Print 1: On Error Resume Next\n', 1),
        ('remapped', '#Line 400\nOn Error Resume Next\n', 2),
        ('other-rejected-syntax', 'On Error Resume Else\n', 0),
        ('handler-resume', 'On Error Goto handler\nGoto finished\nhandler:\nResume Next\nfinished:\n', 0),
        ('quoted', 'Print "On Error Resume Next"\n', 0),
        ('commented', "' On Error Resume Next\nPrint 1\n", 0),
        ('inactive', '#If 0\nOn Error Resume Next\n#EndIf\nPrint 1\n', 0),
    ]:
        source.write_text(text)
        emitted = working / 'probe.asm'
        command = [str(args.compiler.resolve()), '-gen', 'gas64', '-ex', '-r', '-o', str(emitted), str(source)]
        normal = subprocess.run(command, capture_output=True, timeout=60)
        normal_code = emitted.read_bytes() if normal.returncode == 0 and emitted.exists() else None
        observed = subprocess.run(command[:-1] + ['-semantic-diagnostics', str(diagnostic), str(source)],
                                  capture_output=True, timeout=60)
        assert (observed.returncode, observed.stdout, observed.stderr) == (
            normal.returncode, normal.stdout, normal.stderr), name
        if normal_code is not None:
            assert emitted.read_bytes() == normal_code, name
        result = Diagnostics.read(diagnostic, observed.returncode)
        assert result.error_resume_modules == {1}, name
        records = [row for row in result.records if row[5] == 'error-resume-next-syntax']
        assert bool(records) == bool(expected_line), (name, result.records)
        assert all(row[11] == '1' and int(row[12]) == expected_line for row in records), (name, records)
        native = subprocess.run([str(args.reader.resolve()), str(diagnostic), str(source), 'valid',
                                 str(observed.returncode), 'available', 'available', 'available', 'available'],
                                capture_output=True, timeout=30)
        assert native.returncode == 0, (name, native.stdout, native.stderr)
        cases.append({'case': name, 'compiler_exit': observed.returncode, 'syntax_findings': len(records)})
        print(name, 'passed', flush=True)
        if name == 'module':
            mutation = subprocess.run([sys.executable, r'C:\fblint\test_compiler_diagnostic_readers.py',
                                       '--reader', str(args.reader.resolve()), '--reference-reader',
                                       str(Path(__file__).with_name('diagnostics.py')), '--artifact', str(diagnostic)],
                                      capture_output=True, timeout=90)
            assert mutation.returncode == 0, (name, mutation.stdout, mutation.stderr)
            print(mutation.stdout.decode(errors='replace').strip(), flush=True)
assert not working.exists()
assert not [path for path, expected in pins.items() if hashlib.sha256(Path(path).read_bytes()).hexdigest().upper() != expected]
if args.receipt:
    args.receipt.write_text(json.dumps({'passed': True, 'cases': cases, 'provider_sha256': pins,
                                     'normal_diagnostics_and_code_unchanged': True, 'scratch_removed': True}, indent=2) + '\n')
# end of error_resume_syntax.py
