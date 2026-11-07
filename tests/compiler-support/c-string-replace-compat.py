"""Check standard and project-provided Replace declarations.

Project: FreeBASIC string header regression
File: c-string-replace-compat.py
Purpose: Check string helper defaults and the optional Replace opt-out.
Responsibilities: Compile and run native tests and compile cross-target C.
This file intentionally does NOT change or install runtime headers.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = next(path for path in HERE.parents if (path / 'mk/compiler-sources.mk').is_file())


def run(command, cwd, environment):
    result = subprocess.run([str(part) for part in command], cwd=cwd, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    if result.returncode:
        raise RuntimeError('command failed: ' + ' '.join(str(part) for part in command) +
                           '\n' + (result.stdout + result.stderr)[:4000])
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--gcc', required=True, type=Path)
    parser.add_argument('--clang', required=True, type=Path)
    parser.add_argument('--headers', type=Path, default=ROOT / 'inc')
    parser.add_argument('--gas64', action='store_true')
    args = parser.parse_args()
    compiler, prefix = args.fbc.resolve(), args.prefix.resolve()
    gcc, clang, headers = args.gcc.resolve(), args.clang.resolve(), args.headers.resolve()
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')
    counts = {'gcc': 0, 'clang': 0}
    cross_warnings = 0
    sources = [HERE / 'c-string-replace-default.bas', HERE / 'c-string-replace-optout.bas']
    with tempfile.TemporaryDirectory(prefix='fbc-string-replace-') as directory:
        work = Path(directory)
        for source in sources:
            for backend in ('gcc', 'clang'):
                c_file = work / (source.stem + '-' + backend + '.c')
                run([compiler, '-prefix', prefix, '-i', headers, '-i', ROOT / 'inc',
                     '-gen', backend, '-asm', 'intel', '-g', '-w', 'all', '-m', source.stem,
                     '-r', source, '-o', c_file], work, environment)
                for name, native_compiler in (('gcc', gcc), ('clang', clang)):
                    obj = work / (source.stem + '-' + backend + '-' + name + '.o')
                    flags = (['-std=gnu11', '-nostdinc', '-O2', '-Wall', '-Wextra',
                              '-Wpedantic', '-Wconversion', '-Wsign-conversion', '-Wshadow',
                              '-Wformat=2', '-masm=intel'] if name == 'gcc' else
                             ['-std=gnu11', '-nostdinc', '-O2', '-Weverything',
                              '-ferror-limit=0', '-masm=intel'])
                    result = run([native_compiler, *flags, '-c', c_file, '-o', obj], work, environment)
                    counts[name] += len(re.findall(r'(?m)^.*\bwarning:', result.stdout + result.stderr))
                    exe = work / (source.stem + '-' + backend + '-' + name + '.exe')
                    run([compiler, '-prefix', prefix, '-x', exe, obj], work, environment)
                    run([exe], work, environment)
            if args.gas64:
                exe = work / (source.stem + '-gas64.exe')
                run([compiler, '-prefix', prefix, '-i', headers, '-i', ROOT / 'inc',
                     '-gen', 'gas64', '-x', exe, source], work, environment)
                run([exe], work, environment)
        targets = [('win32', None, 'i686-w64-windows-gnu'),
                   ('linux', '686', 'i686-unknown-linux-gnu'),
                   ('linux', None, 'x86_64-unknown-linux-gnu'),
                   ('linux-arm', None, 'armv7-unknown-linux-gnueabihf'),
                   ('linux-aarch64', None, 'aarch64-unknown-linux-gnu')]
        for source in sources:
            for target, arch, triple in targets:
                for backend in ('gcc', 'clang'):
                    label = source.stem + '-' + triple + '-' + backend
                    c_file = work / (label + '.c')
                    command = [compiler, '-prefix', prefix, '-i', headers, '-i', ROOT / 'inc',
                               '-target', target, '-gen', backend, '-g', '-w', 'all',
                               '-m', source.stem, '-r']
                    if arch:
                        command.extend(['-arch', arch])
                    if target not in ('linux-arm', 'linux-aarch64'):
                        command.extend(['-asm', 'intel'])
                    run([*command, source, '-o', c_file], work, environment)
                    result = run([clang, '--target=' + triple, '-std=gnu11', '-nostdinc', '-O2',
                                  '-Weverything', '-ferror-limit=0', '-c', c_file,
                                  '-o', work / (label + '.o')], work, environment)
                    cross_warnings += len(re.findall(r'(?m)^.*\bwarning:', result.stdout + result.stderr))
    print('C_STRING_REPLACE_COMPAT_PASS native_warnings=' + str(counts) +
          ' cross_warnings=' + str(cross_warnings), flush=True)


if __name__ == '__main__':
    main()

# end of c-string-replace-compat.py
