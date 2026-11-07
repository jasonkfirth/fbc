"""Project: FreeBASIC native array value qualification
File: c-array-values.py
Purpose: Compare actual scalar, aggregate and callback array values with GAS64.
Responsibilities: Check native wrapping, instrumentation, checked errors and 32-bit objects.
This file intentionally does NOT execute an access outside owned physical storage.
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
    tools = args.tools.resolve()
    prefix = args.prefix.resolve()
    runtime = prefix / 'lib/freebasic/win64'
    source = Path(__file__).with_suffix('.bas')
    baseline = args.baseline.resolve() if args.baseline else None
    profile_db = args.diagnostic_profile.resolve()
    with sqlite3.connect(profile_db) as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    files = [Path(__file__), compiler, source, profile_db, tools / 'gcc.exe', tools / 'clang.exe',
             runtime / 'fbrt0.o', runtime / 'libfb.a']
    if baseline:
        files.append(baseline)
    pins = {str(p): digest(p) for p in files}
    work = args.output.resolve()
    work.mkdir(exist_ok=False)
    cpp, obj, exe = [work / name for name in ('unit.c', 'unit.o', 'unit.exe')]
    env = dict(os.environ)
    env['PATH'] = str(tools) + os.pathsep + env.get('PATH', '')
    checks = []

    def run(tag, command, expected=0):
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 16 * 1024 * 1024
        result = subprocess.run([str(v) for v in command], cwd=ROOT, env=env, capture_output=True,
                                timeout=90, creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        output = (result.stdout + result.stderr).decode(errors='replace')
        (work / (tag + '.log.zlib')).write_bytes(zlib.compress(output.encode()))
        row = {'tag': tag, 'command': [str(v) for v in command], 'exit': result.returncode,
               'stdout': result.stdout.decode(errors='replace'),
               'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(line for line in output.splitlines() if 'warning:' in line))))}
        checks.append(row)
        (work / 'progress.json').write_text(json.dumps(checks, indent=2) + '\n')
        if expected is not None:
            assert result.returncode == expected, (tag, result.returncode, output[-4000:])
        return row

    def emit(tag, backend, mode, selected_compiler=compiler, architecture=None):
        command = [selected_compiler, '-prefix', prefix, '-gen', backend, '-m', source.stem, '-r', source, '-o', cpp]
        if mode != 'native':
            command += ['-exx', '-d', 'CHECKED_PROOF' if mode == 'checked' else 'EXPECT_BOUNDS_ERROR']
        if architecture:
            command += ['-arch', architecture]
        run(tag + '-emit', command)

    def flags(cc, standard, optimization):
        values = [v for v in profile[cc + '_flags'] if not v.startswith('-std=') and not re.fullmatch(r'-O[0-9sgz]', v)]
        assert not any(v == '-w' or v.startswith('-Wno-') for v in values)
        return [*values, '-std=' + standard, optimization]

    native = checked = failures = objects = 0
    for mode in ('native', 'checked', 'failure'):
        command = [compiler, '-prefix', prefix, '-gen', 'gas64', source, '-x', exe]
        if mode != 'native':
            command += ['-exx', '-d', 'CHECKED_PROOF' if mode == 'checked' else 'EXPECT_BOUNDS_ERROR']
        run('gas64-' + mode + '-build', command)
        executed = run('gas64-' + mode + '-execute', [exe], 6 if mode == 'failure' else 0)
        if mode != 'failure':
            assert executed['stdout'].split() == ['ARRAY_VALUE_MATRIX_PASS']
    for backend in ('gcc', 'clang'):
        emit(backend, backend, 'native')
        (work / (backend + '.c.zlib')).write_bytes(zlib.compress(cpp.read_bytes()))
        for standard in ('gnu11', 'gnu23'):
            for optimization in ('-O0', '-O2'):
                for cc, instrumented in (('gcc', False), ('clang', False), ('clang', True)):
                    tag = '-'.join((backend, standard, optimization[1:], cc, str(int(instrumented))))
                    extra = ['-fsanitize=array-bounds,pointer-overflow,alignment', '-fsanitize-trap=array-bounds,pointer-overflow,alignment'] if instrumented else []
                    run(tag + '-object', [tools / (cc + '.exe'), *flags(cc, standard, optimization), *extra, '-c', cpp, '-o', obj])
                    run(tag + '-link', [tools / 'gcc.exe', obj, runtime / 'fbrt0.o', runtime / 'libfb.a', '-o', exe, '-lm', '-mthreads'])
                    executed = run(tag + '-execute', [exe])
                    assert executed['stdout'].split() == ['ARRAY_VALUE_MATRIX_PASS']
                    native += 1
        for mode in ('checked', 'failure'):
            emit(backend + '-' + mode, backend, mode)
            for cc in ('gcc', 'clang'):
                tag = backend + '-' + mode + '-' + cc
                extra = ['-fsanitize=array-bounds,pointer-overflow,alignment', '-fsanitize-trap=array-bounds,pointer-overflow,alignment'] if cc == 'clang' else []
                run(tag + '-object', [tools / (cc + '.exe'), *flags(cc, 'gnu23', '-O2'), *extra, '-c', cpp, '-o', obj])
                run(tag + '-link', [tools / 'gcc.exe', obj, runtime / 'fbrt0.o', runtime / 'libfb.a', '-o', exe, '-lm', '-mthreads'])
                executed = run(tag + '-execute', [exe], 6 if mode == 'failure' else 0)
                if mode == 'checked':
                    assert executed['stdout'].split() == ['ARRAY_VALUE_MATRIX_PASS']
                    checked += 1
                else:
                    failures += 1
        emit(backend + '-win32', backend, 'native', architecture='686')
        for cc in ('gcc', 'clang'):
            run(backend + '-' + cc + '-win32-object', [tools / (cc + '.exe'), *flags(cc, 'gnu23', '-O2'), '-m32', '-c', cpp, '-o', obj])
            objects += 1
    assert (native, checked, failures, objects) == (24, 4, 4, 4)
    if baseline:
        emit('historical', 'gcc', 'native', selected_compiler=baseline)
        run('historical-object', [tools / 'clang.exe', *flags('clang', 'gnu23', '-O2'), '-fsanitize=array-bounds,pointer-overflow,alignment', '-fsanitize-trap=array-bounds,pointer-overflow,alignment', '-c', cpp, '-o', obj])
        run('historical-link', [tools / 'gcc.exe', obj, runtime / 'fbrt0.o', runtime / 'libfb.a', '-o', exe, '-lm', '-mthreads'])
        old = run('historical-execute', [exe], None)
        assert old['exit'] != 0
    assert all(digest(Path(p)) == sha for p, sha in pins.items())
    (work / 'receipt.json').write_text(json.dumps({'checks_complete': True, 'native_runs': native,
        'checked_native_runs': checked, 'checked_error_cases': failures, 'GAS64_controls': 3,
        'Win32_objects': objects, 'historical_instrumented_trap_observed': baseline is not None, 'checks': checks,
        'source_tool_runtime_sha256': pins, 'foreign_execution_claimed': False}, indent=2) + '\n')
    for p in (cpp, obj, exe):
        p.unlink(missing_ok=True)
    print('ARRAY_VALUE_MATRIX_PASS native=24 checked=4 error_cases=4 GAS64=3 Win32_objects=4', flush=True)


if __name__ == '__main__':
    main()

# end of c-array-values.py
