"""Project: FreeBASIC C backend regression tests.

File: c-private-global-storage.py
Purpose: Exercise private global storage and its initialization dependencies.
Responsibilities: Build both C generators, verify public data and owned lifetimes,
    and clean the private products after checking the runtime results.
This file intentionally does NOT change public data visibility or initialization order.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def run(command):
    checked = subprocess.run(command, capture_output=True, text=True, timeout=180)
    output = checked.stdout + checked.stderr
    if output:
        print(output, end='')
    if checked.returncode != 0 or 'warning:' in output:
        raise RuntimeError('private storage check failed: ' + str(command))
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang')
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    parent = args.work_dir.resolve(strict=True)
    work = Path(tempfile.mkdtemp(prefix='c-private-global-storage-', dir=parent)).resolve()
    if not work.is_relative_to(parent) or work == parent:
        raise RuntimeError('test output escaped the selected work directory')
    flags = ['-std=gnu11', '-O2', '-fno-strict-aliasing', '-fwrapv', '-Wall', '-Wextra',
             '-Wpedantic', '-Wconversion', '-Wsign-conversion', '-Wcast-qual',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2',
             '-Wundef', '-Wwrite-strings', '-Wpointer-arith', '-fno-diagnostics-color']
    tools = [('gcc', args.gcc)]
    if args.clang:
        tools.append(('clang', args.clang))
    try:
        for compiler_name, compiler in tools:
            selected_flags = flags.copy()
            if compiler_name == 'clang':
                selected_flags += ['-Wcast-function-type-strict', '-Wcomma']
            observer = work / ('observer-' + compiler_name + '.o')
            run([compiler, *selected_flags, '-c', str(source / 'c-private-global-storage-caller.c'),
                 '-o', str(observer)])
            for generator in ('gcc', 'clang'):
                key = compiler_name + '-' + generator
                cf, obj = work / (key + '.c'), work / (key + '.o')
                executable = work / (key + ('.exe' if os.name == 'nt' else ''))
                run([args.fbc, '-prefix', args.prefix, '-gen', generator, '-O', '2',
                     '-m', 'c-private-global-storage', '-r', '-c',
                     str(source / 'c-private-global-storage.bas'), '-o', str(cf)])
                run([compiler, *selected_flags, '-c', str(cf), '-o', str(obj)])
                run([args.fbc, '-prefix', args.prefix, str(obj), str(observer),
                     '-x', str(executable)])
                output = run([str(executable)])
                if ('C_PRIVATE_GLOBAL_STORAGE_PASS' not in output
                        or 'C_PRIVATE_GLOBAL_DTOR_PASS' not in output):
                    raise RuntimeError('private storage control did not finish')
        print('C_PRIVATE_GLOBAL_STORAGE_PASS runs=' + str(2 * len(tools)))
    finally:
        # Retire only the resolved, unique child of the explicit work directory.
        shutil.rmtree(work)


if __name__ == '__main__':
    main()

# end of c-private-global-storage.py
