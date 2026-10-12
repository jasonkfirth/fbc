"""Project: FreeBASIC compiler qualification
File: test-preprocessing-locations.py
Purpose: Verify mapped preprocessing preserves runtime diagnostic locations.
Responsibilities: Check includes, continuations, macros and explicit remapping.
This file intentionally does NOT install a compiler or retain generated trees.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    options = parser.parse_args()
    compiler = options.compiler.resolve(strict=True)
    root = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix='fbc-pp-locations-', dir=root / 'build/compiler-lint') as temporary:
        directory = Path(temporary)
        included = directory / 'included.bi'
        included.write_text('''\
'' Project: FreeBASIC preprocessing qualification
'' File: included.bi
'' Purpose: Exercise include locations in runtime checks.
'' Responsibilities: Supply a procedure with a checked runtime call.
'' This file intentionally does NOT define module startup code.
sub IncludedCheck()
    open "missing-input" for input as #1
end sub
'' end of included.bi
''', encoding='utf-8')
        source = directory / 'original.bas'
        source.write_text('''\
'' Project: FreeBASIC preprocessing qualification
'' File: original.bas
'' Purpose: Exercise logical locations during preprocessing.
'' Responsibilities: Cover includes, macro calls, continuations and #line.
'' This file intentionally does NOT execute failing runtime calls.
#include "included.bi"
#macro CheckedOpen()
    open "macro-input" for input as #2
#endmacro
sub OriginalCheck()
    CheckedOpen()
    open "continued-input" _
        for input as #3
#line 120 "logical-source.bas"
    open "remapped-input" for input as #4
end sub
'' end of original.bas
''', encoding='utf-8')
        expanded = directory / 'expanded.bas'
        common = [str(compiler), '-prefix', str(root), '-i', str(root / 'inc'), '-e']

        def compile(arguments: list[str]) -> str:
            result = subprocess.run(common + arguments, cwd=root, capture_output=True,
                                    text=True, timeout=30)
            assert result.returncode == 0, result.stdout + result.stderr
            return result.stdout

        original_c = directory / 'original.c'
        expanded_c = directory / 'expanded.c'
        compile(['-gen', 'gcc', '-r', '-o', str(original_c), str(source)])
        compile(['-pp-lines', '-o', str(expanded), str(source)])
        compile(['-gen', 'gcc', '-r', '-o', str(expanded_c), str(expanded)])
        pattern = r'fb_ErrorThrowAt\( \d+[^;\n]+'
        original_checks = re.findall(pattern, original_c.read_text())
        replay_checks = re.findall(pattern, expanded_c.read_text())
        assert len(original_checks) == 4, original_checks
        assert original_checks == replay_checks, (original_checks, replay_checks, expanded.read_text())
        plain = directory / 'plain.bas'
        compile(['-pp', '-o', str(plain), str(source)])
        assert '#line' not in plain.read_text(), 'Plain preprocessing changed its output contract'
        print('PREPROCESSING_LOCATIONS_PASS checks=4')


if __name__ == '__main__':
    main()

# end of test-preprocessing-locations.py
