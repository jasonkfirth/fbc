"""Test signed left shifts and pointer offsets in emitted C.

Project: FreeBASIC compiler regression tests
File: c-signed-shifts.py
Purpose: Check defined C shifts against BASIC results and native GAS64.
Responsibilities: Run strict native and cross-target C compilation and shift checks.
This file intentionally does NOT claim other emitted C is warning-free.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def execute(command, cwd, environment, expect_success=True):
    result = subprocess.run([str(part) for part in command], cwd=cwd, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    if expect_success and result.returncode:
        raise RuntimeError('command failed: ' + ' '.join(str(x) for x in command) +
                           '\n' + (result.stdout + result.stderr)[:4000])
    return result


def emit(compiler, prefix, source, output, environment, backend, target=None):
    command = [compiler, '-prefix', prefix, '-gen', backend, '-g', '-w', 'all',
               '-m', source.stem, '-r']
    if target:
        command.extend(['-target', target])
    if target not in ('linux-arm', 'linux-aarch64'):
        command.extend(['-asm', 'intel'])
    execute([*command, source, '-o', output], output.parent, environment)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--gcc', required=True, type=Path)
    parser.add_argument('--clang', required=True, type=Path)
    parser.add_argument('--work-dir', required=True, type=Path)
    parser.add_argument('--baseline-fbc', type=Path)
    parser.add_argument('--gas64', action='store_true')
    args = parser.parse_args()
    compiler, prefix, gcc, clang = (path.resolve() for path in (args.fbc, args.prefix, args.gcc, args.clang))
    source = HERE / 'c-signed-shifts.bas'
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment['PATH']
    args.work_dir.mkdir(parents=True, exist_ok=True)
    counts = {}
    with tempfile.TemporaryDirectory(prefix='c-signed-shifts-', dir=args.work_dir.resolve()) as directory:
        work = Path(directory)
        profiles = {
            'gcc': [gcc, '-std=gnu11', '-nostdinc', '-Wall', '-Wextra', '-Wpedantic',
                    '-Wconversion', '-Wformat=2', '-Werror=implicit-function-declaration', '-masm=intel'],
            'clang': [clang, '-std=gnu11', '-nostdinc', '-Weverything', '-ferror-limit=0',
                      '-Werror=implicit-function-declaration', '-masm=intel'],
        }
        for backend in ('gcc', 'clang'):
            c_file = work / (backend + '.c')
            emit(compiler, prefix, source, c_file, environment, backend)
            for name, flags in profiles.items():
                for sanitized in (False, True):
                    suffix = backend + '-' + name + ('-shift-check' if sanitized else '-optimized')
                    obj = work / (suffix + '.o')
                    options = ['-O0', '-fsanitize=shift', '-fsanitize-undefined-trap-on-error'] if sanitized else ['-O2', '-fwrapv']
                    result = execute([*flags, *options, '-c', c_file, '-o', obj], work, environment)
                    counts[suffix] = len(re.findall(r'(?m)^.*\bwarning:', result.stdout + result.stderr))
                    exe = work / (suffix + '-test.exe')
                    execute([compiler, '-prefix', prefix, '-x', exe, obj], work, environment)
                    execute([exe], work, environment)
        if args.gas64:
            exe = work / 'gas64-test.exe'
            execute([compiler, '-prefix', prefix, '-gen', 'gas64', '-x', exe, source], work, environment)
            execute([exe], work, environment)
        if args.baseline_fbc:
            baseline = args.baseline_fbc.resolve()
            c_file = work / 'before.c'
            obj = work / 'before.o'
            exe = work / 'before-test.exe'
            emit(baseline, prefix, source, c_file, environment, 'gcc')
            execute([clang, '-std=gnu11', '-nostdinc', '-O0', '-fsanitize=shift',
                     '-fsanitize-undefined-trap-on-error', '-masm=intel', '-c', c_file, '-o', obj],
                    work, environment)
            execute([baseline, '-prefix', prefix, '-x', exe, obj], work, environment)
            if execute([exe], work, environment, False).returncode == 0:
                raise RuntimeError('the baseline unexpectedly passed the shift sanitizer')
        cross_warnings = 0
        targets = [('win32', 'i686-w64-windows-gnu'), ('linux', 'x86_64-unknown-linux-gnu'),
                   ('linux-arm', 'armv7-unknown-linux-gnueabihf'),
                   ('linux-aarch64', 'aarch64-unknown-linux-gnu')]
        for target, triple in targets:
            for backend in ('gcc', 'clang'):
                c_file = work / (target + '-' + backend + '.c')
                emit(compiler, prefix, source, c_file, environment, backend, target)
                result = execute([clang, '--target=' + triple, '-std=gnu11', '-nostdinc', '-O2',
                                  '-Weverything', '-ferror-limit=0', '-c', c_file,
                                  '-o', work / (target + '-' + backend + '.o')], work, environment)
                cross_warnings += len(re.findall(r'(?m)^.*\bwarning:', result.stdout + result.stderr))
    print('C_SIGNED_SHIFTS_PASS warnings=' + str(counts) + ' cross_warnings=' + str(cross_warnings))


if __name__ == '__main__':
    main()

# end of c-signed-shifts.py
