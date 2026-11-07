"""Compile the compiler's C-backend modules for several pointer widths.

Project: FreeBASIC compiler regression tests
File: c-compiler-pointer-keys.py
Purpose: Catch pointer-to-integer conversions invalid on 32-bit hosts.
Responsibilities: Compile cross-target C objects with aggressive diagnostics.
This file intentionally does NOT execute those objects on their target hosts.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def run(command, cwd, environment):
    result = subprocess.run([str(part) for part in command], cwd=cwd, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    output = result.stdout + result.stderr
    if result.returncode:
        raise RuntimeError('command failed: ' + ' '.join(str(part) for part in command) + '\n' + output[:4000])
    return len(re.findall(r'(?m)^.*\bwarning:', output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--prototype-dir', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    compiler, prefix, clang = Path(args.fbc).resolve(), Path(args.prefix).resolve(), Path(args.clang).resolve()
    prototype = args.prototype_dir.resolve() if args.prototype_dir else None
    environment = os.environ.copy()
    environment['PATH'] = str(clang.parent) + os.pathsep + environment.get('PATH', '')
    args.work_dir.mkdir(parents=True, exist_ok=True)
    targets = [('win32', 'i686-w64-windows-gnu'), ('linux', 'x86_64-unknown-linux-gnu'),
               ('linux-arm', 'armv7-unknown-linux-gnueabihf'),
               ('linux-aarch64', 'aarch64-unknown-linux-gnu')]
    sources = [('ast-node-addr', root / 'src/compiler/ast/nodes/ast-node-addr.bas', 'ast-node-addr.bas'),
               ('ast-node-proc', root / 'src/compiler/ast/nodes/ast-node-proc.bas', 'ast-node-proc-widened.bas'),
               ('ir-hlc', root / 'src/compiler/backend/c/ir-hlc.bas', 'ir-hlc.bas')]
    warnings = 0
    with tempfile.TemporaryDirectory(prefix='c-pointer-keys-', dir=args.work_dir) as directory:
        work = Path(directory)
        for target, triple in targets:
            for name, canonical, staged in sources:
                source = prototype / staged if prototype else canonical
                c_file = work / (target + '-' + name + '.c')
                includes = ['-i', prototype] if prototype else []
                run([compiler, '-prefix', prefix, '-target', target, '-gen', 'gcc', '-e',
                     '-m', 'fbc', '-w', 'pedantic', *includes, '-i', root / 'src/compiler',
                     '-i', root / 'inc', '-r', source, '-o', c_file], work, environment)
                warnings += run([clang, '--target=' + triple, '-std=gnu11', '-nostdinc', '-O2',
                                 '-Weverything', '-ferror-limit=0', '-c', c_file,
                                 '-o', work / (target + '-' + name + '.o')], work, environment)
    print('C_COMPILER_POINTER_KEYS_PASS targets=' + str(len(targets)) +
          ' modules=' + str(len(sources)) + ' clang_warnings=' + str(warnings))


if __name__ == '__main__':
    main()

# end of c-compiler-pointer-keys.py
