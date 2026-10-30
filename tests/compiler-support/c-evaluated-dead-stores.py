"""Project: FreeBASIC C backend regression tests.

File: c-evaluated-dead-stores.py
Purpose: Compare discarded numeric evaluation against a reference compiler.
Responsibilities: Observe rounding modes, exception flags, calls and lifetimes
    with both C generators and two native optimization levels.
This file intentionally does NOT assume identical flags across optimization
    levels or suppress the native compiler's diagnostics.
"""
import argparse
from collections import Counter
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def run(command):
    checked = subprocess.run(command, capture_output=True, text=True, timeout=180)
    diagnostics = checked.stdout + checked.stderr
    if checked.returncode != 0:
        raise RuntimeError(str(command) + '\n' + diagnostics)
    if checked.stderr:
        print(checked.stderr, end='')
    return checked.stdout, Counter(re.findall(r'\[(-W[^\]]+)\]', diagnostics))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--baseline-fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    parent = args.work_dir.resolve(strict=True)
    work = Path(tempfile.mkdtemp(prefix='c-evaluated-dead-stores-', dir=parent)).resolve()
    if not work.is_relative_to(parent) or work == parent:
        raise RuntimeError('test output escaped the selected work directory')
    flags = ['-std=gnu11', '-fno-strict-aliasing', '-fwrapv', '-frounding-math',
             '-fno-math-errno', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion',
             '-Wsign-conversion', '-Wcast-qual', '-Wstrict-prototypes',
             '-Wmissing-prototypes', '-Wformat=2', '-Wundef', '-Wwrite-strings',
             '-Wpointer-arith', '-Wbad-function-cast', '-fno-diagnostics-color',
             '-fno-diagnostics-show-caret']
    observations = {}
    try:
        observer = work / 'observer.o'
        _, warnings = run([args.gcc, *flags, '-O2', '-c',
                           str(source / 'c-evaluated-dead-stores-caller.c'), '-o', str(observer)])
        if warnings:
            raise RuntimeError('floating environment observer has native warnings')
        for generator in ('gcc', 'clang'):
            for label, compiler in (('baseline', args.baseline_fbc), ('candidate', args.fbc)):
                cf = work / 'fixture.c'
                run([compiler, '-prefix', args.prefix, '-gen', generator,
                     '-m', 'c-evaluated-dead-stores', '-r',
                     str(source / 'c-evaluated-dead-stores.bas'), '-o', str(cf)])
                for opt in ('-O0', '-O2'):
                    obj = work / 'fixture.o'
                    executable = work / ('fixture.exe' if os.name == 'nt' else 'fixture')
                    _, warnings = run([args.gcc, *flags, opt, '-c', str(cf), '-o', str(obj)])
                    run([compiler, '-prefix', args.prefix, str(obj), str(observer),
                         '-x', str(executable)])
                    output, _ = run([str(executable)])
                    if 'C_EVALUATED_DEAD_STORES_PASS' not in output:
                        raise RuntimeError('floating environment control did not finish')
                    observations[generator, label, opt] = output, warnings
        for generator in ('gcc', 'clang'):
            for opt in ('-O0', '-O2'):
                before, old_warnings = observations[generator, 'baseline', opt]
                after, warnings = observations[generator, 'candidate', opt]
                if before != after:
                    raise RuntimeError('evaluation differs from reference: ' + generator + opt)
                if warnings['-Wunused-but-set-variable=']:
                    raise RuntimeError('discarded scalar destinations remain')
                if any(count > old_warnings[flag] for flag, count in warnings.items()):
                    raise RuntimeError('new native warnings: ' + generator + opt)
        print('C_EVALUATED_DEAD_STORES_PASS comparisons=4')
    finally:
        # Retire only the resolved, unique child of the explicit work directory.
        shutil.rmtree(work)


if __name__ == '__main__':
    main()

# end of c-evaluated-dead-stores.py
