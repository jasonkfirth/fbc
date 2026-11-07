"""Project: FreeBASIC C backend regression tests
File: c-procedure-deep-reachability.py
Purpose: Check deep procedure analysis and actual debug-storage retention.
Responsibilities: Run native constructors, inspect symbols and check historical controls.
This file intentionally does NOT install a compiler or execute foreign objects.
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

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'baseline', 'walk-baseline', 'prefix', 'gcc', 'clang', 'work-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(exist_ok=True)
    source = HERE / 'c-procedure-deep-reachability.bas'
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    nm = args.gcc.resolve().with_name('nm.exe' if os.name == 'nt' else 'nm')
    files = [source, Path(__file__), args.fbc.resolve(), args.baseline.resolve(),
             args.walk_baseline.resolve(), args.gcc.resolve(), args.clang.resolve(), nm,
             runtime / 'fbrt0.o', runtime / 'libfb.a']
    pins = {str(p): digest(p) for p in files}
    with sqlite3.connect(ROOT / 'build/c-emitter-active-20261101/builtin-current-compiler-audit/results.sqlite3') as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    env = os.environ.copy()
    env['PATH'] = str(args.gcc.resolve().parent) + os.pathsep + env.get('PATH', '')
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

    def run(name, command):
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 16 * 1024 * 1024
        p = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, capture_output=True,
                           text=True, errors='replace', timeout=180)
        log = p.stdout + p.stderr
        assert len(log) < 8 * 1024 * 1024
        row = {'exit': p.returncode, 'command': [str(x) for x in command], 'stdout': p.stdout,
            'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(s for s in log.splitlines() if 'warning:' in s))))}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)', (name, json.dumps(row), zlib.compress(log.encode())))
        db.commit()
        assert p.returncode == 0, name + '\n' + log[-4000:]
        return row

    base = ['-prefix', args.prefix.resolve(), '-i', ROOT / 'inc', '-m', source.stem]
    cfile, obj, exe = [work / name for name in ('unit.c', 'unit.o', 'unit.exe')]
    native = objects = controls = foreign = 0
    for backend in ('gcc', 'clang'):
        for width in (64, 32):
            for debug in (False, True):
                mode = 'debug' if debug else 'release'
                run(f'{backend}-{width}-{mode}-emit', [args.fbc.resolve(), *base, '-gen', backend,
                    '-target', 'win' + str(width), *(['-g'] if debug else []), '-r', source, '-o', cfile])
                code = cfile.read_text()
                assert 'UNUSED_DEEP_LEAF' not in code.upper()
                assert ('PRIVATE_DEEP_DATA' in code.upper()) == debug
                if debug:
                    retained = [s for s in code.splitlines() if 'PRIVATE_DEEP_DATA' in s.upper() and '__attribute__((used))' in s]
                    assert len(retained) == 1
                for cc, tool in [('gcc', args.gcc.resolve()), ('clang', args.clang.resolve())]:
                    flags = profile[cc + '_flags']
                    assert not any(x == '-w' or x.startswith('-Wno-') for x in flags)
                    for standard, opt in ([('gnu11', '0'), ('gnu11', '2'), ('gnu23', '0'), ('gnu23', '2')] if width == 64 else [('gnu23', '2')]):
                        name = f'{backend}-{width}-{mode}-{cc}-{standard}-O{opt}'
                        selected = [*flags, '-std=' + standard, '-O' + opt, '-m' + str(width), *(['-g'] if debug else [])]
                        row = run(name + '-object', [tool, *selected, '-c', cfile, '-o', obj])
                        assert '-Wunused-function' not in row['warnings'] and '-Wunused-variable' not in row['warnings']
                        symbols = run(name + '-symbols', [nm, obj])['stdout'].upper()
                        assert ('PRIVATE_DEEP_DATA' in symbols) == debug
                        objects += 1
                        if width == 64:
                            run(name + '-link', [tool, '-o', exe, runtime / 'fbrt0.o', obj,
                                '-L' + str(runtime), '-lfb', '-lm', '-mthreads'])
                            row = run(name + '-execute', [exe])
                            assert row['stdout'].strip() == 'DEEP_REACHABILITY=PASS'
                            native += 1
                        if debug and width == 64 and standard == 'gnu23' and opt == '2':
                            control = work / 'without-retention.c'
                            old = retained[0]
                            replacement = old.replace(' __attribute__((used))', '')
                            assert old != replacement and code.count(old) == 1
                            control.write_text(code.replace(old, replacement))
                            run(name + '-without-retention-object', [tool, *selected, '-c', control, '-o', obj])
                            assert 'PRIVATE_DEEP_DATA' not in run(name + '-without-retention-symbols', [nm, obj])['stdout'].upper()
                            controls += 1
        print('DEEP_PROCEDURE', backend, flush=True)
    for label, compiler in [('original', args.baseline.resolve()), ('walk-only', args.walk_baseline.resolve())]:
        for backend in ('gcc', 'clang'):
            run(label + '-' + backend + '-emit', [compiler, *base, '-gen', backend, '-g', '-r', source, '-o', cfile])
            code = cfile.read_text().upper()
            assert ('UNUSED_DEEP_LEAF' in code) == (label == 'original')
            assert 'PRIVATE_DEEP_DATA' in code
            for cc, tool in [('gcc', args.gcc.resolve()), ('clang', args.clang.resolve())]:
                name = label + '-' + backend + '-' + cc
                run(name + '-object', [tool, *profile[cc + '_flags'], '-g', '-c', cfile, '-o', obj])
                assert 'PRIVATE_DEEP_DATA' not in run(name + '-symbols', [nm, obj])['stdout'].upper()
                controls += 1
    for target, arch, triple in [('linux', 'x86_64', 'x86_64-unknown-linux-gnu'),
            ('linux', 'armv7a', 'armv7-unknown-linux-gnueabihf'), ('linux', 'aarch64', 'aarch64-unknown-linux-gnu'),
            ('darwin', 'aarch64', 'aarch64-apple-darwin')]:
        for backend in ('gcc', 'clang'):
            name = backend + '-' + triple
            run(name + '-emit', [args.fbc.resolve(), *base, '-gen', backend, '-target', target,
                '-arch', arch, '-g', '-r', source, '-o', cfile])
            run(name + '-object', [args.clang.resolve(), '--target=' + triple, '-std=gnu23', '-nostdinc',
                '-O2', '-Weverything', '-fno-strict-aliasing', '-fwrapv', '-c', cfile, '-o', obj])
            foreign += 1
    assert all(digest(Path(p)) == v for p, v in pins.items())
    (work / 'receipt.json').write_text(json.dumps({'checks_complete': True, 'native_cases': native,
        'C_object_checks': objects, 'foreign_objects': foreign, 'historical_and_annotation_controls': controls,
        'overall_goal_complete': False}, indent=2))
    for path in (cfile, obj, exe, work / 'without-retention.c'):
        path.unlink(missing_ok=True)
    db.close()
    print('DEEP_PROCEDURE_CONTRACT_PASS', native, objects, foreign, controls, flush=True)


if __name__ == '__main__':
    main()

# end of c-procedure-deep-reachability.py
