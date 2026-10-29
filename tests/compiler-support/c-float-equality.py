"""Project: FreeBASIC C comparison regression tests.

File: c-float-equality.py
Purpose: Compare generated equality against independent native C operators.
Responsibilities: Build the separate oracle, test GCC/Clang C output and
    remove private products after checking results and the floating environment.
This file intentionally does NOT use a tolerance or suppress warning flags.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def run(command):
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    if result.stdout:
        print(result.stdout, end='')
    if result.stderr:
        print(result.stderr, end='')
    if result.returncode != 0:
        raise RuntimeError(f'command failed ({result.returncode}): {command}')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    parent = args.work_dir.resolve(strict=True)
    work = Path(tempfile.mkdtemp(prefix='c-float-equality-', dir=parent)).resolve()
    if not work.is_relative_to(parent) or work == parent:
        raise RuntimeError('test output escaped the selected work directory')
    flags = ['-std=gnu11', '-O2', '-frounding-math', '-Wall', '-Wextra', '-Wpedantic',
             '-Wconversion', '-Wsign-conversion', '-Wfloat-equal', '-Wdouble-promotion',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wmissing-declarations',
             '-fno-diagnostics-color']
    try:
        harness, oracle = work / 'harness.o', work / 'oracle.o'
        run([args.gcc, *flags, '-c', str(source / 'c-float-equality.c'), '-o', str(harness)])
        reference = run([args.gcc, *flags, '-c', str(source / 'c-float-equality-oracle.c'), '-o', str(oracle)])
        warnings = [line for line in reference.stderr.splitlines() if 'warning:' in line]
        if len(warnings) != 6 or any('[-Wfloat-equal]' not in line for line in warnings):
            raise RuntimeError('the independent oracle has unexpected diagnostics')
        for backend in ('gcc', 'clang'):
            emitted, obj = work / ('worker-' + backend + '.c'), work / ('worker-' + backend + '.o')
            executable = work / ('comparison-' + backend + ('.exe' if os.name == 'nt' else ''))
            run([args.fbc, '-prefix', args.prefix, '-gen', backend, '-O', '2', '-r', '-c',
                 str(source / 'c-float-equality.bas'), '-o', str(emitted)])
            checked = run([args.gcc, *flags, '-c', str(emitted), '-o', str(obj)])
            if 'warning:' in checked.stdout + checked.stderr:
                raise RuntimeError('the generated equality worker has GCC warnings')
            run([args.fbc, '-prefix', args.prefix, str(obj), str(harness), str(oracle), '-x', str(executable)])
            checked = run([str(executable)])
            if 'C_FLOAT_EQUALITY_PASS checks=1936 modes=4 single_evaluation=1' not in checked.stdout:
                raise RuntimeError('the comparison matrix did not finish')
    finally:
        # Retire only this resolved, unique child of the explicit work directory.
        shutil.rmtree(work)


if __name__ == '__main__':
    main()

# end of c-float-equality.py
