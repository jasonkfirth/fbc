#!/usr/bin/env python3
"""FreeBASIC DOS tests: build-runtime-probes.py.

Link the runtime and audio integration probes against already built DOS
provider archives. Requires a DJGPP toolchain; does not download or install it.
"""

import argparse
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--libdir', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--ac97-only', action='store_true')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    sources = Path(__file__).resolve().parent
    libraries = ['-Wl,--start-group', *[str(args.libdir / name) for name in
                 ['libsfxmt.a', 'libfbmt.a', 'libfbpdmlwp.a']], '-lm', '-Wl,--end-group']
    wrappers = ('-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free,'
                '--wrap=delay,--wrap=__dpmi_int,--wrap=uclock')
    names = ['ac97-playback'] if args.ac97_only else [
        'rtlib-threads', 'sfx-background', 'pcspeaker-background', 'ac97-playback']
    for name in names:
        subprocess.run([args.cc, '-std=gnu11', '-O2', '-mno-sse', '-mno-mmx',
                        '-Wall', '-Wextra', '-Werror=implicit-function-declaration',
                        '-DDISABLE_WCHAR', '-DENABLE_MT', '-DFB_DOS_PDMLWP',
                        str(sources / (name + '.c')), *libraries, wrappers,
                        '-o', str(args.output / (name + '.exe'))], check=True)
        print('Built ' + name)
    # The same hardware test must also work with the ordinary DOS runtime;
    # no provider archive or linker wrappers may enter this executable.
    subprocess.run([args.cc, '-std=gnu11', '-O2', '-Wall', '-Wextra',
                    '-Werror=implicit-function-declaration', '-DDISABLE_WCHAR',
                    str(sources / 'ac97-playback.c'), '-Wl,--start-group',
                    str(args.libdir / 'libsfx.a'), str(args.libdir / 'libfb.a'),
                    '-lm', '-Wl,--end-group',
                    '-o', str(args.output / 'ac97-default.exe')], check=True)
    print('Built ac97-default')


if __name__ == '__main__':
    main()

# end of build-runtime-probes.py
