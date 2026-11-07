"""Check wide literal storage across target character ABIs.

Project: FreeBASIC compiler regression tests
File: c-wide-literal-target.py
Purpose: Check emitted wide literals when C wchar_t differs from FreeBASIC WCHAR.
Responsibilities: Compile local and cross-target C and run local literal tests.
This file intentionally does NOT claim that unrelated emitted C is warning-free.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = next(path for path in HERE.parents if (path / 'mk/compiler-sources.mk').is_file())


def run(command, cwd, environment, success=True):
    result = subprocess.run([str(part) for part in command], cwd=cwd, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    if success and result.returncode:
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
    command.extend([source, '-o', output])
    run(command, output.parent, environment)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--gcc', required=True, type=Path)
    parser.add_argument('--clang', required=True, type=Path)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--gas64', action='store_true')
    args = parser.parse_args()
    compiler, prefix = args.fbc.resolve(), args.prefix.resolve()
    gcc, clang = args.gcc.resolve(), args.clang.resolve()
    sources = [HERE / 'c-wide-literal-target.bas', ROOT / 'tests/wstring/literal-escape-once.bas']
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')
    counts = {'gcc': 0, 'clang': 0}
    cross_warnings = 0
    with tempfile.TemporaryDirectory(prefix='fbc-wide-literal-') as directory:
        work = Path(directory)
        for source in sources:
            for backend in ('gcc', 'clang'):
                c_file = work / (source.stem + '-' + backend + '.c')
                emit(compiler, prefix, source, c_file, environment, backend)
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
                    if target != 'win32':
                        text = c_file.read_text(encoding='utf8')
                        if source == sources[0] and not re.search(r'\{\s*233u,\s*20013u,\s*9786u,\s*0u\s*\}', text):
                            raise RuntimeError('UTF-32 literal code units were not preserved for ' + triple)
                        if source == sources[1] and not re.search(r'128512u,\s*88u', text):
                            raise RuntimeError('supplementary literal code point was not preserved for ' + triple)
                    result = run([clang, '--target=' + triple, '-std=gnu11', '-nostdinc',
                                  '-O2', '-Weverything', '-ferror-limit=0', '-c', c_file,
                                  '-o', work / (label + '.o')], work, environment)
                    cross_warnings += len(re.findall(r'(?m)^.*\bwarning:', result.stdout + result.stderr))
        if args.baseline:
            c_file = work / 'baseline-linux.c'
            emit(args.baseline.resolve(), prefix, sources[0], c_file, environment, 'gcc', 'linux')
            result = run([clang, '--target=x86_64-unknown-linux-gnu', '-std=gnu11', '-nostdinc',
                          '-O2', '-Weverything', '-ferror-limit=0', '-c', c_file,
                          '-o', work / 'baseline-linux.o'], work, environment, False)
            if result.returncode == 0 or 'incompatible wide string literal' not in result.stderr:
                raise RuntimeError('baseline did not reproduce the Linux wide-literal error')
    print('C_WIDE_LITERAL_TARGET_PASS native_warnings=' + str(counts) +
          ' cross_warnings=' + str(cross_warnings), flush=True)


if __name__ == '__main__':
    main()

# end of c-wide-literal-target.py
