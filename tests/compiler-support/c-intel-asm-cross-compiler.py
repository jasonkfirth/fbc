"""Project: FreeBASIC C backend regression tests.

File: c-intel-asm-cross-compiler.py
Purpose: Compile Intel assembly emitted for GCC with both GCC and Clang.
Responsibilities: Exercise memory operands, segment prefixes, local labels,
    quoted data, and mixed-case assembly qualifiers without hiding warnings.
This file intentionally does NOT execute segment-dependent instructions.
"""

import argparse
from collections import Counter
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
CASES = ('inline-asm-syntax.bas', 'inline-asm.bas', 'c-asm-segment-prefix.bas',
         'inline-asm-registers.bas', 'c-asm-local-labels.bas', 'c-asm-compact-ptr.bas')


def run(command):
    result = subprocess.run([str(part) for part in command], cwd=ROOT,
                            capture_output=True, text=True, errors='replace', timeout=120)
    output = result.stdout + result.stderr
    if result.returncode:
        raise RuntimeError('command failed: ' + str(command) + '\n' + output)
    return len(re.findall(r'(?m)^.*\bwarning:', output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    parent = args.work_dir.resolve(strict=True)
    warnings = Counter()

    # TemporaryDirectory owns only its newly created child and removes it on exit.
    with tempfile.TemporaryDirectory(prefix='c-intel-asm-', dir=parent) as directory:
        work = Path(directory).resolve()
        if work.parent != parent:
            raise RuntimeError('temporary output is outside the work directory')
        for name in CASES:
            source = ROOT / 'tests/quirk' / name
            c_file = work / (source.stem + '.c')
            module = 'fbc-tests' if name in CASES[:2] else source.stem
            run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-asm', 'intel',
                 '-r', '-i', ROOT / 'inc', '-i', ROOT / 'tests/fbcunit/inc',
                 '-m', module, source, '-o', c_file])
            common = ['-std=gnu11', '-nostdinc', '-fno-strict-aliasing', '-fwrapv',
                      '-frounding-math', '-fno-math-errno', '-O2', '-masm=intel']
            compilers = (('gcc', args.gcc, ['-Wall', '-Wextra', '-Wpedantic']),
                         ('clang', args.clang, ['-Weverything']))
            for label, compiler, flags in compilers:
                warnings[label] += run([compiler, *common, *flags, '-c', c_file,
                                        '-o', work / (label + '.o')])
    print('C_INTEL_ASM_CROSS_COMPILER_PASS cases=' + str(len(CASES)) +
          ' gcc_warnings=' + str(warnings['gcc']) +
          ' clang_warnings=' + str(warnings['clang']))


if __name__ == '__main__':
    main()


# end of c-intel-asm-cross-compiler.py
