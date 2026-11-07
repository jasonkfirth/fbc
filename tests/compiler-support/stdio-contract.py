"""Project: FreeBASIC C stdio regression tests
File: stdio-contract.py
Purpose: Check native stream ABI, C type binding and FILE-named exports.
Responsibilities: Compare optional baseline diagnostics, run fixed small file
round trips and check Win32 object exports with one owned scratch set.
This file intentionally does NOT install tools or inspect stream internals.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import sqlite3
import subprocess
import sys
import zlib

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'prefix', 'gcc', 'clang', 'work-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--full', action='store_true')
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    owner = work / 'stdio-contract-work.json'
    identity = {'owner': 'fbc-stdio-contract'}
    if owner.is_file():
        if json.loads(owner.read_text()) != identity:
            raise RuntimeError('work directory belongs to another test')
    elif any(work.iterdir()):
        raise RuntimeError('choose an empty work directory for the first run')
    else:
        owner.write_text(json.dumps(identity))
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    compilers = [('candidate', args.fbc.resolve())]
    if args.baseline:
        compilers.insert(0, ('baseline', args.baseline.resolve()))
    nm = args.gcc.resolve().parent / 'nm.exe'
    paths = [HERE / 'stdio-contract.bas', HERE / 'stdio-contract.c', args.gcc.resolve(),
             args.clang.resolve(), nm, runtime / 'fbrt0.o', runtime / 'libfb.a',
             *[p for _, p in compilers], *list((ROOT / 'inc').rglob('*.bi'))]
    pins = {str(p): digest(p) for p in paths}
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('CREATE TABLE IF NOT EXISTS checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')
    provenance = {'source_tool_runtime_sha256': pins, 'runner_sha256': digest(Path(__file__)), 'full': args.full}
    saved = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    if saved and json.loads(saved[0]) != provenance:
        raise RuntimeError('saved qualification inputs changed')
    if not saved:
        db.execute('INSERT INTO metadata VALUES(?,?)', ('inputs', json.dumps(provenance)))
        db.commit()
    env = os.environ.copy()
    env['PATH'] = str(args.gcc.resolve().parent) + os.pathsep + env.get('PATH', '')
    priority = subprocess.BELOW_NORMAL_PRIORITY_CLASS if sys.platform == 'win32' else 0

    def run(name, command):
        result = subprocess.run([str(p) for p in command], cwd=work, env=env,
                                capture_output=True, text=True, errors='replace', timeout=120,
                                creationflags=priority)
        log = result.stdout + result.stderr
        if len(log) > 8 * 1024 * 1024:
            raise RuntimeError('diagnostic output limit exceeded')
        warnings = Counter(re.findall(r'\[(-W[^\]]+)\]', log))
        row = {'command': [str(p) for p in command], 'exit': result.returncode, 'warnings': dict(warnings)}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)',
                   (name, json.dumps(row), zlib.compress(log.encode(), 6)))
        db.commit()
        if result.returncode:
            raise RuntimeError(name + ': ' + log[-3000:])
        return row, log

    common = ['-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-frounding-math', '-fno-math-errno']
    gcc_flags = ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion',
                 '-Wcast-qual', '-Wcast-align=strict', '-Wpointer-arith', '-Wstrict-prototypes',
                 '-Wmissing-prototypes', '-Wmissing-declarations', '-Wold-style-definition',
                 '-Wformat=2', '-Wformat-signedness', '-Wundef', '-Wshadow', '-Wwrite-strings',
                 '-Warray-bounds=2', '-Wstringop-overflow=4', '-Wuninitialized', '-Wmaybe-uninitialized']
    tools = [('gcc', args.gcc.resolve(), gcc_flags),
             ('clang', args.clang.resolve(), ['-Weverything', '-Wwrite-strings'])]
    cfile, obj, bridge, executable, datafile = [work / n for n in ('unit.c', 'unit.o', 'bridge.o', 'test.exe', 'roundtrip.txt')]
    native, objects, emissions, diagnostics = [], [], {}, {}
    for label, fbc in compilers:
        for opaque in (False, True):
            for procedure in (False, True):
                variant = ('opaque' if opaque else 'record') + ('-procedure' if procedure else '-object')
                defines = (['-d', 'STDIO_OPAQUE'] if opaque else []) + (['-d', 'STDIO_FILE_PROC'] if procedure else [])
                base = [fbc, '-prefix', args.prefix.resolve(), '-i', ROOT / 'inc', '-gen', 'gcc', '-asm', 'intel', *defines]
                for target in ('win64', 'win32'):
                    cfile.unlink(missing_ok=True)
                    run(label + '-' + variant + '-' + target + '-emit',
                        [*base, '-target', target, '-m', 'stdio-contract', '-r', HERE / 'stdio-contract.bas', '-o', cfile])
                    if not cfile.is_file() or cfile.stat().st_size > 64 * 1024 * 1024:
                        raise RuntimeError('invalid fresh C output')
                    text = cfile.read_text()
                    has_file = bool(re.match(r'typedef (?:struct |union |void\b).*\bFILE\b', text))
                    if has_file != (label == 'candidate'):
                        raise RuntimeError('unexpected FILE type binding')
                    emissions[label + '-' + variant + '-' + target] = digest(cfile)
                    for cc, tool, flags in tools:
                        standards = ('gnu11', 'gnu23') if args.full and target == 'win64' else ('gnu23',)
                        opts = ('-O0', '-O2') if args.full and target == 'win64' else ('-O2',)
                        for standard in standards:
                            for opt in opts:
                                name = '-'.join((label, variant, target, cc, standard, opt))
                                target_flags = ['-m32'] if target == 'win32' else []
                                row, _ = run(name + '-object', [tool, *common, *flags, '-std=' + standard, opt,
                                                                 *target_flags, '-c', cfile, '-o', obj])
                                diagnostics[(label, variant, target, cc, standard, opt)] = row['warnings']
                                objects.append(name)
                                _, symbols = run(name + '-symbols', [nm, '-g', '--defined-only', obj])
                                symbol = '_FILE' if target == 'win32' else 'FILE'
                                kind = 'T' if procedure else 'D'
                                if not re.search(r'\b' + kind + r'\s+' + symbol + r'\s*$', symbols, re.M):
                                    raise RuntimeError('external FILE symbol changed')
                                if target == 'win32':
                                    continue
                                bridge_flags = ['-DSTDIO_FILE_PROC'] if procedure else []
                                run(name + '-bridge', [tool, *flags, '-std=' + standard, opt, *bridge_flags,
                                                       '-c', HERE / 'stdio-contract.c', '-o', bridge])
                                run(name + '-link', [tool, '-o', executable, runtime / 'fbrt0.o', obj, bridge,
                                                     '-L' + str(runtime), '-lfb', '-lm', '-mthreads'])
                                datafile.unlink(missing_ok=True)
                                run(name + '-execute', [executable, datafile])
                                if datafile.read_bytes() != b'FreeBASIC stdio!?':
                                    raise RuntimeError('native file contents changed')
                                datafile.unlink()
                                native.append(name)
                print('STDIO_CONTRACT', label, variant, flush=True)
    if args.baseline:
        for key, after in diagnostics.items():
            if key[0] != 'candidate':
                continue
            before = Counter(diagnostics[('baseline', *key[1:])])
            before.pop('-Wbuiltin-requires-header', None)
            if Counter(after) != before:
                raise RuntimeError('unexpected warning delta: ' + str(key))
    if any(digest(Path(path)) != value for path, value in pins.items()):
        raise RuntimeError('source, tool or runtime changed during qualification')
    receipt = {'checks_complete': True, 'native_runs': native, 'object_checks': objects,
               'C_sha256': emissions, 'warning_groups': {'|'.join(k): v for k, v in diagnostics.items()},
               'inputs_sha256': digest(work / 'results.sqlite3'), 'scope': 'Win64 execution and Win32 objects'}
    (work / 'receipt.json').write_text(json.dumps(receipt, indent=2))
    db.close()
    for path in (cfile, obj, bridge, executable, datafile):
        path.unlink(missing_ok=True)
    print('STDIO_CONTRACT_PASS native=' + str(len(native)) + ' objects=' + str(len(objects)), flush=True)


if __name__ == '__main__':
    main()

# end of stdio-contract.py
