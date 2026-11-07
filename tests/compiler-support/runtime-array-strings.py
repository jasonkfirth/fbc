"""Project: FreeBASIC runtime regression tests
File: runtime-array-strings.py
Purpose: Qualify native and BASIC string-array retirement with real runtime deletion.
Responsibilities: Bind SDK inputs, preserve diagnostics and bound each reusable scratch set.
This file intentionally does NOT install libraries or treat runtime passes as a C proof.
"""
from collections import Counter
import argparse
import gzip
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
    for name in ('fbc', 'prefix', 'gcc', 'clang', 'corpus', 'work-dir', 'diagnostic-profile', 'baseline-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(exist_ok=True)
    fixture = Path(__file__).with_suffix('.c')
    rtl = ROOT / 'src/rtlib'
    native = rtl / 'arrays/array_destructstr.c'
    core = rtl / 'arrays/array_core.c'
    basic = args.corpus / 'array_destructstr.bas'
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    tools = {name: getattr(args, name).resolve() for name in ('gcc', 'clang')}
    nm = tools['gcc'].with_name('nm.exe')
    source_db = sqlite3.connect(args.diagnostic_profile)
    try:
        profile = json.loads(source_db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    finally:
        source_db.close()
    env = os.environ.copy()
    env['PATH'] = str(tools['gcc'].parent) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = profile['source_date_epoch']
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('CREATE TABLE IF NOT EXISTS checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')

    def run(name, command):
        total = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        assert total < 8 * 1024 * 1024, total
        options = {'creationflags': subprocess.BELOW_NORMAL_PRIORITY_CLASS} if os.name == 'nt' else {}
        p = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, capture_output=True,
            text=True, errors='replace', timeout=180, **options)
        output = p.stdout + p.stderr
        row = {'exit': p.returncode, 'command': [str(x) for x in command], 'stdout': p.stdout,
            'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(l for l in output.splitlines() if 'warning:' in l))))}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)', (name, json.dumps(row), zlib.compress(output.encode())))
        db.commit()
        if p.returncode:
            (work / 'failure.log').write_text(output)
        assert p.returncode == 0, name + '\n' + output[-4000:]
        return row

    dependencies = set()
    for cc in tools:
        for width in (64, 32):
            for source in (fixture, native, core):
                row = run(f'deps-{cc}-{width}-{source.stem}', [tools[cc], '-std=gnu23', '-m' + str(width), '-I', rtl, '-M', source])
                names = row['stdout'].replace('\\\n', ' ').split(': ', 1)[1].split()
                assert not any('\\ ' in n for n in names)
                dependencies.update(Path(n).resolve() for n in names)
    old_c = work / 'before.c'
    old_bas = work / 'before.bas'
    archives = [args.baseline_dir / ('native-array_destructstr.c.gz'), args.baseline_dir / ('corpus-array_destructstr.bas.gz')]
    for destination, archive in zip((old_c, old_bas), archives):
        destination.write_bytes(gzip.decompress(archive.read_bytes()))
    files = [Path(__file__), fixture, native, core, basic, args.fbc.resolve(), *tools.values(), nm,
        runtime / 'fbrt0.o', runtime / 'libfb.a', *archives, old_c, old_bas, *sorted(dependencies)]
    files += list(rtl.rglob('*.h')) + list(args.corpus.rglob('*.bi')) + list((ROOT / 'inc').rglob('*.bi'))
    pins = {str(p): digest(p) for p in files}
    inputs = {'source_tool_runtime_sha256': pins, 'gcc_flags': profile['gcc_flags'], 'clang_flags': profile['clang_flags'],
        'source_date_epoch': env['SOURCE_DATE_EPOCH'],
        'SDK_headers_sha256': {str(p): digest(p) for p in dependencies if p.suffix == '.h'}}
    saved = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    assert saved is None or json.loads(saved[0]) == inputs
    db.execute('INSERT OR IGNORE INTO metadata VALUES(?,?)', ('inputs', json.dumps(inputs)))
    db.commit()
    cpp, module_obj, core_obj, main_obj, exe = [work / n for n in ('unit.c', 'module.o', 'core.o', 'main.o', 'unit.exe')]
    native_runs = instrumented_runs = historical_runs = win32_objects = api_cases = 0
    symbols = []

    def emit(source, backend, width, tag):
        run(tag + '-emit', [args.fbc, '-prefix', args.prefix, '-gen', backend,
            *(['-arch', '686'] if width == 32 else []), '-c', '-i', args.corpus,
            '-d', 'FB_NO_SFXLIB', '-d', '_UCRT=1', '-i', ROOT / 'inc', '-r', source, '-o', cpp])
        generated = cpp.read_bytes()
        (work / (tag + '.c.zlib')).write_bytes(zlib.compress(generated))
        return generated

    def qualify(label, backend, source, cc, width, standard, opt, checked):
        nonlocal native_runs, instrumented_runs, historical_runs, win32_objects, api_cases
        tag = f'{label}-{cc}-{width}-{standard}-O{opt}' + ('-checked' if checked else '')
        generated = emit(source, backend, width, tag) if backend else None
        flags = [*profile[cc + '_flags'], '-std=' + standard, '-O' + opt, '-m' + str(width)]
        assert not any(f == '-w' or f.startswith('-Wno-') for f in flags)
        if checked:
            checks = 'pointer-overflow,object-size,alignment'
            flags += ['-fsanitize=' + checks]
            flags += ['-fsanitize-trap=' + checks] if cc == 'clang' else ['-fsanitize-undefined-trap-on-error']
        sdk_flags = [f for f in flags if f != '-nostdinc']
        run(tag + '-core', [tools[cc], *sdk_flags, '-I', rtl, '-c', core, '-o', core_obj])
        run(tag + '-module', [tools[cc], *(flags if backend else sdk_flags),
            *([] if backend else ['-I', rtl]), '-c', cpp if backend else source, '-o', module_obj])
        if standard == 'gnu23' and opt == '2' and not checked and not label.startswith('before'):
            row = run(tag + '-symbols', [nm, '-g', '--defined-only', module_obj])
            exported = sorted(line.split()[-1] for line in row['stdout'].splitlines() if line.strip())
            assert len(exported) == 2, (tag, exported)
            symbols.append({'implementation': label, 'compiler': cc, 'width': width, 'symbols': exported})
        run(tag + '-fixture', [tools[cc], *sdk_flags, '-I', rtl, '-c', fixture, '-o', main_obj])
        if width == 32:
            win32_objects += 3
            return
        run(tag + '-link', [tools[cc], '-o', exe, runtime / 'fbrt0.o', main_obj, module_obj, core_obj,
            '-Wl,--wrap=fb_StrDelete', runtime / 'libfb.a', '-lm', '-mthreads'])
        result = run(tag + '-execute', [exe])
        match = re.search(r'STRING_RETIREMENT_PASS cases=(\d+)', result['stdout'])
        assert match and int(match[1]) == 446, result
        api_cases += int(match[1])
        if label.startswith('before'):
            historical_runs += 1
        else:
            native_runs += 1
            instrumented_runs += int(checked)
        if generated is not None:
            assert digest(cpp) == hashlib.sha256(generated).hexdigest()

    for label, backend, source in (('native', None, native), ('basic-gcc', 'gcc', basic), ('basic-clang', 'clang', basic)):
        for cc in tools:
            for standard in ('gnu11', 'gnu23'):
                for opt in ('0', '2'):
                    qualify(label, backend, source, cc, 64, standard, opt, False)
            qualify(label, backend, source, cc, 64, 'gnu23', '2', True)
            qualify(label, backend, source, cc, 32, 'gnu23', '2', False)
        print('STRING_RETIREMENT_IMPLEMENTATION_PASS', label, native_runs, flush=True)
    # The old defect forms an unused invalid pointer. Observable behavior still
    # passes these controls; the source change removes that pointer formation.
    qualify('before-native', None, old_c, 'clang', 64, 'gnu23', '2', False)
    qualify('before-basic', 'gcc', old_bas, 'clang', 64, 'gnu23', '2', False)
    for cc in tools:
        for width in (64, 32):
            group = [row for row in symbols if row['compiler'] == cc and row['width'] == width]
            assert len(group) == 3 and len({tuple(row['symbols']) for row in group}) == 1
    (work / 'symbols.json').write_text(json.dumps(symbols, indent=2) + '\n')
    assert all(digest(Path(p)) == sha for p, sha in pins.items())
    for name, sha in profile['installed_sha256'].items():
        assert digest(Path(name)) == sha
    receipt = {'checks_complete': True, 'native_runs': native_runs, 'instrumented_native_runs': instrumented_runs,
        'historical_behavior_controls': historical_runs, 'API_cases': api_cases, 'Win32_objects': win32_objects,
        'SDK_headers_bound': len(inputs['SDK_headers_sha256']), 'installed_binaries_unchanged': True,
        'public_symbol_sets_match': True,
        'overall_goal_complete': False}
    (work / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    for path in (cpp, module_obj, core_obj, main_obj, exe):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    db.close()
    print('STRING_RETIREMENT_QUALIFIED', json.dumps(receipt), flush=True)


if __name__ == '__main__':
    main()

# end of runtime-array-strings.py
