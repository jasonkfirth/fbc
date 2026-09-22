#!/usr/bin/env python3
"""FreeBASIC DOS provider builder: build-dos-pdmlwp.py.

Build the bundled, repaired PDMLWP C/assembly sources with a DJGPP compiler.
Produces an opt-in archive and preserves separate object files for relinking.
Does not download dependencies, install files, or build a DOS executable.
"""

import argparse
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--ar', required=True)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1] / 'contrib/dos/pdmlwp'
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    objects = output.parent / 'pdmlwp-objects'
    objects.mkdir(exist_ok=True)
    common = ['-I' + str(source / 'include'), '-march=i386', '-mno-sse', '-mno-mmx']
    # The imported scheduler locks address ranges delimited by source markers.
    # GCC may otherwise reorder functions/data across those markers.
    subprocess.run([args.cc, *common, '-std=gnu11', '-O1',
                    '-fno-lto', '-fno-toplevel-reorder', '-fno-reorder-functions',
                    '-fno-strict-aliasing', '-fno-omit-frame-pointer',
                    '-c', str(source / 'src/lwp.c'), '-o', str(objects / 'lwp.o')], check=True)
    subprocess.run([args.cc, *common, '-x', 'assembler-with-cpp',
                    '-c', str(source / 'src/lwpasm.s'), '-o', str(objects / 'lwpasm.o')], check=True)
    subprocess.run([args.ar, 'rcs', str(output), str(objects / 'lwp.o'),
                    str(objects / 'lwpasm.o')], check=True)
    print(output)


if __name__ == '__main__':
    main()

# end of build-dos-pdmlwp.py
