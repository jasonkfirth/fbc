"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-error-flow.py
Purpose: Keep checked-array temporary initialization visible to C compilers.
Responsibilities: Exercise -e/-exx with both C backends, target widths and
    optimized/unoptimized object builds, preserving all warning categories.
This file intentionally does NOT require target runtime libraries or run faults.
"""
import argparse
import importlib.util
from pathlib import Path


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'gcc', 'clang', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--expect-uninitialized', action='store_true')
    options = parser.parse_args()
    output = options.output.resolve()
    if not output.is_relative_to(root / 'build'):
        parser.error('output must be below build/')
    output.mkdir(parents=True, exist_ok=False)
    spec = importlib.util.spec_from_file_location('audit_c', root / 'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    source = root / 'tests/quirk/c-checked-array-flow.bas'
    tools = {name:dict(path=str(getattr(options, name).resolve()),
                       sha256=audit.digest(getattr(options, name).resolve()))
             for name in ('fbc', 'gcc', 'clang')}
    records = []
    rejected = 0
    for target, width in (('win64', '64'), ('win32', '32')):
        for backend in ('gcc', 'clang'):
            for mode in ('e', 'exx'):
                folder = output / (target + '-' + backend + '-' + mode)
                folder.mkdir()
                row = dict(target=target, backend=backend, error_checks=mode, cases=[],
                           source=str(source), source_sha256=audit.digest(source))
                row['emit'] = audit.invoke([tools['fbc']['path'], '-prefix', str(root),
                    '-gen', backend, '-target', target, '-r', '-m', source.stem, '-' + mode,
                    str(source), '-o', str(folder / 'fixture.o')], folder, folder / 'emit.log', 30)
                records.append(row)
                audit.write_json(output / 'results.json', dict(records=records, tools=tools))
                if row['emit']['exit']:
                    raise RuntimeError('Checked-array fixture did not emit')
                cfile = folder / 'fixture.c'
                row['c_sha256'] = audit.digest(cfile)
                if not options.expect_uninitialized and '_unusedlabel' in cfile.read_text():
                    raise RuntimeError('An artificial compatibility target remains')
                for compiler in ('gcc', 'clang'):
                    warnings = (audit.GCC_WARNINGS + ['-fno-diagnostics-show-caret'] if compiler == 'gcc'
                                else ['-Weverything', '-fno-caret-diagnostics'])
                    warnings += ['-Werror=uninitialized']
                    if compiler == 'gcc':
                        warnings += ['-Werror=maybe-uninitialized']
                    else:
                        warnings += ['-Werror=conditional-uninitialized', '-Werror=sometimes-uninitialized']
                    for optimization in ('O0', 'O2'):
                        case = dict(compiler=compiler, optimization=optimization)
                        case['object'] = audit.invoke([tools[compiler]['path'], *audit.SEMANTICS,
                            '-m' + width, '-' + optimization, *warnings, '-fdiagnostics-color=never',
                            '-c', str(cfile), '-o', 'NUL'], folder,
                            folder / (compiler + '-' + optimization + '.log'), 30)
                        row['cases'].append(case)
                        errors = [x for x in case['object']['counts'] if x['severity'] in ('error', 'fatal error')]
                        if case['object']['exit']:
                            if (not options.expect_uninitialized or not errors or
                                    any('uninitialized' not in x['flag'] for x in errors)):
                                raise RuntimeError('Unexpected checked-array object rejection')
                            rejected += 1
                        audit.write_json(output / 'results.json', dict(records=records, tools=tools))
    if options.expect_uninitialized and not rejected:
        raise RuntimeError('The baseline did not reproduce an uninitialized warning')
    audit.write_json(output / 'qualification.json', dict(emissions=len(records),
        object_checks=sum(len(x['cases']) for x in records), rejected=rejected,
        expected_baseline_rejections=options.expect_uninitialized, tools=tools))
    print('C_EMITTER_ERROR_FLOW_' + ('BASELINE_REJECTED' if options.expect_uninitialized else 'PASS'),
          'emissions=' + str(len(records)), 'objects=32', 'rejected=' + str(rejected), flush=True)


if __name__ == '__main__':
    main()

# end of test-c-emitter-error-flow.py
