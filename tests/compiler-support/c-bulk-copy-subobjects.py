"""Project: FreeBASIC C backend regression tests
File: c-bulk-copy-subobjects.py
Purpose: Preserve a valid containing-record copy that GCC diagnoses after optimization.
Responsibilities: Bind SDK inputs, retain warnings and execute ordinary/checked copies.
This file intentionally does NOT suppress the GCC diagnostic or alter generated C.
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
    for name in ('gcc', 'clang', 'work-dir', 'diagnostic-profile'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(exist_ok=True)
    source = Path(__file__).with_suffix('.c')
    tools = {cc: getattr(args, cc).resolve() for cc in ('gcc', 'clang')}
    with sqlite3.connect(args.diagnostic_profile) as db:
        profile = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    env = os.environ.copy()
    env['PATH'] = str(tools['gcc'].parent) + os.pathsep + env.get('PATH', '')
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('CREATE TABLE IF NOT EXISTS checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')

    def run(name, command):
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 8 * 1024 * 1024
        options = {'creationflags': subprocess.BELOW_NORMAL_PRIORITY_CLASS} if os.name == 'nt' else {}
        p = subprocess.run([str(x) for x in command], cwd=work, env=env, capture_output=True, text=True,
            errors='replace', timeout=120, **options)
        output = p.stdout + p.stderr
        row = {'command': [str(x) for x in command], 'exit': p.returncode, 'stdout': p.stdout,
            'warnings': dict(Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(s for s in output.splitlines() if 'warning:' in s))))}
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)', (name, json.dumps(row), zlib.compress(output.encode())))
        db.commit()
        assert p.returncode == 0, name + '\n' + output[-3000:]
        return row

    dependencies = set()
    for cc in tools:
        for width in (64, 32):
            row = run(f'deps-{cc}-{width}', [tools[cc], '-std=gnu23', '-m' + str(width), '-M', source])
            paths = row['stdout'].replace('\\\n', ' ').split(': ', 1)[1].split()
            assert not any('\\ ' in n for n in paths)
            dependencies.update(Path(p).resolve() for p in paths)
    files = [source, Path(__file__), *tools.values(), *sorted(dependencies)]
    pins = {str(p): digest(p) for p in files}
    inputs = {'source_tool_SDK_sha256': pins, 'gcc_flags': profile['gcc_flags'], 'clang_flags': profile['clang_flags']}
    previous = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    assert previous is None or json.loads(previous[0]) == inputs
    db.execute('INSERT OR IGNORE INTO metadata VALUES(?,?)', ('inputs', json.dumps(inputs)))
    db.commit()
    exe, obj = work / 'copy.exe', work / 'copy.o'
    native = instrumented_runs = cross_objects = 0
    for cc in tools:
        for standard in ('gnu11', 'gnu23'):
            for opt in ('0', '2'):
                for checked in (False, True):
                    tag = cc + '-' + standard + '-O' + opt + ('-checked' if checked else '-ordinary')
                    flags = [f for f in profile[cc + '_flags'] if f != '-nostdinc'] + ['-std=' + standard, '-O' + opt]
                    assert not any(f == '-w' or f.startswith('-Wno-') for f in flags)
                    extra = []
                    if checked:
                        extra = ['-fsanitize=pointer-overflow,object-size,alignment']
                        extra += ['-fsanitize-trap=pointer-overflow,object-size,alignment'] if cc == 'clang' else ['-fsanitize-undefined-trap-on-error']
                    result = run(tag + '-compile', [tools[cc], *flags, *extra, source, '-o', exe])
                    if not checked:
                        count = result['warnings'].get('-Warray-bounds=', 0)
                        assert count == int(cc == 'gcc' and opt == '2')
                    result = run(tag + '-execute', [exe])
                    assert result['stdout'].strip() == 'BULK_SUBOBJECT_COPY=PASS'
                    native += 1
                    instrumented_runs += int(checked)
            flags = [f for f in profile[cc + '_flags'] if f != '-nostdinc']
            run(cc + '-' + standard + '-win32-object', [tools[cc], *flags, '-std=' + standard, '-O2', '-m32', '-c', source, '-o', obj])
            cross_objects += 1
    assert all(digest(Path(p)) == sha for p, sha in pins.items())
    receipt = {'checks_complete': True, 'native_runs': native, 'instrumented_runs': instrumented_runs,
        'Win32_objects': cross_objects, 'SDK_headers_bound': len([p for p in dependencies if p.suffix == '.h']),
        'GCC_O2_false_positive_reproduced': True, 'overall_goal_complete': False}
    (work / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    for path in (exe, obj):
        assert path.resolve().parent == work
        path.unlink(missing_ok=True)
    db.close()
    print('BULK_SUBOBJECT_CONTRACT_PASS', receipt, flush=True)


if __name__ == '__main__':
    main()

# end of c-bulk-copy-subobjects.py
