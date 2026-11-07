"""Project: FreeBASIC C backend regression tests.

File: c-debug-array-descriptors.py
Purpose: Check unused fixed-array descriptors in debug C output.
Responsibilities: Exercise observable array data and initialization,
    descriptor arguments and inline assembly with the available backends.
This file intentionally does NOT replace debugger or full-suite testing.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'tests/compiler-support/c-debug-array-descriptors.bas'


def run(command, environment):
    result = subprocess.run([str(part) for part in command], cwd=ROOT, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=180)
    output = result.stdout + result.stderr
    if result.returncode:
        raise RuntimeError('command failed: ' + str(command) + '\n' + output[:5000])
    return result, len(re.findall(r'(?m)^.*\bwarning:', output))


def function_body(source, name):
    header = re.compile(r'(?m)^static\s+void\s+' + re.escape(name) +
                        r'\s*\([^;\n]*\)\s*(?:#line[^\n]*\n\s*)?\{')
    match = header.search(source)
    if match is None:
        raise RuntimeError('missing emitted function ' + name)
    depth = 1
    for end in range(match.end(), len(source)):
        if source[end] == '{':
            depth += 1
        elif source[end] == '}':
            depth -= 1
            if depth == 0:
                return source[match.end():end]
    raise RuntimeError('unterminated emitted function ' + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    parser.add_argument('--gas64', action='store_true', help='also run the GAS64 backend')
    args = parser.parse_args()
    args.work_dir.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(args.gcc).resolve().parent) + os.pathsep + environment.get('PATH', '')
    counts = {}

    with tempfile.TemporaryDirectory(prefix='debug-arrays-', dir=args.work_dir) as temporary:
        work = Path(temporary)
        c_file = work / 'arrays.c'
        run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-g', '-exx',
             '-m', SOURCE.stem, '-r', '-o', c_file, SOURCE], environment)
        source = c_file.read_text(encoding='utf-8')
        for name, should_keep in (('CHECKDIRECT', False), ('CHECKPASSED', True),
                                  ('CHECKOPAQUE', True), ('CHECKAFTEROPAQUE', False)):
            body = function_body(source, name)
            has_descriptor = bool(re.search(r'\.DATA\s*=.*\bfb_c_sym_VALUES_', body))
            if has_descriptor != should_keep:
                raise RuntimeError('unexpected values descriptor in ' + name)

        for label, compiler in (('gcc', args.gcc), ('clang', args.clang)):
            obj = work / (label + '.o')
            flags = ['-std=gnu11', '-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-O2', '-g',
                     '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Warray-bounds',
                     '-Wunused-but-set-variable']
            if label == 'clang':
                flags.append('-Weverything')
            _, counts[label] = run([compiler, *flags, '-c', c_file, '-o', obj], environment)
            exe = work / (label + ('.exe' if os.name == 'nt' else ''))
            run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-g', '-x', exe, obj], environment)
            result, _ = run([exe], environment)
            if result.stdout.strip() != 'C_DEBUG_ARRAY_DESCRIPTORS_PASS':
                raise RuntimeError(label + ' produced an unexpected result')

        if args.gas64:
            exe = work / ('gas64.exe' if os.name == 'nt' else 'gas64')
            run([args.fbc, '-prefix', args.prefix, '-gen', 'gas64', '-g', '-exx',
                 '-x', exe, SOURCE], environment)
            result, _ = run([exe], environment)
            if result.stdout.strip() != 'C_DEBUG_ARRAY_DESCRIPTORS_PASS':
                raise RuntimeError('GAS64 produced an unexpected result')

    print('C_DEBUG_ARRAY_DESCRIPTORS_PASS gcc_warnings=' + str(counts['gcc']) +
          ' clang_warnings=' + str(counts['clang']))


if __name__ == '__main__':
    main()


# end of c-debug-array-descriptors.py
