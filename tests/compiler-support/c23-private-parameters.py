"""Project: FreeBASIC C backend regression tests
File: c23-private-parameters.py
Purpose: Check unnamed private procedure parameters in C23 output.
Responsibilities: Verify call signatures, argument evaluation and legacy output.
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
SOURCE = HERE / 'c23-private-parameters.bas'
CONDITION = '#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L'


def run(command, cwd, environment):
    result = subprocess.run([str(part) for part in command], cwd=cwd, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    if result.returncode:
        raise RuntimeError('command failed: ' + ' '.join(str(part) for part in command) +
                           '\n' + result.stdout + result.stderr)
    return result.stdout + result.stderr


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--gcc', required=True, type=Path)
    parser.add_argument('--clang', required=True, type=Path)
    parser.add_argument('--gas64', action='store_true')
    args = parser.parse_args()
    fbc, prefix = args.fbc.resolve(), args.prefix.resolve()
    gcc, clang = args.gcc.resolve(), args.clang.resolve()
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')
    counts = {}
    with tempfile.TemporaryDirectory(prefix='fbc-private-parameters-') as directory:
        work = Path(directory)

        def invoke(command):
            return run(command, work, environment)

        def emit(backend, output, options=()):
            invoke([fbc, '-prefix', prefix, '-i', ROOT / 'inc', '-m', SOURCE.stem,
                    '-gen', backend, *options, '-r', SOURCE, '-o', output])

        for backend in ('gcc', 'clang'):
            c_file = work / (backend + '.c')
            emit(backend, c_file)
            generated = c_file.read_text(encoding='utf-8')
            for name in ('DIRECT_VALUE', 'DIRECT_VARARG', 'ADDRESS_VALUE', 'WRITE_REF', 'FACTORIAL'):
                pattern = (re.escape(CONDITION) + r'\s*\n([^\n]*\b' + name +
                           r'\([^\n]*)\n#else\n([^\n]*\b' + name + r'\([^\n]*)\n#endif')
                match = re.search(pattern, generated)
                if not match or 'IGNORED' in match.group(1) or 'IGNORED' not in match.group(2):
                    raise RuntimeError('incorrect conditional definition for ' + name)
            for name, cc in (('gcc', gcc), ('clang', clang)):
                for standard in ('gnu11', 'gnu23'):
                    obj = work / (backend + '-' + name + '-' + standard + '.o')
                    warnings = (['-Weverything', '-ferror-limit=0'] if name == 'clang'
                                else ['-Wall', '-Wextra', '-Wpedantic'])
                    output = invoke([cc, '-std=' + standard, '-nostdinc', '-O2',
                                     *warnings, '-c', c_file, '-o', obj])
                    count = output.count('[-Wunused-parameter]')
                    counts[(backend, name, standard)] = count
                    if standard == 'gnu11' and count < 5:
                        raise RuntimeError('GNU11 did not report the five unused parameters')
                    if standard == 'gnu23' and count:
                        raise RuntimeError('C23 retained an unused parameter name')
                    exe = work / (backend + '-' + name + '-' + standard + '.exe')
                    invoke([fbc, '-prefix', prefix, '-gen', backend, '-x', exe, obj])
                    if 'C23_PRIVATE_PARAMETERS_PASS' not in invoke([exe]):
                        raise RuntimeError('argument evaluation or private call changed')
            debug_c = work / (backend + '-debug.c')
            emit(backend, debug_c, ('-g',))
            if CONDITION in debug_c.read_text(encoding='utf-8'):
                raise RuntimeError('debug output did not retain the parameter names')
        if args.gas64:
            exe = work / 'native.exe'
            invoke([fbc, '-prefix', prefix, '-gen', 'gas64', '-x', exe, SOURCE])
            if 'C23_PRIVATE_PARAMETERS_PASS' not in invoke([exe]):
                raise RuntimeError('native comparison failed')
        for target, triple in (('win32', 'i686-w64-windows-gnu'),
                               ('linux', 'x86_64-unknown-linux-gnu'),
                               ('linux-arm', 'armv7-unknown-linux-gnueabihf'),
                               ('linux-aarch64', 'aarch64-unknown-linux-gnu')):
            c_file = work / (triple + '.c')
            emit('clang', c_file, ('-target', target))
            for standard in ('gnu11', 'gnu23'):
                invoke([clang, '--target=' + triple, '-std=' + standard, '-nostdinc',
                        '-fsyntax-only', '-Wall', '-Wextra', '-Wpedantic', c_file])
    print('PRIVATE_C23_PARAMETERS_PASS unused_counts=' + repr(counts), flush=True)


if __name__ == '__main__':
    main()

# end of c23-private-parameters.py
