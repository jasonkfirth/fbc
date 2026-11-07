"""Project: FreeBASIC C backend tests
File: c-pointer-values.py
Purpose: Qualify native address values independently from valid object accesses.
Responsibilities: Strict C diagnostics, GAS64 controls, instrumented runs and foreign objects.
This file intentionally does NOT execute foreign objects or install compiler/runtime files.
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
    for name in ('fbc', 'baseline', 'prefix', 'gcc', 'clang', 'diagnostic-profile', 'work-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(exist_ok=True)
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    tools = {'gcc': args.gcc.resolve(), 'clang': args.clang.resolve()}
    fixture = ROOT / 'tests/compiler-support/c-pointer-values.bas'
    with sqlite3.connect(args.diagnostic_profile) as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    files = [Path(__file__), fixture, args.fbc.resolve(), args.baseline.resolve(), *tools.values(),
             runtime / 'fbrt0.o', runtime / 'libfb.a']
    pins = {str(path): digest(path) for path in files}
    env = dict(os.environ)
    env['PATH'] = str(tools['gcc'].parent) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = profile['source_date_epoch']
    records, native, foreign, controls = [], 0, 0, 0
    cpp, obj, exe = work / 'unit.c', work / 'unit.o', work / 'unit.exe'

    def run(tag, command, success=True):
        assert sum(path.stat().st_size for path in work.rglob('*') if path.is_file()) < 16 * 1024 * 1024
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, capture_output=True,
                                text=True, errors='replace', timeout=180,
                                creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name == 'nt' else 0)
        output = result.stdout + result.stderr
        (work / (tag + '.log.zlib')).write_bytes(zlib.compress(output.encode()))
        records.append({'tag': tag, 'command': [str(x) for x in command], 'exit': result.returncode,
            'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(
                line for line in output.splitlines() if 'warning:' in line)))), 'stdout': result.stdout})
        (work / 'progress.json').write_text(json.dumps(records, indent=2) + '\n')
        if success:
            assert result.returncode == 0, tag + '\n' + output[-4000:]
        return result

    def emit(tag, compiler, backend, checked, target=None, arch=None):
        command = [compiler, '-prefix', args.prefix.resolve(), '-gen', backend, '-m', fixture.stem,
                   '-r', fixture, '-o', cpp]
        if checked:
            command += ['-exx']
        if target:
            command += ['-target', target, '-arch', arch]
        run(tag + '-emit', command)
        return cpp.read_bytes()

    def flags(cc, standard, optimization):
        selected = [flag for flag in profile[cc + '_flags'] if not re.fullmatch(r'-O[0-9sgz]', flag)
                    and not flag.startswith('-std=')]
        assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in selected)
        return [*selected, '-std=' + standard, optimization]

    for checked in (False, True):
        tag = 'gas64-' + str(int(checked))
        command = [args.fbc.resolve(), '-prefix', args.prefix.resolve(), '-gen', 'gas64', fixture, '-x', exe]
        if checked:
            command += ['-exx']
        run(tag + '-build', command)
        result = run(tag + '-execute', [exe])
        assert re.search(r'POINTER_VALUES_PASS\s+330', result.stdout), result.stdout
        controls += 1
    for backend in ('gcc', 'clang'):
        for checked in (False, True):
            tag = backend + '-' + str(int(checked))
            generated = emit(tag, args.fbc.resolve(), backend, checked)
            (work / (tag + '.c.zlib')).write_bytes(zlib.compress(generated))
            for cc, tool in tools.items():
                for standard in ('gnu11', 'gnu23'):
                    for optimization in ('-O0', '-O2'):
                        variants = (False, True) if cc == 'clang' else (False,)
                        for instrumented in variants:
                            name = '-'.join((tag, cc, standard, optimization[1:], str(int(instrumented))))
                            extra = ['-fsanitize=pointer-overflow', '-fsanitize-trap=pointer-overflow'] if instrumented else []
                            run(name + '-object', [tool, *flags(cc, standard, optimization), *extra, '-c', cpp, '-o', obj])
                            run(name + '-link', [tool, '-o', exe, runtime / 'fbrt0.o', obj, runtime / 'libfb.a', '-lm', '-mthreads'])
                            result = run(name + '-execute', [exe])
                            assert re.search(r'POINTER_VALUES_PASS\s+330', result.stdout), (name, result.stdout)
                            native += 1
                    name = '-'.join((tag, cc, standard, 'win32'))
                    emit(name, args.fbc.resolve(), backend, checked, 'win32', 'x86')
                    cross = ['-m32'] if cc == 'gcc' else ['--target=i686-w64-mingw32']
                    run(name + '-object', [tool, *flags(cc, standard, '-O2'), *cross, '-c', cpp, '-o', obj])
                    foreign += 1
                    # Restore native C once; no other output copies are retained.
                    cpp.write_bytes(generated)
            print('POINTER_NATIVE_MATRIX', tag, 'native_runs=' + str(native), flush=True)
        # The accepted compiler must reproduce the real address-formation failure.
        tag = backend + '-historical'
        emit(tag, args.baseline.resolve(), backend, False)
        run(tag + '-object', [tools['clang'], *flags('clang', 'gnu11', '-O0'),
            '-fsanitize=pointer-overflow', '-fsanitize-trap=pointer-overflow', '-c', cpp, '-o', obj])
        run(tag + '-link', [tools['clang'], '-o', exe, runtime / 'fbrt0.o', obj, runtime / 'libfb.a', '-lm', '-mthreads'])
        result = run(tag + '-execute', [exe], success=False)
        assert result.returncode != 0 and 'POINTER_VALUES_PASS' not in result.stdout
        controls += 1
        for target, arch, triple in (('linux', 'x86_64', 'x86_64-linux-gnu'),
                ('linux', 'aarch64', 'aarch64-linux-gnu'), ('linux', 'armv7a', 'armv7-linux-gnueabihf'),
                ('darwin', 'aarch64', 'aarch64-apple-darwin'), ('win32', 'aarch64', 'aarch64-w64-mingw32')):
            tag = '-'.join((backend, target, arch))
            emit(tag, args.fbc.resolve(), backend, False, target, arch)
            run(tag + '-object', [tools['clang'], *flags('clang', 'gnu11', '-O2'), '--target=' + triple,
                                  '-c', cpp, '-o', obj])
            foreign += 1
    assert native == 48 and foreign == 26 and controls == 4, (native, foreign, controls)
    assert all(digest(Path(path)) == sha for path, sha in pins.items())
    (work / 'receipt.json').write_text(json.dumps({'checks_complete': True, 'source_tool_runtime_sha256': pins,
        'native_runs': native, 'instrumented_native_runs': 16, 'cases_per_run': 330,
        'GAS64_controls': 2, 'historical_trap_controls': 2, 'Win32_objects': 16, 'other_foreign_objects': 10,
        'foreign_runtime_execution_claimed': False, 'checks': records, 'overall_goal_complete': False}, indent=2) + '\n')
    for path in (cpp, obj, exe):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    print('POINTER_VALUE_MATRIX_PASS native=48 cases=330 foreign_objects=26 controls=4', flush=True)


if __name__ == '__main__':
    main()

# end of c-pointer-values.py
