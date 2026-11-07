"""Check implicit constructor reachability in generated C.

Project: FreeBASIC compiler regression tests
File: c-constructor-reachability.py
Purpose: Check constructor calls and cleanup across C backends.
Responsibilities: Exercise native and cross-target compilation of lifetime fixtures.
This file intentionally does NOT claim all emitted C is warning-free.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def execute(command, cwd, environment):
    result = subprocess.run([str(part) for part in command], cwd=cwd, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    if result.returncode:
        raise RuntimeError('command failed: ' + ' '.join(str(x) for x in command) +
                           '\n' + (result.stdout + result.stderr)[:4000])
    return result.stdout + result.stderr


def emit(compiler, prefix, source, output, environment, backend, target=None):
    command = [compiler, '-prefix', prefix, '-gen', backend, '-g', '-w', 'all',
               '-m', source.stem, '-r']
    if target:
        command.extend(['-target', target])
    if target not in ('linux-arm', 'linux-aarch64'):
        command.extend(['-asm', 'intel'])
    execute([*command, source, '-o', output], output.parent, environment)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--gcc', required=True, type=Path)
    parser.add_argument('--clang', required=True, type=Path)
    parser.add_argument('--work-dir', required=True, type=Path)
    parser.add_argument('--baseline-fbc', type=Path)
    parser.add_argument('--gas64', action='store_true')
    args = parser.parse_args()
    compiler, prefix, gcc, clang = (path.resolve() for path in (args.fbc, args.prefix, args.gcc, args.clang))
    source = HERE / 'c-constructor-reachability.bas'
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment['PATH']
    args.work_dir.mkdir(parents=True, exist_ok=True)
    counts = {}
    expected = ''
    with tempfile.TemporaryDirectory(prefix='c-constructors-', dir=args.work_dir.resolve()) as directory:
        work = Path(directory)
        profiles = {
            'gcc': [gcc, '-std=gnu11', '-nostdinc', '-fwrapv', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                    '-Wconversion', '-Wformat=2', '-Werror=implicit-function-declaration', '-masm=intel'],
            'clang': [clang, '-std=gnu11', '-nostdinc', '-fwrapv', '-O2', '-Weverything', '-ferror-limit=0',
                      '-Werror=implicit-function-declaration', '-masm=intel'],
        }
        for backend in ('gcc', 'clang'):
            c_file = work / (backend + '.c')
            emit(compiler, prefix, source, c_file, environment, backend)
            code = '\n'.join(line for line in c_file.read_text(encoding='utf8').splitlines()
                             if not line.lstrip().startswith('//')).upper()
            if 'UNUSEDTHINGC1' in code or 'INNERONLYC1' not in code:
                raise RuntimeError('dead or implicitly called constructor reachability changed')
            for name, flags in profiles.items():
                obj = work / (backend + '-' + name + '.o')
                log = execute([*flags, '-c', c_file, '-o', obj], work, environment)
                counts[backend + '-' + name] = len(re.findall(r'(?m)^.*\bwarning:', log))
                exe = work / (backend + '-' + name + '-test.exe')
                execute([compiler, '-prefix', prefix, '-x', exe, obj], work, environment)
                if execute([exe], work, environment).strip() != expected:
                    raise RuntimeError('constructor or destructor results changed')
        # An assembler block can name procedures outside the parsed call graph.
        # Keep the pre-existing emission path for the whole translation unit.
        fallback = work / 'c-constructor-reachability-asm.bas'
        fallback_source = source.read_text(encoding='utf8').replace(
            'File: c-constructor-reachability.bas', 'File: c-constructor-reachability-asm.bas').replace(
            'end of c-constructor-reachability.bas', 'end of c-constructor-reachability-asm.bas')
        marker = '\nexercise()\n'
        if fallback_source.count(marker) != 1:
            raise RuntimeError('the constructor fixture changed unexpectedly')
        fallback_source = fallback_source.replace(marker, '\nasm\n    nop\nend asm\nexercise()\n')
        fallback.write_text(fallback_source, encoding='utf8', newline='\n')
        fallback_c = work / 'constructor-asm.c'
        emit(compiler, prefix, fallback, fallback_c, environment, 'gcc')
        if 'UNUSEDTHINGC1' not in fallback_c.read_text(encoding='utf8').upper():
            raise RuntimeError('inline assembler did not retain the conservative emission path')
        fallback_obj = work / 'constructor-asm.o'
        fallback_log = execute([*profiles['gcc'], '-c', fallback_c, '-o', fallback_obj], work, environment)
        counts['asm-gcc'] = len(re.findall(r'(?m)^.*\bwarning:', fallback_log))
        fallback_exe = work / 'constructor-asm-test.exe'
        execute([compiler, '-prefix', prefix, '-x', fallback_exe, fallback_obj], work, environment)
        execute([fallback_exe], work, environment)

        if args.gas64:
            exe = work / 'gas64-test.exe'
            execute([compiler, '-prefix', prefix, '-gen', 'gas64', '-x', exe, source], work, environment)
            if execute([exe], work, environment).strip() != expected:
                raise RuntimeError('GAS64 constructor or destructor results changed')
        if args.baseline_fbc:
            baseline = args.baseline_fbc.resolve()
            c_file = work / 'before.c'
            emit(baseline, prefix, source, c_file, environment, 'gcc')
            if 'UNUSEDTHINGC1' not in c_file.read_text(encoding='utf8').upper():
                raise RuntimeError('the baseline no longer emits the unused constructor')
        cross_warnings = 0
        targets = [('win32', 'i686-w64-windows-gnu'), ('linux', 'x86_64-unknown-linux-gnu'),
                   ('linux-arm', 'armv7-unknown-linux-gnueabihf'),
                   ('linux-aarch64', 'aarch64-unknown-linux-gnu')]
        for target, triple in targets:
            for backend in ('gcc', 'clang'):
                c_file = work / (target + '-' + backend + '.c')
                emit(compiler, prefix, source, c_file, environment, backend, target)
                code = c_file.read_text(encoding='utf8').upper()
                if 'UNUSEDTHINGC1' in code or 'INNERONLYC1' not in code:
                    raise RuntimeError('cross-target constructor reachability changed: ' + target)
                log = execute([clang, '--target=' + triple, '-std=gnu11', '-nostdinc', '-O2',
                               '-Weverything', '-ferror-limit=0', '-c', c_file,
                               '-o', work / (target + '-' + backend + '.o')], work, environment)
                cross_warnings += len(re.findall(r'(?m)^.*\bwarning:', log))
    print('C_CONSTRUCTOR_REACHABILITY_PASS warnings=' + str(counts) + ' cross_warnings=' + str(cross_warnings))


if __name__ == '__main__':
    main()

# end of c-constructor-reachability.py
