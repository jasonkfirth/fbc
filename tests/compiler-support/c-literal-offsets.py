"""Project: FreeBASIC C backend regression.
File: c-literal-offsets.py
Purpose: Preserve native format checking across bounded literal choices.
Responsibilities: Accept the proof cases and retain dynamic/type diagnostics.
Runtime values and pointer identity have paired BASIC and C fixtures.
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
    parser.add_argument('--cc')
    args = parser.parse_args()
    compiler = args.cc or os.environ.get(args.backend.upper()) or args.backend
    source = Path(__file__).with_suffix('.bas')
    root = source.parents[2]
    variants = ('', 'BAD_ARGUMENT', 'DYNAMIC', 'UNSIGNED', 'EMPTY', 'NEGATIVE')
    with tempfile.TemporaryDirectory(prefix='fbc-literal-offsets-') as directory:
        work = Path(directory)
        cpp, obj = work / 'case.c', work / 'case.o'
        for variant in variants:
            defines = ['-d', 'LITERAL_OFFSET_' + variant] if variant else []
            emitted = subprocess.run([str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
                '-i', str(root / 'inc'), '-gen', args.backend, '-w', 'all', '-m', source.stem,
                '-c', '-r', *defines, str(source), '-o', str(cpp)], capture_output=True, text=True)
            if emitted.returncode:
                raise RuntimeError(emitted.stdout + emitted.stderr)
            for optimization in ('0', '2'):
                checked = subprocess.run([compiler, '-std=gnu11', '-O' + optimization,
                    '-Wall', '-Wextra', '-Wformat=2', '-Werror=format', '-Werror=format-nonliteral',
                    '-c', str(cpp), '-o', str(obj)], capture_output=True, text=True)
                output = checked.stdout + checked.stderr
                if variant:
                    expected = 'format' if variant == 'BAD_ARGUMENT' else 'format-nonliteral'
                    pattern = r'\[-W(?:error=|error,-W)' + expected + r'(?:=)?\]'
                    if checked.returncode == 0 or re.search(pattern, output) is None:
                        raise RuntimeError(f'{variant}: expected native diagnostic\n{output}')
                elif checked.returncode:
                    raise RuntimeError('bounded literal format failed\n' + output)
    print(f'{args.backend}: LITERAL_OFFSET_DIAGNOSTICS_PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of c-literal-offsets.py
