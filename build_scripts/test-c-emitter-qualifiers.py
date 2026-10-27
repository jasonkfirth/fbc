"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-qualifiers.py
Purpose: Reject call casts that lose pointer qualifiers in emitted C.
Responsibilities: Check GCC/Clang emission and C frontend diagnostics at both widths.
This file intentionally does NOT suppress warnings or require cross-target SDKs.
"""

import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import sys


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--expect-rejection', action='store_true')
    parser.add_argument('--runtime-dir', type=Path, help='Link and run Win64 checks against this runtime')
    options = parser.parse_args()
    if options.runtime_dir:
        options.runtime_dir = options.runtime_dir.resolve()
        if options.expect_rejection or not all((options.runtime_dir / name).is_file()
                for name in ('fbrt0.o', 'libfb.a')):
            parser.error('runtime checks require valid Win64 runtime files and an accepted compiler')
    output = options.output.resolve()
    if not output.is_relative_to(root / 'build'):
        parser.error('output must be below build/')
    output.mkdir(parents=True, exist_ok=False)
    spec = importlib.util.spec_from_file_location('audit_c', root / 'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    source = root / 'tests/pointers/const-nested-arg-casts.bas'
    records = []
    for target, width in [('win64', '64'), ('win32', '32')]:
        for backend in ('gcc', 'clang'):
            work = output / (target + '-' + backend)
            work.mkdir()
            command = [str(options.fbc.resolve()), '-prefix', str(root), '-i', str(root / 'inc'),
                       '-gen', backend, '-target', target, '-r', '-m', source.stem,
                       str(source), '-o', str(work / 'fixture.o')]
            emit = audit.invoke(command, root, work / 'emit.log', 20)
            assert emit['exit'] == 0, emit
            record = {'target': target, 'backend': backend, 'emit': emit}
            for compiler in ('gcc', 'clang'):
                flags = ['-Wall', '-Wextra', '-Wpedantic', '-Wcast-qual',
                         '-Werror=incompatible-pointer-types']
                if compiler == 'clang':
                    flags += ['-Weverything', '-Werror=incompatible-pointer-types-discards-qualifiers']
                else:
                    flags += ['-Werror=discarded-qualifiers']
                command = [str(getattr(options, compiler).resolve()), *audit.SEMANTICS,
                           '-m' + width, *flags, '-fsyntax-only', str(work / 'fixture.c')]
                check = audit.invoke(command, work, work / (compiler + '.log'), 20)
                record[compiler] = check
                rejected = check['exit'] != 0
                assert rejected == options.expect_rejection, (target, backend, compiler, check)
                if options.expect_rejection:
                    assert any(row['severity'] == 'error' and 'incompatible-pointer-types' in row['flag']
                               for row in check['counts']), 'Baseline failed for an unrelated reason'
                if options.runtime_dir and target == 'win64':
                    obj = work / (compiler + '.o')
                    exe = work / (compiler + '.exe')
                    command = [str(getattr(options, compiler).resolve()), *audit.SEMANTICS,
                               '-m64', *flags, '-O2', '-c', str(work / 'fixture.c'), '-o', str(obj)]
                    record[compiler + '-object'] = audit.invoke(command, work, work / (compiler + '-object.log'), 20)
                    assert record[compiler + '-object']['exit'] == 0
                    command = [str(options.gcc.resolve()), str(obj), str(options.runtime_dir / 'fbrt0.o'),
                               str(options.runtime_dir / 'libfb.a'), '-o', str(exe), '-lm', '-mthreads']
                    record[compiler + '-link'] = audit.invoke(command, work, work / (compiler + '-link.log'), 20)
                    assert record[compiler + '-link']['exit'] == 0
                    record[compiler + '-run'] = audit.invoke([str(exe)], work, work / (compiler + '-run.log'), 10)
                    assert record[compiler + '-run']['exit'] == 0
            records.append(record)
    audit.write_json(output / 'results.json', records)
    mode = 'BASELINE_REJECTED' if options.expect_rejection else 'PASS'
    print('C_EMITTER_QUALIFIERS_' + mode + ' emissions=4 c_checks=8')
    if options.runtime_dir:
        print('C_EMITTER_QUALIFIERS_RUNTIME_PASS executions=4')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-c-emitter-qualifiers.py
