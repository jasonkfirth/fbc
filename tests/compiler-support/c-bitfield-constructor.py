"""Project: FreeBASIC compiler regression tests.

File: c-bitfield-constructor.py
Purpose: Check storage initialization for automatically constructed bitfields.
Responsibilities: Exercise packed fields and neighboring data, check the
    threadcall regression with GCC, and compile the fixture for several targets.
This file intentionally does NOT replace the compiler's full bitfield suite.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'tests/compiler-support/c-bitfield-constructor.bas'
THREADCALL = ROOT / 'tests/threads/threadcall.bas'


def run(command, environment, expect_success=True):
    result = subprocess.run([str(part) for part in command], cwd=ROOT, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=180)
    output = result.stdout + result.stderr
    if (result.returncode == 0) != expect_success:
        raise RuntimeError('unexpected command result: ' + str(command) + '\n' + output[:5000])
    return result, len(re.findall(r'(?m)^.*\bwarning:', output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--baseline-fbc')
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    parser.add_argument('--gas64', action='store_true', help='also run the GAS64 backend')
    parser.add_argument('--llvm', action='store_true', help='also run emitted LLVM IR with Clang')
    args = parser.parse_args()
    args.work_dir.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(args.gcc).resolve().parent) + os.pathsep + environment.get('PATH', '')
    counts = {'gcc': 0, 'clang': 0}

    with tempfile.TemporaryDirectory(prefix='bitfield-ctor-', dir=args.work_dir) as temporary:
        work = Path(temporary)
        c_file = work / 'bitfield-ctor.c'
        run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-m', SOURCE.stem,
             '-r', SOURCE, '-o', c_file], environment)
        common = ['-std=gnu11', '-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-O2']
        for label, compiler, flags in (
                ('gcc', args.gcc, ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wuninitialized']),
                ('clang', args.clang, ['-Weverything', '-fsanitize=alignment,null,object-size,pointer-overflow',
                                       '-fsanitize-trap=alignment,null,object-size,pointer-overflow'])):
            obj = work / (label + '.o')
            _, counts[label] = run([compiler, *common, *flags, '-c', c_file, '-o', obj], environment)
            exe = work / (label + ('.exe' if os.name == 'nt' else ''))
            run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-x', exe, obj], environment)
            result, _ = run([exe], environment)
            if result.stdout.strip() != 'C_BITFIELD_CONSTRUCTOR_PASS':
                raise RuntimeError(label + ' changed bitfield constructor behavior')

        if args.gas64:
            exe = work / ('gas64.exe' if os.name == 'nt' else 'gas64')
            run([args.fbc, '-prefix', args.prefix, '-gen', 'gas64', '-x', exe, SOURCE], environment)
            result, _ = run([exe], environment)
            if result.stdout.strip() != 'C_BITFIELD_CONSTRUCTOR_PASS':
                raise RuntimeError('GAS64 changed bitfield constructor behavior')

        if args.llvm:
            ir = work / 'bitfield-ctor.ll'
            obj = work / 'llvm.o'
            exe = work / ('llvm.exe' if os.name == 'nt' else 'llvm')
            run([args.fbc, '-prefix', args.prefix, '-gen', 'llvm', '-m', SOURCE.stem,
                 '-r', SOURCE, '-o', ir], environment)
            run([args.clang, '-x', 'ir', '-O2', '-Weverything', '-c', ir, '-o', obj], environment)
            run([args.fbc, '-prefix', args.prefix, '-gen', 'llvm', '-x', exe, obj], environment)
            result, _ = run([exe], environment)
            if result.stdout.strip() != 'C_BITFIELD_CONSTRUCTOR_PASS':
                raise RuntimeError('LLVM changed bitfield constructor behavior')

        targets = [('win32', 'i686-w64-windows-gnu'),
                   ('linux-x86_64', 'x86_64-unknown-linux-gnu'),
                   ('linux-arm', 'armv7-unknown-linux-gnueabihf'),
                   ('linux-aarch64', 'aarch64-unknown-linux-gnu')]
        for target, triple in targets:
            target_c = work / (target + '.c')
            run([args.fbc, '-prefix', args.prefix, '-target', target, '-gen', 'gcc',
                 '-m', SOURCE.stem, '-r', SOURCE, '-o', target_c], environment)
            _, warnings = run([args.clang, '-target', triple, *common, '-Weverything',
                               '-c', target_c, '-o', work / (target + '.o')], environment)
            counts['clang'] += warnings

        # The threadcall test exposes a real read of an uninitialized bitfield
        # storage unit in the generated default constructor. Keep it visible
        # as an error here, even when the optional old compiler is unavailable.
        def check_threadcall(compiler, label, expect_success):
            generated = work / (label + '-threadcall.c')
            run([compiler, '-prefix', args.prefix, '-gen', 'gcc', '-mt',
                 '-i', ROOT / 'inc', '-i', ROOT / 'tests/fbcunit/inc',
                 '-m', 'fbc-tests', '-r', THREADCALL, '-o', generated], environment)
            result, warnings = run([args.gcc, *common, '-Wall', '-Wextra', '-Wuninitialized',
                                    '-Werror=uninitialized', '-c', generated,
                                    '-o', work / (label + '-threadcall.o')], environment,
                                   expect_success=expect_success)
            if not expect_success and '-Werror=uninitialized' not in result.stderr:
                raise RuntimeError('the baseline failed for a different reason')
            return warnings

        counts['gcc'] += check_threadcall(args.fbc, 'current', True)
        if args.baseline_fbc:
            check_threadcall(args.baseline_fbc, 'baseline', False)

    print('C_BITFIELD_CONSTRUCTOR_PASS gcc_warnings=' + str(counts['gcc']) +
          ' clang_warnings=' + str(counts['clang']))


if __name__ == '__main__':
    main()


# end of c-bitfield-constructor.py
