#!/usr/bin/env python3
"""FreeBASIC Windows DLL data-import regression
File: test-import-data.py
Purpose: Exercise target import names against real native C DLLs.
Responsibilities: Both pointer widths, both backends and cross-language writes.
This file contains no import-library symbol workaround or compiler installation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
parser.add_argument('--fbc', type=Path, required=True)
parser.add_argument('--baseline', type=Path)
parser.add_argument('--outdir', type=Path)
args = parser.parse_args()
root = args.root.resolve()
compiler = args.fbc.resolve()
work = args.outdir.resolve() if args.outdir else root / 'out/test-import-data'
work.mkdir(parents=True, exist_ok=True)
records = []


def run(command, directory, environment, label):
    command = [str(part) for part in command]
    result = subprocess.run(command, cwd=directory, env=environment,
                            capture_output=True, text=True, timeout=60)
    (directory / (label + '.log')).write_text(repr(command) + '\n' + result.stdout + result.stderr)
    return result


for bits in (64, 32):
    directory = work / ('win' + str(bits))
    directory.mkdir(exist_ok=True)
    tools = Path('C:/msys64/mingw' + str(bits) + '/bin')
    environment = os.environ.copy()
    environment['PATH'] = str(directory) + ';' + str(tools) + ';C:/FreeBASIC/bin/win' + str(bits) + ';' + environment['PATH']
    environment.update(GCC=str(tools / 'gcc.exe'), AS=str(tools / 'as.exe'), LD=str(tools / 'ld.exe'))
    native = directory / 'import-data.dll'
    native.unlink(missing_ok=True)
    result = run([tools / 'gcc.exe', '-Wall', '-Wextra', '-Werror', '-shared',
                  root / 'tests/bindings/import-data.c', '-o', native,
                  '-Wl,--out-implib,' + str(directory / 'libimport-data.dll.a')],
                 directory, environment, 'c-dll')
    if result.returncode or not native.is_file():
        raise RuntimeError('Real C DLL build failed for Win' + str(bits))
    for backend in ('gcc', 'gas64' if bits == 64 else 'gas'):
        record = dict(bits=bits, backend=backend, passed=False,
                      compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),
                      library_sha256=hashlib.sha256(native.read_bytes()).hexdigest())
        try:
            executable = directory / ('import-' + backend + '.exe')
            executable.unlink(missing_ok=True)
            flags = ['-i', root / 'inc', '-p', directory, '-gen', backend, '-w', 'all']
            if bits == 32:
                flags += ['-target', 'win32', '-prefix', 'C:/FreeBASIC']
            else:
                flags += ['-prefix', root, '-p', root / 'lib/freebasic/win64']
            source = root / 'tests/bindings/import-data.bas'
            if args.baseline:
                baseline_exe = directory / ('baseline-' + backend + '.exe')
                baseline_exe.unlink(missing_ok=True)
                previous = run([args.baseline.resolve()] + flags + [source, '-x', baseline_exe],
                               directory, environment, 'baseline-' + backend)
                if bits == 64:
                    if previous.returncode == 0 or '_imp__import_' not in previous.stderr:
                        raise RuntimeError('Baseline did not reproduce the Win64 import-name failure')
                    record['baseline'] = 'expected import-name link failure reproduced'
                else:
                    if previous.returncode:
                        raise RuntimeError('Win32 baseline compilation failed')
                    previous = run([baseline_exe], directory, environment, 'baseline-run-' + backend)
                    if previous.returncode or 'DLL_IMPORT_DATA=PASS' not in previous.stdout:
                        raise RuntimeError('Win32 baseline runtime failed')
                    record['baseline'] = 'native Win32 runtime passed'
            result = run([compiler] + flags + [source, '-x', executable],
                         directory, environment, 'compile-' + backend)
            if result.returncode or not executable.is_file():
                raise RuntimeError('Candidate compilation failed')
            result = run([executable], directory, environment, 'run-' + backend)
            if result.returncode or 'DLL_IMPORT_DATA=PASS' not in result.stdout:
                raise RuntimeError('Candidate runtime failed')
            # Also qualify the actual manual example with its independent C DLL.
            manual_dll = directory / 'mydll.dll'
            manual_dll.unlink(missing_ok=True)
            result = run([tools / 'gcc.exe', '-Wall', '-Wextra', '-Werror', '-shared',
                          root / 'examples/manual/module/mydll.c', '-o', manual_dll,
                          '-Wl,--out-implib,' + str(directory / 'libmydll.dll.a')],
                         directory, environment, 'manual-c-' + backend)
            if result.returncode:
                raise RuntimeError('Manual DLL compilation failed')
            manual_exe = directory / ('manual-' + backend + '.exe')
            manual_exe.unlink(missing_ok=True)
            result = run([compiler] + flags + [root / 'examples/manual/module/import.bas', '-x', manual_exe],
                         directory, environment, 'manual-compile-' + backend)
            if result.returncode or not manual_exe.is_file():
                raise RuntimeError('Manual compilation failed')
            result = run([manual_exe], directory, environment, 'manual-run-' + backend)
            if result.returncode or result.stdout.strip() != '&h1234':
                raise RuntimeError('Manual DLL value differs from native C output')
            record['passed'] = True
        except Exception as error:
            record['error'] = str(error)
        records.append(record)
        (work / 'results.json').write_text(json.dumps(records, indent=2) + '\n')
        print('Win' + str(bits), backend, 'PASS' if record['passed'] else record['error'], flush=True)
raise SystemExit(not all(record['passed'] for record in records))
# end of test-import-data.py
