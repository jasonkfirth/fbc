"""Project: FreeBASIC C backend regression tests.

File: c-procedure-prototypes.py
Purpose: Check procedure declarations and C layout assertions for duplicates.
Responsibilities: Compile existing constructor fixtures with GCC and Clang,
    check startup order, and verify C11 and older-C layout assertion failures.
This file intentionally does NOT assume an order for equal-priority callbacks.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def run(command, environment, expect_success=True):
    result = subprocess.run([str(part) for part in command], cwd=ROOT, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=180)
    output = result.stdout + result.stderr
    if (result.returncode == 0) != expect_success:
        raise RuntimeError('unexpected command result: ' + str(command) + '\n' + output[:5000])
    return result, len(re.findall(r'(?m)^.*\bwarning:', output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    parent = args.work_dir.resolve(strict=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(args.gcc).resolve().parent) + os.pathsep + environment.get('PATH', '')
    counts = {'gcc': 0, 'clang': 0}

    # TemporaryDirectory owns only its newly created child and removes it on exit.
    with tempfile.TemporaryDirectory(prefix='c-procedure-prototypes-', dir=parent) as directory:
        work = Path(directory).resolve()
        if work.parent != parent:
            raise RuntimeError('temporary output is outside the work directory')
        sources = (('unit', ROOT / 'tests/dim/union-init.bas', 'fbc-tests'),
                   ('startup', ROOT / 'tests/semantic-sidecar/startup-lifecycle.bas', 'startup-lifecycle'))
        startup_c = None
        for case, source, module in sources:
            c_file = work / (case + '.c')
            run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-i', ROOT / 'inc',
                 '-i', ROOT / 'tests/fbcunit/inc', '-m', module, '-r', source, '-o', c_file], environment)
            if case == 'startup':
                startup_c = c_file
            for label, compiler in (('gcc', args.gcc), ('clang', args.clang)):
                obj = work / (case + '-' + label + '.o')
                flags = ['-std=gnu11', '-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-O2',
                         '-Wredundant-decls', '-Werror=redundant-decls']
                flags += ['-Wall', '-Wextra', '-Wpedantic', '-Wmissing-attributes'] if label == 'gcc' else ['-Weverything']
                _, warnings = run([compiler, *flags, '-c', c_file, '-o', obj], environment)
                counts[label] += warnings
                if case == 'startup':
                    exe = work / ('startup-' + label + ('.exe' if os.name == 'nt' else ''))
                    run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-x', exe, obj], environment)
                    result, _ = run([exe], environment)
                    lines = result.stdout.splitlines()
                    for marker in ('EARLY', 'I1', 'I2', 'I10', 'MAIN', 'D10', 'D2', 'D1', 'LATE'):
                        if lines.count(marker) != 1:
                            raise RuntimeError('startup fixture did not run each lifecycle event once')
                    ordered = ('EARLY', 'I1', 'I2', 'I10', 'MAIN', 'D10', 'D2', 'D1', 'LATE')
                    if [lines.index(item) for item in ordered] != sorted(lines.index(item) for item in ordered):
                        raise RuntimeError('startup or teardown ordering changed')

        text = startup_c.read_text(encoding='utf-8')
        match = re.search(r'(enum\s*\{\s*fb_c_assert_\d+\s*=\s*1\s*/\s*\(\()(.*?)(\)\s*\?\s*1\s*:\s*0\)\s*\};)', text)
        if match is None:
            raise RuntimeError('the startup fixture did not emit an ABI layout assertion')
        invalid = work / 'wrong-layout.c'
        invalid.write_text(text[:match.start(2)] + '0' + text[match.end(2):], encoding='utf-8')
        for label, compiler in (('gcc', args.gcc), ('clang', args.clang)):
            for standard in ('gnu11', 'gnu99'):
                flags = ['-std=' + standard, '-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-O2',
                         '-Wall', '-Wextra', '-Wpedantic', '-Wredundant-decls', '-Werror=redundant-decls']
                run([compiler, *flags, '-c', startup_c, '-o', work / 'valid.o'], environment)
                run([compiler, *flags, '-c', invalid, '-o', work / 'invalid.o'], environment, expect_success=False)

    print('C_PROCEDURE_PROTOTYPES_PASS gcc_warnings=' + str(counts['gcc']) +
          ' clang_warnings=' + str(counts['clang']))


if __name__ == '__main__':
    main()


# end of c-procedure-prototypes.py
