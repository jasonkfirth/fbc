"""Project: FreeBASIC C backend tests
File: c-gosub-values.py
Purpose: Qualify local GOSUB values, node ownership and legacy runtime behavior.
Responsibilities: Run both C producers/tools under unsuppressed diagnostic flags.
This file intentionally does NOT install a compiler or qualify ERROR unwinding.
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


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--tools', type=Path, required=True)
    parser.add_argument('--diagnostic-profile', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root, compiler, prefix, tools, work = [path.resolve() for path in
        (args.root, args.compiler, args.prefix, args.tools, args.output)]
    fixtures = root / 'tests/compiler-support'
    rtl = root / 'src/rtlib'
    runtime = prefix / 'lib/freebasic/win64'
    work.mkdir(exist_ok=False)
    with sqlite3.connect(args.diagnostic_profile.resolve().as_uri() + '?mode=ro', uri=True) as previous:
        profile = json.loads(previous.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    files = [Path(__file__), compiler, args.diagnostic_profile.resolve(), tools / 'gcc.exe', tools / 'clang.exe',
        *[path for path in runtime.iterdir() if path.is_file()],
        *[fixtures / ('c-gosub-' + name) for name in ('values.bas', 'threads.bas', 'lifecycle.bas', 'runtime-targets.c', 'lifecycle-observer.c')],
        rtl / 'core/gosub.c', *list(rtl.rglob('*.h')), *list((root / 'inc').rglob('*.bi'))]
    pins = {str(path): digest(path) for path in files}
    inputs = {'sha256': pins, 'profile': profile}
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('INSERT INTO metadata VALUES(?,?)', ('inputs', json.dumps(inputs)))
    db.execute('CREATE TABLE checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')
    db.commit()
    environment = dict(os.environ)
    environment['PATH'] = str(tools) + os.pathsep + environment.get('PATH', '')

    def run(tag, command, expected=0):
        assert sum(path.stat().st_size for path in work.rglob('*') if path.is_file()) < 64 * 1024 * 1024
        flags = list(map(str, command))
        assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
        result = subprocess.run(flags, cwd=root, env=environment, capture_output=True, text=True, errors='replace',
            timeout=90, creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name == 'nt' else 0)
        output = result.stdout + result.stderr
        row = {'command': flags, 'exit': result.returncode, 'expected_exit': expected,
            'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(line for line in output.splitlines() if 'warning:' in line))))}
        db.execute('INSERT INTO checks VALUES(?,?,?)', (tag, json.dumps(row), zlib.compress(output.encode())))
        db.commit()
        if result.returncode != expected:
            (work / 'failure.log').write_text(output)
        assert result.returncode == expected, tag + ' exit ' + str(result.returncode) + '\n' + output[-4000:]
        return result

    # Only this production translation unit sees the observer macros. SDK
    # declarations retain their real names and the process allocator is intact.
    observed = work / 'observed.c'
    observed.write_text('''/* GOSUB-only allocation observer. */
#include "fb.h"
#include "fb_gosub.h"
#include <setjmp.h>
void *gosub_test_malloc(size_t bytes);
void gosub_test_free(void *pointer);
#define malloc gosub_test_malloc
#define free gosub_test_free
''' + '#include "' + (rtl / 'core/gosub.c').as_posix() + '"\n' +
        '#undef malloc\n#undef free\n/* end of observed.c */\n')
    cpp, obj, exe = [work / name for name in ('unit.c', 'unit.o', 'unit.exe')]
    for cc in ('gcc', 'clang'):
        flags = [flag for flag in profile[cc + '_flags'] if flag != '-nostdinc']
        flags += ['-std=gnu23', '-O2', '-I', rtl]
        run(cc + '-observed-runtime', [tools / (cc + '.exe'), *flags, '-c', observed, '-o', work / (cc + '-runtime.o')])
        run(cc + '-observer', [tools / (cc + '.exe'), *flags, '-c', fixtures / 'c-gosub-lifecycle-observer.c', '-o', work / (cc + '-observer.o')])
    counts = Counter()
    for backend in ('gcc', 'clang'):
        for case in ('values', 'threads', 'unchecked', 'checked', 'allocation-failure'):
            source = fixtures / ('c-gosub-' + (case if case in ('values', 'threads') else 'lifecycle') + '.bas')
            extra = ['-mt', '-exx'] if case == 'threads' else (['-exx'] if case == 'values' else [])
            if case == 'checked':
                extra += ['-ex', '-d', 'CHECKED_RETURN']
            elif case == 'allocation-failure':
                extra += ['-ex', '-d', 'ALLOCATION_FAILURE']
            tag = backend + '-' + case
            run(tag + '-emit', [compiler, '-prefix', prefix, '-i', root / 'inc', '-gen', backend,
                '-m', source.stem, '-r', *extra, source, '-o', cpp])
            generated = cpp.read_bytes()
            assert b'fb_GosubPushTarget' in generated and b'_setjmp(' not in generated
            (work / (tag + '.c.zlib')).write_bytes(zlib.compress(generated))
            for cc in ('gcc', 'clang'):
                for standard in ('gnu11', 'gnu23'):
                    for opt in ('0', '2'):
                        name = tag + '-' + cc + '-' + standard + '-O' + opt
                        run(name + '-object', [tools / (cc + '.exe'), *profile[cc + '_flags'], '-std=' + standard, '-O' + opt, '-c', cpp, '-o', obj])
                        extra_objects = [] if case in ('values', 'threads') else [work / (cc + '-runtime.o'), work / (cc + '-observer.o')]
                        library = runtime / ('libfbmt.a' if case == 'threads' else 'libfb.a')
                        run(name + '-link', [tools / (cc + '.exe'), obj, *extra_objects, runtime / 'fbrt0.o', library, '-o', exe, '-lm', '-mthreads'])
                        result = run(name + '-execute', [exe], 4 if case == 'allocation-failure' else 0)
                        if case != 'allocation-failure':
                            marker = 'GOSUB_' + ('VALUES' if case == 'values' else 'THREADS' if case == 'threads' else 'LIFECYCLE') + '_PASS'
                            assert result.stdout.split() == [marker]
                        counts[case] += 1
            print('C_GOSUB_NATIVE_CASE_PASS', tag, flush=True)
    for cc in ('gcc', 'clang'):
        for standard in ('gnu11', 'gnu23'):
            for opt in ('0', '2'):
                name = 'api-' + cc + '-' + standard + '-O' + opt
                flags = [flag for flag in profile[cc + '_flags'] if flag != '-nostdinc']
                flags += ['-std=' + standard, '-O' + opt, '-I', rtl]
                run(name + '-runtime', [tools / (cc + '.exe'), *flags, '-c', observed, '-o', work / 'api-runtime.o'])
                run(name + '-object', [tools / (cc + '.exe'), *flags, '-c', fixtures / 'c-gosub-runtime-targets.c', '-o', obj])
                run(name + '-link', [tools / (cc + '.exe'), obj, work / 'api-runtime.o', runtime / 'fbrt0.o', runtime / 'libfb.a', '-o', exe, '-lm', '-mthreads'])
                result = run(name + '-execute', [exe])
                assert result.stdout.split() == ['GOSUB_RUNTIME_TARGETS_PASS']
                counts['runtime-api'] += 1
    assert all(digest(Path(path)) == sha for path, sha in pins.items())
    assert all(digest(Path(path)) == sha for path, sha in profile.get('installed_sha256', {}).items())
    receipt = {'checks_complete': True, 'native_runs': dict(counts), 'input_sha256': pins,
        'allocation_failure_exit': 4, 'balanced_owner_exit_nodes_per_execution': 1000}
    (work / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    db.close()
    for path in (cpp, obj, exe, work / 'api-runtime.o', *list(work.glob('*-runtime.o')), *list(work.glob('*-observer.o'))):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    print('C_GOSUB_VALUES_QUALIFIED', dict(counts), flush=True)


if __name__ == '__main__':
    main()

# end of c-gosub-values.py
