#!/usr/bin/env python3
"""FreeBASIC GNU libjit native qualification
File: test-jit.py
Purpose: Compare real C layouts and execute original examples on both backends.
Responsibilities: Private SDK selection, actual result checks and bounded processes.
This file contains no simulated JIT calls or system installation.
"""
from pathlib import Path
import argparse, hashlib, json, os, re, subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
parser.add_argument('--sdk-prefix', type=Path, required=True)
parser.add_argument('--bits', type=int, choices=(32, 64), default=64)
parser.add_argument('--fbc', type=Path)
parser.add_argument('--cc', type=Path)
parser.add_argument('--outdir', type=Path)
args = parser.parse_args()
root = args.root.resolve()
prefix = args.sdk_prefix.resolve()
windows = os.name == 'nt'
work = args.outdir.resolve() if args.outdir else root / ('out/test-jit-' + str(args.bits))
work.mkdir(parents=True, exist_ok=True)
environment = os.environ.copy()
if windows:
    compiler = root / 'bin/fbc-serialization.exe'
    cc = Path('C:/msys64/mingw64/bin/gcc.exe')
    suffix = '.exe'
    libraries = [root / 'lib/freebasic/win64', prefix / 'lib', Path('C:/FreeBASIC/lib/win64')]
    environment['PATH'] = str(prefix / 'bin') + ';C:/msys64/mingw64/bin;C:/FreeBASIC/bin/win64;' + environment['PATH']
    environment.update(GCC=str(cc), AS='C:/msys64/mingw64/bin/as.exe', LD='C:/msys64/mingw64/bin/ld.exe')
    if args.bits == 32:
        compiler = Path('C:/FreeBASIC/fbc.exe')
        cc = Path('C:/msys64/mingw32/bin/gcc.exe')
        libraries = [prefix / 'lib', Path('C:/FreeBASIC/lib/win32')]
        environment['PATH'] = str(prefix / 'bin') + ';C:/msys64/mingw32/bin;C:/FreeBASIC/bin/win32;' + environment['PATH']
        environment.update(GCC=str(cc), AS='C:/msys64/mingw32/bin/as.exe', LD='C:/msys64/mingw32/bin/ld.exe')
else:
    compiler = root / 'bin/fbc'
    cc = Path('/usr/bin/gcc')
    suffix = ''
    libraries = [prefix / 'lib']
    environment['LD_LIBRARY_PATH'] = str(prefix / 'lib')

if args.fbc:
    compiler = args.fbc.resolve()
if args.cc:
    cc = args.cc.resolve()
    environment['GCC'] = str(cc)

def run(command, log):
    command = [str(part) for part in command]
    result = subprocess.run(command, cwd=work, env=environment, capture_output=True, text=True, timeout=60)
    log.write_text(repr(command) + '\n' + result.stdout + result.stderr)
    if result.returncode:
        raise RuntimeError('Exit ' + str(result.returncode) + ': ' + str(log))
    return result.stdout.strip()

c_probe = work / ('c-abi' + suffix)
run([cc, '-Wall', '-Wextra', '-Werror', '-I' + str(prefix / 'include'), root / 'tests/bindings/jit-abi.c',
     '-L' + str(prefix / 'lib'), '-ljit', '-o', c_probe], work / 'c-compile.log')
c_output = run([c_probe], work / 'c-abi.log')
expected_abi = list(map(int, c_output.splitlines()[0].removeprefix('ABI').split()))
if len(expected_abi) != 12 or expected_abi[6] != 8 or 'LIBJIT_C_TYPES=PASS' not in c_output:
    raise ValueError('This qualification requires the real double native-float configuration')
helper = work / 'c-helper.o'
run([cc, '-Wall', '-Wextra', '-Werror', '-DJIT_ABI_HELPER', '-I' + str(prefix / 'include'),
     '-c', root / 'tests/bindings/jit-abi.c', '-o', helper], work / 'c-helper.log')
records = []
for backend in ('gcc', 'gas' if windows and args.bits == 32 else 'gas64'):
    for label, relative in (
            ('abi', 'tests/bindings/jit-abi.bas'),
            ('runtime', 'tests/bindings/jit-runtime.bas'),
            ('mul-add', 'examples/manual/libraries/jit/mul_add.bas'),
            ('gcd', 'examples/manual/libraries/jit/gcd.bas')):
        record = dict(backend=backend, check=label, source=relative, passed=False)
        executable = work / (label + '-' + backend + suffix)
        executable.unlink(missing_ok=True)
        try:
            command = [compiler, '-i', root / 'inc', '-d', 'JIT_NFLOAT_IS_DOUBLE', '-gen', backend,
                       '-w', 'all', root / relative]
            if args.bits == 32 and windows:
                command += ['-target', 'win32', '-prefix', 'C:/FreeBASIC']
            if label == 'runtime':
                command += [helper]
            for library in libraries:
                command += ['-p', library]
            command += ['-x', executable]
            run(command, work / (label + '-' + backend + '-compile.log'))
            if not executable.is_file():
                raise RuntimeError('No newly linked executable')
            output = run([executable], work / (label + '-' + backend + '-run.log'))
            if label == 'abi':
                observed = list(map(int, output.removeprefix('ABI').split()))
                record.update(c_abi=expected_abi, freebasic_abi=observed)
                if observed != expected_abi:
                    raise ValueError('C and FreeBASIC layouts differ')
            elif label == 'runtime':
                if 'LIBJIT_RUNTIME=PASS' not in output:
                    raise ValueError('Runtime checks did not complete')
            else:
                expression = r'mul_add\( *3, *5, *2\) *= *17' if label == 'mul-add' else r'gcd\( *21, *14\) *= *7'
                if not re.fullmatch(expression, output):
                    raise ValueError('Unexpected original example result: ' + output)
            record.update(passed=True, output=output)
        except Exception as error:
            record['error'] = str(error)
        records.append(record)
        receipt = dict(sdk_prefix=str(prefix), compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),
                       native_float='double', checks=records)
        (work / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n')
        print(backend, label, record['passed'], record.get('output', record.get('error', '')), flush=True)
raise SystemExit(not all(record['passed'] for record in records))
# end of test-jit.py
