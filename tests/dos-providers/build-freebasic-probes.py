#!/usr/bin/env python3
"""FreeBASIC DOS tests: build-freebasic-probes.py.

Build language-level integration probes with the real compiler driver and
DJGPP tools. Stage archives privately to support both compiler installation
layouts; do not modify the host compiler's installed runtime directories.
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--cc', required=True, type=Path)
    parser.add_argument('--libdir', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--graphics', action='store_true',
                        help='also exercise graphics with preempted workers and queued mutex handoff')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    stage = output / 'fb-prefix'
    for layout in ['lib/dos', 'lib/freebasic/dos']:
        destination = stage / layout
        destination.mkdir(parents=True, exist_ok=True)
        for name in ['i386go32.x', 'fbrt0.o', 'fbrt1.o', 'fbrt2.o',
                     'libfb.a', 'libfbmt.a', 'libfbpdmlwp.a', 'libsfx.a', 'libsfxmt.a']:
            shutil.copyfile(args.libdir / name, destination / name)
        if args.graphics:
            shutil.copyfile(args.libdir / 'libfbgfxmt.a', destination / 'libfbgfxmt.a')
    cc = args.cc.resolve()
    env = dict(os.environ, GCC=str(cc))
    for variable, tool in [('AS', 'as'), ('LD', 'ld'), ('AR', 'ar')]:
        env[variable] = str(cc.with_name(cc.name.replace('gcc', tool)))
    libraries = []
    for archive in ['libc.a', 'libgcc.a', 'crt0.o']:
        path = subprocess.check_output([str(cc), '-print-file-name=' + archive], text=True).strip()
        if not Path(path).is_file():
            raise RuntimeError('DJGPP could not locate ' + archive)
        libraries += ['-p', str(Path(path).parent)]
        if archive == 'crt0.o':
            for layout in ['lib/dos', 'lib/freebasic/dos']:
                shutil.copyfile(path, stage / layout / archive)
    common = [str(args.fbc.resolve()), '-target', 'dos', '-prefix', str(stage),
              '-i', str(root / 'inc'), *libraries]
    for backend in ['gas', 'gcc']:
        subprocess.run([*common, '-dos-threads', 'pdmlwp', '-gen', backend,
                        str(root / 'tests/dos-providers/freebasic-threads.bas'),
                        '-x', str(output / ('fbthreads-' + backend + '.exe'))], env=env, check=True)
        if args.graphics:
            subprocess.run([*common, '-dos-threads', 'pdmlwp', '-gen', backend,
                            str(root / 'tests/dos-providers/freebasic-graphics-threads.bas'),
                            '-x', str(output / ('fbgfxthreads-' + backend + '.exe'))], env=env, check=True)
    subprocess.run([*common, str(root / 'tests/dos-providers/qb-default.bas'),
                    '-x', str(output / 'qb-default.exe')], env=env, check=True)
    subprocess.run([*common, '-dos-threads', 'pdmlwp',
                    str(root / 'tests/dos-providers/pcspeaker-demo.bas'),
                    '-x', str(output / 'pcspeaker-demo.exe')], env=env, check=True)


if __name__ == '__main__':
    main()

# end of build-freebasic-probes.py
