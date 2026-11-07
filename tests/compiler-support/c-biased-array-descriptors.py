"""Project: FreeBASIC C backend regression tests.

File: c-biased-array-descriptors.py
Purpose: Check static array descriptors with nonzero lower bounds.
Responsibilities: Test C initialization, array calls, native execution, and
    compile-only output for representative 32-bit and 64-bit targets.
This file intentionally does NOT qualify the runtime's dynamic array allocator.
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
    parser.add_argument('--baseline-fbc')
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    parent = args.work_dir.resolve(strict=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(args.gcc).resolve().parent) + os.pathsep + environment.get('PATH', '')
    counts = {'gcc': 0, 'clang': 0}

    # The temporary child is owned by this test and removed when it finishes.
    with tempfile.TemporaryDirectory(prefix='c-biased-array-', dir=parent) as directory:
        work = Path(directory).resolve()
        if work.parent != parent:
            raise RuntimeError('temporary output is outside the work directory')
        fixture = work / 'biased-array-descriptor.bas'
        fixture.write_text("""/'
    Project: FreeBASIC C backend regression tests
    File: biased-array-descriptor.bas
    Purpose: Exercise fixed array descriptors with nonzero lower bounds.
    Responsibilities: Check positive, negative, and multidimensional bounds.
    This file intentionally does NOT allocate dynamic array storage.
'/
Dim Shared positive(1 To 8) As Long
Dim Shared negative(-4 To -2) As Long
Dim Shared matrix(2 To 4, -3 To -1) As Long

Function SumEnds(values() As Long) As Long
    Return values(LBound(values)) + values(UBound(values))
End Function

Function SumCorners(values() As Long) As Long
    Return values(2, -3) + values(4, -1)
End Function

positive(1) = 11
positive(8) = 31
negative(-4) = 12
negative(-2) = 30
matrix(2, -3) = 19
matrix(4, -1) = 23
If SumEnds(positive()) <> 42 Then End 1
If SumEnds(negative()) <> 42 Then End 2
If SumCorners(matrix()) <> 42 Then End 3
Print "BIASED_ARRAY_DESCRIPTOR_PASS"
/' end of biased-array-descriptor.bas '/
""", encoding='utf-8')
        sources = [(fixture, 'BIASED_ARRAY_DESCRIPTOR_PASS'),
                   (ROOT / 'tests/compiler-support/c-static-array-storage.bas', 'C_STATIC_ARRAY_STORAGE_PASS')]

        if args.baseline_fbc:
            baseline_c = work / 'baseline.c'
            original = sources[1][0]
            run([args.baseline_fbc, '-prefix', args.prefix, '-gen', 'gcc', '-i', ROOT / 'inc',
                 '-m', original.stem, '-r', original, '-o', baseline_c], environment)
            result, _ = run([args.clang, '-std=gnu11', '-nostdinc', '-Weverything',
                             '-Werror=array-bounds-pointer-arithmetic', '-fsyntax-only', baseline_c],
                            environment, expect_success=False)
            if '-Werror,-Warray-bounds-pointer-arithmetic' not in result.stderr:
                raise RuntimeError('the baseline did not reproduce the descriptor warning')

        for source, marker in sources:
            for mode, options in (('unchecked', []), ('checked', ['-exx'])):
                c_file = work / (source.stem + '-' + mode + '.c')
                run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-i', ROOT / 'inc',
                     *options, '-m', source.stem, '-r', source, '-o', c_file], environment)
                for label, compiler in (('gcc', args.gcc), ('clang', args.clang)):
                    obj = work / (source.stem + '-' + mode + '-' + label + '.o')
                    flags = ['-std=gnu11', '-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-O2',
                             '-Wall', '-Wextra', '-Wpedantic', '-Wconversion',
                             '-Warray-bounds=2', '-Werror=array-bounds']
                    if label == 'clang':
                        flags.remove('-Warray-bounds=2')
                        flags += ['-Weverything', '-Werror=array-bounds-pointer-arithmetic',
                                  '-fsanitize=null,pointer-overflow,alignment',
                                  '-fsanitize-trap=null,pointer-overflow,alignment']
                    _, warnings = run([compiler, *flags, '-c', c_file, '-o', obj], environment)
                    counts[label] += warnings
                    exe = work / (source.stem + '-' + mode + '-' + label + ('.exe' if os.name == 'nt' else ''))
                    run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-x', exe, obj], environment)
                    result, _ = run([exe], environment)
                    if result.stdout.strip() != marker:
                        raise RuntimeError(label + ' did not preserve descriptor behavior')
                gas_exe = work / (source.stem + '-' + mode + '-gas64' + ('.exe' if os.name == 'nt' else ''))
                run([args.fbc, '-prefix', args.prefix, '-gen', 'gas64', '-i', ROOT / 'inc',
                     *options, '-x', gas_exe, source], environment)
                result, _ = run([gas_exe], environment)
                if result.stdout.strip() != marker:
                    raise RuntimeError('GAS64 did not preserve descriptor behavior')

        targets = [('win32', 'i686-w64-windows-gnu'),
                   ('linux-x86_64', 'x86_64-unknown-linux-gnu'),
                   ('linux-arm', 'armv7-unknown-linux-gnueabihf'),
                   ('linux-aarch64', 'aarch64-unknown-linux-gnu')]
        for target, triple in targets:
            c_file = work / (target + '.c')
            run([args.fbc, '-prefix', args.prefix, '-target', target, '-gen', 'gcc',
                 '-i', ROOT / 'inc', '-exx', '-m', fixture.stem, '-r', fixture, '-o', c_file], environment)
            _, warnings = run([args.clang, '-target', triple, '-std=gnu11', '-nostdinc',
                               '-fno-strict-aliasing', '-fwrapv', '-O2', '-Weverything',
                               '-Werror=array-bounds-pointer-arithmetic', '-c', c_file,
                               '-o', work / (target + '.o')], environment)
            counts['clang'] += warnings

        # A genuinely short descriptor must still be diagnosed by GCC. This
        # file is compiled only; it is deliberately never executed.
        invalid = work / 'short-descriptor.c'
        invalid.write_text("""/*
    Project: FreeBASIC C backend regression tests
    File: short-descriptor.c
    Purpose: Confirm that a real descriptor overrun remains detectable.
    Responsibilities: Supply a source object shorter than the requested read.
    This file intentionally does NOT run as a program.
*/
typedef long long word;
struct descriptor_one { unsigned char header[48]; word dimensions[3]; };
static struct descriptor_one array;
int main(void)
{
    return (int)*((word*)__builtin_memcpy(&(word){0},
        &((const unsigned char (*)[80 + sizeof(word)])(&array))[0][80],
        sizeof(word)));
}
/* end of short-descriptor.c */
""", encoding='utf-8')
        result, _ = run([args.gcc, '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                         '-Warray-bounds=2', '-Werror=array-bounds', '-c', invalid,
                         '-o', work / 'short-descriptor.o'], environment, expect_success=False)
        if '-Werror=array-bounds' not in result.stderr:
            raise RuntimeError('GCC did not diagnose the deliberately short descriptor')

    print('C_BIASED_ARRAY_DESCRIPTORS_PASS gcc_warnings=' + str(counts['gcc']) +
          ' clang_warnings=' + str(counts['clang']))


if __name__ == '__main__':
    main()


# end of c-biased-array-descriptors.py
