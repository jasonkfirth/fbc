"""Project: FreeBASIC local format provenance regression
File: c-format-locals.py
Purpose: Check native literal contracts and their storage/control-flow boundaries.
Responsibilities: Compile normal entry points and retain actual format diagnostics.
Runtime values are covered by the paired BASIC and C fixtures.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--backend', choices=('gcc', 'clang'), required=True)
    parser.add_argument('--cc', default=os.environ.get('GCC') or 'gcc')
    args = parser.parse_args()
    source = Path(__file__).with_suffix('.bas')
    root = source.parents[2]
    with tempfile.TemporaryDirectory(prefix='fbc-format-locals-') as directory:
        work = Path(directory)
        cpp, obj = work / 'case.c', work / 'case.o'
        for variant in ('', 'FORMAT_LOCAL_BAD_ARGUMENT', 'FORMAT_LOCAL_DYNAMIC',
                        'FORMAT_LOCAL_BRANCH', 'FORMAT_LOCAL_ESCAPE',
                        'FORMAT_LOCAL_AFTER_CALL', 'FORMAT_LOCAL_BAD_AFTER_CALL',
                        'FORMAT_LOCAL_NONLOCAL',
                        'FORMAT_LOCAL_ALIAS_WRITE',
                        'FORMAT_LOCAL_COPY', 'FORMAT_LOCAL_STATIC',
                        'FORMAT_TRANSLATED_BAD_ARGUMENT', 'FORMAT_TRANSLATED_BAD_ONE',
                        'FORMAT_TRANSLATED_BAD_MANY', 'FORMAT_TRANSLATED_DYNAMIC',
                        'FORMAT_TRANSLATED_AFTER_CALL', 'FORMAT_TRANSLATED_ARGUMENT_CALL',
                        'FORMAT_TRANSLATED_MUTATE', 'FORMAT_TRANSLATED_ESCAPE',
                        'FORMAT_TRANSLATED_BRANCH', 'FORMAT_TRANSLATED_ALIAS_WRITE',
                        'FORMAT_TRANSLATED_INTEGER'):
            defines = ['-d', variant] if variant else []
            command = [str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
                       '-i', str(root / 'inc'), '-gen', args.backend, '-w', 'all',
                       '-d', 'FORMAT_LOCAL_EXPORTS', '-c', '-r', *defines,
                       str(source), '-o', str(cpp)]
            emitted = subprocess.run(command, capture_output=True, text=True)
            if emitted.returncode:
                raise RuntimeError(emitted.stdout + emitted.stderr)
            checked = subprocess.run(
                [args.cc, '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Wformat=2',
                 '-Werror=format', '-Werror=format-nonliteral', '-c', str(cpp), '-o', str(obj)],
                capture_output=True, text=True)
            output = checked.stdout + checked.stderr
            if variant not in ('', 'FORMAT_LOCAL_AFTER_CALL'):
                expected = 'format' if variant in ('FORMAT_LOCAL_BAD_ARGUMENT',
                    'FORMAT_LOCAL_BAD_AFTER_CALL', 'FORMAT_TRANSLATED_BAD_ARGUMENT',
                    'FORMAT_TRANSLATED_BAD_ONE', 'FORMAT_TRANSLATED_BAD_MANY') else 'format-nonliteral'
                diagnostic = r'\[-W(?:error=|error,-W)' + expected + r'(?:=)?\]'
                if checked.returncode == 0 or re.search(diagnostic, output) is None:
                    raise RuntimeError(f'{variant}: expected native format diagnostic\n{output}')
            elif checked.returncode:
                raise RuntimeError('local literal format contract failed\n' + output)
    print(f'{args.backend}: FORMAT_LOCAL_DIAGNOSTICS_PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of c-format-locals.py
