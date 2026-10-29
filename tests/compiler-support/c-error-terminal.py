"""Project: FreeBASIC C emitter regression tests.

File: c-error-terminal.py
Purpose: Run the terminal error worker with a real owner and a bad return.
Responsibilities: Build GCC/Clang C output, link the C observer, check both
    control paths and remove the private build products.
This file intentionally does NOT qualify foreign legacy-label transfers.
Requires a native FreeBASIC prefix, GCC and a GNU linker with --wrap support.
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
    runtime = source.parents[1] / 'src/rtlib'
    parent = args.work_dir.resolve(strict=True)
    work = Path(tempfile.mkdtemp(prefix='c-error-terminal-', dir=parent)).resolve()
    if not work.is_relative_to(parent) or work == parent:
        raise RuntimeError('test output escaped the selected work directory')
    flags = ['-std=gnu11', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion',
             '-Wsign-conversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wmissing-declarations', '-fno-diagnostics-color']
    try:
        harness = work / 'harness.o'
        run([args.gcc, *flags, '-I', str(runtime), '-c', str(source / 'c-error-terminal.c'),
             '-o', str(harness)])
        for backend in ('gcc', 'clang'):
            emitted = work / ('worker-' + backend + '.c')
            obj = work / ('worker-' + backend + '.o')
            executable = work / ('terminal-' + backend + ('.exe' if os.name == 'nt' else ''))
            run([args.fbc, '-prefix', args.prefix, '-gen', backend, '-e', '-O', '2', '-r', '-c',
                 str(source / 'c-error-terminal.bas'), '-o', str(emitted)])
            checked = run([args.gcc, *flags, '-include', str(source / 'c-error-terminal-shim.h'),
                           '-D__builtin_abort=fb_test_invalid_transfer', '-c', str(emitted),
                           '-o', str(obj)])
            if 'warning:' in checked.stdout + checked.stderr:
                raise RuntimeError('the generated terminal worker has GCC warnings')
            run([args.fbc, '-prefix', args.prefix, str(obj), str(harness),
                 '-Wl', '--wrap=fb_ErrorThrowEx', '-x', str(executable)])
            run([str(executable)])
            run([str(executable), 'inject'])
        print('C_ERROR_TERMINAL_PASS cases=4 generated_worker_warnings=0')
    finally:
        # The unique directory was resolved and checked inside the explicit
        # parent before any build. No source or shared build directory is removed.
        shutil.rmtree(work)


if __name__ == '__main__':
    main()

# end of c-error-terminal.py
