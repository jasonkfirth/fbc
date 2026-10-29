"""Project: FreeBASIC compiler AST regression tests.

File: ast-effective-value.py
Purpose: Exercise current production effective-value queries on real AST layouts.
Responsibilities: Assemble the two exact functions, run the fixture and clean up.
This file intentionally does NOT initialize or rebuild the complete compiler.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--work-dir', required=True, type=Path)
    parser.add_argument('--backend', action='append')
    args = parser.parse_args()
    support = Path(__file__).resolve().parent
    compiler = support.parents[1] / 'src/compiler'
    production = (compiler / 'ast/ast.bas').read_text()
    functions = []
    for name in ('astGetEffectiveNode', 'astGetEffectiveClass'):
        match = re.search(r'^function ' + name + r'\(.*?^end function',
                          production, re.MULTILINE | re.DOTALL)
        if match is None:
            raise RuntimeError('production function not found: ' + name)
        functions.append(match.group())
    parent = args.work_dir.resolve(strict=True)
    work = Path(tempfile.mkdtemp(prefix='ast-effective-value-', dir=parent)).resolve()
    if not work.is_relative_to(parent) or work == parent:
        raise RuntimeError('test output escaped the selected work directory')
    backends = args.backend or ['gcc', 'clang']
    try:
        source = work / 'ast-effective-value.bas'
        text = (support / 'ast-effective-value.bas').read_text()
        source.write_text(text.replace("' @PRODUCTION_FUNCTIONS@",
                                       '\n\n'.join(functions)).rstrip() + '\n')
        for backend in backends:
            executable = work / (backend + ('.exe' if os.name == 'nt' else ''))
            subprocess.run([args.fbc, '-prefix', args.prefix, '-i', str(compiler),
                            '-gen', backend, '-O', '2', str(source), '-x', str(executable)],
                           check=True, timeout=120)
            subprocess.run([str(executable)], check=True, timeout=120)
        print('AST_EFFECTIVE_VALUE_PASS checks=' + str(13 * len(backends)))
    finally:
        # The unique child was resolved inside the explicit parent before use.
        shutil.rmtree(work)


if __name__ == '__main__':
    main()

# end of ast-effective-value.py
