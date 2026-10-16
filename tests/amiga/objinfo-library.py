#!/usr/bin/env python3
"""Project: FreeBASIC Amiga object metadata qualification
File: objinfo-library.py
Purpose: Build programs whose dependencies are carried by Hunk metadata.
Responsibilities: Exercise archive and direct-object dependency discovery.
This file intentionally does NOT execute target programs or edit corpus inputs.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    sources = {
        'dependency.bas': 'function DependencyValue() as long\nreturn 123\nend function\n',
        'parent.bas': '#inclib "dependency"\ndeclare function DependencyValue() as long\nfunction ParentValue() as long\nreturn DependencyValue()+1\nend function\n',
        'main.bas': '#inclib "parent"\ndeclare function ParentValue() as long\nif ParentValue() <> 124 then end 1\nprint "NATIVE_OBJINFO_OK"\n',
        'direct.bas': 'declare function ParentValue() as long\nif ParentValue() <> 124 then end 1\nprint "NATIVE_OBJECT_INFO_OK"\n',
    }
    for name, text in sources.items():
        (args.output / name).write_text(text)
    common = [str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
              '-target', 'amiga-m68k', '-p', str(args.output.resolve())]
    cases = [
        ('dependency', ['-lib', 'dependency.bas', '-x', 'libdependency.a'], 'libdependency.a'),
        ('parent', ['-lib', 'parent.bas', '-x', 'libparent.a'], 'libparent.a'),
        ('program', ['main.bas', '-x', 'program'], 'program'),
        ('object', ['-c', '-m', 'direct', 'parent.bas', '-o', 'parent.o'], 'parent.o'),
        ('direct-program', ['direct.bas', 'parent.o', '-x', 'direct-program'], 'direct-program'),
    ]
    records = []
    for name, flags, artifact in cases:
        output = args.output / artifact
        output.unlink(missing_ok=True)
        command = common + flags
        result = subprocess.run(command, cwd=args.output, capture_output=True, text=True, timeout=90)
        (args.output / (name + '.log')).write_text(result.stdout + result.stderr)
        passed = result.returncode == 0 and output.is_file() and output.stat().st_size > 0
        records.append(dict(name=name, command=command, return_code=result.returncode, passed=passed))
        print(name + ': ' + ('pass' if passed else 'fail'), flush=True)
        (args.output / 'results.json').write_text(json.dumps(records, indent=2) + '\n')
        if not passed:
            return 1
    return 0

if __name__ == '__main__':
    raise SystemExit(main())

# end of objinfo-library.py
