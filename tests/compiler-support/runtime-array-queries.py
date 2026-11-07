"""Project: FreeBASIC runtime regression tests
File: runtime-array-queries.py
Purpose: Qualify query implementations through native SDK and BASIC callers.
Responsibilities: Bind inputs, retain unsuppressed diagnostics and check old controls.
This file intentionally does NOT install libraries or retain a build per case.
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
NAMES = ('array_lbound', 'array_ubound', 'array_boundchk')
MODES = ('zero', 'zero-dimension', 'ordinary', 'empty', 'extreme', 'dimension', 'indices')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'prefix', 'gcc', 'clang', 'corpus', 'work-dir', 'diagnostic-profile', 'baseline-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(exist_ok=True)
    fixture = Path(__file__).with_suffix('.c')
    caller = Path(__file__).with_suffix('.bas')
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    tools = {name: getattr(args, name).resolve() for name in ('gcc', 'clang')}
    rtl = ROOT / 'src/rtlib'
    with sqlite3.connect(args.diagnostic_profile) as original_db:
        profile = json.loads(original_db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    env = os.environ.copy()
    env['PATH'] = str(tools['gcc'].parent) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = profile['source_date_epoch']
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('CREATE TABLE IF NOT EXISTS checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')

    def run(name, command, expected=0):
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 32 * 1024 * 1024
        options = {'creationflags': subprocess.BELOW_NORMAL_PRIORITY_CLASS} if os.name == 'nt' else {}
        p = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, capture_output=True,
            text=True, errors='replace', timeout=180, **options)
        output = p.stdout + p.stderr
        assert len(output) < 8 * 1024 * 1024
        row = {'exit': p.returncode, 'expected_exit': expected, 'command': [str(x) for x in command],
            'stdout': p.stdout, 'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]',
                '\n'.join(line for line in output.splitlines() if 'warning:' in line))))}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)', (name, json.dumps(row), zlib.compress(output.encode())))
        db.commit()
        assert p.returncode in (3221225501, -4) if expected == 'diagnostic-trap' else p.returncode == expected, name + '\n' + output[-3000:]
        return row

    # Dependency output binds the real SDK rather than a guessed include list.
    dependencies = set()
    for cc in tools:
        for width in (64, 32):
            for source in [fixture, *[rtl / ('arrays/' + n + '.c') for n in NAMES]]:
                row = run(f'deps-{cc}-{width}-{source.stem}', [tools[cc], '-std=gnu23', '-m' + str(width), '-I', rtl, '-M', source])
                names = row['stdout'].replace('\\\n', ' ').split(': ', 1)[1].split()
                assert not any('\\ ' in n for n in names)
                dependencies.update(Path(n).resolve() for n in names)
    historical = []
    for native, suffix in ((True, '.c'), (False, '.bas')):
        folder = work / ('historical-native' if native else 'historical-corpus')
        folder.mkdir(exist_ok=True)
        for name in NAMES:
            path = folder / (name + suffix)
            path.write_bytes((args.baseline_dir / (('native-' if native else 'corpus-') + name + suffix + '.preimage')).read_bytes())
            historical.append(path)
    producer_sources = [rtl / ('arrays/' + n + '.c') for n in ('array_core', 'array_redim', 'array_redimpresv', 'array_redimto')]
    files = [Path(__file__), fixture, caller, args.fbc.resolve(), *tools.values(),
        runtime / 'fbrt0.o', runtime / 'libfb.a', *producer_sources, *historical,
        *[rtl / ('arrays/' + n + '.c') for n in NAMES], *[args.corpus / (n + '.bas') for n in NAMES]]
    files += list(rtl.rglob('*.h')) + list(args.corpus.rglob('*.bi')) + list((ROOT / 'inc').rglob('*.bi')) + sorted(dependencies)
    pins = {str(p): digest(p) for p in files}
    inputs = {'source_tool_runtime_sha256': pins, 'gcc_flags': profile['gcc_flags'], 'clang_flags': profile['clang_flags'],
        'source_date_epoch': env['SOURCE_DATE_EPOCH'], 'SDK_headers_sha256': {str(p): digest(p) for p in dependencies if p.suffix == '.h'}}
    saved = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    assert saved is None or json.loads(saved[0]) == inputs
    db.execute('INSERT OR IGNORE INTO metadata VALUES(?,?)', ('inputs', json.dumps(inputs)))
    db.commit()
    cpp, main_obj, caller_obj, exe = [work / n for n in ('unit.c', 'main.o', 'caller.o', 'unit.exe')]
    objects = [work / (name + '.o') for name in NAMES]
    producer_objects = [work / (p.stem + '.o') for p in producer_sources]
    for source, obj in zip(producer_sources, producer_objects):
        flags = [f for f in profile['clang_flags'] if f != '-nostdinc']
        run('producer-' + source.stem, [tools['clang'], *flags, '-fno-wrapv', '-fsanitize=pointer-overflow,signed-integer-overflow',
            '-fsanitize-trap=pointer-overflow,signed-integer-overflow', '-I', rtl, '-c', source, '-o', obj])
    implementations = [('native', None), ('basic-gcc', 'gcc'), ('basic-clang', 'clang')]
    api_cases = basic_cases = object_checks = negative_controls = 0
    symbols = {}
    for implementation, backend in implementations:
        for cc in tools:
            for width in (64, 32):
                cases = [('gnu11', '0'), ('gnu11', '2'), ('gnu23', '0'), ('gnu23', '2')] if width == 64 else [('gnu23', '2')]
                for standard, opt in cases:
                    for instrumented in ((False, True) if width == 64 else (False,)):
                        tag = f'{implementation}-{cc}-{width}-{standard}-O{opt}' + ('-checked' if instrumented else '-ordinary')
                        flags = [f for f in profile[cc + '_flags'] if f != '-nostdinc'] + ['-std=' + standard, '-O' + opt, '-m' + str(width)]
                        assert not any(f == '-w' or f.startswith('-Wno-') for f in flags)
                        checks = []
                        if instrumented:
                            checks = ['-fsanitize=pointer-overflow,signed-integer-overflow' if backend is None else '-fsanitize=pointer-overflow']
                            checks += [checks[0].replace('-fsanitize=', '-fsanitize-trap=')] if cc == 'clang' else ['-fsanitize-undefined-trap-on-error']
                        for name, obj in zip(NAMES, objects):
                            if backend is None:
                                source = rtl / ('arrays/' + name + '.c')
                                source_flags = [*flags, '-fno-wrapv']
                            else:
                                source = args.corpus / (name + '.bas')
                                run(tag + '-' + name + '-emit', [args.fbc, '-prefix', args.prefix, '-gen', backend, '-c', '-i', args.corpus,
                                    '-i', ROOT / 'inc', '-d', '_UCRT=1', '-d', 'FB_NO_SFXLIB', *(['-arch', '686'] if width == 32 else []), '-r', source, '-o', cpp])
                                source, source_flags = cpp, [*flags, '-nostdinc']
                            run(tag + '-' + name + '-object', [tools[cc], *source_flags, *checks, '-I', rtl, '-c', source, '-o', obj])
                            object_checks += 1
                            if standard == 'gnu23' and opt == '2' and not instrumented:
                                nm = tools[cc].with_name('nm.exe')
                                row = run(tag + '-' + name + '-symbols', [nm, '-g', '--defined-only', obj])
                                symbols[(implementation, cc, width, name)] = sorted(line.split()[-1] for line in row['stdout'].splitlines() if line.strip())
                        run(tag + '-fixture-object', [tools[cc], *flags, *checks, '-fno-wrapv', '-I', rtl, '-c', fixture, '-o', main_obj])
                        object_checks += 1
                        if width == 32:
                            continue
                        run(tag + '-link', [tools[cc], '-o', exe, runtime / 'fbrt0.o', main_obj, *objects, runtime / 'libfb.a', '-lm', '-mthreads'])
                        for mode in MODES:
                            row = run(tag + '-' + mode, [exe, mode])
                            assert row['stdout'].strip() == 'ARRAY_QUERY=PASS'
                            api_cases += 1
                        if standard == 'gnu23' and opt == '2' and instrumented:
                            for emitter in ('gcc', 'clang'):
                                for checked in (False, True):
                                    case = tag + '-caller-' + emitter + ('-exx' if checked else '-ordinary')
                                    run(case + '-emit', [args.fbc, '-prefix', args.prefix, '-gen', emitter, '-i', ROOT / 'inc', '-m', caller.stem,
                                        *(['-exx'] if checked else []), '-r', caller, '-o', cpp])
                                    caller_checks = ['-fsanitize=pointer-overflow']
                                    caller_checks += ['-fsanitize-trap=pointer-overflow'] if cc == 'clang' else ['-fsanitize-undefined-trap-on-error']
                                    run(case + '-object', [tools[cc], *flags, '-nostdinc', *caller_checks, '-c', cpp, '-o', caller_obj])
                                    run(case + '-link', [tools[cc], '-o', exe, runtime / 'fbrt0.o', caller_obj, *objects, *producer_objects, runtime / 'libfb.a', '-lm', '-mthreads'])
                                    row = run(case + '-execute', [exe])
                                    assert row['stdout'].strip() == 'ARRAY_QUERY_BASIC=PASS'
                                    basic_cases += 1
                        print('ARRAY_QUERY', tag, api_cases, basic_cases, flush=True)
    for implementation, backend in implementations:
        flags = [f for f in profile['clang_flags'] if f != '-nostdinc'] + ['-std=gnu23', '-O2']
        checks = ['-fsanitize=pointer-overflow,signed-integer-overflow', '-fsanitize-trap=pointer-overflow,signed-integer-overflow', '-fno-wrapv'] if backend is None else ['-fsanitize=pointer-overflow', '-fsanitize-trap=pointer-overflow']
        for name, obj in zip(NAMES, objects):
            if backend is None:
                source, source_flags = work / 'historical-native' / (name + '.c'), flags
            else:
                source = work / 'historical-corpus' / (name + '.bas')
                run('historical-' + implementation + '-' + name + '-emit', [args.fbc, '-prefix', args.prefix, '-gen', backend, '-c', '-i', args.corpus,
                    '-i', ROOT / 'inc', '-d', '_UCRT=1', '-d', 'FB_NO_SFXLIB', '-r', source, '-o', cpp])
                source, source_flags = cpp, [*flags, '-nostdinc']
            run('historical-' + implementation + '-' + name + '-object', [tools['clang'], *source_flags, *checks, '-I', rtl, '-c', source, '-o', obj])
            row = run('historical-' + implementation + '-' + name + '-symbols', [tools['clang'].with_name('nm.exe'), '-g', '--defined-only', obj])
            assert sorted(line.split()[-1] for line in row['stdout'].splitlines() if line.strip()) == symbols[(implementation, 'clang', 64, name)]
        run('historical-' + implementation + '-fixture', [tools['clang'], *flags, '-I', rtl, '-c', fixture, '-o', main_obj])
        run('historical-' + implementation + '-link', [tools['clang'], '-o', exe, runtime / 'fbrt0.o', main_obj, *objects, runtime / 'libfb.a', '-lm', '-mthreads'])
        for mode in MODES:
            expected = 11 if mode == 'zero' else 13 if mode == 'zero-dimension' else 'diagnostic-trap' if mode == 'extreme' and backend is None else 0
            run('historical-' + implementation + '-' + mode, [exe, mode], expected)
            negative_controls += int(expected != 0)
    assert all(digest(Path(p)) == sha for p, sha in pins.items())
    receipt = {'checks_complete': True, 'native_API_cases': api_cases, 'native_BASIC_callers': basic_cases,
        'runtime_and_fixture_objects': object_checks, 'historical_failed_controls': negative_controls,
        'SDK_headers_bound': len(inputs['SDK_headers_sha256']), 'overall_goal_complete': False}
    (work / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    (work / 'symbols.json').write_text(json.dumps([{'implementation': i, 'compiler': cc, 'width': w, 'source': n, 'symbols': s}
        for (i, cc, w, n), s in symbols.items()], indent=2) + '\n')
    for path in [cpp, main_obj, caller_obj, exe, *objects, *producer_objects]:
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    db.close()
    print('ARRAY_QUERY_CONTRACT_PASS', receipt, flush=True)


if __name__ == '__main__':
    main()

# end of runtime-array-queries.py
