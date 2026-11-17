"""FreeBASIC C variadic promotion diagnostic regression.
File: c-va-promotions.py
Check native unsigned range preservation and retained source format errors.
Runtime argument values belong to the paired BASIC and C fixture.
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
    clang = 'clang' in Path(args.cc).name.lower()
    format_flags = ['-Wformat-pedantic'] if clang else ['-Wformat-signedness']
    source = Path(__file__).with_suffix('.bas')
    root = source.parents[2]
    variants = ['', 'PROMOTION_EXPLICIT_LONG', 'PROMOTION_WRONG_ARGUMENT',
        'PROMOTION_WRONG_WIDE_ARGUMENT', 'PROMOTION_WRONG_SIGNED_ARGUMENT',
        'PROMOTION_DYNAMIC_FORMAT']
    with tempfile.TemporaryDirectory(prefix='fbc-va-promotions-') as directory:
        cpp = Path(directory) / 'case.c'
        obj = Path(directory) / 'case.o'
        for variant in variants:
            defines = ['-d', variant] if variant else []
            emitted = subprocess.run([str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
                '-i', str(root / 'inc'), '-gen', args.backend, '-w', 'all', '-d', 'PROMOTION_EXPORTS',
                *defines, '-c', '-r', str(source), '-o', str(cpp)], capture_output=True, text=True)
            if emitted.returncode:
                raise RuntimeError(emitted.stdout + emitted.stderr)
            checked = subprocess.run([args.cc, '-std=gnu11', '-O2', '-Wall', '-Wextra',
                '-Wformat=2', *format_flags, '-c', str(cpp), '-o', str(obj)], capture_output=True, text=True)
            output = checked.stdout + checked.stderr
            if checked.returncode:
                raise RuntimeError(output)
            warnings = re.findall(r'warning:.*?\[(-W[^\]]+)\]', output)
            expected = '-Wformat-nonliteral' if variant == 'PROMOTION_DYNAMIC_FORMAT' else '-Wformat'
            if variant.startswith('PROMOTION_WRONG_') or variant == 'PROMOTION_DYNAMIC_FORMAT':
                if expected not in warnings and expected + '=' not in warnings:
                    raise RuntimeError(f'{variant} lost its native format diagnostic\n{output}')
            elif not variant and any(warning.startswith('-Wformat') for warning in warnings):
                raise RuntimeError(f'Valid narrow unsigned arguments gained diagnostics\n{output}')
            if not variant:
                call = next(line for line in cpp.read_text().splitlines() if '= snprintf(' in line)
                if '(int32)' in call:
                    raise RuntimeError(f'Unexpected explicit integer promotions\n{call}')
            elif variant == 'PROMOTION_EXPLICIT_LONG':
                call = next(line for line in cpp.read_text().splitlines() if '= snprintf(' in line)
                if call.count('(int32)') < 2:
                    raise RuntimeError(f'Explicit source LONG casts were removed\n{call}')
    print(f'{args.backend}: VA_PROMOTION_DIAGNOSTICS_PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of c-va-promotions.py
