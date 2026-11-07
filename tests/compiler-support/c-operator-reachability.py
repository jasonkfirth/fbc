"""Check private division operator reachability in the C backend.

Project: FreeBASIC compiler regression tests
File: c-operator-reachability.py
Purpose: Check live and unreachable private division overloads.
Responsibilities: Test native and cross-target emission and the assembly fallback.
This file intentionally does NOT claim other generated C is warning-free.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def run(command, cwd, environment):
    result = subprocess.run([str(part) for part in command], cwd=cwd, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    if result.returncode:
        raise RuntimeError('command failed: ' + ' '.join(str(part) for part in command) +
                           '\n' + (result.stdout + result.stderr)[:4000])
    return result


def emit(compiler, prefix, source, output, environment, backend, target=None, arch=None):
    command = [compiler, '-prefix', prefix, '-gen', backend, '-g', '-w', 'all',
               '-m', source.stem, '-r']
    if target:
        command.extend(['-target', target])
    if arch:
        command.extend(['-arch', arch])
    if target not in ('linux-arm', 'linux-aarch64'):
        command.extend(['-asm', 'intel'])
    run([*command, source, '-o', output], output.parent, environment)


def check_markers(source, c_file):
    text = c_file.read_text(encoding='utf8')
    if source.name.endswith('-asm.bas'):
        if 'ASM_FALLBACK_DIVISION_MARKER' not in text:
            raise RuntimeError('the inline-assembly fallback discarded an operator')
    elif 'UNUSED_PRIVATE_DIVISION_MARKER' in text:
        raise RuntimeError('unreachable private division operator was emitted')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--gcc', required=True, type=Path)
    parser.add_argument('--clang', required=True, type=Path)
    parser.add_argument('--baseline-fbc', type=Path)
    parser.add_argument('--gas64', action='store_true')
    args = parser.parse_args()
    compiler, prefix = args.fbc.resolve(), args.prefix.resolve()
    gcc, clang = args.gcc.resolve(), args.clang.resolve()
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')
    sources = [HERE / 'c-operator-reachability.bas', HERE / 'c-operator-reachability-asm.bas']
    counts = {'gcc': 0, 'clang': 0}
    cross_warnings = 0
    with tempfile.TemporaryDirectory(prefix='fbc-division-reachability-') as directory:
        work = Path(directory)
        for source in sources:
            for backend in ('gcc', 'clang'):
                c_file = work / (source.stem + '-' + backend + '.c')
                emit(compiler, prefix, source, c_file, environment, backend)
                check_markers(source, c_file)
                for name, native_compiler in (('gcc', gcc), ('clang', clang)):
                    obj = work / (source.stem + '-' + backend + '-' + name + '.o')
                    flags = (['-std=gnu11', '-nostdinc', '-O2', '-Wall', '-Wextra',
                              '-Wpedantic', '-Wconversion', '-Wsign-conversion',
                              '-Wshadow', '-Wformat=2', '-masm=intel'] if name == 'gcc' else
                             ['-std=gnu11', '-nostdinc', '-O2', '-Weverything',
                              '-ferror-limit=0', '-masm=intel'])
                    result = run([native_compiler, *flags, '-c', c_file, '-o', obj], work, environment)
                    counts[name] += len(re.findall(r'(?m)^.*\bwarning:', result.stdout + result.stderr))
                    exe = work / (source.stem + '-' + backend + '-' + name + '.exe')
                    run([compiler, '-prefix', prefix, '-x', exe, obj], work, environment)
                    run([exe], work, environment)
            if args.gas64:
                exe = work / (source.stem + '-gas64.exe')
                run([compiler, '-prefix', prefix, '-gen', 'gas64', '-x', exe, source], work, environment)
                run([exe], work, environment)
        targets = [('win32', None, 'i686-w64-windows-gnu'),
                   ('linux', '686', 'i686-unknown-linux-gnu'),
                   ('linux', None, 'x86_64-unknown-linux-gnu'),
                   ('linux-arm', None, 'armv7-unknown-linux-gnueabihf'),
                   ('linux-aarch64', None, 'aarch64-unknown-linux-gnu')]
        for source in sources:
            for target, arch, triple in targets:
                for backend in ('gcc', 'clang'):
                    label = source.stem + '-' + triple + '-' + backend
                    c_file = work / (label + '.c')
                    emit(compiler, prefix, source, c_file, environment, backend, target, arch)
                    check_markers(source, c_file)
                    result = run([clang, '--target=' + triple, '-std=gnu11', '-nostdinc',
                                  '-O2', '-Weverything', '-ferror-limit=0', '-c', c_file,
                                  '-o', work / (label + '.o')], work, environment)
                    cross_warnings += len(re.findall(r'(?m)^.*\bwarning:', result.stdout + result.stderr))
        if args.baseline_fbc:
            baseline_c = work / 'baseline.c'
            emit(args.baseline_fbc.resolve(), prefix, sources[0], baseline_c, environment, 'gcc')
            if 'UNUSED_PRIVATE_DIVISION_MARKER' not in baseline_c.read_text(encoding='utf8'):
                raise RuntimeError('baseline no longer reproduces the unreachable operator')
    print('C_OPERATOR_REACHABILITY_PASS native_warnings=' + str(counts) +
          ' cross_warnings=' + str(cross_warnings), flush=True)


if __name__ == '__main__':
    main()

# end of c-operator-reachability.py
