"""Project: FreeBASIC external prototype regression
File: c-external-prototypes.py
Purpose: Check emitted declaration closure in release, debug and opaque outputs.
Responsibilities: Execute live interop roots and inspect discarded prototype controls.
This file intentionally does NOT treat successful object creation as execution.
"""
from collections import Counter
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--tools', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    work = args.output.resolve()
    work.mkdir(parents=True, exist_ok=False)
    source = Path(__file__).with_suffix('.bas')
    shim = source.with_suffix('.c')
    tools = args.tools.resolve()
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    files = [Path(__file__), source, shim, args.compiler.resolve(), tools / 'gcc.exe', tools / 'clang.exe',
             runtime / 'fbrt0.o', runtime / 'libfb.a']
    if args.baseline:
        files.append(args.baseline.resolve())
    pins = {str(p): digest(p) for p in files}
    env = dict(os.environ)
    env['PATH'] = str(tools) + os.pathsep + env.get('PATH', '')
    cpp, obj, shimobj, exe = [work / p for p in ('unit.c', 'unit.o', 'shim.o', 'unit.exe')]
    checks = []
    flags = {
        'gcc': ['-std=gnu23', '-O2', '-fno-strict-aliasing', '-fwrapv', '-Wall', '-Wextra', '-Wpedantic',
                '-Wconversion', '-Wsign-conversion', '-Wcast-qual', '-Wcast-align=strict',
                '-Wpointer-arith', '-Wformat=2', '-Wstrict-prototypes', '-Wmissing-prototypes'],
        'clang': ['-std=gnu23', '-O2', '-fno-strict-aliasing', '-fwrapv', '-Weverything']}

    def run(tag, command):
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 16 * 1024 * 1024
        result = subprocess.run([str(v) for v in command], cwd=ROOT, env=env, capture_output=True,
                                timeout=90, creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        output = (result.stdout + result.stderr).decode(errors='replace')
        (work / (tag + '.log.zlib')).write_bytes(zlib.compress(output.encode()))
        row = {'tag': tag, 'command': [str(v) for v in command], 'exit': result.returncode,
               'stdout': result.stdout.decode(errors='replace'),
               'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(
                   line for line in output.splitlines() if 'warning:' in line))))}
        checks.append(row)
        assert result.returncode == 0, (tag, output[-4000:])
        return row

    runs = objects32 = old_controls = 0
    for backend in ('gcc', 'clang'):
        for mode, extra in (('release', []), ('debug', ['-g']), ('opaque', ['-d', 'OPAQUE_PROOF']),
                            ('intrinsic', ['-d', 'WIN_INTRINSIC_PROOF'])):
            tag = backend + '-' + mode
            command = [args.compiler.resolve(), '-prefix', args.prefix.resolve(), '-i', ROOT / 'inc',
                       '-gen', backend, '-m', source.stem, *extra, '-r', source, '-o', cpp]
            run(tag + '-emit', command)
            text = cpp.read_text()
            declaration = r'\bprototype_dead\s*\([^;{}]*\)\s*;'
            retained = bool(re.search(declaration, text))
            assert retained == (mode in ('debug', 'opaque')), (tag, retained)
            if mode == 'intrinsic':
                for identifier in ('_InterlockedExchangeAdd', '_InterlockedExchangeAdd64'):
                    assert re.search(r'\b' + identifier + r'\s*\([^;{}]*\)\s*;', text), (tag, identifier)
            for identifier in ('prototype_live', 'prototype_callback', 'prototype_public_target'):
                assert re.search(r'\b' + identifier + r'\s*\([^;{}]*\)\s*;', text), (tag, identifier)
            (work / (tag + '.c.zlib')).write_bytes(zlib.compress(cpp.read_bytes()))
            if args.baseline and mode == 'release':
                old = command[:]
                old[0] = args.baseline.resolve()
                old[-1] = work / 'baseline.c'
                run(tag + '-baseline', old)
                assert re.search(declaration, (work / 'baseline.c').read_text())
                old_controls += 1
                (work / 'baseline.c').unlink()
            for cc in ('gcc', 'clang'):
                run(tag + '-' + cc + '-object', [tools / (cc + '.exe'), *flags[cc], '-nostdinc', '-masm=intel', '-c', cpp, '-o', obj])
                run(tag + '-' + cc + '-shim', [tools / (cc + '.exe'), *flags[cc], '-c', shim, '-o', shimobj])
                run(tag + '-' + cc + '-link', [tools / 'gcc.exe', obj, shimobj, runtime / 'fbrt0.o', runtime / 'libfb.a', '-o', exe, '-lm', '-mthreads'])
                executed = run(tag + '-' + cc + '-execute', [exe])
                assert executed['stdout'].split() == ['EXTERNAL_PROTOTYPES_PASS']
                runs += 1
            if mode == 'release':
                command.insert(command.index('-r'), '-arch')
                command.insert(command.index('-r'), '686')
                run(tag + '-win32-emit', command)
                assert not re.search(declaration, cpp.read_text())
                for cc in ('gcc', 'clang'):
                    run(tag + '-' + cc + '-win32-object', [tools / (cc + '.exe'), *flags[cc], '-nostdinc', '-masm=intel', '-m32', '-c', cpp, '-o', obj])
                    objects32 += 1
    assert (runs, objects32) == (16, 4)
    assert all(digest(Path(p)) == sha for p, sha in pins.items())
    (work / 'receipt.json').write_text(json.dumps({'checks_complete': True, 'native_runs': runs,
        'Win32_objects': objects32, 'historical_stale_declaration_controls': old_controls,
        'source_tool_runtime_sha256': pins, 'checks': checks, 'foreign_execution_claimed': False}, indent=2) + '\n')
    for path in (cpp, obj, shimobj, exe):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    print('EXTERNAL_PROTOTYPE_CLOSURE_PASS native=16 Win32_objects=4', flush=True)


if __name__ == '__main__':
    main()

# end of c-external-prototypes.py
