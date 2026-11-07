"""Project: FreeBASIC C backend tests
File: c-startup-groups.py
Purpose: Compare real compiler startup registrations under GCC and Clang.
Responsibilities: Linked-unit order, priorities, callback symbols and Win32 objects.
This file intentionally does NOT execute cross-target objects or install tools.
"""
from collections import Counter
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sqlite3
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'baseline', 'gcc', 'clang', 'diagnostic-profile', 'work-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(exist_ok=True)
    tools = {'gcc': args.gcc.resolve(), 'clang': args.clang.resolve()}
    nm = tools['gcc'].with_name('nm.exe' if os.name == 'nt' else 'nm')
    objdump = nm.with_name('objdump.exe' if os.name == 'nt' else 'objdump')
    fixture = ROOT / 'tests/compiler-support/c-startup-groups.bas'
    with sqlite3.connect(args.diagnostic_profile) as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    files = [Path(__file__), fixture, args.fbc.resolve(), args.baseline.resolve(), *tools.values(), nm, objdump]
    pins = {str(p): digest(p) for p in files}
    env = dict(os.environ)
    env['PATH'] = str(tools['gcc'].parent) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = profile['source_date_epoch']
    records, symbols, sections, outputs = [], {}, {}, {}

    def run(tag, command):
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 16 * 1024 * 1024
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, capture_output=True,
                                text=True, errors='replace', timeout=180,
                                creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name == 'nt' else 0)
        output = result.stdout + result.stderr
        (work / (tag + '.log.zlib')).write_bytes(zlib.compress(output.encode()))
        records.append({'tag': tag, 'command': [str(x) for x in command], 'exit': result.returncode,
                        'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(
                            line for line in output.splitlines() if 'warning:' in line)))),
                        'stdout': result.stdout})
        (work / 'progress.json').write_text(json.dumps(records, indent=2) + '\n')
        assert result.returncode == 0, tag + '\n' + output[-4000:]
        return result.stdout

    driver = work / 'driver.c'
    driver.write_text('''/* Project: FreeBASIC startup tests
File: driver.c
Purpose: Observe real module callbacks without a BASIC main module.
Responsibilities: Print callback values and the main-entry marker.
This file intentionally does NOT register startup callbacks itself. */
#include <stdio.h>
void startup_mark(int value);
void startup_mark(int value) { (void)printf("%d,", value); }
int main(void) { (void)printf("MAIN,"); return 0; }
/* end of driver.c */
''')
    cpp, obj, exe = work / 'unit.c', work / 'unit.o', work / 'trace.exe'
    for cc, tool in tools.items():
        assert not any(f == '-w' or f.startswith('-Wno-') for f in profile[cc + '_flags'])
        for bits in (32, 64):
            target = 'win32-x86' if bits == 32 else 'win32-x86_64'
            cross = ['-m32'] if cc == 'gcc' else ['--target=i686-w64-mingw32']
            if bits == 64:
                cross = []
            for optimization in ('-O0', '-O2'):
                for label, compiler in (('before', args.baseline.resolve()), ('grouped', args.fbc.resolve())):
                    objects = []
                    for unit in ('first', 'second'):
                        tag = '-'.join((cc, str(bits), optimization[1:], label, unit))
                        command = [compiler, '-gen', cc, '-target', target, '-m', 'startup_driver',
                                   '-r', fixture, '-o', cpp]
                        if unit == 'second':
                            command += ['-d', 'SECOND_STARTUP_UNIT']
                        run(tag + '-emit', command)
                        flags = [f for f in profile[cc + '_flags'] if not re.fullmatch('-O[0-9sgz]', f)]
                        selected_obj = work / (unit + '.o')
                        run(tag + '-object', [tool, *flags, optimization, *cross, '-c', cpp, '-o', selected_obj])
                        names = run(tag + '-symbols', [nm, '--defined-only', selected_obj])
                        callbacks = {line.split()[-1] for line in names.splitlines()
                                     if re.search('EARLYFIRST|EARLYSECOND|DEFAULTFIRST|DEFAULTSECOND|PRIORITYCLEANUP|DEFAULTCLEANUP', line)}
                        assert len(callbacks) == 8, (tag, callbacks)
                        symbols[cc, bits, optimization, label, unit] = callbacks
                        headers = run(tag + '-sections', [objdump, '-h', selected_obj])
                        entries = {name: int(size, 16) for name, size in
                                   re.findall(r'^\s*\d+\s+(\.(?:ctors|dtors)(?:\.\d+)?)\s+([0-9a-fA-F]+)', headers, re.M)}
                        assert len(entries) == 4, (tag, entries)
                        assert sum(entries.values()) == (8 if label == 'before' else 4) * (bits // 8), (tag, entries)
                        sections[tag] = entries
                        objects.append(selected_obj)
                    if bits == 64:
                        for order in ('forward', 'reverse'):
                            chosen = objects if order == 'forward' else objects[::-1]
                            tag = '-'.join((cc, str(bits), optimization[1:], label, order))
                            run(tag + '-link', [tool, *[f for f in flags if f != '-nostdinc'], optimization,
                                               driver, *chosen, '-o', exe])
                            output = run(tag + '-execute', [exe])
                            assert output.count('MAIN,') == 1 and len(output.split(',')) == 18, (tag, output)
                            outputs[cc, optimization, label, order] = output
    for key, names in symbols.items():
        cc, bits, opt, label, unit = key
        assert names == symbols[cc, bits, opt, 'before', unit]
    for key, output in outputs.items():
        cc, opt, label, order = key
        assert output == outputs[cc, opt, 'before', order], (key, output, outputs[cc, opt, 'before', order])
    assert all(digest(Path(p)) == sha for p, sha in pins.items())
    (work / 'receipt.json').write_text(json.dumps({'checks_complete': True, 'source_tool_sha256': pins,
        'native_runs': len(outputs), 'object_checks': len(symbols), 'callbacks_retained_per_object': 8,
        'registration_entries_before': 8, 'registration_entries_after': 4, 'sections': sections,
        'checks': records, 'cross_target_execution_claimed': False, 'overall_goal_complete': False}, indent=2) + '\n')
    for path in (cpp, obj, exe, driver, work / 'first.o', work / 'second.o'):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    print('STARTUP_ORDER_PASS native_runs=' + str(len(outputs)) + ' objects=' + str(len(symbols)), flush=True)


if __name__ == '__main__':
    main()

# end of c-startup-groups.py
