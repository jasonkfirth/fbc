"""Project: FreeBASIC C backend regression tests
File: c23-keyword-check.py
Purpose: Check C23 keyword identifiers and Windows TLS object aliases.
Responsibilities: Compile, link and run small mixed-language fixtures.
This test does not install tools or qualify untested platforms.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent
KEYWORDS = ('alignas', 'alignof', 'bool', 'constexpr', 'false', 'nullptr',
            'static_assert', 'thread_local', 'true', 'typeof_unqual')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--tls-baseline', type=Path)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    args = parser.parse_args()
    compiler, prefix, gcc, clang = (path.resolve() for path in
                                    (args.fbc, args.prefix, args.gcc, args.clang))
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')
    counts = {'runtime': 0, 'tls_runtime': 0, 'forward_tls': 0, 'standard_keyword_tls': 0,
              'single_module_tls': 0, 'cross_objects': 0}
    with tempfile.TemporaryDirectory(prefix='fbc-c23-keywords-') as directory:
        work = Path(directory)

        def run(command, expected=0):
            result = subprocess.run([str(part) for part in command], cwd=work, env=environment,
                                    capture_output=True, text=True, errors='replace', timeout=180)
            if (result.returncode == 0) != (expected == 0):
                raise RuntimeError('unexpected exit from ' + ' '.join(str(part) for part in command) +
                                   '\n' + result.stdout + result.stderr)
            return result.stdout + result.stderr

        def emit(fbc, source, dest, backend='gcc', options=()):
            run([fbc, '-prefix', prefix, '-m', source.stem, '-gen', backend, *options, '-r', source, '-o', dest])

        def compile_c(cc, source, obj, standard, extra=()):
            return run([cc, *extra, '-std=' + standard, '-nostdinc', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                        '-c', source, '-o', obj])

        # Confirm that the control failure comes from C23 keyword declarations.
        if args.baseline:
            source = HERE / 'c23-keyword-variables.bas'
            baseline_c = work / 'baseline.c'
            emit(args.baseline.resolve(), source, baseline_c)
            compile_c(gcc, baseline_c, work / 'baseline.o', 'gnu11')
            failure = run([gcc, '-std=gnu23', '-nostdinc', '-fsyntax-only', baseline_c], expected=1)
            if 'error:' not in failure or not any(name in failure for name in KEYWORDS):
                raise RuntimeError('the baseline did not reproduce the keyword failure')

        for fixture, marker in (('c23-keyword-variables', 'C23_KEYWORD_VARIABLES_PASS'),
                                ('c23-keyword-aliases', 'C23_KEYWORD_ALIASES_PASS'),
                                ('c23-keyword-defined', 'C23_KEYWORD_DEFINED_PASS')):
            source = HERE / (fixture + '.bas')
            helper_name = 'c23-keyword-helper.c' if fixture.endswith('aliases') else fixture + '-helper.c'
            helper = HERE / helper_name
            for backend in ('gcc', 'clang'):
                c_file = work / (fixture + '-' + backend + '.c')
                emit(compiler, source, c_file, backend)
                for standard in ('gnu11', 'gnu23'):
                    for cc in (gcc, clang):
                        obj, helper_obj = work / 'module.o', work / 'helper.o'
                        compile_c(cc, c_file, obj, standard)
                        compile_c(cc, helper, helper_obj, standard)
                        exe = work / 'keyword.exe'
                        run([compiler, '-prefix', prefix, '-gen', backend, '-x', exe, obj, helper_obj])
                        if marker not in run([exe]):
                            raise RuntimeError(fixture + ' produced an unexpected result')
                        counts['runtime'] += 1
            # Mix the native emitter with a C helper to check object names.
            obj, helper_obj = work / 'native.o', work / 'native-helper.o'
            run([compiler, '-prefix', prefix, '-m', fixture, '-gen', 'gas64', '-c', source, '-o', obj])
            compile_c(gcc, helper, helper_obj, 'gnu23')
            exe = work / 'native-keyword.exe'
            run([compiler, '-prefix', prefix, '-gen', 'gas64', '-x', exe, obj, helper_obj])
            if marker not in run([exe]):
                raise RuntimeError('native ' + fixture + ' failed')
            counts['runtime'] += 1

        for standard in ('gnu11', 'gnu23'):
            objects = {}
            for backend in ('gcc', 'clang'):
                for part in ('provider', 'consumer'):
                    source = HERE / ('tls-keyword-' + part + '.bas')
                    obj = work / (standard + '-' + backend + '-' + part + '.o')
                    command = [compiler, '-prefix', prefix, '-mt', '-gen', backend, '-Wc', '-std=' + standard,
                               '-c', source, '-o', obj]
                    if part == 'consumer':
                        command[1:1] = ['-m', source.stem]
                    run(command)
                    objects[backend, part] = obj
            observer = work / 'tls-c-observer.o'
            compile_c(gcc, HERE / 'tls-keyword-c-helper.c', observer, 'gnu11')
            for consumer in ('gcc', 'clang'):
                for provider in ('gcc', 'clang'):
                    exe = work / 'tls.exe'
                    run([compiler, '-prefix', prefix, '-mt', '-gen', consumer, '-x', exe,
                         objects[provider, 'provider'], objects[consumer, 'consumer'], observer])
                    if 'KEYWORD_TLS_TEST_PASS' not in run([exe]):
                        raise RuntimeError('keyword TLS did not preserve per-thread storage')
                    counts['tls_runtime'] += 1
                for cc in (gcc, clang):
                    c_provider = work / 'tls-c-provider.o'
                    compile_c(cc, HERE / 'tls-keyword-c-helper.c', c_provider, 'gnu11', ('-DTLS_C_PROVIDER',))
                    exe = work / 'tls-c-provider.exe'
                    run([compiler, '-prefix', prefix, '-mt', '-gen', consumer, '-x', exe,
                         c_provider, objects[consumer, 'consumer']])
                    if 'KEYWORD_TLS_TEST_PASS' not in run([exe]):
                        raise RuntimeError('C-provided TLS did not preserve per-thread storage')
                    counts['tls_runtime'] += 1
            for provider in ('gcc', 'clang'):
                source = HERE / 'tls-forward-provider.bas'
                obj = work / (standard + '-forward-' + provider + '.o')
                run([compiler, '-prefix', prefix, '-mt', '-gen', provider, '-Wc', '-std=' + standard,
                     '-c', source, '-o', obj])
                for consumer in ('gcc', 'clang'):
                    exe = work / 'forward-tls.exe'
                    run([compiler, '-prefix', prefix, '-mt', '-gen', consumer, '-x', exe,
                         obj, objects[consumer, 'consumer'], observer])
                    if 'KEYWORD_TLS_TEST_PASS' not in run([exe]):
                        raise RuntimeError('forward-declared TLS changed: ' + standard)
                    counts['forward_tls'] += 1

            objects = {}
            for backend in ('gcc', 'clang'):
                for part in ('provider', 'consumer'):
                    source = HERE / ('tls-standard-' + part + '.bas')
                    obj = work / (standard + '-standard-' + backend + '-' + part + '.o')
                    command = [compiler, '-prefix', prefix, '-mt', '-gen', backend, '-Wc', '-std=' + standard,
                               '-c', source, '-o', obj]
                    if part == 'consumer':
                        command[1:1] = ['-m', source.stem]
                    run(command)
                    objects[backend, part] = obj
            for consumer in ('gcc', 'clang'):
                for provider in ('gcc', 'clang'):
                    exe = work / 'standard-tls.exe'
                    run([compiler, '-prefix', prefix, '-mt', '-gen', consumer, '-x', exe,
                         objects[provider, 'provider'], objects[consumer, 'consumer']])
                    if 'STANDARD_KEYWORD_TLS_PASS' not in run([exe]):
                        raise RuntimeError('ordinary C keyword TLS changed: ' + standard)
                    counts['standard_keyword_tls'] += 1
            for backend in ('gcc', 'clang'):
                source = HERE / 'tls-keyword-private.bas'
                exe = work / 'single-module-tls.exe'
                run([compiler, '-prefix', prefix, '-mt', '-m', source.stem, '-gen', backend,
                     '-Wc', '-std=' + standard, '-x', exe, source])
                if 'KEYWORD_PRIVATE_TLS_PASS' not in run([exe]):
                    raise RuntimeError('single-module keyword TLS changed: ' + standard)
                counts['single_module_tls'] += 1

        if args.tls_baseline:
            old_compiler = args.tls_baseline.resolve()
            objects = []
            for part in ('provider', 'consumer'):
                source = HERE / ('tls-standard-' + part + '.bas')
                obj = work / ('old-standard-' + part + '.o')
                command = [old_compiler, '-prefix', prefix, '-mt', '-gen', 'gcc', '-Wc', '-std=gnu23',
                           '-c', source, '-o', obj]
                if part == 'consumer':
                    command[1:1] = ['-m', source.stem]
                run(command)
                objects.append(obj)
            failure = run([old_compiler, '-prefix', prefix, '-mt', '-gen', 'gcc', '-x',
                           work / 'old-standard-tls.exe', *objects], expected=1)
            if 'relocation truncated' not in failure or 'int@secrel32' not in failure:
                raise RuntimeError('the older TLS relocation failure did not reproduce')

        # Cross-compile both the definitions and the references. Win32 prepends
        # an underscore to each object symbol; emitted directives must match.
        for fixture, name in (('keyword', 'bool'), ('standard', 'int')):
            for part in ('provider', 'consumer'):
                source = HERE / ('tls-' + fixture + '-' + part + '.bas')
                c_file = work / ('tls-32-' + part + '.c')
                emit(compiler, source, c_file, options=('-mt', '-target', 'win32'))
                for cc, options in ((gcc, ('-m32',)), (clang, ('--target=i686-w64-windows-gnu',))):
                    obj = work / ('tls-32-' + part + '-' + cc.stem + '.o')
                    compile_c(cc, c_file, obj, 'gnu23', options)
                    symbols = run([gcc.parent / 'nm.exe', obj])
                    kind = 'U' if part == 'consumer' else '[A-Z]'
                    if not re.search(r'\b' + kind + r'\s+_' + name + r'\s*$', symbols, re.MULTILINE):
                        raise RuntimeError('wrong Win32 TLS object name:\n' + symbols)
                    counts['cross_objects'] += 1

        variables = HERE / 'c23-keyword-variables.bas'
        c_file, obj = work / 'variables-js.c', work / 'variables-js.o'
        emit(compiler, variables, c_file, 'clang', ('-target', 'js-asmjs'))
        compile_c(clang, c_file, obj, 'gnu23', ('--target=wasm32-unknown-unknown',))
        counts['cross_objects'] += 1

    print('C23_KEYWORD_CHECK_PASS', counts, flush=True)


if __name__ == '__main__':
    main()

# end of c23-keyword-check.py
