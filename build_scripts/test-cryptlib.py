#!/usr/bin/env python3
"""FreeBASIC cryptlib native qualification
File: test-cryptlib.py
Purpose: Compare native layouts and hash real binary files on both backends.
Responsibilities: Private SDK selection, independent digests and failure checks.
This file contains no replacement cryptlib implementation or SDK installation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
parser.add_argument('--sdk-prefix', type=Path, required=True)
parser.add_argument('--bits', type=int, choices=(32, 64), default=64)
parser.add_argument('--outdir', type=Path)
args = parser.parse_args()
root = args.root.resolve()
prefix = args.sdk_prefix.resolve()
windows = os.name == 'nt'
work = args.outdir.resolve() if args.outdir else root / ('out/test-cryptlib-' + str(args.bits))
work.mkdir(parents=True, exist_ok=True)
environment = os.environ.copy()
if windows:
    tool = Path('C:/msys64/mingw' + str(args.bits) + '/bin')
    cc = tool / 'gcc.exe'
    compiler = root / 'bin/fbc-serialization.exe' if args.bits == 64 else Path('C:/FreeBASIC/fbc.exe')
    environment['PATH'] = str(prefix / 'bin') + ';' + str(tool) + ';C:/FreeBASIC/bin/win' + str(args.bits) + ';' + environment['PATH']
    environment.update(GCC=str(cc), AS=str(tool / 'as.exe'), LD=str(tool / 'ld.exe'))
    suffix = '.exe'
else:
    cc = Path('/usr/bin/gcc')
    compiler = root / 'bin/fbc'
    environment['LD_LIBRARY_PATH'] = str(prefix / 'lib') + ':' + environment.get('LD_LIBRARY_PATH', '')
    suffix = ''
records = []


def invoke(command, label, expected_exit=0):
    command = [str(part) for part in command]
    result = subprocess.run(command, cwd=work, env=environment, capture_output=True,
                            text=True, timeout=60)
    (work / (label + '.log')).write_text(repr(command) + '\n' + result.stdout + result.stderr)
    if result.returncode != expected_exit:
        raise RuntimeError(f'{label}: expected exit {expected_exit}, got {result.returncode}')
    return result.stdout


def compile_fb(relative, label, backend):
    executable = work / (label + suffix)
    executable.unlink(missing_ok=True)
    command = [compiler, '-i', root / 'inc', '-gen', backend, '-w', 'all', root / relative,
               '-p', prefix / 'lib']
    if windows:
        if args.bits == 32:
            command += ['-target', 'win32', '-prefix', 'C:/FreeBASIC']
        else:
            command += ['-p', root / 'lib/freebasic/win64']
        command += ['-p', Path('C:/FreeBASIC/lib/win' + str(args.bits))]
    command += ['-x', executable]
    diagnostics = invoke(command, label + '-compile')
    if diagnostics.strip() or not executable.is_file():
        raise RuntimeError('Compiler diagnostics or missing fresh executable: ' + label)
    return executable


# C facts come from the pinned SDK header and capability queries from the
# loaded library. Header-only layout agreement cannot prove native API support.
c_probe = work / ('c-abi' + suffix)
c_probe.unlink(missing_ok=True)
command = [cc, '-Wall', '-Wextra', '-Werror', '-I' + str(prefix / 'include'),
           root / 'tests/bindings/cryptlib-abi.c', '-L' + str(prefix / 'lib'), '-lcl', '-o', c_probe]
if windows:
    # The upstream public header uses Visual C++ warning pragmas on Windows.
    # GCC ignores those directives; keep all diagnostics for the probe itself.
    command += ['-Wno-unknown-pragmas']
invoke(command, 'c-compile')
c_output = invoke([c_probe], 'c-run')
expected = re.findall(r'(?m)^(ABI|CONSTANTS)\s+(.*)$', c_output)
if len(expected) != 2 or 'CRYPTLIB_C_CAPABILITIES=PASS' not in c_output:
    raise RuntimeError('Native C capability or layout check did not finish')
expected = [(name, list(map(int, numbers.split()))) for name, numbers in expected]
fixtures = {'empty': b'', 'abc': b'abc', 'block': bytes(range(256)) * 32,
            'partial': bytes(range(256)) * 32 + b'last\x00binary\xffblock'}
for name, data in fixtures.items():
    (work / (name + '.bin')).write_bytes(data)

for backend in ('gcc', 'gas' if windows and args.bits == 32 else 'gas64'):
    for label, relative in (
            ('abi', 'tests/bindings/cryptlib-abi.bas'),
            ('runtime', 'tests/bindings/cryptlib-runtime.bas'),
            ('manual', 'examples/manual/libraries/cryptlib.bas'),
            ('hashing', 'examples/math/cryptlib/hashing.bas')):
        tag = label + '-' + backend
        record = dict(backend=backend, check=label, source=relative, passed=False)
        try:
            executable = compile_fb(relative, tag, backend)
            if label == 'abi':
                output = invoke([executable], tag + '-run')
                observed = [(name, list(map(int, numbers.split()))) for name, numbers in
                            re.findall(r'(?m)^(ABI|CONSTANTS)\s+(.*)$', output)]
                record.update(c_abi=expected, freebasic_abi=observed)
                if observed != expected:
                    raise RuntimeError('C and FreeBASIC layouts differ')
            elif label == 'runtime':
                command = [executable]
                if 'CRYPTLIB_RUNTIME=PASS' not in invoke(command, tag + '-run'):
                    raise RuntimeError('Canary and incremental hash checks did not finish')
            else:
                checks = []
                for name, data in fixtures.items():
                    output = invoke([executable, work / (name + '.bin'), '--no-pause'],
                                    tag + '-' + name)
                    digests = dict(re.findall(r'(?im)^(sha256|sha1):\s*([0-9a-f]+)\s*$', output))
                    if digests.get('sha256') != hashlib.sha256(data).hexdigest().upper() or \
                       digests.get('sha1') != hashlib.sha1(data).hexdigest().upper():
                        raise RuntimeError('Digest differs from independent reference: ' + name)
                    checks.append(dict(fixture=name, bytes=len(data), passed=True))
                for name, arguments, message in (
                        ('no-file', [], 'Usage:'),
                        ('bad-mode', [work / 'abc.bin', '--invalid'], 'Usage:'),
                        ('extra-argument', [work / 'abc.bin', '--no-pause', 'extra'], 'Usage:'),
                        ('missing-file', [work / 'absent.bin', '--no-pause'],
                         'SHA-256 failed:')):
                    output = invoke([executable] + arguments, tag + '-' + name, 1)
                    if message not in output or re.search(r'(?im)^(sha256|sha1):', output):
                        raise RuntimeError('Invalid input was reported as successful: ' + name)
                    checks.append(dict(fixture=name, passed=True))
                record['checks'] = checks
            record['passed'] = True
        except Exception as error:
            record['error'] = str(error)
        records.append(record)
        (work / 'results.json').write_text(json.dumps(records, indent=2) + '\n')
        print(tag, 'PASS' if record['passed'] else record['error'], flush=True)
raise SystemExit(not all(record['passed'] for record in records))
# end of test-cryptlib.py
