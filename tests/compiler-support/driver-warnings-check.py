"""Project: FreeBASIC C backend regression tests
File: driver-warnings-check.py
Purpose: Check that the C driver leaves compiler diagnostics enabled.
Responsibilities: Inspect GCC/Clang commands and an uninitialized-value warning.
This script compiles its fixture but does not run the uninitialized program.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = next(path for path in HERE.parents if (path / 'mk/compiler-sources.mk').is_file())
DISABLED = ('-Wno-main', '-Wno-misleading-indentation', '-Wno-builtin-requires-header',
            '-Wno-uninitialized', '-Wno-warn-absolute-paths')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--tool-bin', required=True, type=Path)
    parser.add_argument('--js-prefix', type=Path, help='also compile and link a JavaScript-target program')
    parser.add_argument('--emcc-bin', type=Path, help='directory containing emcc')
    args = parser.parse_args()
    fbc, prefix = args.fbc.resolve(), args.prefix.resolve()
    environment = os.environ.copy()
    environment['PATH'] = str(args.tool_bin.resolve()) + os.pathsep + environment.get('PATH', '')
    if args.emcc_bin:
        environment['PATH'] = str(args.emcc_bin.resolve()) + os.pathsep + environment['PATH']
    with tempfile.TemporaryDirectory(prefix='fbc-driver-warnings-') as directory:
        work = Path(directory)
        for backend in ('gcc', 'clang'):
            command = [fbc, '-prefix', prefix, '-gen', backend, '-O', '2', '-Wc', '-Wextra',
                       '-v', '-c', HERE / 'driver-warning-probe.bas', '-o', work / (backend + '.o')]
            result = subprocess.run([str(item) for item in command], cwd=ROOT, env=environment,
                                    capture_output=True, text=True, errors='replace', timeout=120)
            output = result.stdout + result.stderr
            if result.returncode:
                raise RuntimeError('driver compilation failed:\n' + output)
            if '-Wall' not in output or any(flag in output for flag in DISABLED):
                raise RuntimeError('the driver disabled a C warning:\n' + output)
            if backend == 'clang' and not re.search(r'\[-W(?:(?:sometimes|conditional)-)?uninitialized\]', output):
                raise RuntimeError('Clang did not report the uninitialized value:\n' + output)
        if args.js_prefix:
            js_file = work / 'driver-warning-js.js'
            command = [fbc, '-prefix', args.js_prefix.resolve(), '-target', 'js-asmjs', '-gen', 'gcc',
                       '-v', '-x', js_file, HERE / 'js-link-probe.bas']
            result = subprocess.run([str(item) for item in command], cwd=ROOT, env=environment,
                                    capture_output=True, text=True, errors='replace', timeout=120)
            output = result.stdout + result.stderr
            if result.returncode or not js_file.is_file() or not js_file.with_suffix('.wasm').is_file():
                raise RuntimeError('JavaScript-target compilation failed:\n' + output)
            if '-Wno-' in output:
                raise RuntimeError('JavaScript-target compilation disabled a warning:\n' + output)
    print('DRIVER_WARNINGS_CHECK_PASS', flush=True)


if __name__ == '__main__':
    main()

# end of driver-warnings-check.py
