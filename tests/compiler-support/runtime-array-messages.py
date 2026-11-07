"""Project: FreeBASIC runtime regression tests
File: runtime-array-messages.py
Purpose: Qualify fixed-size array diagnostic assembly through real runtime entry points.
Responsibilities: Bind SDK/runtime inputs and retain ordinary/checked C diagnostics.
This file intentionally does NOT install libraries or run old truncated-message paths.
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
    for name in ('fbc', 'prefix', 'gcc', 'clang', 'corpus', 'work-dir', 'diagnostic-profile'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(exist_ok=True)
    fixture = Path(__file__).with_suffix('.c')
    native = ROOT / 'src/rtlib/arrays/array_boundchk.c'
    basic = args.corpus.resolve() / 'array_boundchk.bas'
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    rtl = ROOT / 'src/rtlib'
    tools = {cc: getattr(args, cc).resolve() for cc in ('gcc', 'clang')}
    with sqlite3.connect(args.diagnostic_profile) as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    env = os.environ.copy()
    env['PATH'] = str(tools['gcc'].parent) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = profile['source_date_epoch']
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('CREATE TABLE IF NOT EXISTS checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')

    def run(name, command):
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 16 * 1024 * 1024
        options = {'creationflags': subprocess.BELOW_NORMAL_PRIORITY_CLASS} if os.name == 'nt' else {}
        p = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, capture_output=True,
            text=True, errors='replace', timeout=180, **options)
        output = p.stdout + p.stderr
        assert len(output) < 8 * 1024 * 1024
        row = {'command': [str(x) for x in command], 'exit': p.returncode, 'stdout': p.stdout,
            'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(s for s in output.splitlines() if 'warning:' in s))))}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)', (name, json.dumps(row), zlib.compress(output.encode())))
        db.commit()
        assert p.returncode == 0, name + '\n' + output[-3000:]
        return row

    hook = work / 'format-hook.h'
    hook.write_text('#define snprintf test_snprintf\n')
    dependencies = set()
    for cc in tools:
        for width in (64, 32):
            for source in (fixture, native):
                row = run(f'deps-{cc}-{width}-{source.stem}', [tools[cc], '-std=gnu23', '-m' + str(width), '-I', rtl, '-M', source])
                names = row['stdout'].replace('\\\n', ' ').split(': ', 1)[1].split()
                assert not any('\\ ' in n for n in names)
                dependencies.update(Path(n).resolve() for n in names)
    files = [Path(__file__), fixture, native, basic, hook, args.fbc.resolve(), *tools.values(),
        tools['gcc'].with_name('nm.exe'), runtime / 'fbrt0.o', runtime / 'libfb.a', *sorted(dependencies)]
    files += list(rtl.rglob('*.h')) + list(args.corpus.rglob('*.bi')) + list((ROOT / 'inc').rglob('*.bi'))
    pins = {str(p): digest(p) for p in files}
    inputs = {'source_tool_runtime_sha256': pins, 'gcc_flags': profile['gcc_flags'], 'clang_flags': profile['clang_flags'],
        'source_date_epoch': env['SOURCE_DATE_EPOCH'], 'SDK_headers_sha256': {str(p): digest(p) for p in dependencies if p.suffix == '.h'}}
    saved = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    assert saved is None or json.loads(saved[0]) == inputs
    db.execute('INSERT OR IGNORE INTO metadata VALUES(?,?)', ('inputs', json.dumps(inputs)))
    db.commit()
    cpp, obj, main_obj, exe = [work / n for n in ('unit.c', 'runtime.o', 'main.o', 'unit.exe')]
    native_runs = object_checks = API_cases = 0
    symbols = []
    for implementation, backend in (('native', None), ('basic-gcc', 'gcc'), ('basic-clang', 'clang')):
        for cc in tools:
            for width in (64, 32):
                cases = [('gnu11', '0'), ('gnu11', '2'), ('gnu23', '0'), ('gnu23', '2')] if width == 64 else [('gnu23', '2')]
                for standard, opt in cases:
                    for checked in ((False, True) if width == 64 else (False,)):
                        tag = f'{implementation}-{cc}-{width}-{standard}-O{opt}' + ('-checked' if checked else '-ordinary')
                        flags = [f for f in profile[cc + '_flags'] if f != '-nostdinc'] + ['-std=' + standard, '-O' + opt, '-m' + str(width)]
                        assert not any(f == '-w' or f.startswith('-Wno-') for f in flags)
                        extra = []
                        if checked:
                            extra = ['-fsanitize=pointer-overflow,object-size,alignment']
                            extra += ['-fsanitize-trap=pointer-overflow,object-size,alignment'] if cc == 'clang' else ['-fsanitize-undefined-trap-on-error']
                        if backend is None:
                            source, source_flags = native, [*flags, '-fno-wrapv']
                        else:
                            run(tag + '-emit', [args.fbc, '-prefix', args.prefix, '-gen', backend, '-c', '-i', args.corpus,
                                '-i', ROOT / 'inc', '-d', '_UCRT=1', '-d', 'FB_NO_SFXLIB', *(['-arch', '686'] if width == 32 else []), '-r', basic, '-o', cpp])
                            source, source_flags = cpp, [*flags, '-nostdinc', '-include', hook]
                        run(tag + '-runtime-object', [tools[cc], *source_flags, *extra, '-I', rtl, '-c', source, '-o', obj])
                        run(tag + '-fixture-object', [tools[cc], *flags, *extra, '-I', rtl,
                            *(['-DBASIC_FORMATTER'] if backend else []), '-c', fixture, '-o', main_obj])
                        object_checks += 2
                        if standard == 'gnu23' and opt == '2' and not checked:
                            row = run(tag + '-symbols', [tools[cc].with_name('nm.exe'), '-g', '--defined-only', obj])
                            symbols.append({'implementation': implementation, 'compiler': cc, 'width': width, 'symbols': sorted(s.split()[-1] for s in row['stdout'].splitlines() if s.strip())})
                        if width == 32:
                            continue
                        run(tag + '-link', [tools[cc], '-o', exe, runtime / 'fbrt0.o', main_obj, obj, runtime / 'libfb.a', '-lm', '-mthreads'])
                        row = run(tag + '-execute', [exe])
                        assert row['stdout'].strip() == 'ARRAY_MESSAGES=PASS'
                        native_runs += 1
                        API_cases += 33 if backend else 30
        print('ARRAY_MESSAGES', implementation, native_runs, API_cases, flush=True)
    assert all(digest(Path(p)) == sha for p, sha in pins.items())
    receipt = {'checks_complete': True, 'native_runs': native_runs, 'API_cases': API_cases,
        'runtime_and_fixture_objects': object_checks, 'Win32_objects': 12, 'SDK_headers_bound': len(inputs['SDK_headers_sha256']),
        'overall_goal_complete': False}
    (work / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    (work / 'symbols.json').write_text(json.dumps(symbols, indent=2) + '\n')
    for path in (cpp, obj, main_obj, exe):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    db.close()
    print('ARRAY_MESSAGES_CONTRACT_PASS', receipt, flush=True)


if __name__ == '__main__':
    main()

# end of runtime-array-messages.py
