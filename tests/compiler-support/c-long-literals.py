"""Project: FreeBASIC C backend regression tests
File: c-long-literals.py
Purpose: Check long literal storage under unsuppressed native C diagnostics.
Responsibilities: Run native byte checks and compile foreign character ABIs,
retain compressed receipts, and reject a compiler that still emits long quotes.
This file intentionally does NOT install tools or execute foreign objects.
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
    for name in ('fbc', 'prefix', 'gcc', 'clang', 'work-dir', 'baseline'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    owner = work / 'owner.json'
    identity = {'owner': 'fbc-long-literal-contract'}
    if owner.exists():
        assert json.loads(owner.read_text()) == identity
    else:
        assert not any(work.iterdir()), 'choose an empty work directory'
        owner.write_text(json.dumps(identity))
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    source = HERE / 'c-long-literals.bas'
    files = [source, Path(__file__), args.fbc.resolve(), args.baseline.resolve(),
             args.gcc.resolve(), args.clang.resolve(), runtime / 'fbrt0.o', runtime / 'libfb.a']
    pins = {str(p): digest(p) for p in files}
    sys.path.insert(0, str(ROOT / 'build/c-emitter-active-20261101'))
    from audit_linux_compiler_cross import check_resources
    with sqlite3.connect(ROOT / 'build/c-emitter-active-20261101/builtin-current-compiler-audit/results.sqlite3') as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    env = os.environ.copy()
    env['PATH'] = str(args.gcc.resolve().parent) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = profile['source_date_epoch']
    inputs = {'file_sha256': pins, 'gcc_flags': profile['gcc_flags'],
              'clang_flags': profile['clang_flags'], 'source_date_epoch': env['SOURCE_DATE_EPOCH']}
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('CREATE TABLE IF NOT EXISTS checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')
    saved = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    assert saved is None or json.loads(saved[0]) == inputs
    db.execute('INSERT OR IGNORE INTO metadata VALUES(?,?)', ('inputs', json.dumps(inputs)))
    db.commit()

    def run(name, command):
        check_resources()
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 32 * 1024 * 1024
        p = subprocess.run([str(x) for x in command], cwd=ROOT, env=env,
                           capture_output=True, text=True, errors='replace', timeout=180,
                           creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        log = p.stdout + p.stderr
        assert len(log) < 8 * 1024 * 1024
        row = {'exit': p.returncode, 'command': [str(x) for x in command],
               'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', log)))}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)',
                   (name, json.dumps(row), zlib.compress(log.encode())))
        db.commit()
        assert p.returncode == 0, name + '\n' + log[-3000:]
        return row

    cfile, obj, exe = work / 'unit.c', work / 'unit.o', work / 'unit.exe'
    base = ['-prefix', args.prefix.resolve(), '-w', 'all', '-m', source.stem,
            '-r', source, '-o', cfile]
    native, objects, foreign = 0, 0, 0
    for backend in ('gcc', 'clang'):
        for width in (64, 32):
            run(f'emit-{backend}-{width}', [args.fbc.resolve(), '-gen', backend,
                '-target', 'win' + str(width), *base])
            for cc, tool in (('gcc', args.gcc.resolve()), ('clang', args.clang.resolve())):
                flags = profile[cc + '_flags']
                assert not any(f == '-w' or f.startswith('-Wno-') for f in flags)
                modes = [('gnu11', '-O0', []), ('gnu11', '-O2', []),
                         ('gnu23', '-O0', []), ('gnu23', '-O2', []),
                         ('gnu23', '-O2', ['-funsigned-char'])] if width == 64 else [('gnu23', '-O2', [])]
                for standard, opt, extra in modes:
                    name = f'{backend}-{width}-{cc}-{standard}-{opt[1:]}-' + ('unsigned' if extra else 'default')
                    row = run(name, [tool, *flags, '-std=' + standard, opt,
                                     '-m' + str(width), *extra, '-c', cfile, '-o', obj])
                    assert '-Woverlength-strings' not in row['warnings']
                    objects += 1
                    if width == 32:
                        continue
                    run(name + '-link', [tool, '-o', exe, runtime / 'fbrt0.o', obj,
                        '-L' + str(runtime), '-lfb', '-lm', '-mthreads'])
                    run(name + '-execute', [exe])
                    native += 1
        for target, arch, triple in [('linux', 'x86_64', 'x86_64-unknown-linux-gnu'),
                ('linux', 'armv7a', 'armv7-unknown-linux-gnueabihf'),
                ('linux', 'aarch64', 'aarch64-unknown-linux-gnu'),
                ('darwin', 'aarch64', 'aarch64-apple-darwin')]:
            name = f'{backend}-{target}-{arch}'
            run(name + '-emit', [args.fbc.resolve(), '-gen', backend, '-target', target, '-arch', arch, *base])
            code = cfile.read_text()
            arrays = re.findall(r'static\s+(?:const\s+)?uint32\s+\w+\[\].*?=\s*\{([^}]+)\}', code, re.S)
            expected = [8364, 128512, 88] * 2048 + [0]
            assert any([int(x) for x in re.findall(r'\d+', a)] == expected for a in arrays), name
            row = run(name + '-object', [args.clang.resolve(), '--target=' + triple,
                '-std=gnu23', '-nostdinc', '-O2', '-Weverything', '-Wwrite-strings',
                '-fno-strict-aliasing', '-fwrapv', '-c', cfile, '-o', obj])
            assert '-Woverlength-strings' not in row['warnings']
            foreign += 1
        print('LONG_LITERAL', backend, flush=True)
    run('baseline-emit', [args.baseline.resolve(), '-gen', 'gcc', *base])
    old = run('baseline-object', [args.gcc.resolve(), *profile['gcc_flags'], '-c', cfile, '-o', obj])
    assert old['warnings'].get('-Woverlength-strings', 0) > 0
    assert all(digest(Path(p)) == value for p, value in pins.items())
    result = {'checks_complete': True, 'native_cases': native,
              'native_object_checks': objects, 'foreign_object_checks': foreign,
              'old_compiler_warning_reproduced': True, 'overall_goal_complete': False}
    (work / 'receipt.json').write_text(json.dumps(result, indent=2))
    for p in (cfile, obj, exe):
        p.unlink(missing_ok=True)
    db.close()
    print('LONG_LITERAL_CONTRACT_PASS', result, flush=True)


if __name__ == '__main__':
    main()

# end of c-long-literals.py
