"""Project: FreeBASIC semantic diagnostics tests
File: unclosed_procedures.py
Purpose: Verify native EOF procedure context and independent diagnostic readers.
Responsibilities: Actual parser outcomes, unchanged diagnostics and bounded mutation tests.
This file intentionally does NOT accept rejected AST facts or execute fixtures.
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
with tempfile.TemporaryDirectory(prefix='fblint-unclosed-producer-', dir='E:/') as temporary:
    working = Path(temporary).resolve()
    source, diagnostic = working / 'probe.bas', working / 'probe.dia'
    for name, text, expected in [
        ('sub', 'Sub Sample()\nPrint 1\n', True),
        ('function', 'Function Sample() As Long\nReturn 1\n', True),
        ('inner-if', 'Sub Sample()\nIf 1 Then\nPrint 1\n', True),
        ('inner-for', 'Sub Sample()\nFor i As Long = 1 To 2\nPrint 1\n', True),
        ('inner-do', 'Sub Sample()\nDo\nPrint 1\n', True),
        ('matched', 'Sub Sample()\nPrint 1\nEnd Sub\nSample()\n', False),
        ('rejected-inner-end', 'Sub Sample()\nIf 1 Then\nPrint 1\nEnd Sub\n', True),
        ('module-block', 'If 1 Then\nPrint 1\n', False),
        ('accepted-after-recovery', 'Sub Sample()\nIf 1 Then\nPrint 1\nEnd If\nEnd Sub\nSample()\n', False),
    ]:
        source.write_text(text)
        output = working / 'probe.asm'
        command = [str(args.compiler.resolve()), '-gen', 'gas64', '-r', '-o', str(output), str(source)]
        normal = subprocess.run(command, capture_output=True, timeout=60)
        normal_code = output.read_bytes() if normal.returncode == 0 and output.exists() else None
        observed = subprocess.run(command[:-1] + ['-semantic-diagnostics', str(diagnostic), str(source)],
                                  capture_output=True, timeout=60)
        assert (observed.returncode, observed.stdout, observed.stderr) == (
            normal.returncode, normal.stdout, normal.stderr), name
        if normal_code is not None:
            assert output.read_bytes() == normal_code, name
        result = Diagnostics.read(diagnostic, observed.returncode)
        assert result.unclosed_modules == {1}, name
        records = [row for row in result.records if row[5] == 'unclosed-procedure']
        assert bool(records) == expected, (name, result.records)
        assert all(row[11] == '1' for row in records), name
        native = subprocess.run([str(args.reader.resolve()), str(diagnostic), str(source), 'valid',
                                 str(observed.returncode), 'available', 'available', 'available'],
                                capture_output=True, timeout=30)
        assert native.returncode == 0, (name, native.stdout, native.stderr)
        cases.append({'case': name, 'compiler_exit': observed.returncode, 'unclosed': len(records)})
        print(name, 'passed', flush=True)
        if expected and name in ('sub', 'rejected-inner-end'):
            # Exercise the production and independent validators using a real
            # compiler artifact, including capability removal and bad totals.
            mutation = subprocess.run([sys.executable, str(Path(r'C:\fblint\test_compiler_diagnostic_readers.py')),
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
# end of unclosed_procedures.py
