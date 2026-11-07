"""Project: FreeBASIC C backend regression tests
File: c23-parameters.py
Purpose: Check C23 unnamed parameters and the GNU11 compatibility path.
Responsibilities: Compile and run callbacks, varargs and managed values.
This script does not install a compiler or suppress C diagnostics.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = next(path for path in HERE.parents if (path / 'mk/compiler-sources.mk').is_file())
CONDITION = '#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L'


def run(command, cwd, environment):
    result = subprocess.run([str(part) for part in command], cwd=cwd, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    if result.returncode:
        raise RuntimeError('command failed: ' + ' '.join(str(part) for part in command) +
                           '\n' + result.stdout + result.stderr)
    return result.stdout + result.stderr


def emit(compiler, prefix, source, output, backend, environment, extra=()):
    run([compiler, '-prefix', prefix, '-i', ROOT / 'inc', '-gen', backend, *extra,
         '-r', source, '-o', output], output.parent, environment)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--gcc', required=True, type=Path)
    parser.add_argument('--clang', required=True, type=Path)
    parser.add_argument('--gas64', action='store_true')
    args = parser.parse_args()
    compiler, prefix = args.fbc.resolve(), args.prefix.resolve()
    gcc, clang = args.gcc.resolve(), args.clang.resolve()
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')
    counts = {}
    with tempfile.TemporaryDirectory(prefix='fbc-c23-parameters-') as directory:
        work = Path(directory)
        for backend in ('gcc', 'clang'):
            source = HERE / 'c23-parameters.bas'
            c_file = work / (backend + '.c')
            emit(compiler, prefix, source, c_file, backend, environment)
            generated = c_file.read_text(encoding='utf-8')
            for name in ('SCALAR_CASE', 'POINTER_CASE', 'CALLBACK_CASE',
                         'STRING_CASE', 'MANAGED_CASE', 'UNUSED_STRING_CASE', 'VARARG_CASE'):
                pattern = (re.escape(CONDITION) + r'\s*\n([^\n]*\b' + name +
                           r'\([^\n]*)\n#else\n([^\n]*\b' + name + r'\([^\n]*)\n#endif')
                match = re.search(pattern, generated)
                if not match or 'IGNORED' in match.group(1) or 'IGNORED' not in match.group(2):
                    raise RuntimeError('wrong conditional definition for ' + name)
                if '__attribute__((unused))' in match.group(2):
                    raise RuntimeError('unused parameter warning is suppressed for ' + name)
            for name, cc in (('gcc', gcc), ('clang', clang)):
                for standard in ('gnu11', 'gnu23'):
                    output = run([cc, '-std=' + standard, '-nostdinc', '-O2', '-fsyntax-only',
                                  '-Wall', '-Wextra', '-Wpedantic', '-fno-diagnostics-color',
                                  c_file], work, environment)
                    unused = output.count('[-Wunused-parameter]')
                    counts[(backend, name, standard)] = unused
                    if standard == 'gnu11' and unused < 7:
                        raise RuntimeError('GNU11 did not report the unused ABI parameters')
                    if standard == 'gnu23' and unused:
                        raise RuntimeError('C23 retained unused ABI parameter names')
            for standard in ('gnu11', 'gnu23'):
                exe = work / (backend + '-' + standard + '.exe')
                output = run([compiler, '-prefix', prefix, '-i', ROOT / 'inc', '-gen', backend,
                              '-Wc', '-std=' + standard + ',-Wall,-Wextra,-Wpedantic',
                              '-v', '-x', exe, source], work, environment)
                if '-Wno-unused' in output:
                    raise RuntimeError('driver suppressed unused diagnostics')
                if standard == 'gnu11' and '[-Wunused-parameter]' not in output:
                    raise RuntimeError('driver did not expose the GNU11 parameter warnings')
                if standard == 'gnu23' and '[-Wunused-parameter]' in output:
                    raise RuntimeError('driver retained the C23 parameter warnings')
                if 'C23_PARAMETER_PASS' not in run([exe], work, environment):
                    raise RuntimeError('unexpected runtime result')
        if args.gas64:
            exe = work / 'native.exe'
            run([compiler, '-prefix', prefix, '-gen', 'gas64', '-x', exe,
                 HERE / 'c23-parameters.bas'], work, environment)
            if 'C23_PARAMETER_PASS' not in run([exe], work, environment):
                raise RuntimeError('native comparison failed')
        for options, source in (((), 'c23-parameters-asm.bas'), (('-g',), 'c23-parameters.bas')):
            c_file = work / ('opaque-' + source + '.c')
            emit(compiler, prefix, HERE / source, c_file, 'gcc', environment, options)
            if CONDITION in c_file.read_text(encoding='utf-8'):
                raise RuntimeError('assembly or debug output did not retain named parameters')
            for cc in (gcc, clang):
                run([cc, '-std=gnu23', '-nostdinc', '-fsyntax-only', '-Wall', '-Wextra',
                     '-Wpedantic', c_file], work, environment)
        for backend in ('gcc', 'clang') + (('gas64',) if args.gas64 else ()):
            exe = work / ('asm-' + backend + '.exe')
            command = [compiler, '-prefix', prefix, '-gen', backend]
            if backend != 'gas64':
                command += ['-Wc', '-std=gnu23']
            run([*command, '-x', exe, HERE / 'c23-parameters-asm.bas'], work, environment)
            if 'C23_PARAMETER_ASM_PASS' not in run([exe], work, environment):
                raise RuntimeError('assembly fallback runtime failed')
        for target, arch, triple in (('win32', None, 'i686-w64-windows-gnu'),
                                      ('linux', '686', 'i686-unknown-linux-gnu'),
                                      ('linux', None, 'x86_64-unknown-linux-gnu'),
                                      ('linux-arm', None, 'armv7-unknown-linux-gnueabihf'),
                                      ('linux-aarch64', None, 'aarch64-unknown-linux-gnu')):
            options = ['-target', target]
            if arch:
                options += ['-arch', arch]
            c_file = work / (triple + '.c')
            emit(compiler, prefix, HERE / 'c23-parameters.bas', c_file, 'clang', environment, options)
            for standard in ('gnu11', 'gnu23'):
                run([clang, '--target=' + triple, '-std=' + standard, '-nostdinc', '-fsyntax-only',
                     '-Wall', '-Wextra', '-Wpedantic', c_file], work, environment)
    print('C23_PARAMETERS_PASS unused_counts=' + repr(counts), flush=True)


if __name__ == '__main__':
    main()

# end of c23-parameters.py
