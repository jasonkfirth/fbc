"""Project: FreeBASIC C backend regression tests
File: c-anonymous-header.py
Purpose: Check mixed C declarations under GCC LTO and execute their native ABI.
Responsibilities: Test both C producers with bounded scratch and normal runtime linking.
This file intentionally does NOT use Clang LTO or install runtime libraries.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def run(command, execute=False):
    command = list(map(str, command))
    result = subprocess.run(command, capture_output=True, text=True,
        timeout=15 if execute else 90,
        creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name == 'nt' else 0)
    output = result.stdout + result.stderr
    if result.returncode:
        raise RuntimeError('anonymous C header check failed: ' + repr(command) + '\n' + output[-5000:])
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'prefix', 'gcc'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    sources = Path(__file__).resolve().parent
    parent = args.work_dir.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix='anonymous-header-', dir=parent) as directory:
        work = Path(directory).resolve(strict=True)
        if work == parent or not work.is_relative_to(parent):
            raise RuntimeError('anonymous header scratch escaped the selected directory')
        cpp, obj, observer, combined = [work / n for n in ('unit.c', 'unit.o', 'observer.o', 'combined.o')]
        exe = work / ('unit.exe' if os.name == 'nt' else 'unit')
        for producer in ('gcc', 'clang'):
            run([args.fbc, '-prefix', args.prefix, '-gen', producer, '-e',
                '-m', 'c-anonymous-header', '-r', sources / 'c-anonymous-header.bas', '-o', cpp])
            match = re.search(r'struct\s+(fb_type_\w*HEADERSTATE)\s*\{', cpp.read_text())
            if not match:
                raise RuntimeError('anonymous header record tag was not emitted')
            flags = ['-std=gnu23', '-O2', '-flto', '-fno-strict-aliasing', '-fwrapv',
                '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion',
                '-Wstrict-prototypes', '-Wmissing-prototypes']
            run([args.gcc, *flags, '-nostdinc', '-c', cpp, '-o', obj])
            run([args.gcc, *flags, '-DFB_TEST_RECORD=' + match.group(1),
                '-c', sources / 'c-anonymous-header.c', '-o', observer])
            # Complete LTO before the normal FreeBASIC link. This checks the
            # declarations without duplicating platform runtime-library lists.
            run([args.gcc, '-flto', '-O2', '-nostdlib', '-r', '-flinker-output=nolto-rel',
                '-Werror=lto-type-mismatch', '-o', combined, obj, observer])
            run([args.fbc, '-prefix', args.prefix, combined, '-x', exe])
            output = run([exe], execute=True)
            if output.strip() != 'C_ANONYMOUS_HEADER_PASS':
                raise RuntimeError('anonymous header observers did not finish: ' + output)
            if sum(p.stat().st_size for p in work.iterdir() if p.is_file()) > 8 * 1024 * 1024:
                raise RuntimeError('anonymous header scratch exceeded its byte cap')
    print('C_ANONYMOUS_HEADER_PASS runs=2')


if __name__ == '__main__':
    main()

# end of c-anonymous-header.py
