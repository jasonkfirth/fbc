"""Project: FreeBASIC native format result regression
File: c-format-results.py
Purpose: Retain native format checking through evaluated translation calls.
Responsibilities: Accept literal contracts and reject mismatches or lost formats.
Runtime translation values are covered by the paired BASIC and C fixtures.
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
    with tempfile.TemporaryDirectory(prefix='fbc-format-results-') as directory:
        work = Path(directory)
        cpp, obj = work / 'case.c', work / 'case.o'
        declarations = source.with_name('crt-format-declarations.bas')
        declaration_C = work / 'declarations.c'
        emitted = subprocess.run(
            [str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()), '-i', str(root / 'inc'),
             '-gen', args.backend, '-w', 'all', '-c', '-r', str(declarations), '-o', str(declaration_C)],
            capture_output=True, text=True)
        if emitted.returncode:
            raise RuntimeError(emitted.stdout + emitted.stderr)
        observer = work / 'observer.c'
        observer.write_text('#include <stdio.h>\n#include "declarations.c"\n'
            '_Static_assert(__builtin_types_compatible_p(__typeof__(&snprintf), '
            '__typeof__(probe_snprintf_address())), "snprintf signature");\n'
            '_Static_assert(__builtin_types_compatible_p(__typeof__(&vsnprintf), '
            '__typeof__(probe_vsnprintf_address())), "vsnprintf signature");\n')
        checked = subprocess.run([args.cc, '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                                  '-Werror', '-c', str(observer), '-o', str(obj)],
                                 capture_output=True, text=True)
        if checked.returncode:
            raise RuntimeError('CRT formatter declaration contract failed\n' + checked.stdout + checked.stderr)
        for variant in ('', 'FORMAT_BAD_ARGUMENT', 'FORMAT_DYNAMIC', 'FORMAT_SAVED',
                        'FORMAT_INDIRECT', 'FORMAT_WRONG_DECL', 'FORMAT_CONST_RESULT', 'FORMAT_LOCAL'):
            defines = ['-d', variant] if variant else []
            command = [str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
                       '-i', str(root / 'inc'), '-gen', args.backend, '-w', 'all',
                       '-m', source.stem, '-c', '-r', *defines, str(source), '-o', str(cpp)]
            emitted = subprocess.run(command, capture_output=True, text=True)
            if emitted.returncode:
                raise RuntimeError(emitted.stdout + emitted.stderr)
            checked = subprocess.run(
                [args.cc, '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Wformat=2',
                 '-Werror=format', '-Werror=format-nonliteral', '-c', str(cpp), '-o', str(obj)],
                capture_output=True, text=True)
            output = checked.stdout + checked.stderr
            if variant:
                expected = 'format' if variant == 'FORMAT_BAD_ARGUMENT' else 'format-nonliteral'
                diagnostic = r'\[-W(?:error=|error,-W)' + expected + r'(?:=)?\]'
                if checked.returncode == 0 or re.search(diagnostic, output) is None:
                    raise RuntimeError(f'{variant}: expected native format diagnostic\n{output}')
            elif checked.returncode:
                raise RuntimeError('literal translation contract failed\n' + output)
    print(f'{args.backend}: FORMAT_RESULT_DIAGNOSTICS_PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of c-format-results.py
