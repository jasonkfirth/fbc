"""Project: FreeBASIC C backend regression tests.

File: c-main-argv.py
Purpose: Check the generated C entry point's argument type and behavior.
Responsibilities: Compile the same program with GCC and Clang, then verify
    argument values through the FreeBASIC runtime.
This file intentionally does NOT replace the compiler's full unit suite.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def run(command, environment):
    result = subprocess.run([str(part) for part in command], cwd=ROOT, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=180)
    output = result.stdout + result.stderr
    if result.returncode:
        raise RuntimeError('command failed: ' + str(command) + '\n' + output[:5000])
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
    counts = {}

    # The temporary child is owned by this test and removed when it finishes.
    with tempfile.TemporaryDirectory(prefix='c-main-argv-', dir=parent) as directory:
        work = Path(directory).resolve()
        if work.parent != parent:
            raise RuntimeError('temporary output is outside the work directory')
        source = work / 'main-argv.bas'
        source.write_text("""/'
    Project: FreeBASIC C backend regression tests
    File: main-argv.bas
    Purpose: Exercise the implicit main procedure's argument handling.
    Responsibilities: Check ordinary and space-containing arguments.
    This file intentionally does NOT test platform-specific character encodings.
'/
If Command(1) <> "first" Then End 1
If Command(2) <> "two words" Then End 2
If Command(3) <> "" Then End 3
Print "ARGV_OK"
/' end of main-argv.bas '/
""", encoding='utf-8')
        c_file = work / 'main-argv.c'
        run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-i', ROOT / 'inc',
             '-m', 'main-argv', '-r', source, '-o', c_file], environment)
        text = c_file.read_text(encoding='utf-8')
        if re.search(r'\bmain\s*\(\s*int32\s+\w+\s*,\s*char\s*\*\s*\*\s*\w+\s*\)', text) is None:
            raise RuntimeError('the generated main signature does not use int and char **')
        for label, compiler in (('gcc', args.gcc), ('clang', args.clang)):
            obj = work / (label + '.o')
            flags = ['-std=gnu11', '-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-O2',
                     '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wredundant-decls']
            if label == 'clang':
                flags.append('-Weverything')
            _, counts[label] = run([compiler, *flags, '-c', c_file, '-o', obj], environment)
            exe = work / (label + ('.exe' if os.name == 'nt' else ''))
            run([args.fbc, '-prefix', args.prefix, '-gen', 'gcc', '-x', exe, obj], environment)
            result, _ = run([exe, 'first', 'two words'], environment)
            if result.stdout.strip() != 'ARGV_OK':
                raise RuntimeError('the generated main received incorrect arguments')

    print('C_MAIN_ARGV_PASS gcc_warnings=' + str(counts['gcc']) +
          ' clang_warnings=' + str(counts['clang']))


if __name__ == '__main__':
    main()


# end of c-main-argv.py
