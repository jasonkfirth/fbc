"""Project: FreeBASIC compiler verification
File: test-c-emitter-audit.py
Purpose: Check isolation, failure receipts and timeout behavior of the C audit.
Responsibilities: Exercise real emission without changing source directories.
This file intentionally does NOT assert that generated C is warning-free.
"""

import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    parser.add_argument('--work-dir', type=Path, default=root / 'build')
    args = parser.parse_args()
    audit_path = root / 'build_scripts/audit-c-emitter.py'
    spec = importlib.util.spec_from_file_location('audit_c', audit_path)
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    checks = 0
    args.work_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=args.work_dir, prefix='c-audit-test-') as name:
        work = Path(name)
        source = work / 'source'
        source.mkdir()
        good = source / 'good.bas'
        bad = source / 'bad.bas'
        good.write_text("/' Project: C audit fixture. File: good.bas. Purpose: Test emission isolation.\n"
                        "Responsibilities: Use a public header. Does NOT execute external code. '/\n"
                        '#lang "fb"\n#include once "fbgfx.bi"\nprint 1\n'
                        "' end of good.bas\n")
        bad.write_text("' Project: C audit fixture. File: bad.bas. Purpose: Reject invalid syntax.\n"
                       "' Responsibilities: Exercise failure receipts. Does NOT compile successfully.\n"
                       '#lang "fb"\ndim as integer =\n'
                       "' end of bad.bas\n")
        pins = {p.name: p.read_bytes() for p in source.iterdir()}
        plan = work / 'plan.json'
        plan.write_text(json.dumps({'units': [dict(id=p.stem, source=str(p), cwd=str(source),
                        group='test', project='audit', flags=[]) for p in (good, bad)]}))
        output = work / 'results'
        command = [sys.executable, str(audit_path), '--root', str(root), '--plan', str(plan),
                   '--artifact-root', str(work), '--output', str(output), '--fbc', str(args.fbc.resolve()),
                   '--gcc', str(args.gcc.resolve()), '--clang', str(args.clang.resolve()),
                   '--jobs', '1', '--timeout', '20']
        subprocess.run(command, check=True, timeout=100)
        success = json.loads((output / 'good/result.json').read_text())
        failure = json.loads((output / 'bad/result.json').read_text())
        assert success['status'] == 'emitted'
        assert success['gcc']['exit'] == 0 and success['clang']['exit'] == 0
        assert (output / 'good/unit.c').is_file()
        checks += 1
        assert '-Weverything' in success['clang']['command']
        assert '-Wwrite-strings' in success['clang']['command']
        assert '-Wconversion' in success['gcc']['command']
        assert not any(arg == '-w' or arg.startswith('-Wno-') for arg in success['gcc']['command'])
        checks += 1
        literal = work / 'literal.c'
        literal.write_text('/* Project: FreeBASIC C audit. File: literal.c.\n'
                           '   Purpose: Exercise literal-const diagnostics.\n'
                           '   Responsibilities: Supply one const-discarding conversion.\n'
                           '   This file intentionally does NOT execute code. */\n'
                           'static char *literal_view = (char *)"literal";\n'
                           'int literal_probe(void);\n'
                           'int literal_probe(void) { return literal_view[0]; }\n'
                           '/* end of literal.c */\n')
        literal_command = success['clang']['command'].copy()
        literal_command[literal_command.index('-S') + 1] = str(literal)
        literal_probe = audit.invoke(literal_command, work, work / 'literal.log', 20)
        assert literal_probe['exit'] == 0
        assert any(row['flag'] == '-Wcast-qual' and row['count'] > 0 for row in literal_probe['counts'])
        checks += 1
        mock_root = work / 'mock-root'
        forbidden = mock_root / 'src' / 'results'
        refused = subprocess.run([*command, '--root', str(mock_root),
                                  '--artifact-root', str(mock_root / 'src'), '--output', str(forbidden)],
                                 capture_output=True, text=True, timeout=20)
        assert refused.returncode != 0 and 'must stay below build/' in refused.stderr
        assert not forbidden.exists()
        checks += 1
        assert failure['status'] == 'emission-failed' and failure['emit']['exit'] != 0
        assert 'gcc' not in failure and not (output / 'bad/unit.c').exists()
        checks += 1
        assert {p.name: p.read_bytes() for p in source.iterdir()} == pins
        checks += 1
        receipt_pins = {p: p.stat().st_mtime_ns for p in output.glob('*/result.json')}
        subprocess.run(command, check=True, timeout=30)
        assert all(p.stat().st_mtime_ns == stamp for p, stamp in receipt_pins.items())
        checks += 1
        good.write_text(good.read_text() + "' changed input\n")
        changed = subprocess.run(command, capture_output=True, text=True, timeout=30)
        assert changed.returncode != 0 and 'Inputs changed' in changed.stderr
        checks += 1
        timed = audit.invoke([sys.executable, '-c', 'import time; time.sleep(10)'],
                             work, work / 'timeout.log', 0.1)
        assert timed['exit'] == 124 and Path(timed['log']).is_file()
        checks += 1
        assert not (output / '.running').exists()
        checks += 1
        # A project can override an installed header with the same basename.
        # The audit must preserve that owning build's explicit include order.
        priority = work / 'header-priority'
        compiler_root = priority / 'compiler-root'
        compiler_inc = compiler_root / 'inc'
        project_inc = priority / 'project-headers'
        priority_output = priority / 'results'
        for folder in (compiler_inc, project_inc, priority_output):
            folder.mkdir(parents=True)
        fixture = priority / 'include-priority.bas'
        fixture.write_text(
            "' Project: C audit fixture. File: include-priority.bas.\n"
            "' Purpose: Verify project header precedence. Responsibilities: Check a marker.\n"
            "' This file intentionally does NOT run application code.\n"
            '#lang "fb"\n#include once "include_priority_probe.bi"\n'
            '#if PROJECT_MARKER <> 2\n#error WRONG_INCLUDE_PRIORITY\n#endif\n'
            'dim shared include_probe as long = PROJECT_MARKER\n'
            "' end of include-priority.bas\n")
        for folder, marker in ((compiler_inc, 1), (project_inc, 2)):
            (folder / 'include_priority_probe.bi').write_text(
                "' Project: C audit fixture. File: include_priority_probe.bi.\n"
                "' Purpose: Distinguish include roots. Responsibilities: Define a marker.\n"
                "' This file intentionally does NOT declare runtime interfaces.\n"
                '#define PROJECT_MARKER ' + str(marker) + '\n'
                "' end of include_priority_probe.bi\n")
        fixture_pins = {path: path.read_bytes() for path in
                        (fixture, compiler_inc / 'include_priority_probe.bi',
                         project_inc / 'include_priority_probe.bi')}
        priority_options = argparse.Namespace(root=compiler_root, output=priority_output,
            fbc=args.fbc.resolve(), gcc=args.gcc.resolve(), clang=args.clang.resolve(),
            timeout=20, structural=False)
        priority_row = audit.run_unit(dict(id='priority', source=str(fixture),
            cwd=str(priority), group='test', project='audit',
            flags=['-i', str(project_inc)]), priority_options, 'header-priority-test')
        assert priority_row['status'] == 'emitted'
        assert priority_row['gcc']['exit'] == priority_row['clang']['exit'] == 0
        assert all(path.read_bytes() == data for path, data in fixture_pins.items())
        checks += 1
    print('C_EMITTER_AUDIT_TESTS_PASS checks=' + str(checks))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-c-emitter-audit.py
