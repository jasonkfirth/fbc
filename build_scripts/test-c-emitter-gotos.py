"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-gotos.py
Purpose: Keep direct and error-handler control flow valid in emitted C.
Responsibilities: Check direct and error-handler flow with both C backends,
    Win32/Win64 widths and unsuppressed GCC/Clang diagnostics.
This file intentionally does NOT require target runtime libraries or SDKs.
"""

import argparse
import importlib.util
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--expect-rejection', action='store_true')
    options = parser.parse_args()
    output = options.output.resolve()
    if not output.is_relative_to(root / 'build'):
        parser.error('output must be below build/')
    output.mkdir(parents=True, exist_ok=False)
    spec = importlib.util.spec_from_file_location('audit_c', root / 'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    sources = [root / 'tests/quirk/c-computed-goto-workaround.bas',
               root / 'tests/qb/on-error-goto.bas', root / 'tests/fblite/resume.bas',
               root / 'tests/fblite/local-error-early-exit.bas']
    records = []
    for index, source in enumerate(sources):
        for target, width in [('win64', '64'), ('win32', '32')]:
            for backend in ('gcc', 'clang'):
                work = output / (source.stem + '-' + target + '-' + backend)
                work.mkdir()
                command = [str(options.fbc.resolve()), '-prefix', str(root), '-gen', backend,
                           '-target', target, '-r', '-m', source.stem,
                           str(source), '-o', str(work / 'fixture.o')]
                row = dict(source=str(source), target=target, backend=backend,
                           source_sha256=audit.digest(source),
                           fbc_sha256=audit.digest(options.fbc.resolve()))
                row['emit'] = audit.invoke(command, work, work / 'emit.log', 30)
                if row['emit']['exit']:
                    raise RuntimeError('Fixture did not emit: ' + str(source))
                cfile = work / 'fixture.c'
                ctext = cfile.read_text()
                if not options.expect_rejection:
                    if '_llvmbug18658' in ctext:
                        raise RuntimeError('Unused computed-goto temporary remains')
                    if index == 0 and ('_unusedlabel' in ctext or '&&' in ctext):
                        raise RuntimeError('Direct control flow contains a compatibility label')
                row['c_sha256'] = audit.digest(cfile)
                for tool in ('gcc', 'clang'):
                    flags = ['-Wall', '-Wextra', '-Wpedantic', '-Werror=unused-variable',
                             '-Werror=uninitialized']
                    if index == 0 and not options.expect_rejection:
                        flags += ['-Werror=unused-label']
                    if tool == 'clang':
                        flags += ['-Weverything', '-Werror=sometimes-uninitialized']
                        if index == 0:
                            flags += ['-Werror=gnu-label-as-value']
                    executable = getattr(options, tool).resolve()
                    command = [str(executable), *audit.SEMANTICS, '-m' + width,
                               *flags, '-fsyntax-only', str(cfile)]
                    row[tool] = audit.invoke(command, work, work / (tool + '.log'), 30)
                    rejected = row[tool]['exit'] != 0
                    if rejected != options.expect_rejection:
                        raise RuntimeError('Unexpected C result: ' + str((source, target, backend, tool)))
                    if options.expect_rejection and not any(
                            entry['severity'] == 'error' and 'unused-variable' in entry['flag']
                            for entry in row[tool]['counts']):
                        raise RuntimeError('Baseline failed for another reason')
                records.append(row)
    audit.write_json(output / 'results.json', records)
    audit.write_json(output / 'tools.json', {
        name: dict(path=str(getattr(options, name).resolve()),
                   sha256=audit.digest(getattr(options, name).resolve()))
        for name in ('fbc', 'gcc', 'clang')})
    mode = 'BASELINE_REJECTED' if options.expect_rejection else 'PASS'
    print('C_EMITTER_GOTOS_' + mode + ' emissions=' + str(len(records))
          + ' c_checks=' + str(len(records) * 2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-c-emitter-gotos.py
