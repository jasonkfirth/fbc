"""Project: FreeBASIC C record layout regression
File: c-record-layout.py
Purpose: Prove member-offset guards detect errors hidden by an unchanged record size.
Responsibilities: Run native layouts and compare failed guards with size-only controls.
This file intentionally does NOT execute deliberately corrupted record declarations.
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


def normalized(source):
    lines = [line for line in source.replace('\r\n', '\n').replace('\r', '\n').splitlines()
             if line.strip() and '__builtin_offsetof(' not in line]
    return re.sub(r'\bfb_c_assert_\d+\b', 'fb_c_assert_ID', '\n'.join(lines))


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
    tools = args.tools.resolve()
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    files = [Path(__file__), source, args.compiler.resolve(), tools / 'gcc.exe', tools / 'clang.exe',
        runtime / 'fbrt0.o', runtime / 'libfb.a']
    if args.baseline:
        files.append(args.baseline.resolve())
    pins = {str(path): digest(path) for path in files}
    environment = dict(os.environ)
    environment['PATH'] = str(tools) + os.pathsep + environment.get('PATH', '')
    checks = []
    cpp, obj, exe = work / 'unit.c', work / 'unit.o', work / 'unit.exe'
    flags = {
        'gcc': ['-std=gnu23', '-O2', '-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-Wall', '-Wextra',
            '-Wpedantic', '-Wconversion', '-Wsign-conversion', '-Wcast-qual', '-Wcast-align=strict',
            '-Wpointer-arith', '-Wformat=2', '-Wstrict-prototypes', '-Wmissing-prototypes'],
        'clang': ['-std=gnu23', '-O2', '-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-Weverything', '-Wwrite-strings']}

    def run(tag, command):
        completed = subprocess.run([str(value) for value in command], cwd=ROOT, env=environment,
            capture_output=True, timeout=90, creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        output = (completed.stdout + completed.stderr).decode(errors='replace')
        (work / (tag + '.log.zlib')).write_bytes(zlib.compress(output.encode()))
        row = {'tag': tag, 'command': [str(value) for value in command], 'exit': completed.returncode,
            'stdout': completed.stdout.decode(errors='replace'),
            'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(
                line for line in output.splitlines() if 'warning:' in line))))}
        checks.append(row)
        return row

    native, negative, controls, objects32, identities = 0, 0, 0, 0, 0
    for backend in ('gcc', 'clang'):
        command = [args.compiler.resolve(), '-prefix', args.prefix.resolve(), '-i', ROOT / 'inc',
            '-gen', backend, '-m', source.stem, '-r', source, '-o', cpp]
        emitted = run(backend + '-emit', command)
        assert emitted['exit'] == 0, emitted
        text = cpp.read_text()
        record = re.search(r'struct ([A-Za-z_][A-Za-z_0-9]*LAYOUTRECORD)\s*\{[^}]*?\buint16 MIDDLE;', text)
        assert record, 'record declaration not found'
        tag_name = record[1]
        needle = '__builtin_offsetof(struct ' + tag_name + ', MIDDLE)'
        guard_lines = [line for line in text.splitlines(True) if needle in line]
        assert len(guard_lines) == 1
        (work / (backend + '-original.c.zlib')).write_bytes(zlib.compress(text.encode()))
        if args.baseline:
            old_command = command[:]
            old_command[0] = args.baseline.resolve()
            old_cpp = work / 'baseline.c'
            old_command[-1] = old_cpp
            old = run(backend + '-baseline-emit', old_command)
            assert old['exit'] == 0 and normalized(old_cpp.read_text()) == normalized(text)
            identities += 1
            assert old_cpp.resolve().parent == work
            old_cpp.unlink()
        mutated = text[:record.start()] + text[record.start():].replace('uint16 MIDDLE;',
            'uint8 MIDDLE;\n\tuint16 fb_c_probe_reserved;', 1)
        size_only = mutated.replace(guard_lines[0], '')
        assert needle not in size_only
        for cc in ('gcc', 'clang'):
            cpp.write_text(text)
            compiled = run(backend + '-' + cc + '-positive', [tools / (cc + '.exe'), *flags[cc], '-c', cpp, '-o', obj])
            assert compiled['exit'] == 0, compiled
            linked = run(backend + '-' + cc + '-link', [tools / 'gcc.exe', obj, runtime / 'fbrt0.o',
                runtime / 'libfb.a', '-o', exe, '-lm', '-mthreads'])
            assert linked['exit'] == 0, linked
            executed = run(backend + '-' + cc + '-execute', [exe])
            assert executed['exit'] == 0 and executed['stdout'].split() == ['RECORD_LAYOUT_PASS', '6'], executed
            native += 1
            cpp.write_text(mutated)
            rejected = run(backend + '-' + cc + '-negative', [tools / (cc + '.exe'), *flags[cc], '-c', cpp, '-o', obj])
            assert rejected['exit'] != 0
            negative += 1
            cpp.write_text(size_only)
            accepted = run(backend + '-' + cc + '-size-only-control', [tools / (cc + '.exe'), *flags[cc], '-c', cpp, '-o', obj])
            assert accepted['exit'] == 0, accepted
            controls += 1
        command = [args.compiler.resolve(), '-prefix', args.prefix.resolve(), '-i', ROOT / 'inc',
            '-gen', backend, '-arch', '686', '-m', source.stem, '-r', source, '-o', cpp]
        emitted32 = run(backend + '-win32-emit', command)
        assert emitted32['exit'] == 0, emitted32
        for cc in ('gcc', 'clang'):
            compiled = run(backend + '-' + cc + '-win32-object', [tools / (cc + '.exe'), *flags[cc], '-m32', '-c', cpp, '-o', obj])
            assert compiled['exit'] == 0, compiled
            objects32 += 1
    assert (native, negative, controls, objects32) == (4, 4, 4, 4)
    assert all(digest(Path(path)) == sha for path, sha in pins.items())
    receipt = {'checks_complete': True, 'native_runs': native, 'rejected_bad_offsets': negative,
        'same_size_controls_accepted': controls, 'Win32_objects': objects32,
        'original_C_identity_after_guard_removal': identities, 'checks': checks,
        'source_tool_runtime_sha256': pins, 'foreign_runtime_execution_claimed': False}
    (work / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    for path in (cpp, obj, exe):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    print('RECORD_LAYOUT_GUARDS_PASS native=4 negative=4 size_only_controls=4 Win32_objects=4', flush=True)


if __name__ == '__main__':
    main()

# end of c-record-layout.py
