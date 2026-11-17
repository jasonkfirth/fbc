"""Project: FreeBASIC C backend regression
File: c-bulk-memory-arguments.py
Purpose: Preserve actual native overrun diagnostics at byte-argument boundaries.
Responsibilities: Compile valid and invalid spans through the normal compiler path.
Runtime byte comparisons belong to the paired BASIC and C fixtures.
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
    version = subprocess.run([args.cc, '--version'], capture_output=True, text=True, check=True)
    if 'clang' in version.stdout.lower():
        diagnostics = ['-Warray-bounds', '-Wfortify-source',
            '-Werror=array-bounds', '-Werror=fortify-source']
    else:
        diagnostics = ['-Warray-bounds=2', '-Wstringop-overflow=4',
            '-Werror=array-bounds', '-Werror=stringop-overflow']
    with tempfile.TemporaryDirectory(prefix='fbc-memory-arguments-') as directory:
        work = Path(directory)
        cpp, obj = work / 'case.c', work / 'case.o'
        for invalid in (False, True):
            command = [str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
                '-i', str(root / 'inc'), '-gen', args.backend, '-w', 'all',
                '-d', 'MEMORY_ARGUMENT_EXPORTS', '-c', '-r',
                *(['-d', 'MEMORY_ARGUMENT_BAD_EXTENT'] if invalid else []),
                str(source), '-o', str(cpp)]
            emitted = subprocess.run(command, capture_output=True, text=True)
            if emitted.returncode:
                raise RuntimeError(emitted.stdout + emitted.stderr)
            if not invalid and re.search(r'\b(?:memset|memcpy|memmove)\([^\n]*->(?:FIRST|FOLLOWING)', cpp.read_text()):
                raise RuntimeError('byte argument retained a scalar C subobject origin')
            checked = subprocess.run([args.cc, '-std=gnu11', '-O2', '-Wall', '-Wextra',
                *diagnostics, '-c', str(cpp), '-o', str(obj)],
                capture_output=True, text=True)
            output = checked.stdout + checked.stderr
            if invalid:
                if checked.returncode == 0 or re.search(r'\[-W(?:error=|error,-W)(?:array-bounds|stringop-overflow|fortify-source)', output) is None:
                    raise RuntimeError('expected native overrun diagnostic\n' + output)
            elif checked.returncode:
                raise RuntimeError('valid record byte operations failed\n' + output)
    print(f'{args.backend}: BULK_MEMORY_ARGUMENT_DIAGNOSTICS_PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of c-bulk-memory-arguments.py
