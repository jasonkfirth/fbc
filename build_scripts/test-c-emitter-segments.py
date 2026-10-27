"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-segments.py
Purpose: Keep Intel segment operands valid in the GCC and Clang C backends.
Responsibilities: Build real Win32/Win64 objects, preserve quoted assembler data,
    and optionally verify the old Clang rejection and unchanged GCC output.
This file intentionally does NOT execute segment-dependent instructions.
"""
import argparse
import importlib.util
from pathlib import Path
import re


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    options = parser.parse_args()
    output = options.output.resolve()
    if not output.is_relative_to(root / 'build'):
        parser.error('output must be below build/')
    output.mkdir(parents=True, exist_ok=False)
    spec = importlib.util.spec_from_file_location('audit_c', root / 'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    source = root / 'tests/quirk/c-asm-segment-prefix.bas'
    source_pin = audit.digest(source)
    tools = {name: dict(path=str(getattr(options, name).resolve()),
                        sha256=audit.digest(getattr(options, name).resolve()))
             for name in ('fbc', 'gcc', 'clang')}
    compilers = [('candidate', options.fbc.resolve())]
    if options.baseline:
        compilers.insert(0, ('baseline', options.baseline.resolve()))
        tools['baseline'] = dict(path=str(options.baseline.resolve()),
                                 sha256=audit.digest(options.baseline.resolve()))
    records = []
    texts = {}
    for version, compiler in compilers:
        for target, width in [('win32', '32'), ('win64', '64')]:
            for backend in ('gcc', 'clang'):
                work = output / (version + '-' + target + '-' + backend)
                work.mkdir()
                command = [str(compiler), '-prefix', str(root), '-gen', backend,
                    '-target', target, '-asm', 'intel', '-r', str(source), '-o', str(work / 'unit.o')]
                row = dict(version=version, target=target, backend=backend, cases=[])
                row['emit'] = audit.invoke(command, work, work / 'emit.log', 30)
                assert row['emit']['exit'] == 0
                cfile = work / 'unit.c'
                text = cfile.read_text()
                texts[version, target, backend] = text
                row['c_sha256'] = audit.digest(cfile)
                literals = [line for line in text.splitlines() if '.ascii' in line]
                assert len(literals) == 2 and '[gs:16]' in literals[0] and '[fs:20]' in literals[1]
                if version == 'candidate' and backend == 'clang':
                    instructions = '\n'.join(line for line in text.splitlines() if '.ascii' not in line)
                    assert not re.search(r'\[\s*(?:cs|ds|es|fs|gs|ss)\s*:', instructions, re.I)
                    assert 'gs:%0' in instructions and 'fS:%0' in instructions
                records.append(row)
                for optimization in ('O0', 'O2'):
                    flags = (audit.GCC_WARNINGS + ['-fno-diagnostics-show-caret']
                             if backend == 'gcc' else ['-Weverything', '-fno-caret-diagnostics'])
                    command = [tools[backend]['path'], *audit.SEMANTICS, '-' + optimization,
                        *flags, '-fdiagnostics-color=never', '-m' + width,
                        '-masm=intel', '-c', 'unit.c', '-o', 'NUL']
                    check = audit.invoke(command, work, work / (optimization + '.log'), 30)
                    row['cases'].append(dict(optimization=optimization, object=check))
                    audit.write_json(output / 'results.json', dict(records=records, tools=tools,
                        source_sha256=source_pin, executed_programs=0))
                    expected_failure = version == 'baseline' and backend == 'clang'
                    assert (check['exit'] != 0) == expected_failure
                    if expected_failure:
                        assert check['exit'] == 1 and any(sample['severity'] == 'error'
                            and 'unexpected token in argument list' in sample['message']
                            for sample in check['samples'])
    if options.baseline:
        for target in ('win32', 'win64'):
            assert texts['baseline', target, 'gcc'] == texts['candidate', target, 'gcc']
            for backend in ('gcc', 'clang'):
                assert ([line for line in texts['baseline', target, backend].splitlines() if '.ascii' in line]
                        == [line for line in texts['candidate', target, backend].splitlines() if '.ascii' in line])
    assert audit.digest(source) == source_pin
    assert all(audit.digest(Path(tool['path'])) == tool['sha256'] for tool in tools.values())
    print('C_EMITTER_SEGMENTS_PASS emissions=' + str(len(records))
          + ' object_checks=' + str(2 * len(records))
          + ' expected_baseline_rejections=' + str(4 if options.baseline else 0))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-c-emitter-segments.py
