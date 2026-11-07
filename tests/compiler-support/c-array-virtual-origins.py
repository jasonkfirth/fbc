"""Project: FreeBASIC C backend regression tests
File: c-array-virtual-origins.py
Purpose: Check descriptor DATA arithmetic through valid extreme-bound arrays.
Responsibilities: Run ordinary and instrumented programs and inspect foreign objects.
This file intentionally does NOT qualify the runtime's allocation-size limits.
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
SOURCE = Path(__file__).with_suffix('.bas')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'baseline', 'prefix', 'gcc', 'clang', 'work-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(exist_ok=True)
    compiler, baseline = args.fbc.resolve(), args.baseline.resolve()
    tools = {'gcc': args.gcc.resolve(), 'clang': args.clang.resolve()}
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    files = [SOURCE, Path(__file__), compiler, baseline, *tools.values(),
        runtime / 'fbrt0.o', runtime / 'libfb.a']
    files += list((ROOT / 'inc').rglob('*.bi'))
    pins = {str(path): digest(path) for path in files}
    with sqlite3.connect(ROOT / 'build/c-emitter-active-20261101/builtin-current-compiler-audit/results.sqlite3') as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    env = os.environ.copy()
    env['PATH'] = str(tools['gcc'].parent) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = profile['source_date_epoch']
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('CREATE TABLE IF NOT EXISTS checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')
    inputs = {'source_tool_runtime_sha256': pins, 'gcc_flags': profile['gcc_flags'],
        'clang_flags': profile['clang_flags'], 'source_date_epoch': env['SOURCE_DATE_EPOCH']}
    saved = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    assert saved is None or json.loads(saved[0]) == inputs
    db.execute('INSERT OR IGNORE INTO metadata VALUES(?,?)', ('inputs', json.dumps(inputs)))
    db.commit()

    def run(name, command, expected=0):
        assert sum(path.stat().st_size for path in work.rglob('*') if path.is_file()) < 16 * 1024 * 1024
        p = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, capture_output=True,
            text=True, errors='replace', timeout=180)
        output = p.stdout + p.stderr
        assert len(output) < 8 * 1024 * 1024
        row = {'exit': p.returncode, 'expected_exit': expected, 'command': [str(x) for x in command],
            'stdout': p.stdout, 'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]',
                '\n'.join(line for line in output.splitlines() if 'warning:' in line))))}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)', (name, json.dumps(row), zlib.compress(output.encode())))
        db.commit()
        if expected == 'pointer-overflow-trap':
            assert p.returncode in (3221225501, -4), name + '\n' + output[-3000:]
        else:
            assert p.returncode == expected, name + '\n' + output[-3000:]
        return row

    cfile, obj, exe = [work / name for name in ('unit.c', 'unit.o', 'unit.exe')]
    base = ['-prefix', args.prefix.resolve(), '-i', ROOT / 'inc', '-m', SOURCE.stem]
    native = objects = instrumented = foreign = controls = 0
    for backend in ('gcc', 'clang'):
        for negative in (False, True):
            bounds = 'negative' if negative else 'positive'
            for checked in (False, True):
                mode = 'checked' if checked else 'unchecked'
                tag = backend + '-' + bounds + '-' + mode
                for width in (64, 32):
                    run(tag + '-' + str(width) + '-emit', [compiler, *base, '-gen', backend,
                        '-target', 'win' + str(width), *(['-d', 'NEGATIVE_BOUNDS'] if negative else []),
                        *(['-exx'] if checked else []), '-r', SOURCE, '-o', cfile])
                    for cc, tool in tools.items():
                        cases = [('gnu11', '0'), ('gnu11', '2'), ('gnu23', '0'), ('gnu23', '2')] if width == 64 else [('gnu23', '2')]
                        for standard, opt in cases:
                            flags = [*profile[cc + '_flags'], '-m' + str(width), '-std=' + standard, '-O' + opt]
                            assert not any(x == '-w' or x.startswith('-Wno-') for x in flags)
                            for sanitized in ((False, True) if cc == 'clang' and width == 64 else (False,)):
                                name = tag + '-' + str(width) + '-' + cc + '-' + standard + '-O' + opt
                                if sanitized:
                                    name += '-pointer-check'
                                extra = ['-fsanitize=pointer-overflow', '-fsanitize-trap=pointer-overflow'] if sanitized else []
                                run(name + '-object', [tool, *flags, *extra, '-c', cfile, '-o', obj])
                                objects += 1
                                if width == 32:
                                    continue
                                run(name + '-link', [tool, '-o', exe, runtime / 'fbrt0.o',
                                    obj, runtime / 'libfb.a', '-lm', '-mthreads'])
                                row = run(name + '-execute', [exe])
                                assert row['stdout'].strip() == 'ARRAY_ORIGIN=PASS'
                                native += 1
                                instrumented += int(sanitized)
        print('ARRAY_ORIGIN', backend, flush=True)
    # Both old C producers accept the valid BASIC source and trip only in the
    # instrumented native execution. Compile and link success are separate gates.
    for backend in ('gcc', 'clang'):
        name = 'historical-' + backend
        run(name + '-emit', [baseline, *base, '-gen', backend, '-r', SOURCE, '-o', cfile])
        run(name + '-object', [tools['clang'], *profile['clang_flags'], '-std=gnu23', '-O2',
            '-fsanitize=pointer-overflow', '-fsanitize-trap=pointer-overflow', '-c', cfile, '-o', obj])
        run(name + '-link', [tools['clang'], '-o', exe, runtime / 'fbrt0.o', obj,
            runtime / 'libfb.a', '-lm', '-mthreads'])
        run(name + '-execute', [exe], 'pointer-overflow-trap')
        controls += 1
    for target, arch, triple in [('linux', 'x86_64', 'x86_64-unknown-linux-gnu'),
            ('linux', 'armv7a', 'armv7-unknown-linux-gnueabihf'),
            ('linux', 'aarch64', 'aarch64-unknown-linux-gnu'),
            ('darwin', 'aarch64', 'aarch64-apple-darwin')]:
        for backend in ('gcc', 'clang'):
            for negative in (False, True):
                name = backend + '-' + triple + ('-negative' if negative else '-positive')
                run(name + '-emit', [compiler, *base, '-gen', backend, '-target', target, '-arch', arch,
                    *(['-d', 'NEGATIVE_BOUNDS'] if negative else []), '-exx', '-r', SOURCE, '-o', cfile])
                run(name + '-object', [tools['clang'], '--target=' + triple, '-std=gnu23', '-nostdinc',
                    '-O2', '-Weverything', '-fno-strict-aliasing', '-fwrapv', '-c', cfile, '-o', obj])
                foreign += 1
    assert all(digest(Path(path)) == sha for path, sha in pins.items())
    (work / 'receipt.json').write_text(json.dumps({'checks_complete': True, 'native_cases': native,
        'instrumented_native_cases': instrumented, 'C_object_checks': objects,
        'foreign_objects': foreign, 'historical_controls': controls, 'overall_goal_complete': False}, indent=2) + '\n')
    for path in (cfile, obj, exe):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    db.close()
    print('ARRAY_ORIGIN_CONTRACT_PASS', native, instrumented, objects, foreign, controls, flush=True)


if __name__ == '__main__':
    main()

# end of c-array-virtual-origins.py
