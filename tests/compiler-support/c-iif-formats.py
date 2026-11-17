"""Project: FreeBASIC conditional-format regression
File: c-iif-formats.py
Purpose: Verify native checks for every conditional source-format branch.
Responsibilities: Keep wrong-type and dynamic-format diagnostics observable.
The paired BASIC/C fixture owns output and evaluation-count qualification.
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
    with tempfile.TemporaryDirectory(prefix='fbc-iif-formats-') as directory:
        work = Path(directory)
        cpp, obj = work / 'case.c', work / 'case.o'
        variants = ('', 'IIF_FORMAT_BAD_LEFT', 'IIF_FORMAT_BAD_RIGHT',
                        'IIF_FORMAT_BAD_PLURAL_LEFT', 'IIF_FORMAT_BAD_PLURAL_RIGHT',
                        'IIF_FORMAT_DYNAMIC', 'IIF_FORMAT_MUTABLE_LITERAL')
        for debug, variant in ((debug, variant) for debug in (False, True) for variant in variants):
            defines = ['-d', variant] if variant else []
            if debug:
                defines += ['-g']
            emitted = subprocess.run([str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
                '-i', str(root / 'inc'), '-gen', args.backend, '-w', 'all', '-d', 'IIF_FORMAT_EXPORTS',
                '-c', '-r', *defines, str(source), '-o', str(cpp)], capture_output=True, text=True)
            if emitted.returncode:
                raise RuntimeError(emitted.stdout + emitted.stderr)
            checked = subprocess.run([args.cc, '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Wformat=2',
                '-Werror=format', '-Werror=format-nonliteral', '-c', str(cpp), '-o', str(obj)],
                capture_output=True, text=True)
            output = checked.stdout + checked.stderr
            if variant:
                expected = 'format-nonliteral' if variant in ('IIF_FORMAT_DYNAMIC', 'IIF_FORMAT_MUTABLE_LITERAL') else 'format'
                if checked.returncode == 0 or re.search(r'\[-W(?:error=|error,-W)' + expected + r'(?:=)?\]', output) is None:
                    raise RuntimeError(variant + ': expected native diagnostic\n' + output)
            elif checked.returncode:
                raise RuntimeError('conditional literal format contract failed\n' + output)
    print(args.backend + ': IIF_FORMAT_DIAGNOSTICS_PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of c-iif-formats.py
