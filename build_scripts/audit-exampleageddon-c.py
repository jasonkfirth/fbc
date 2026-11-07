#!/usr/bin/env python3
"""Project: FreeBASIC C backend diagnostic audit
File: audit-exampleageddon-c.py
Purpose: Add the complete Exampleageddon inventory to the emitted-C warning queues.
Responsibilities: Preserve classifications, source pins, unsuppressed diagnostics and bounded storage.
This file intentionally does NOT link, execute examples, copy assets or install binaries.
"""
from collections import Counter
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import sqlite3
import subprocess
import sys
import time
import zlib

ROOT = Path(__file__).resolve().parent.parent
TOOLS = Path('C:/freebasic-js/toolchain/ucrt64/bin')
CAP_BYTES = 64 * 1024 * 1024


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_context(path):
    """Use the example's existing include owner and library build contract."""
    path = path.resolve()
    relative = path.relative_to(ROOT).as_posix()
    arguments = []
    for directory in ('examples/win32/variants', 'examples/graphics/SDL3'):
        owner = ROOT / directory
        if path.is_relative_to(owner) and path.parent != owner:
            arguments += ['-i', str(owner)]
    variant_module = path.is_relative_to(ROOT / 'examples/win32/variants/libvariant')
    if variant_module:
        # The archive's GNUmakefile selects the Windows VARIANT declarations.
        # Its public consumer interface alone cannot compile the implementation.
        arguments += ['-d', 'VARIANT_ISBUILDING']
    if relative == 'examples/manual/module/opts.bas':
        arguments += ['-d', 'TEST']
    return {'arguments': arguments, 'main': not variant_module and
            relative != 'examples/win32/winmm/winmmhlp.bas'}


def load_runner():
    spec = importlib.util.spec_from_file_location('exampleageddon_c_inventory', ROOT / 'build_scripts/exampleageddon-freebasic.py')
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def retained_bytes(folder):
    return sum(p.stat().st_size for p in folder.rglob('*') if p.is_file())


