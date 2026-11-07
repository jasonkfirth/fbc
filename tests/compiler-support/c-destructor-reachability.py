"""Project: FreeBASIC C backend qualification
File: c-destructor-reachability.py
Purpose: Execute private destructor ownership cases with aggressive C diagnostics.
Responsibilities: Bind inputs, inspect omitted bodies and verify cleanup output.
This file intentionally does NOT install compilers or execute foreign objects.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sqlite3
import subprocess

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT = SCRIPT_DIR.parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'baseline', 'prefix', 'gcc', 'clang', 'work-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--root-control', type=Path)
    args = parser.parse_args()
    HERE = args.work_dir.resolve()
    HERE.mkdir(exist_ok=True)
    compiler = args.fbc.resolve()
    source = SCRIPT_DIR / 'c-destructor-reachability.bas'
    tools = {'gcc': args.gcc.resolve(), 'clang': args.clang.resolve()}
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    with sqlite3.connect(ROOT / 'build/c-emitter-active-20261101/builtin-current-compiler-audit/results.sqlite3') as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    pins = {str(p): digest(p) for p in [source, Path(__file__), compiler, args.baseline.resolve(), tools['gcc'],
        tools['clang'], runtime / 'fbrt0.o', runtime / 'libfb.a']}
    env = os.environ.copy()
    env['PATH'] = str(tools['gcc'].parent) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = profile['source_date_epoch']
    records = []

    def run(name, command):
        assert sum(p.stat().st_size for p in HERE.rglob('*') if p.is_file()) < 16 * 1024 * 1024
        p = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, capture_output=True,
            text=True, errors='replace', timeout=120, creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        result = {'name': name, 'command': [str(x) for x in command], 'exit': p.returncode,
            'stdout': p.stdout, 'stderr': p.stderr}
        records.append(result)
        (HERE / 'root-checks.json').write_text(json.dumps(records, indent=2) + '\n')
        assert p.returncode == 0, name + '\n' + (p.stdout + p.stderr)[-3000:]
        return p

    cfile, obj, exe = [HERE / name for name in ('roots.c', 'roots.o', 'roots.exe')]
    count = objects = 0
    for backend in ('gcc', 'clang'):
        for debug in (False, True):
            for width in (64, 32):
                run(backend + str(width) + ('-debug' if debug else '-release') + '-emit', [compiler, '-i', ROOT / 'inc', '-gen', backend,
                    *(['-g'] if debug else []), '-target', 'win' + str(width), '-m', source.stem, '-r', source, '-o', cfile])
                code = cfile.read_text().upper()
                assert 'UNOBSERVED_DTOR' not in code
                assert 'STATIC_ROOT' in code and 'VIRTUAL_ITEM' in code
                for cc in ('gcc', 'clang'):
                    for standard, opt in ([('gnu11', '0'), ('gnu11', '2'), ('gnu23', '0'), ('gnu23', '2')]
                                          if width == 64 else [('gnu23', '2')]):
                        name = backend + '-' + str(width) + ('-debug-' if debug else '-release-') + cc + '-' + standard + '-O' + opt
                        flags = [*profile[cc + '_flags'], '-std=' + standard, '-O' + opt, '-m' + str(width)]
                        assert not any(f == '-w' or f.startswith('-Wno-') for f in flags)
                        result = run(name + '-object', [tools[cc], *flags, '-c', cfile, '-o', obj])
                        assert '[-Wunused-function]' not in result.stderr
                        objects += 1
                        if width == 32:
                            continue
                        run(name + '-link', [tools[cc], '-o', exe, runtime / 'fbrt0.o',
                            obj, runtime / 'libfb.a', '-lm', '-mthreads'])
                        text = run(name + '-execute', [exe]).stdout
                        assert len(re.findall(r'(?m)^LEAF=\s*0\s*$', text)) == 8, text
                        assert len(re.findall(r'(?m)^STATIC_ROOT=\s*17\s*$', text)) == 1, text
                        for marker in ('VIRTUAL_BASE', 'VIRTUAL_ITEM', 'DESTRUCTOR_ROOTS_MAIN=PASS'):
                            assert text.splitlines().count(marker) == 1, text
                        assert 'UNOBSERVED_DTOR' not in text
                        count += 1
    controls = foreign = 0
    for label, control in [('historical', args.baseline), ('without-static-root', args.root_control)]:
        if control is None:
            continue
        pins[str(control.resolve())] = digest(control.resolve())
        for backend in ('gcc', 'clang'):
            run(label + '-' + backend, [control.resolve(), '-i', ROOT / 'inc', '-gen', backend,
                '-g', '-m', source.stem, '-r', source, '-o', cfile])
            code = cfile.read_text().upper()
            if label == 'historical':
                assert 'UNOBSERVED_DTOR' in code
            else:
                assert 'STATIC_ROOT' not in code
            controls += 1
    for target, arch, triple in [('linux', 'x86_64', 'x86_64-unknown-linux-gnu'),
            ('linux', 'armv7a', 'armv7-unknown-linux-gnueabihf'),
            ('linux', 'aarch64', 'aarch64-unknown-linux-gnu'),
            ('darwin', 'aarch64', 'aarch64-apple-darwin')]:
        for backend in ('gcc', 'clang'):
            name = backend + '-' + triple
            run(name + '-emit', [compiler, '-i', ROOT / 'inc', '-gen', backend, '-target', target,
                '-arch', arch, '-g', '-m', source.stem, '-r', source, '-o', cfile])
            run(name + '-object', [tools['clang'], '--target=' + triple, '-std=gnu23', '-nostdinc',
                '-O2', '-Weverything', '-fno-strict-aliasing', '-fwrapv', '-c', cfile, '-o', obj])
            foreign += 1
    for path in (cfile, obj, exe):
        assert path.resolve().parent == HERE
        path.unlink()
    assert all(digest(Path(p)) == v for p, v in pins.items())
    (HERE / 'roots-receipt.json').write_text(json.dumps({'checks_complete': True,
        'input_sha256': pins, 'source_date_epoch': env['SOURCE_DATE_EPOCH'],
        'gcc_flags': profile['gcc_flags'], 'clang_flags': profile['clang_flags'],
        'native_executions': count, 'objects': objects,
        'foreign_objects': foreign, 'historical_controls': controls, 'overall_goal_complete': False}, indent=2) + '\n')
    print('DESTRUCTOR_ROOTS_PASS', count, objects, foreign, controls, flush=True)


if __name__ == '__main__':
    main()

# end of c-destructor-reachability.py
