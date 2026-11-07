"""Project: FreeBASIC compiler regression tests
File: windows-vararg-aggregates-check.py
Purpose: Check Windows C variadic aggregate calling conventions.
Responsibilities: Compile and run C and LLVM callers against a C receiver.
This test does not install a compiler or claim non-Windows execution.
"""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    parser.add_argument('--llc', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    args = parser.parse_args()
    compiler, prefix, gcc, clang, llc = (path.resolve() for path in
                                         (args.fbc, args.prefix, args.gcc, args.clang, args.llc))
    if os.name != 'nt':
        raise RuntimeError('this test requires a Windows host')
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + str(llc.parent) + os.pathsep + environment.get('PATH', '')
    source = HERE / 'windows-vararg-aggregates.bas'
    helper = HERE / 'windows-vararg-aggregates-helper.c'
    counts = {'llvm': 0, 'c': 0}
    with tempfile.TemporaryDirectory(prefix='fbc-windows-varargs-') as directory:
        work = Path(directory)

        def run(command):
            result = subprocess.run([str(value) for value in command], cwd=work, env=environment,
                                    capture_output=True, text=True, errors='replace', timeout=300)
            if result.returncode:
                raise RuntimeError('command failed: ' + ' '.join(str(value) for value in command) +
                                   '\n' + result.stdout + result.stderr)
            return result.stdout + result.stderr

        if run([compiler, '-print', 'target']).strip() != 'win64':
            raise RuntimeError('this test requires the Win64 compiler target')
        module_ir = work / 'candidate.ll'
        run([compiler, '-prefix', prefix, '-m', source.stem, '-gen', 'llvm', '-r', source, '-o', module_ir])
        if args.baseline:
            previous_ir = work / 'baseline.ll'
            run([args.baseline.resolve(), '-prefix', prefix, '-m', source.stem, '-gen', 'llvm',
                 '-r', source, '-o', previous_ir])
            if previous_ir.read_bytes() != module_ir.read_bytes():
                raise RuntimeError('the variadic aggregate IR differs from the baseline')
        for optimization in (0, 2):
            module = work / 'llvm.o'
            run([llc, '-mtriple=x86_64-w64-windows-gnu', '-filetype=obj', '-O=' + str(optimization),
                 module_ir, '-o', module])
            for cc in (gcc, clang):
                helper_object, exe = work / 'helper.o', work / 'vararg.exe'
                run([cc, '-std=gnu23', '-O' + str(optimization), '-Wall', '-Wextra', '-Wpedantic',
                     '-c', helper, '-o', helper_object])
                run([compiler, '-prefix', prefix, '-gen', cc.stem, '-x', exe, module, helper_object])
                if 'WINDOWS_VARARG_AGGREGATES_PASS' not in run([exe]):
                    raise RuntimeError('LLVM variadic argument test failed')
                counts['llvm'] += 1
        for cc in (gcc, clang):
            helper_object, exe = work / 'helper.o', work / 'vararg-c.exe'
            run([cc, '-std=gnu23', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-c', helper, '-o', helper_object])
            run([compiler, '-prefix', prefix, '-gen', cc.stem, '-m', source.stem, '-x', exe, source, helper_object])
            if 'WINDOWS_VARARG_AGGREGATES_PASS' not in run([exe]):
                raise RuntimeError('C variadic argument test failed')
            counts['c'] += 1
    print('WINDOWS_VARARG_AGGREGATES_CHECK_PASS', counts, flush=True)


if __name__ == '__main__':
    main()

# end of windows-vararg-aggregates-check.py
