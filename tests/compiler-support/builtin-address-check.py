"""Project: FreeBASIC C backend regression tests
File: builtin-address-check.py
Purpose: Check C library builtin addresses across generated C backends.
Responsibilities: Compile and run global/local addresses and direct calls.
This test does not install a compiler or claim foreign-target execution.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--include', type=Path, required=True)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    args = parser.parse_args()
    compiler, prefix, gcc, clang = (p.resolve() for p in (args.fbc, args.prefix, args.gcc, args.clang))
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')
    counts = {'runtime': 0, 'win32_objects': 0, 'wasm_objects': 0}
    warnings = {}

    def run(command):
        result = subprocess.run([str(x) for x in command], cwd=HERE, env=environment,
                                capture_output=True, text=True, errors='replace', timeout=120)
        text = result.stdout + result.stderr
        if result.returncode:
            raise RuntimeError('command failed: ' + repr(command) + '\n' + text[-5000:])
        return text

    def emit(source, output, backend, options=()):
        asm_options = [] if 'js-asmjs' in options else ['-asm', 'intel']
        run([compiler, '-prefix', prefix, '-gen', backend, *asm_options, '-w', 'all',
             '-c', '-i', args.include.resolve(), '-m', source.stem, *options, '-r', source, '-o', output])

    def compile_c(cc, source, obj, standard, opt, options=()):
        diagnostics = (['-Wall', '-Wextra', '-Wpedantic'] if cc == gcc else
                       ['-Weverything', '-ferror-limit=0'])
        asm_options = [] if '--target=wasm32-unknown-unknown' in options else ['-masm=intel']
        text = run([cc, '-std=' + standard, '-nostdinc', *asm_options, opt,
                    *diagnostics, *options, '-c', source, '-o', obj])
        warnings[cc.stem] = warnings.get(cc.stem, 0) + len(re.findall(r'\bwarning:', text))

    with tempfile.TemporaryDirectory(prefix='fbc-builtin-address-') as tmp:
        work = Path(tmp)
        source = HERE / 'builtin-address.bas'
        for backend in ('gcc', 'clang'):
            emitted = work / (backend + '.c')
            emit(source, emitted, backend)
            text = emitted.read_text(encoding='utf-8')
            if backend == 'gcc':
                if len(re.findall(r'\b__builtin_memcpy\s*\(', text)) < 2:
                    raise RuntimeError('a direct GCC builtin call was rewritten')
                if '&__builtin_memcpy' in text:
                    raise RuntimeError('the GCC builtin address was not redirected')
            for cc in (gcc, clang):
                for standard in ('gnu11', 'gnu23'):
                    for opt in ('-O0', '-O2'):
                        obj, executable = work / 'module.o', work / 'test.exe'
                        compile_c(cc, emitted, obj, standard, opt)
                        run([compiler, '-prefix', prefix, '-gen', backend, '-x', executable, obj])
                        if 'BUILTIN_ADDRESS_PASS' not in run([executable]):
                            raise RuntimeError('global or local builtin address test failed')
                        counts['runtime'] += 1

            emitted = work / (backend + '-win32.c')
            emit(source, emitted, backend, ('-target', 'win32'))
            for cc, options in ((gcc, ('-m32',)), (clang, ('--target=i686-w64-windows-gnu',))):
                for standard in ('gnu11', 'gnu23'):
                    obj = work / 'win32.o'
                    compile_c(cc, emitted, obj, standard, '-O0', options)
                    symbols = run([gcc.parent / ('nm.exe' if os.name == 'nt' else 'nm'), '-u', obj])
                    for name in ('memcpy', 'memcmp', 'strlen', 'memmove', 'memset'):
                        if not re.search(r'\b_' + name + r'\b', symbols):
                            raise RuntimeError('missing Win32 libc symbol _' + name + '\n' + symbols)
                    counts['win32_objects'] += 1

            emitted = work / (backend + '-wasm.c')
            emit(source, emitted, backend, ('-target', 'js-asmjs'))
            for standard in ('gnu11', 'gnu23'):
                compile_c(clang, emitted, work / 'wasm.o', standard, '-O0',
                          ('--target=wasm32-unknown-unknown',))
                counts['wasm_objects'] += 1
    print('BUILTIN_ADDRESS_CHECK_PASS', counts, 'warnings', warnings, flush=True)


if __name__ == '__main__':
    main()

# end of builtin-address-check.py
