"""Project: FreeBASIC C backend regression tests
File: pointer-sized-types-check.py
Purpose: Check C spellings for pointer-sized CRT declaration types.
Responsibilities: Compare ABI declarations and run native C calls.
This test does not claim foreign-target execution.
"""

import argparse
from collections import Counter
import os
from pathlib import Path
import re
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent
TYPES = (('size', '__SIZE_TYPE__'), ('ssize', '__PTRDIFF_TYPE__'),
         ('difference', '__PTRDIFF_TYPE__'), ('intptr', '__INTPTR_TYPE__'),
         ('uintptr', '__UINTPTR_TYPE__'))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--include', type=Path, required=True)
    parser.add_argument('--fallback-include', type=Path, required=True)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    args = parser.parse_args()
    compiler, prefix, include, fallback, gcc, clang = (p.resolve() for p in
        (args.fbc, args.prefix, args.include, args.fallback_include, args.gcc, args.clang))
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')

    def run(command):
        result = subprocess.run([str(x) for x in command], cwd=HERE, env=environment,
                                capture_output=True, text=True, errors='replace', timeout=120)
        if result.returncode:
            raise RuntimeError('command failed: ' + repr(command) + '\n' + (result.stdout + result.stderr)[-5000:])
        return result.stdout + result.stderr

    if os.name != 'nt':
        raise RuntimeError('this test matrix currently requires the Windows cross toolchain')
    targets = {
        'win64': ((gcc, ()), (clang, ('--target=x86_64-w64-windows-gnu',))),
        'win32': ((gcc, ('-m32',)), (clang, ('--target=i686-w64-windows-gnu',))),
        'linux': ((clang, ('--target=x86_64-pc-linux-gnu',)),),
        'js-asmjs': ((clang, ('--target=wasm32-unknown-unknown',)),),
    }
    count = 0
    runtime = 0
    warnings = Counter()
    with tempfile.TemporaryDirectory(prefix='fbc-pointer-types-') as tmp:
        work = Path(tmp)
        for target, compilers in targets.items():
            for backend in ('gcc', 'clang'):
                emitted = work / (target + '-' + backend + '.c')
                run([compiler, '-prefix', prefix, '-target', target, '-gen', backend,
                     '-i', include, '-i', fallback, '-r', HERE / 'pointer-sized-types.bas', '-o', emitted])
                text = emitted.read_text(encoding='utf-8')
                checks = '\n'
                for name, ctype in TYPES:
                    pattern = r'\b([a-zA-Z_]\w*)\s*\([^;\n]*\)\s*__asm__\("_?fbc_test_' + name + r'"\);'
                    match = re.search(pattern, text)
                    if match is None:
                        raise RuntimeError('missing ABI declaration for ' + target + ': ' + name)
                    checks += f'{ctype} {match.group(1)}({ctype});\n'
                emitted.write_text(text + checks, encoding='utf-8')
                for cc, flags in compilers:
                    diagnostics = (['-Wall', '-Wextra', '-Wpedantic'] if cc == gcc else
                                   ['-Weverything', '-ferror-limit=0'])
                    for standard in ('gnu11', 'gnu23'):
                        optimizations = ('-O0', '-O2') if target == 'win64' else ('-O2',)
                        for optimization in optimizations:
                            module = work / 'types.o'
                            log = run([cc, *flags, '-std=' + standard, '-nostdinc', optimization,
                                       *diagnostics, '-c', emitted, '-o', module])
                            warnings.update(re.findall(r'\[(-W[^]]+)\]', log))
                            count += 1
                            if target == 'win64':
                                helper = work / 'helper.o'
                                log = run([cc, *flags, '-std=' + standard, optimization, *diagnostics,
                                           '-c', HERE / 'pointer-sized-types-helper.c', '-o', helper])
                                warnings.update(re.findall(r'\[(-W[^]]+)\]', log))
                                executable = work / 'types.exe'
                                run([cc, *flags, module, helper, '-o', executable])
                                run([executable])
                                runtime += 1
    print('POINTER_SIZED_TYPES_CHECK_PASS', count, 'objects', runtime,
          'runtime', 'warnings', dict(warnings), flush=True)


if __name__ == '__main__':
    main()

# end of pointer-sized-types-check.py
