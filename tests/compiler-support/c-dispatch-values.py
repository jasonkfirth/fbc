"""Project: FreeBASIC C dispatch qualification
File: c-dispatch-values.py
Purpose: Compare original and header dispatch with actual local continuations.
Responsibilities: Run both C producers/tools, standards and optimization levels.
This file intentionally does NOT claim cross-procedure ERROR unwinding.
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
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--tools', type=Path, required=True)
    parser.add_argument('--diagnostic-profile', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    compiler = args.compiler.resolve()
    TOOLS = args.tools.resolve()
    profile_db = args.diagnostic_profile.resolve()
    with sqlite3.connect(profile_db.as_uri() + '?mode=ro', uri=True) as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    providers = ([('baseline', args.baseline.resolve())] if args.baseline else []) + [('candidate', compiler)]
    source = Path(__file__).with_suffix('.bas')
    prefix = args.prefix.resolve()
    runtime = prefix / 'lib/freebasic/win64'
    pins = {str(path): digest(path) for path in [Path(__file__), source, profile_db, *[p for _, p in providers],
        runtime / 'fbrt0.o', runtime / 'libfb.a', TOOLS / 'gcc.exe', TOOLS / 'clang.exe']}
    work = args.output.resolve()
    work.mkdir(exist_ok=False)
    environment = dict(os.environ)
    environment['PATH'] = str(TOOLS) + os.pathsep + environment.get('PATH', '')
    environment['SOURCE_DATE_EPOCH'] = profile['source_date_epoch']
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('INSERT INTO metadata VALUES(?,?)', ('inputs', json.dumps({'sha256': pins, 'profile': profile})))
    db.execute('CREATE TABLE checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')
    db.commit()
    cpp, obj, exe = [work / name for name in ('unit.c', 'unit.o', 'unit.exe')]

    def run(name, command, allowed=(0,)):
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 64 * 1024 * 1024
        result = subprocess.run([str(value) for value in command], cwd=ROOT, env=environment,
            capture_output=True, text=True, errors='replace', timeout=90,
            creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        output = result.stdout + result.stderr
        row = {'command': [str(value) for value in command], 'exit': result.returncode, 'stdout': result.stdout,
            'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(
                line for line in output.splitlines() if 'warning:' in line))))}
        db.execute('INSERT INTO checks VALUES(?,?,?)', (name, json.dumps(row), zlib.compress(output.encode())))
        db.commit()
        if result.returncode not in allowed:
            (work / 'failure.log').write_text(output)
        assert result.returncode in allowed, name + '\n' + str(result.returncode) + '\n' + output[-3500:]
        return row

    native = 0
    native_passes = 0
    native_results = []
    diagnostic_counts = Counter()
    for provider, compiler in providers:
        for backend in ('gcc', 'clang'):
            for variant in (('normal', 'terminate', 'automatic') if args.baseline else ('normal', 'terminate')):
                terminate = variant == 'terminate'
                tag = provider + '-' + backend + '-' + variant
                extra = ['-d', 'TERMINATE_PROOF'] if terminate else (['-d', 'AUTOMATIC_GOSUB_PROOF'] if variant == 'automatic' else [])
                run(tag + '-emit', [compiler, '-prefix', prefix, '-gen', backend, '-ex', '-r',
                    '-m', source.stem, *extra, source, '-o', cpp])
                generated = cpp.read_bytes()
                assert (b'fb_c_jump_dispatch_end' in generated) == (provider == 'baseline')
                assert b'goto *fb_c_jump_target;' in generated
                (work / (tag + '.c.zlib')).write_bytes(zlib.compress(generated))
                for cc in ('gcc', 'clang'):
                    for standard in ('gnu11', 'gnu23'):
                        for optimization in ('0', '2'):
                            label = f'{tag}-{cc}-{standard}-O{optimization}'
                            flags = [*profile[cc + '_flags'], '-std=' + standard, '-O' + optimization]
                            assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
                            row = run(label + '-object', [TOOLS / (cc + '.exe'), *flags, '-c', cpp, '-o', obj])
                            if cc == 'clang':
                                diagnostic_counts[provider] += row['warnings'].get('-Wunreachable-code', 0)
                                if provider == 'candidate':
                                    assert row['warnings'].get('-Wunreachable-code', 0) == 0, row
                            run(label + '-link', [TOOLS / (cc + '.exe'), '-o', exe, runtime / 'fbrt0.o', obj,
                                runtime / 'libfb.a', '-lm', '-mthreads'])
                            row = run(label + '-execute', [exe], (0, 51) if variant == 'automatic' else (0,))
                            if row['exit'] == 0:
                                assert row['stdout'].split() == ['DISPATCH_TERMINATE_PASS' if terminate else 'DISPATCH_VALUES_PASS']
                                native_passes += 1
                            native_results.append({'provider': provider, 'backend': backend, 'variant': variant,
                                'cc': cc, 'standard': standard, 'optimization': optimization, 'exit': row['exit']})
                            native += 1
                print('DISPATCH_NATIVE_COMPLETED', tag, native, flush=True)
    objects = 0
    for backend in ('gcc', 'clang'):
        for terminate in (False, True):
            extra = ['-d', 'TERMINATE_PROOF'] if terminate else []
            tag = 'win32-' + backend + '-' + str(int(terminate))
            run(tag + '-emit', [compiler, '-prefix', prefix, '-gen', backend, '-arch', '686',
                '-ex', '-r', '-m', source.stem, *extra, source, '-o', cpp])
            for cc in ('gcc', 'clang'):
                run(tag + '-' + cc + '-object', [TOOLS / (cc + '.exe'), *profile[cc + '_flags'], '-std=gnu23',
                    '-O2', '-m32', '-c', cpp, '-o', obj])
                objects += 1
    assert diagnostic_counts['candidate'] == 0
    if args.baseline:
        assert diagnostic_counts['baseline'] > 0
    def identity(row):
        return tuple(row[key] for key in ('backend', 'variant', 'cc', 'standard', 'optimization'))
    baseline_results = {identity(row): row for row in native_results if row['provider'] == 'baseline'}
    regressions = [row for row in native_results if row['provider'] == 'candidate' and row['exit'] != 0
        and baseline_results[identity(row)]['exit'] == 0] if args.baseline else []
    assert not regressions, regressions
    assert all(digest(Path(path)) == sha for path, sha in pins.items())
    assert all(digest(Path(path)) == sha for path, sha in profile['installed_sha256'].items())
    receipt = {'checks_complete': True, 'native_runs': native, 'native_passes': native_passes,
        'native_results': native_results, 'new_native_failures': regressions, 'Win32_objects': objects,
        'automatic_GOSUB_lifetime_repaired': False,
        'fixture_unreachable_observations': dict(diagnostic_counts), 'source_tool_runtime_sha256': pins,
        'full_compiler_qualification_complete': False, 'overall_goal_complete': False}
    (work / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    db.close()
    for path in (cpp, obj, exe):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    print('DISPATCH_NATIVE_COMPLETE', native, objects, dict(diagnostic_counts), flush=True)


if __name__ == '__main__':
    main()

# end of c-dispatch-values.py
