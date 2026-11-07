"""Project: FreeBASIC C backend regression tests.

File: c-vararg-integer-promotions.py
Purpose: Check narrow integer arguments in C variadic calls.
Responsibilities: Compile with GCC and Clang, run native backends, and check
    emitted C for Windows, Linux, ARM and AArch64 targets.
This file intentionally does NOT install or replace a compiler.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'tests/quirk/c-vararg-integer-promotions.bas'


def run(command, environment, expected_success=True):
    result = subprocess.run([str(part) for part in command], cwd=ROOT, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    output = result.stdout + result.stderr
    if (result.returncode == 0) != expected_success:
        raise RuntimeError('unexpected exit from ' + str(command) + '\n' + output)
    return result, output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    parser.add_argument('--baseline-fbc')
    parser.add_argument('--gas64', action='store_true')
    args = parser.parse_args()
    parent = args.work_dir.resolve(strict=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(args.gcc).resolve().parent) + os.pathsep + environment.get('PATH', '')
    warning_counts = {}

    # Own only the newly created child directory; large build trees are untouched.
    with tempfile.TemporaryDirectory(prefix='c-vararg-promotions-', dir=parent) as directory:
        work = Path(directory).resolve()
        if work.parent != parent:
            raise RuntimeError('temporary output is outside the work directory')
        c_file = work / 'case.c'
        emit = [args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-m', SOURCE.stem,
                '-i', ROOT / 'inc', '-r', SOURCE, '-o', c_file]
        run(emit, environment)
        gcc_flags = ['-std=gnu11', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                     '-Wformat=2', '-Wformat-signedness', '-Werror=format']
        clang_flags = ['-std=gnu11', '-O2', '-Weverything', '-Werror=format']
        for name, compiler, flags in (('gcc', args.gcc, gcc_flags), ('clang', args.clang, clang_flags)):
            obj = work / (name + '.o')
            _, output = run([compiler, *flags, '-c', c_file, '-o', obj], environment)
            warning_counts[name] = len(re.findall(r'(?m)^.*\bwarning:', output))
            exe = work / (name + ('.exe' if os.name == 'nt' else ''))
            run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-x', exe, obj], environment)
            result, _ = run([exe], environment)
            if result.stdout.strip() != 'C_VARARG_INTEGER_PROMOTIONS_PASS':
                raise RuntimeError(name + ' produced an unexpected result')

        if args.gas64:
            exe = work / ('gas64.exe' if os.name == 'nt' else 'gas64')
            run([args.fbc, '-prefix', args.prefix, '-gen', 'gas64', '-i', ROOT / 'inc',
                 '-x', exe, SOURCE], environment)
            result, _ = run([exe], environment)
            if result.stdout.strip() != 'C_VARARG_INTEGER_PROMOTIONS_PASS':
                raise RuntimeError('GAS64 produced an unexpected result')

        if args.baseline_fbc:
            baseline_c = work / 'baseline.c'
            run([args.baseline_fbc, '-prefix', args.prefix, '-gen', 'gcc', '-m', SOURCE.stem,
                 '-i', ROOT / 'inc', '-r', SOURCE, '-o', baseline_c], environment)
            _, output = run([args.gcc, *gcc_flags, '-c', baseline_c, '-o', work / 'baseline.o'],
                            environment, expected_success=False)
            if 'format' not in output.lower():
                raise RuntimeError('the baseline failed for an unrelated reason')

        targets = [('win32', 'i686-w64-windows-gnu'), ('linux', 'x86_64-unknown-linux-gnu'),
                   ('linux-arm', 'armv7-unknown-linux-gnueabihf'),
                   ('linux-aarch64', 'aarch64-unknown-linux-gnu')]
        for target, triple in targets:
            target_c = work / (target + '.c')
            run([args.fbc, '-prefix', args.prefix, '-target', target, '-gen', 'gcc',
                 '-m', SOURCE.stem, '-i', ROOT / 'inc', '-r', SOURCE, '-o', target_c], environment)
            _, output = run([args.clang, '--target=' + triple, '-std=gnu11', '-nostdinc',
                             '-O2', '-Weverything', '-ferror-limit=0', '-c', target_c,
                             '-o', work / (target + '.o')], environment)
            warning_counts[target] = len(re.findall(r'(?m)^.*\bwarning:', output))

    print('C_VARARG_INTEGER_PROMOTIONS_PASS targets=4 gcc_warnings=' + str(warning_counts['gcc']) +
          ' clang_warnings=' + str(warning_counts['clang']))


if __name__ == '__main__':
    main()


# end of c-vararg-integer-promotions.py
