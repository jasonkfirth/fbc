"""Project: FreeBASIC C backend regression
File: c-pointer-equality-views.py
Purpose: Check object pointer equality and genuine const removal diagnostics.
Responsibilities: Use normal compiler entry points and native qualification errors.
Runtime byte and layout checks belong to the paired C observer.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

from native_c_support import native_command, native_machine_flags


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--backend', choices=('gcc', 'clang'), required=True)
    parser.add_argument('--cc')
    args = parser.parse_args()
    compiler = args.cc or os.environ.get(args.backend.upper()) or args.backend
    source = Path(__file__).with_suffix('.bas')
    with tempfile.TemporaryDirectory(prefix='fbc-pointer-equality-') as directory:
        work = Path(directory)
        cpp, obj = work / 'case.c', work / 'case.o'
        for explicit_drop in (False, True):
            defines = ['-d', 'POINTER_COMPARE_CONST_DROP'] if explicit_drop else []
            command = [str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
                '-gen', args.backend, '-w', 'all', '-w', 'constness',
                '-d', 'POINTER_COMPARE_LIBRARY', '-c', '-r', *defines, str(source), '-o', str(cpp)]
            emitted = subprocess.run(command, cwd=work, capture_output=True, text=True)
            output = emitted.stdout + emitted.stderr
            if emitted.returncode:
                raise RuntimeError(output)
            source_warning = re.search(r'warning 40\([^)]*\): CONST qualifier discarded', output) is not None
            if source_warning != explicit_drop:
                raise RuntimeError('Unexpected source const-removal diagnostic\n' + output)
            for optimization in ('0', '2'):
                command = [compiler, '-std=gnu11', '-O' + optimization,
                    *native_machine_flags(args.fbc.resolve()), '-Wall', '-Wextra',
                    '-Wcast-qual', '-Werror', '-c', str(cpp), '-o', str(obj)]
                checked = subprocess.run(native_command(command), cwd=work, capture_output=True, text=True)
                output = checked.stdout + checked.stderr
                if explicit_drop:
                    diagnostic = re.search(r'\[-W(?:error=|error,-W)cast-qual\]', output) is not None
                    if checked.returncode == 0 or not diagnostic:
                        raise RuntimeError('Explicit const removal lost its native diagnostic\n' + output)
                elif checked.returncode:
                    raise RuntimeError('Valid pointer comparison failed\n' + output)
                else:
                    observer = work / 'observer.o'
                    executable = work / ('values.exe' if os.name == 'nt' else 'values')
                    observer_command = command[:command.index('-c')] + [
                        '-c', str(source.with_suffix('.c')), '-o', str(observer)]
                    observed = subprocess.run(native_command(observer_command), cwd=work,
                        capture_output=True, text=True)
                    if observed.returncode:
                        raise RuntimeError(observed.stdout + observed.stderr)
                    linked = subprocess.run(native_command([compiler,
                        *native_machine_flags(args.fbc.resolve()), str(obj), str(observer),
                        '-o', str(executable)]), cwd=work, capture_output=True, text=True)
                    if linked.returncode:
                        raise RuntimeError(linked.stdout + linked.stderr)
                    executed = subprocess.run([str(executable)], cwd=work, capture_output=True, text=True)
                    if executed.returncode or executed.stdout.strip() != 'POINTER_EQUALITY_PASS cases=580 checks=5764':
                        raise RuntimeError('Pointer equality reference check failed\n' + executed.stdout + executed.stderr)
        invalid = subprocess.run([str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
            '-gen', args.backend, '-d', 'POINTER_COMPARE_LIBRARY', '-d', 'POINTER_COMPARE_BAD_SCALAR',
            '-c', '-r', str(source), '-o', str(cpp)], cwd=work, capture_output=True, text=True)
        if invalid.returncode == 0 or 'Type mismatch' not in invalid.stdout + invalid.stderr:
            raise RuntimeError('Invalid scalar comparison lost its frontend diagnostic\n' + invalid.stdout + invalid.stderr)
    print(f'{args.backend}: POINTER_EQUALITY_DIAGNOSTICS_PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of c-pointer-equality-views.py
