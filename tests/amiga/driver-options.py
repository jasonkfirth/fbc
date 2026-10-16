#!/usr/bin/env python3
"""Project: FreeBASIC compiler driver qualification
File: driver-options.py
Purpose: Verify rejected assembly options fail before producing output.
Responsibilities: Check exit status, diagnostics and accepted emission controls.
This file intentionally does NOT invoke assemblers, linkers or emulators.
"""
import argparse
import json
from pathlib import Path
import subprocess

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    source = args.output / 'program.bas'
    source.write_text('end 0\n')
    cases = [
        ('amiga-att', ['-target', 'amiga-m68k', '-asm', 'att'], False, '.c'),
        ('amiga-intel', ['-target', 'amiga-m68k', '-asm', 'intel'], False, '.c'),
        ('gas-att', ['-target', 'linux-x86', '-gen', 'gas', '-asm', 'att'], False, '.asm'),
        ('gas64-att', ['-target', 'linux-x86_64', '-gen', 'gas64', '-asm', 'att'], False, '.asm'),
        ('amiga-default', ['-target', 'amiga-m68k'], True, '.c'),
        ('gcc-att', ['-target', 'linux-x86_64', '-gen', 'gcc', '-asm', 'att'], True, '.c'),
        ('gas64-intel', ['-target', 'linux-x86_64', '-gen', 'gas64', '-asm', 'intel'], True, '.asm'),
    ]
    records = []
    for name, flags, accepted, suffix in cases:
        output = args.output / (name + suffix)
        output.unlink(missing_ok=True)
        command = [str(args.fbc.resolve()), *flags, '-r', str(source.resolve()), '-o', str(output.resolve())]
        result = subprocess.run(command, cwd=args.output, capture_output=True, text=True, timeout=30)
        text = result.stdout + result.stderr
        (args.output / (name + '.log')).write_text(text)
        if accepted:
            passed = result.returncode == 0 and output.is_file() and output.stat().st_size > 0 and 'error ' not in text
        else:
            passed = result.returncode == 1 and not output.exists() and 'error ' in text
        records.append(dict(name=name, command=command, return_code=result.returncode,
                            emitted=output.exists(), accepted=accepted, passed=passed))
        print(name + ': ' + ('pass' if passed else 'fail'), flush=True)
    (args.output / 'results.json').write_text(json.dumps(records, indent=2) + '\n')
    return int(any(not record['passed'] for record in records))

if __name__ == '__main__':
    raise SystemExit(main())

# end of driver-options.py
