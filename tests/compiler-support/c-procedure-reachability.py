"""Exercise C-backend reachability of private procedures.

Project: FreeBASIC compiler regression tests
File: c-procedure-reachability.py
Purpose: Check callbacks, methods, static cleanup and conservative assembly handling.
Responsibilities: Compile and run the focused fixtures and cross-target cases.
This file intentionally does NOT claim that the rest of the emitted C is warning-free.
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
    output = result.stdout + result.stderr
    if result.returncode:
        raise RuntimeError('command failed: ' + ' '.join(str(part) for part in command) + '\n' + output[:4000])
    return output


def code_without_line_comments(path):
    return '\n'.join(line for line in path.read_text(encoding='utf8').splitlines()
                     if not line.lstrip().startswith('//')).upper()


def emit(compiler, prefix, source, output, environment, target=None, backend='gcc'):
    command = [compiler, '-prefix', prefix, '-gen', backend, '-g', '-w', 'all',
               '-m', source.stem, '-r']
    if target:
        command.extend(['-target', target])
    if target not in ('linux-arm', 'linux-aarch64'):
        command.extend(['-asm', 'intel'])
    run([*command, source, '-o', output], output.parent, environment)


def check_native(compiler, prefix, source, output, gcc, clang, environment, fallback):
    c_file = output / (source.stem + '.c')
    emit(compiler, prefix, source, c_file, environment)
    code = code_without_line_comments(c_file)
    if ('DEAD_LEAF' in code) != fallback or ('DEAD_CYCLE_A' in code) != fallback:
        raise RuntimeError('private procedure reachability or conservative fallback changed')
    if 'ESCAPED_CALLBACK' not in code or 'LIVE_LEAF' not in code:
        raise RuntimeError('a reachable procedure or saved callback was removed')
    if not fallback and ('DEADMETHODLEAF' in code or 'LIVEMETHODLEAF' not in code):
        raise RuntimeError('private method reachability changed')

    counts = {}
    profiles = {
        'gcc': [gcc, '-std=gnu11', '-nostdinc', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                '-Wconversion', '-Wformat=2', '-Werror=implicit-function-declaration', '-masm=intel'],
        'clang': [clang, '-std=gnu11', '-nostdinc', '-O2', '-Weverything', '-ferror-limit=0',
                  '-Werror=implicit-function-declaration', '-masm=intel'],
    }
    for name, flags in profiles.items():
        obj = output / (source.stem + '-' + name + '.o')
        log = run([*flags, '-c', c_file, '-o', obj], output, environment)
        counts[name] = len(re.findall(r'(?m)^.*\bwarning:', log))
        if not fallback and counts[name]:
            raise RuntimeError('unexpected warning in the reachability fixture:\n' + log[:4000])
        executable = output / (source.stem + '-test-' + name + '.exe')
        run([compiler, '-prefix', prefix, '-x', executable, obj], output, environment)
        run([executable], output, environment)
    return counts


def check_static_destructor(compiler, prefix, source, output, gcc, clang, environment, gas64, nested):
    c_file = output / (source.stem + '.c')
    emit(compiler, prefix, source, c_file, environment)
    counts = {}
    profiles = {
        'gcc': [gcc, '-std=gnu11', '-nostdinc', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                '-Werror=implicit-function-declaration', '-masm=intel'],
        'clang': [clang, '-std=gnu11', '-nostdinc', '-O2', '-Weverything', '-ferror-limit=0',
                  '-Werror=implicit-function-declaration', '-masm=intel'],
    }

    def check_output(text):
        pattern = r'(?:INNER|OUTER)=\s*0' if nested else r'DTOR=\s*42'
        expected = 2 if nested else 3
        if len(re.findall(pattern, text)) != expected or 'UNEXPECTED' in text:
            raise RuntimeError('incorrect static destructor output:\n' + text[:4000])

    for name, flags in profiles.items():
        obj = output / (source.stem + '-' + name + '.o')
        log = run([*flags, '-c', c_file, '-o', obj], output, environment)
        if "declared 'static' but never defined" in log or '[-Wundefined-internal]' in log:
            raise RuntimeError('a discarded cleanup wrapper left an undefined prototype:\n' + log[:4000])
        counts[name] = len(re.findall(r'(?m)^.*\bwarning:', log))
        executable = output / (source.stem + '-test-' + name + '.exe')
        run([compiler, '-prefix', prefix, '-x', executable, obj], output, environment)
        check_output(run([executable], output, environment))

    if gas64:
        executable = output / (source.stem + '-test-gas64.exe')
        run([compiler, '-prefix', prefix, '-gen', 'gas64', '-x', executable, source], output, environment)
        check_output(run([executable], output, environment))
    return counts


def check_copyback(compiler, prefix, source, output, gcc, clang, environment):
    c_file = output / (source.stem + '.c')
    emit(compiler, prefix, source, c_file, environment)
    code = code_without_line_comments(c_file)
    if 'STRING_INDEX' not in code or 'UNUSED_INDEX_CALLER' in code:
        raise RuntimeError('the copied BYREF argument changed procedure reachability')
    counts = {}
    profiles = {
        'gcc': [gcc, '-std=gnu11', '-nostdinc', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                '-Werror=implicit-function-declaration', '-masm=intel'],
        'clang': [clang, '-std=gnu11', '-nostdinc', '-O2', '-Weverything', '-ferror-limit=0',
                  '-Werror=implicit-function-declaration', '-masm=intel'],
    }
    for name, flags in profiles.items():
        obj = output / (source.stem + '-' + name + '.o')
        log = run([*flags, '-c', c_file, '-o', obj], output, environment)
        counts[name] = len(re.findall(r'(?m)^.*\bwarning:', log))
        executable = output / (source.stem + '-test-' + name + '.exe')
        run([compiler, '-prefix', prefix, '-x', executable, obj], output, environment)
        run([executable], output, environment)
    # GAS64 has a separately reproduced, pre-existing fixed-string copy-back
    # failure for this fixture. The reachability change applies to C output.
    return counts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    parser.add_argument('--gas64', action='store_true', help='also run the native GAS64 backend')
    args = parser.parse_args()
    compiler, prefix = Path(args.fbc).resolve(), Path(args.prefix).resolve()
    gcc, clang = Path(args.gcc).resolve(), Path(args.clang).resolve()
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')
    args.work_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix='c-proc-reachability-', dir=args.work_dir) as directory:
        work = Path(directory)
        normal = HERE / 'c-procedure-reachability.bas'
        asm = HERE / 'c-procedure-reachability-asm.bas'
        normal_counts = check_native(compiler, prefix, normal, work, gcc, clang, environment, False)
        asm_counts = check_native(compiler, prefix, asm, work, gcc, clang, environment, True)

        # A very deep AST takes the original emission path rather than risking
        # unbounded recursion in the reachability inspection.
        deep = work / 'c-procedure-reachability-deep.bas'
        expression = ' + '.join(['inputValue'] * 300)
        source = normal.read_text(encoding='utf8')
        source = source.replace('if live_middle() <> 19 then end 1',
                                'dim inputValue as long = 1\n'
                                'dim totalValue as long = ' + expression + '\n'
                                'if totalValue <> 300 then end 5\n'
                                'if live_middle() <> 19 then end 1')
        deep.write_text(source, encoding='utf8', newline='\n')
        deep_counts = check_native(compiler, prefix, deep, work, gcc, clang, environment, False)

        # Definitions are reversed so each live callee precedes its caller.
        # This exercises long call chains without relying on source order.
        count = 600
        chain = work / 'c-procedure-reachability-chain.bas'
        lines = ["/' Project: FreeBASIC compiler regression",
                 "   File: c-procedure-reachability-chain.bas",
                 "   Purpose: Check a long reverse-ordered live call chain.",
                 "   This fixture does not contain inline assembly. '/"]
        for index in range(count):
            lines.append('private declare function chain_%04d() as long' % index)
        for index in range(count - 1, -1, -1):
            lines.append('private function chain_%04d() as long' % index)
            lines.append('    return 1' if index == count - 1 else
                         '    return chain_%04d() + 1' % (index + 1))
            lines.append('end function')
        lines.extend(['if chain_0000() <> %d then end 1' % count, 'end 0',
                      "' end of c-procedure-reachability-chain.bas"])
        chain.write_text('\n'.join(lines) + '\n', encoding='utf8', newline='\n')
        chain_c = work / 'chain.c'
        emit(compiler, prefix, chain, chain_c, environment)
        chain_obj = work / 'chain.o'
        run([gcc, '-std=gnu11', '-nostdinc', '-O2', '-Wall', '-Wextra', '-Wpedantic',
             '-c', chain_c, '-o', chain_obj], work, environment)
        chain_executable = work / 'chain-test.exe'
        run([compiler, '-prefix', prefix, '-x', chain_executable, chain_obj], work, environment)
        run([chain_executable], work, environment)

        targets = [('win32', 'i686-w64-windows-gnu'), ('linux', 'x86_64-unknown-linux-gnu'),
                   ('linux-arm', 'armv7-unknown-linux-gnueabihf'),
                   ('linux-aarch64', 'aarch64-unknown-linux-gnu')]
        cross_warnings = 0
        for target, triple in targets:
            for backend in ('gcc', 'clang'):
                c_file = work / (target + '-' + backend + '.c')
                emit(compiler, prefix, normal, c_file, environment, target, backend)
                code = code_without_line_comments(c_file)
                if 'DEAD_LEAF' in code or 'ESCAPED_CALLBACK' not in code:
                    raise RuntimeError('cross-target procedure reachability changed: ' + target)
                log = run([clang, '--target=' + triple, '-std=gnu11', '-nostdinc', '-O2',
                           '-Weverything', '-ferror-limit=0', '-c', c_file,
                           '-o', work / (target + '-' + backend + '.o')], work, environment)
                cross_warnings += len(re.findall(r'(?m)^.*\bwarning:', log))

        static_counts = check_static_destructor(compiler, prefix, HERE / 'c-static-local-destructor.bas',
                                                work, gcc, clang, environment, args.gas64, False)
        nested_counts = check_static_destructor(compiler, prefix, HERE / 'c-static-local-destructor-nested.bas',
                                                work, gcc, clang, environment, args.gas64, True)

        copyback_counts = check_copyback(compiler, prefix, HERE / 'c-procedure-reachability-copyback.bas',
                                         work, gcc, clang, environment)

        if args.gas64:
            executable = work / 'reachability-gas64-test.exe'
            run([compiler, '-prefix', prefix, '-gen', 'gas64', '-x', executable, normal], work, environment)
            run([executable], work, environment)

    print('C_PROCEDURE_REACHABILITY_PASS normal=' + str(normal_counts) +
          ' asm=' + str(asm_counts) + ' deep=' + str(deep_counts) +
          ' static=' + str(static_counts) + ' nested=' + str(nested_counts) +
          ' copyback=' + str(copyback_counts) + ' cross_warnings=' + str(cross_warnings))


if __name__ == '__main__':
    main()

# end of c-procedure-reachability.py
