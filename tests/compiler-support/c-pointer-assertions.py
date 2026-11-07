"""Project: FreeBASIC C backend regression tests
File: c-pointer-assertions.py
Purpose: Check declaration placement without weakening pointer ABI assertions.
Responsibilities: Reuse the callback and qualified-pointer runtime controls,
compare old/new objects, and check GNU11/GNU23, O0/O2, debug emission and Win32.
This file intentionally does NOT execute foreign targets or install compilers.
"""
import argparse
from collections import Counter
import hashlib
import difflib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ASSERTION = re.compile(rb'\s*enum \{ fb_c_assert_\d+ =')


def assertion_lines(raw):
    return sorted(line.strip() for line in raw.splitlines() if ASSERTION.match(line))


def without_assertions(raw):
    lines = []
    for line in raw.splitlines(keepends=True):
        if ASSERTION.match(line):
            if lines and lines[-1].lstrip().startswith(b'#line '):
                lines.pop()
        else:
            lines.append(line)
    return b''.join(lines)


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

    def run(command, cwd, success=True):
        result = subprocess.run([str(x) for x in command], cwd=cwd, env=environment,
                                capture_output=True, text=True, errors='replace', timeout=60)
        output = result.stdout + result.stderr
        if (result.returncode == 0) != success:
            raise RuntimeError(' '.join(str(x) for x in command) + '\n' + output[-3000:])
        return output

    with tempfile.TemporaryDirectory(prefix='ptr-assert-', dir=args.work_dir.resolve()) as directory:
        work = Path(directory)
        cfile, obj, exe, observer = (work / name for name in ('unit.c', 'unit.o', 'unit.exe', 'observer.o'))
        for source in (HERE / 'c-pointer-qualification-values.bas', HERE / 'c-code-pointer-load.bas'):
            code_pointer = source.stem == 'c-code-pointer-load'
            markers = (['CODE_POINTER_LOAD_PASS', 'CODE_POINTER_ERASURE_PASS', 'CODE_POINTER_SIGNATURES_PASS']
                       if code_pointer else ['C_POINTER_QUALIFICATION_PASS'])
            for producer in ('gcc', 'clang'):
                for target in ('win64', 'win32'):
                    for debug in (False, True):
                        emitted = {}
                        for label, compiler in (('before', args.baseline_fbc), ('after', args.fbc)):
                            if compiler is None:
                                continue
                            run([compiler.resolve(), '-prefix', args.prefix.resolve(), '-gen', producer,
                                 '-asm', 'intel', '-target', target, *(['-g'] if debug else []),
                                 '-w', 'all', '-m', source.stem, '-r', source, '-o', cfile], work)
                            emitted[label] = cfile.read_bytes()
                        if 'before' in emitted:
                            if (without_assertions(emitted['before']) != without_assertions(emitted['after']) or
                                    assertion_lines(emitted['before']) != assertion_lines(emitted['after'])):
                                diff = list(difflib.unified_diff(without_assertions(emitted['before']).decode().splitlines(),
                                            without_assertions(emitted['after']).decode().splitlines()))
                                raise RuntimeError('unexpected C change: ' + source.name + ' ' + repr((producer, target, debug)) +
                                                   '\n' + '\n'.join(diff[:30]))
                            counts['equivalent_C_pairs'] += 1
                        for name, compiler, diagnostics in (
                            ('gcc', args.gcc, ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion',
                                               '-Wformat=2', '-Wdeclaration-after-statement']),
                            ('clang', args.clang, ['-Weverything', '-Wwrite-strings', '-ferror-limit=0'])):
                            if target == 'win64' and code_pointer:
                                run([compiler.resolve(), '-std=gnu11', '-O2', '-c',
                                     HERE / 'c-code-pointer-load.c', '-o', observer], work)
                            for standard in ('gnu11', 'gnu23'):
                                for optimization in ('-O0', '-O2'):
                                    target_flags = (['-m32'] if name == 'gcc' else
                                                    ['--target=i686-w64-windows-gnu']) if target == 'win32' else []
                                    flags = ['-std=' + standard, '-nostdinc', '-fno-strict-aliasing',
                                             '-fwrapv', '-masm=intel', optimization, *target_flags, *diagnostics]
                                    before_hash = None
                                    for label, raw in emitted.items():
                                        cfile.write_bytes(raw)
                                        output = run([compiler.resolve(), *flags,
                                                      *(['-Werror=declaration-after-statement'] if label == 'after' else []),
                                                      '-c', cfile, '-o', obj], work)
                                        warnings[label].update(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))
                                        digest = hashlib.sha256(obj.read_bytes()).hexdigest()
                                        if label == 'before':
                                            before_hash = digest
                                        elif before_hash and digest != before_hash:
                                            raise RuntimeError('pointer assertion object bytes changed')
                                        elif before_hash:
                                            counts['identical_object_pairs'] += 1
                                        counts[label + '_objects'] += 1
                                        if target == 'win64':
                                            run([args.fbc.resolve(), '-prefix', args.prefix.resolve(),
                                                 '-x', exe, obj, *([observer] if code_pointer else [])], work)
                                            output = run([exe], work)
                                            if not all(marker in output for marker in markers):
                                                raise RuntimeError('pointer runtime control did not finish')
                                            counts[label + '_runs'] += 1
                                    if target == 'win64' and not debug and optimization == '-O2':
                                        broken, changed = re.subn(rb'(enum \{ fb_c_assert_\d+ = )[^\r\n]+',
                                                                 rb'\g<1>1 / 0 };', emitted['after'], count=1)
                                        if changed != 1:
                                            raise RuntimeError('ABI assertion was removed')
                                        cfile.write_bytes(broken)
                                        run([compiler.resolve(), *flags, '-fsyntax-only', cfile], work, False)
                                        counts['failed_assertion_controls'] += 1
    if args.baseline_fbc and not warnings['before']['-Wdeclaration-after-statement']:
        raise RuntimeError('baseline did not exercise declaration placement')
    print('C_POINTER_ASSERTIONS_PASS ' + json.dumps({'checks': dict(counts),
          'warnings': {name: dict(value) for name, value in warnings.items()}}, sort_keys=True))


if __name__ == '__main__':
    main()

# end of c-pointer-assertions.py
