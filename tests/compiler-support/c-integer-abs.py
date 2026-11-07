"""Project: FreeBASIC C backend regression tests
File: c-integer-abs.py
Purpose: Check integer ABS semantics and declaration order in emitted C.
Responsibilities: Exercise the existing FB and QB controls with both C producers,
GCC/Clang, GNU11/GNU23, O0/O2 and Win32/Win64 object compilation. An optional
baseline proves the declaration repair preserves object bytes and runtime output.
This file intentionally does NOT install compilers or execute foreign targets.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ABS_DECLARATION = re.compile(
    rb'(uint(?:8|16|32|64) bits = [^\n]+;\r?\n)'
    rb'(int(?:8|16|32|64) result;\r?\n)'
    rb'(if \(value < 0\) bits = [^\n]+;\r?\n)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'prefix', 'gcc', 'clang', 'work-dir'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--baseline-fbc', type=Path)
    args = parser.parse_args()
    environment = os.environ.copy()
    environment['PATH'] = str(args.gcc.resolve().parent) + os.pathsep + environment.get('PATH', '')
    args.work_dir.mkdir(parents=True, exist_ok=True)
    counts = Counter()
    warnings = {'before': Counter(), 'after': Counter()}

    def run(command, cwd):
        result = subprocess.run([str(x) for x in command], cwd=cwd, env=environment,
                                capture_output=True, text=True, errors='replace', timeout=60)
        output = result.stdout + result.stderr
        if result.returncode:
            raise RuntimeError(' '.join(str(x) for x in command) + '\n' + output[-3000:])
        return output

    with tempfile.TemporaryDirectory(prefix='abs-', dir=args.work_dir.resolve()) as directory:
        work = Path(directory)
        c_file, obj, exe = (work / name for name in ('unit.c', 'unit.o', 'unit.exe'))
        for source in (HERE / 'c-integer-abs.bas', HERE / 'c-integer-abs-qb.bas'):
            marker = 'C_INTEGER_ABS_QB_PASS' if source.stem.endswith('-qb') else 'C_INTEGER_ABS_PASS'
            for producer in ('gcc', 'clang'):
                for target in ('win64', 'win32'):
                    emitted = {}
                    for label, compiler in (('before', args.baseline_fbc), ('after', args.fbc)):
                        if compiler is None:
                            continue
                        run([compiler.resolve(), '-prefix', args.prefix.resolve(), '-gen', producer,
                             '-asm', 'intel', '-target', target, '-w', 'all', '-m', source.stem,
                             '-r', source, '-o', c_file], work)
                        emitted[label] = c_file.read_bytes()
                    if 'before' in emitted:
                        restored, changes = ABS_DECLARATION.subn(rb'\1\3\2', emitted['after'])
                        if not changes or restored != emitted['before']:
                            raise RuntimeError('unexpected emitted C change: ' + source.name)
                        counts['equivalent_C_pairs'] += 1
                    for name, compiler, diagnostics in (
                        ('gcc', args.gcc, ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion',
                                           '-Wformat=2', '-Wdeclaration-after-statement']),
                        ('clang', args.clang, ['-Weverything', '-Wwrite-strings', '-ferror-limit=0'])):
                        for standard in ('gnu11', 'gnu23'):
                            for optimization in ('-O0', '-O2'):
                                before_hash = None
                                for label, raw in emitted.items():
                                    c_file.write_bytes(raw)
                                    target_flags = (['-m32'] if name == 'gcc' else
                                                    ['--target=i686-w64-windows-gnu']) if target == 'win32' else []
                                    output = run([compiler.resolve(), '-std=' + standard, '-nostdinc',
                                                  '-fno-strict-aliasing', '-fwrapv', '-masm=intel',
                                                  optimization, *target_flags, *diagnostics,
                                                  '-c', c_file, '-o', obj], work)
                                    groups = Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))
                                    warnings[label].update(groups)
                                    if label == 'after' and groups['-Wdeclaration-after-statement']:
                                        raise RuntimeError('late ABS declaration remains')
                                    digest = hashlib.sha256(obj.read_bytes()).hexdigest()
                                    if label == 'before':
                                        before_hash = digest
                                    elif before_hash:
                                        if digest != before_hash:
                                            raise RuntimeError('ABS object bytes changed')
                                        counts['identical_object_pairs'] += 1
                                    counts[label + '_objects'] += 1
                                    if target == 'win64':
                                        run([args.fbc.resolve(), '-prefix', args.prefix.resolve(),
                                             '-x', exe, obj], work)
                                        if run([exe], work).strip() != marker:
                                            raise RuntimeError('ABS runtime marker missing')
                                        counts[label + '_runs'] += 1
    if args.baseline_fbc and not warnings['before']['-Wdeclaration-after-statement']:
        raise RuntimeError('baseline did not exercise the repaired warning')
    print('C_INTEGER_ABS_PASS ' + json.dumps({'checks': dict(counts),
          'warnings': {name: dict(value) for name, value in warnings.items()}}, sort_keys=True))


if __name__ == '__main__':
    main()

# end of c-integer-abs.py
