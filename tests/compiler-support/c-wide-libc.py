"""Project: FreeBASIC C backend regression tests
File: c-wide-libc.py
Purpose: Qualify checked wide-library calls and unchanged native address identity.
Responsibilities: Run native C modes, cross objects and incompatible-declaration controls.
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
import zlib

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'prefix', 'gcc', 'clang', 'work-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    owner = work / 'wide-libc-work.json'
    identity = {'owner': 'fbc-wide-libc-contract'}
    if owner.is_file():
        assert json.loads(owner.read_text()) == identity
    else:
        assert not any(work.iterdir()), 'choose an empty work directory'
        owner.write_text(json.dumps(identity))
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    sources = [HERE / name for name in ('c-wide-libc.bas', 'c-wide-libc-native.c', 'c-wide-libc-defined.bas', 'c-wide-libc-wrong.bas', 'c-wide-libc-forward.bas')]
    files = [*sources, Path(__file__), args.fbc.resolve(), args.gcc.resolve(), args.clang.resolve(), runtime / 'fbrt0.o', runtime / 'libfb.a']
    pins = {str(p): digest(p) for p in files}
    env = os.environ.copy()
    env['PATH'] = str(args.gcc.resolve().parent) + os.pathsep + env.get('PATH', '')
    for tool in (args.gcc.resolve(), args.clang.resolve()):
        p = subprocess.run([str(tool), '-M', str(sources[1])], cwd=work, env=env, capture_output=True, text=True, timeout=60)
        assert p.returncode == 0, p.stderr
        dependencies = p.stdout.replace('\\\n', ' ').split(': ', 1)[1]
        for word in re.findall(r'(?:\\ |[^\s])+', dependencies):
            word = word.replace('\\ ', ' ')
            path = Path(word)
            assert path.is_file(), word
            pins[str(path)] = digest(path)
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')
    db.execute('CREATE TABLE IF NOT EXISTS metadata(name TEXT PRIMARY KEY,value TEXT)')
    provenance = {'source_tool_runtime_sdk_sha256': pins}
    saved = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    assert saved is None or json.loads(saved[0]) == provenance
    db.execute('INSERT OR IGNORE INTO metadata VALUES(?,?)', ('inputs', json.dumps(provenance)))
    db.commit()

    def run(name, command, accept_failure=False):
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 16 * 1024 * 1024
        p = subprocess.run([str(v) for v in command], cwd=work, env=env, capture_output=True,
            text=True, errors='replace', timeout=180, creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        log = p.stdout + p.stderr
        assert len(log) < 8 * 1024 * 1024
        warnings = Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(s for s in log.splitlines() if 'warning:' in s)))
        row = {'exit': p.returncode, 'command': [str(v) for v in command], 'warnings': dict(warnings)}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)', (name, json.dumps(row), zlib.compress(log.encode())))
        db.commit()
        assert accept_failure or p.returncode == 0, name + ': ' + log[-2500:]
        return row, log

    cfile, obj, bridge, exe = [work / name for name in ('unit.c', 'unit.o', 'bridge.o', 'unit.exe')]
    common = ['-fno-strict-aliasing', '-fwrapv', '-frounding-math', '-fno-math-errno']
    gcc_flags = ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion', '-Wcast-qual',
        '-Wcast-align=strict', '-Wpointer-arith', '-Wstrict-prototypes', '-Wmissing-prototypes',
        '-Wmissing-declarations', '-Wformat=2', '-Wshadow', '-Wwrite-strings', '-Wredundant-decls']
    native, objects, cross, controls = 0, 0, 0, 0
    for backend in ('gcc', 'clang'):
        base = [args.fbc.resolve(), '-prefix', args.prefix.resolve(), '-i', ROOT / 'inc', '-gen', backend]
        for width in (64, 32):
            run(f'{backend}-{width}-emit', [*base, '-target', 'win' + str(width), '-m', sources[0].stem, '-r', sources[0], '-o', cfile])
            text = cfile.read_text()
            assert all(name in text for name in ('fb_c_wcslen(', 'fb_c_wcsncmp(', 'fb_c_wcschr('))
            assert '&wcslen' in text and '&fb_c_wcslen' not in text
            assert '&wcschr' in text and '&fb_c_wcschr' not in text
            for cc, tool, flags in [('gcc', args.gcc.resolve(), gcc_flags), ('clang', args.clang.resolve(), ['-Weverything', '-Wwrite-strings'])]:
                modes = [('gnu11', '-O0'), ('gnu11', '-O2'), ('gnu23', '-O0'), ('gnu23', '-O2')] if width == 64 else [('gnu23', '-O2')]
                for standard, opt in modes:
                    name = f'{backend}-{width}-{cc}-{standard}-{opt[1:]}'
                    row, _ = run(name, [tool, *common, *flags, '-std=' + standard, opt,
                        '-m' + str(width), '-nostdinc', '-c', cfile, '-o', obj])
                    assert '-Walign-mismatch' not in row['warnings']
                    objects += 1
                    if width == 32:
                        continue
                    run(name + '-bridge', [tool, *flags, '-std=' + standard, opt, '-c', sources[1], '-o', bridge])
                    run(name + '-link', [tool, '-o', exe, runtime / 'fbrt0.o', obj, bridge, '-L' + str(runtime), '-lfb', '-lm', '-mthreads'])
                    run(name + '-execute', [exe])
                    native += 1
        for source in sources[2:]:
            run(backend + '-' + source.stem + '-emit', [*base, '-target', 'win64', '-r', source, '-o', cfile])
            assert all(name not in cfile.read_text() for name in ('fb_c_wcslen', 'fb_c_wcschr'))
            if source.stem.endswith('wrong'):
                row, log = run(backend + '-wrong-native', [args.clang.resolve(), '-std=gnu23', '-Weverything', '-nostdinc', '-fsyntax-only', cfile], True)
                assert 'wcslen' in log and ('warning:' in log or 'error:' in log)
            controls += 1
        for target, arch, triple in [('linux', 'x86_64', 'x86_64-unknown-linux-gnu'),
                ('linux', 'armv7a', 'armv7-unknown-linux-gnueabihf'),
                ('linux', 'aarch64', 'aarch64-unknown-linux-gnu'),
                ('darwin', 'aarch64', 'aarch64-apple-darwin')]:
            name = f'{backend}-{target}-{arch}'
            run(name + '-emit', [*base, '-target', target, '-arch', arch, '-m', sources[0].stem, '-r', sources[0], '-o', cfile])
            assert 'fb_c_wcslen(' in cfile.read_text()
            row, _ = run(name + '-object', [args.clang.resolve(), '--target=' + triple, '-std=gnu23',
                '-Weverything', '-Wwrite-strings', *common, '-nostdinc', '-O2', '-c', cfile, '-o', obj])
            assert '-Walign-mismatch' not in row['warnings']
            cross += 1
        print('WIDE_LIBC', backend, flush=True)
    assert all(digest(Path(path)) == value for path, value in pins.items())
    result = {'checks_complete': True, 'native_cases': native, 'native_object_checks': objects,
        'foreign_object_checks': cross, 'declaration_controls': controls, 'overall_goal_complete': False}
    (work / 'receipt.json').write_text(json.dumps(result, indent=2))
    for p in (cfile, obj, bridge, exe):
        p.unlink(missing_ok=True)
    db.close()
    print('WIDE_LIBC_CONTRACT_PASS', result, flush=True)


if __name__ == '__main__':
    main()

# end of c-wide-libc.py