def run_command(command, cwd, env, timeout):
    """Only terminate the owned process tree if its deadline expires.

    FBC -r has no assembler child. GCC/Clang drivers can have children; retain
    the parent handle until taskkill has terminated that exact owned tree.
    """
    started = time.monotonic()
    process = subprocess.Popen(list(map(str, command)), cwd=cwd, env=env,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
    timed_out = False
    try:
        output, _ = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        timed_out = True
        if process.poll() is None:
            subprocess.run([str(Path(os.environ['SystemRoot']) / 'System32/taskkill.exe'),
                '/PID', str(process.pid), '/T', '/F'], capture_output=True, timeout=15)
        output, _ = process.communicate(timeout=15)
    text = output.decode(errors='replace')
    warnings = Counter(re.findall(r'\[(-W[^\]]+)\]', '\n'.join(
        line for line in text.splitlines() if 'warning:' in line)))
    return {'exit': process.returncode, 'timeout': timed_out,
            'seconds': round(time.monotonic() - started, 3), 'warnings': dict(warnings)}, output


def report(db, out, inventory):
    totals = {name: {cc: Counter() for cc in ('gcc', 'clang')} for name in ('default', 'checked')}
    statuses = {name: Counter() for name in totals}
    samples = {name: {cc: {} for cc in ('gcc', 'clang')} for name in totals}
    for profile, source, status, encoded in db.execute('SELECT profile,source,status,result FROM units'):
        row = json.loads(encoded)
        statuses[profile][status] += 1
        for cc in ('gcc', 'clang'):
            warnings = row.get('checks', {}).get(cc, {}).get('warnings', {})
            totals[profile][cc].update(warnings)
            for flag, count in warnings.items():
                samples[profile][cc].setdefault(flag, []).append({'source': source, 'count': count,
                    'source_bytes': row['source_bytes'], 'classification': row['classification']})
    for profile in samples.values():
        for cc in profile.values():
            for flag in cc:
                cc[flag] = sorted(cc[flag], key=lambda x: (x['source_bytes'], x['source']))[:8]
    completed = sum(sum(counts.values()) for counts in statuses.values())
    result = {'inventory_sources': len(inventory), 'planned_profiles': 2 * len(inventory),
        'completed_profiles': completed, 'checks_complete': completed == 2 * len(inventory),
        'statuses': statuses, 'warning_classes': totals,
        'warning_totals': {p: {cc: sum(counts.values()) for cc, counts in tools.items()} for p, tools in totals.items()},
        'smallest_warning_examples': samples, 'programs_executed': 0, 'programs_linked': 0,
        'png_excluded': True, 'retained_bytes': retained_bytes(out), 'resource_cap_bytes': CAP_BYTES,
        'overall_goal_complete': False}
    (out / 'summary.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'build/c-emitter-exampleageddon-20261103')
    parser.add_argument('--accepted', type=Path, default=ROOT / 'build/c-emitter-c-gosub-20261103')
    parser.add_argument('--limit', type=int, help='Process at most this many new profile rows; a later run resumes.')
    args = parser.parse_args()
    out, accepted = args.out.resolve(), args.accepted.resolve()
    assert out.is_relative_to(ROOT / 'build') and out != ROOT / 'build'
    out.mkdir(exist_ok=True)
    compiler = accepted / 'canonical/fbc.exe'
    prefix = accepted / 'prefix'
    prior = sqlite3.connect((accepted / 'canonical/results.sqlite3').as_uri() + '?mode=ro', uri=True)
    profile = json.loads(prior.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    prior.close()
    runner = load_runner()
    inventory = []
    for path in sorted((ROOT / 'examples').rglob('*.bas')):
        relative = path.relative_to(ROOT).as_posix()
        classification = runner.classify(path, ROOT, 'windows', True)
        text = path.read_text(encoding='utf-8', errors='replace')
        inventory.append({'source': relative, 'sha256': digest(path), 'source_bytes': path.stat().st_size,
            'compile_context': source_context(path),
            'classification': classification.group, 'classification_reason': classification.reason,
            'excluded_png': 'png' in relative.lower() or bool(re.search(r'\b(?:png|png_[a-z0-9_]+|loadpng)\b', text, re.I)),
            'included_owner': runner.INCLUDED_SOURCE_OWNERS.get(relative)})
    source_pins = {str(p): digest(p) for p in (ROOT / 'examples').rglob('*')
                   if p.is_file() and p.suffix.lower() in ('.bas', '.bi', '.bif', '.inc', '.h')}
    flags = {cc: profile[cc + '_flags'] for cc in ('gcc', 'clang')}
    assert all(not any(x == '-w' or x.startswith('-Wno-') for x in values) for values in flags.values())
    runtime = prefix / 'lib/freebasic/win64'
    pins = {str(p): digest(p) for p in [Path(__file__), ROOT / 'build_scripts/exampleageddon-freebasic.py',
        ROOT / 'examples/win32/variants/libvariant/GNUmakefile', compiler,
        TOOLS / 'gcc.exe', TOOLS / 'clang.exe', *list(runtime.iterdir())]}
    inputs = {'sha256': pins, 'example_source_sha256': source_pins,
        'canonical_source_sha256': profile['source_sha256'], 'installed_sha256': profile['installed_sha256'],
        'C_flags': flags, 'inventory': inventory, 'source_date_epoch': profile['source_date_epoch'],
        'profiles': {'default': [], 'checked': ['-exx']},
        'translation_units_only': True, 'no_dependency_builds': True, 'heavy_worker_cap': 1}
    assert all(digest(ROOT / name) == sha for name, sha in inputs['canonical_source_sha256'].items())
    db = sqlite3.connect(out / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS metadata(name TEXT PRIMARY KEY,value TEXT)')
    db.execute('CREATE TABLE IF NOT EXISTS units(profile TEXT,ordinal INTEGER,source TEXT,status TEXT,result TEXT,PRIMARY KEY(profile,source))')
    db.execute('CREATE TABLE IF NOT EXISTS C(sha256 TEXT PRIMARY KEY,raw_bytes INTEGER,content BLOB)')
    db.execute('CREATE TABLE IF NOT EXISTS checks(sha256 TEXT,tool TEXT,result TEXT,log BLOB,PRIMARY KEY(sha256,tool))')
    db.execute('CREATE TABLE IF NOT EXISTS frontend(profile TEXT,source TEXT,log BLOB,PRIMARY KEY(profile,source))')
    saved = db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()
    assert saved is None or json.loads(saved[0]) == inputs, 'Input drift; preserve this queue and choose a new output directory.'
    db.execute('INSERT OR IGNORE INTO metadata VALUES(?,?)', ('inputs', json.dumps(inputs)))
    db.commit()
    (out / 'plan.json').write_text(json.dumps({'inventory': inventory, 'profiles': inputs['profiles'],
        'source_sha256': source_pins, 'translation_units_only': True}, indent=2) + '\n')
    env = dict(os.environ)
    env['PATH'] = str(TOOLS) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = inputs['source_date_epoch']
    cpp, obj = out / 'unit.c', out / 'unit.o'
    order = sorted(enumerate(inventory), key=lambda pair: (
        pair[1]['classification'] != 'self-contained', pair[1]['source_bytes'], pair[1]['source']))
    processed = 0
    for ordinal, unit in order:
        for name, extra in inputs['profiles'].items():
            if db.execute('SELECT 1 FROM units WHERE profile=? AND source=?', (name, unit['source'])).fetchone():
                continue
            if args.limit is not None and processed >= args.limit:
                break
            assert retained_bytes(out) < CAP_BYTES
            path = ROOT / unit['source']
            assert digest(path) == unit['sha256'], unit['source']
            row = {'classification': unit['classification'], 'classification_reason': unit['classification_reason'],
                   'source_bytes': unit['source_bytes'], 'checks': {}}
            if unit['excluded_png']:
                status = 'excluded-png'
            elif unit['included_owner']:
                status = 'included-by-owner'
                row['owner'] = unit['included_owner']
            else:
                command = [compiler, '-prefix', prefix, '-i', ROOT / 'inc', '-i', path.parent,
                           '-gen', 'gcc', *extra, *runner.EXAMPLE_FBC_ARGUMENTS.get(unit['source'], ()),
                           *unit['compile_context']['arguments']]
                if unit['source'] in runner.LIBRARY_MODULES:
                    mode, module_flags = runner.LIBRARY_MODULES[unit['source']]
                    command += [mode, *module_flags]
                elif unit['compile_context']['main']:
                    command += ['-m', path.stem]
                if unit['source'].lower().startswith('examples/gui/gtk+/goocanvas/'):
                    command += ['-d', '__USE_GTK3__']
                command += ['-r', path, '-o', cpp]
                assert cpp.resolve().parent == obj.resolve().parent == out
                cpp.unlink(missing_ok=True)
                emitted, log = run_command(command, path.parent, env, 90)
                row['frontend'] = emitted
                row['frontend_command'] = list(map(str, command))
                db.execute('INSERT INTO frontend VALUES(?,?,?)', (name, unit['source'], zlib.compress(log)))
                if emitted['timeout'] or emitted['exit'] != 0 or not cpp.is_file():
                    status = 'frontend-timeout' if emitted['timeout'] else 'frontend-failed'
                else:
                    data = cpp.read_bytes()
                    c_sha = hashlib.sha256(data).hexdigest()
                    row['C_sha256'] = c_sha
                    row['C_bytes'] = len(data)
                    db.execute('INSERT OR IGNORE INTO C VALUES(?,?,?)', (c_sha, len(data), zlib.compress(data)))
                    for cc in ('gcc', 'clang'):
                        old = db.execute('SELECT result FROM checks WHERE sha256=? AND tool=?', (c_sha, cc)).fetchone()
                        if old:
                            checked = json.loads(old[0])
                        else:
                            assert retained_bytes(out) < CAP_BYTES
                            obj.unlink(missing_ok=True)
                            checked, c_log = run_command([TOOLS / (cc + '.exe'), *flags[cc], '-c', cpp, '-o', obj], ROOT, env, 90)
                            checked['fresh_object'] = obj.is_file() and obj.stat().st_size > 0
                            db.execute('INSERT INTO checks VALUES(?,?,?,?)', (c_sha, cc, json.dumps(checked), zlib.compress(c_log)))
                        row['checks'][cc] = checked
                    status = 'C-passed' if all(r['exit'] == 0 and not r['timeout'] and r['fresh_object']
                                             for r in row['checks'].values()) else 'C-failed'
                assert digest(path) == unit['sha256']
                cpp.unlink(missing_ok=True)
                obj.unlink(missing_ok=True)
            db.execute('INSERT INTO units VALUES(?,?,?,?,?)', (name, ordinal, unit['source'], status, json.dumps(row)))
            db.commit()
            processed += 1
            if processed % 25 == 0:
                summary = report(db, out, inventory)
                print('EXAMPLEAGEDDON_C_PROGRESS', summary['completed_profiles'], summary['warning_totals'], flush=True)
        if args.limit is not None and processed >= args.limit:
            break
    assert all(digest(Path(path)) == sha for path, sha in source_pins.items())
    assert all(digest(Path(path)) == sha for path, sha in pins.items())
    assert all(digest(Path(path)) == sha for path, sha in inputs['installed_sha256'].items())
    assert all(digest(ROOT / path) == sha for path, sha in inputs['canonical_source_sha256'].items())
    summary = report(db, out, inventory)
    assert summary['retained_bytes'] < CAP_BYTES
    db.close()
    print('EXAMPLEAGEDDON_C_QUEUE', json.dumps({k: summary[k] for k in (
        'inventory_sources', 'planned_profiles', 'completed_profiles', 'checks_complete', 'statuses', 'warning_totals')}), flush=True)


if __name__ == '__main__':
    main()

# end of audit-exampleageddon-c.py
