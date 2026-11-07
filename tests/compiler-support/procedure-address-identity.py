"""Project: FreeBASIC backend regression tests
File: procedure-address-identity.py
Purpose: Qualify native procedure-address comparison semantics.
Responsibilities: Run C and GAS64 cases with unsuppressed diagnostics, check
foreign objects, and reject the preceding incorrect compiler behaviors.
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
import sys
import zlib

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'baseline', 'ordering-baseline', 'cast-baseline', 'prefix', 'gcc', 'clang', 'work-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    owner = work / 'owner.json'
    identity = {'owner': 'fbc-procedure-address-identity'}
    if owner.exists():
        assert json.loads(owner.read_text()) == identity
    else:
        assert not any(work.iterdir())
        owner.write_text(json.dumps(identity))
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    source, helper = HERE / 'procedure-address-identity.bas', HERE / 'procedure-address-identity.c'
    files = [source, helper, Path(__file__), args.fbc.resolve(), args.baseline.resolve(),
             args.ordering_baseline.resolve(), args.cast_baseline.resolve(), args.gcc.resolve(), args.clang.resolve(),
             runtime / 'fbrt0.o', runtime / 'libfb.a']
    pins = {str(p): digest(p) for p in files}
    sys.path.insert(0, str(ROOT / 'build/c-emitter-active-20261101'))
    from audit_linux_compiler_cross import check_resources
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
    old = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    assert old is None or json.loads(old[0]) == inputs
    db.execute('INSERT OR IGNORE INTO metadata VALUES(?,?)', ('inputs', json.dumps(inputs)))
    db.commit()

    def run(name, command, allow_failure=False):
        check_resources()
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 16 * 1024 * 1024
        p = subprocess.run([str(x) for x in command], cwd=ROOT, env=env,
                           capture_output=True, text=True, errors='replace', timeout=180,
                           creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        log = p.stdout + p.stderr
        assert len(log) < 8 * 1024 * 1024
        row = {'exit': p.returncode, 'command': [str(x) for x in command],
               'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(s for s in log.splitlines() if 'warning:' in s))))}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)', (name, json.dumps(row), zlib.compress(log.encode())))
        db.commit()
        assert allow_failure or p.returncode == 0, name + '\n' + log[-3000:]
        return row

    cfile, obj, bridge, exe = [work / name for name in ('unit.c', 'unit.o', 'bridge.o', 'unit.exe')]
    base = ['-prefix', args.prefix.resolve(), '-i', ROOT / 'inc', '-m', source.stem]
    native, c_objects, gas_objects, cross_objects = 0, 0, 0, 0
    for backend in ('gcc', 'clang'):
        for width in (64, 32):
            run(f'{backend}-{width}-emit', [args.fbc.resolve(), *base, '-gen', backend,
                '-target', 'win' + str(width), '-r', source, '-o', cfile])
            for cc, tool in [('gcc', args.gcc.resolve()), ('clang', args.clang.resolve())]:
                flags = profile[cc + '_flags']
                assert not any(f == '-w' or f.startswith('-Wno-') for f in flags)
                modes = [('gnu11', '-O0'), ('gnu11', '-O2'), ('gnu23', '-O0'), ('gnu23', '-O2')] if width == 64 else [('gnu23', '-O2')]
                for standard, opt in modes:
                    name = f'{backend}-{width}-{cc}-{standard}-{opt[1:]}'
                    row = run(name, [tool, *flags, '-std=' + standard, opt, '-m' + str(width), '-c', cfile, '-o', obj])
                    assert '-Wbad-function-cast' not in row['warnings']
                    c_objects += 1
                    run(name + '-reference', [tool, *flags, '-std=' + standard, opt, '-m' + str(width), '-c', helper, '-o', bridge])
                    if width == 32:
                        continue
                    run(name + '-link', [tool, '-o', exe, runtime / 'fbrt0.o', obj, bridge,
                        '-L' + str(runtime), '-lfb', '-lm', '-mthreads'])
                    run(name + '-execute', [exe])
                    native += 1
        print('PROCEDURE_ADDRESS', backend, flush=True)
    for opt in ('0', '2'):
        run('gas64-O' + opt + '-object', [args.fbc.resolve(), *base, '-gen', 'gas64', '-O', opt, '-c', source, '-o', obj])
        gas_objects += 1
        for cc, tool in [('gcc', args.gcc.resolve()), ('clang', args.clang.resolve())]:
            name = 'gas64-O' + opt + '-' + cc
            run(name + '-reference', [tool, *profile[cc + '_flags'], '-c', helper, '-o', bridge])
            run(name + '-link', [tool, '-o', exe, runtime / 'fbrt0.o', obj, bridge, '-L' + str(runtime), '-lfb', '-lm', '-mthreads'])
            run(name + '-execute', [exe])
            native += 1
    for target, arch, triple in [('linux', 'x86_64', 'x86_64-unknown-linux-gnu'),
            ('linux', 'armv7a', 'armv7-unknown-linux-gnueabihf'), ('linux', 'aarch64', 'aarch64-unknown-linux-gnu'),
            ('darwin', 'aarch64', 'aarch64-apple-darwin')]:
        flags = ['--target=' + triple, '-std=gnu23', '-nostdinc', '-O2', '-Weverything', '-Wwrite-strings', '-fno-strict-aliasing', '-fwrapv']
        run(triple + '-reference', [args.clang.resolve(), *flags, '-c', helper, '-o', bridge])
        cross_objects += 1
        for backend in ('gcc', 'clang'):
            name = backend + '-' + triple
            run(name + '-emit', [args.fbc.resolve(), *base, '-gen', backend, '-target', target, '-arch', arch, '-r', source, '-o', cfile])
            run(name + '-object', [args.clang.resolve(), *flags, '-c', cfile, '-o', obj])
            cross_objects += 1
    run('baseline-reference', [args.gcc.resolve(), *profile['gcc_flags'], '-c', helper, '-o', bridge])
    run('baseline-C-emit', [args.baseline.resolve(), *base, '-gen', 'gcc', '-r', source, '-o', cfile])
    run('baseline-C-object', [args.clang.resolve(), *profile['clang_flags'], '-c', cfile, '-o', obj])
    run('baseline-C-link', [args.gcc.resolve(), '-o', exe, runtime / 'fbrt0.o', obj, bridge, '-L' + str(runtime), '-lfb', '-lm', '-mthreads'])
    assert run('baseline-C-execute', [exe], True)['exit'] != 0
    for label, compiler in [('baseline-gas64', args.baseline.resolve()),
            ('ordering-baseline-gas64', args.ordering_baseline.resolve()), ('cast-baseline-gas64', args.cast_baseline.resolve())]:
        run(label + '-object', [compiler, *base, '-gen', 'gas64', '-c', source, '-o', obj])
        run(label + '-link', [args.gcc.resolve(), '-o', exe, runtime / 'fbrt0.o', obj, bridge, '-L' + str(runtime), '-lfb', '-lm', '-mthreads'])
        assert run(label + '-execute', [exe], True)['exit'] != 0
    assert all(digest(Path(p)) == value for p, value in pins.items())
    result = {'checks_complete': True, 'native_cases': native, 'C_native_object_checks': c_objects,
              'GAS64_object_checks': gas_objects, 'foreign_objects': cross_objects,
              'negative_controls': 4, 'overall_goal_complete': False}
    (work / 'receipt.json').write_text(json.dumps(result, indent=2))
    for p in (cfile, obj, bridge, exe):
        p.unlink(missing_ok=True)
    db.close()
    print('PROCEDURE_ADDRESS_CONTRACT_PASS', result, flush=True)


if __name__ == '__main__':
    main()

# end of procedure-address-identity.py
