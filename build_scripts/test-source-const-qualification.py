#!/usr/bin/env python3
"""Project: FreeBASIC source diagnostic regression tests
File: test-source-const-qualification.py
Purpose: Diagnose unsafe pointer qualification independently of the backend.
Responsibilities: Check selected operations, source locations and warning policy.
This file intentionally does NOT execute the compile-only fault examples.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--work-dir', type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    source = root / 'tests/compiler-support/source-const-qualification.bas'
    locations = {
        1: 'BorrowSlot(slot)',
        2: 'dim qualified as const ConstPayload ptr ptr = nested',
        3: 'qualified = nested',
        4: 'BorrowNested(nested)',
        5: 'BorrowArray(slots())',
        6: 'BorrowDeep(deep)',
        14: 'dim byref qualified as const ConstPayload ptr = slot',
        16: 'BorrowSlot(FetchAlias())',
    }
    lines = source.read_text().splitlines()
    locations = {case: lines.index(text) + 1 for case, text in locations.items()}
    warning = re.compile(r'^(.+?)\((\d+)\)(?: error)? warning 58\((\d+)\): (.+)$', re.M)
    checks = identities = 0
    with tempfile.TemporaryDirectory(prefix='const-qualification-', dir=args.work_dir) as temporary:
        for bits in (32, 64):
            for backend in ('gcc', 'clang', 'gas' if bits == 32 else 'gas64'):
                output = Path(temporary) / ('unit.c' if backend in ('gcc', 'clang') else 'unit.asm')
                for case in range(1, 18):
                    reference = None
                    for policy in ('default', 'all', 'none', 'error'):
                        command = [str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
                                   '-gen', backend, '-target', 'linux', '-arch',
                                   '686' if bits == 32 else 'x86_64', '-asm', 'intel',
                                   '-r', '-m', source.stem, '-d', f'QUALIFICATION_CASE={case}',
                                   str(source), '-o', str(output)]
                        if policy != 'default':
                            command += ['-w', policy]
                        output.unlink(missing_ok=True)
                        result = subprocess.run(command, cwd=root, text=True,
                                                capture_output=True, timeout=180)
                        text = (result.stdout + result.stderr).replace('\r', '')
                        found = list(warning.finditer(text))
                        wanted = int(case in locations and policy != 'none')
                        assert len(found) == wanted, (command, text)
                        assert result.returncode == int(policy == 'error' and wanted), (command, text)
                        for match in found:
                            assert int(match[2]) == locations[case] and int(match[3]) == 1, text
                            assert match[1].replace('\\', '/').casefold().endswith('/' + source.name.casefold()), text
                            assert 'Unsafe nested CONST qualification' in match[4], text
                        if policy == 'none':
                            assert not re.search(r'warning [0-9]+\(', text), text
                        if policy != 'error':
                            digest = hashlib.sha256(output.read_bytes()).hexdigest()
                            if reference is None:
                                reference = digest
                            else:
                                assert digest == reference, (command, 'Warning policy changed emitted output')
                                identities += 1
                        checks += 1
    print(f'SOURCE_CONST_QUALIFICATION_PASS cases={checks} emitted_identities={identities}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-source-const-qualification.py
