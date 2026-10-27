"""Project: FreeBASIC compiler verification
File: audit-c-emitter.py
Purpose: Expose diagnostics hidden by the normal C backend build flags.
Responsibilities: Replay explicit BASIC roots, retain C, run unsuppressed C checks,
    and save resumable, compiler-pinned receipts with bounded resource use.
This file intentionally does NOT repair project sources or classify warnings as bugs.
"""

from __future__ import annotations

import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time


# Generated C depends on these language semantics. Removing them would audit a
# different program: FB permits pointer aliasing and wrapping signed arithmetic.
SEMANTICS = ['-std=gnu11', '-nostdinc', '-fno-strict-aliasing', '-fwrapv',
             '-frounding-math', '-fno-math-errno']
GCC_WARNINGS = ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion',
                '-Wshadow=local', '-Wcast-qual', '-Wcast-align=strict', '-Wpointer-arith',
                '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wmissing-declarations',
                '-Wold-style-definition', '-Wformat=2', '-Wformat-signedness', '-Wundef',
                '-Wwrite-strings', '-Wduplicated-cond', '-Wduplicated-branches', '-Wlogical-op',
                '-Wnull-dereference', '-Wjump-misses-init', '-Wbad-function-cast',
                '-Wdouble-promotion', '-Wfloat-equal', '-Wvla', '-Walloca',
                '-Warray-bounds=2', '-Wstringop-overflow=4', '-Wstringop-overread',
                '-Wshift-overflow=2', '-Wtrampolines', '-Wdate-time']
DIAGNOSTIC = re.compile(r'^(.+?):(\d+)(?::(\d+))?: (warning|error|fatal error|style|performance|portability|information): (.*?)(?: \[([^]]+)\])?$')


def digest(path: Path) -> str:
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write_json(path: Path, value) -> None:
    pending = path.with_suffix(path.suffix + '.tmp')
    pending.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')
    pending.replace(path)


def invoke(command: list[str], cwd: Path, log: Path, timeout: int) -> dict:
    """No shell interprets project arguments; each stage has its own receipt."""
    start = time.monotonic()
    with log.open('wb') as stream:
        process = subprocess.Popen(command, cwd=cwd, stdin=subprocess.DEVNULL,
                                   stdout=stream, stderr=subprocess.STDOUT)
        try:
            status = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            # GCC can launch cc1. Kill only this owned process tree on Windows.
            if os.name == 'nt':
                subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
            else:
                process.kill()
            process.wait()
            status = 124
    text = log.read_text(encoding='utf-8', errors='replace')
    diagnostics = []
    for line in text.splitlines():
        match = DIAGNOSTIC.match(line)
        if match:
            file, row, col, severity, message, flag = match.groups()
            diagnostics.append({'file': file, 'line': int(row), 'column': int(col or 0),
                                'severity': severity, 'message': message, 'flag': flag or ''})
    compressed = log.with_suffix(log.suffix + '.gz')
    with log.open('rb') as source, gzip.open(compressed, 'wb') as target:
        shutil.copyfileobj(source, target)
    log.unlink()
    details = log.with_suffix('.diagnostics.json.gz')
    with gzip.open(details, 'wt', encoding='utf-8') as stream:
        json.dump(diagnostics, stream)
    buckets = Counter((row['severity'], row['flag']) for row in diagnostics)
    samples = []
    seen = Counter()
    for row in diagnostics:
        key = row['severity'], row['flag']
        if seen[key] < 3:
            samples.append(row)
        seen[key] += 1
    return {'command': command, 'cwd': str(cwd), 'exit': status,
            'seconds': round(time.monotonic() - start, 3), 'log': str(compressed),
            'diagnostics_file': str(details), 'samples': samples,
            'counts': [{'severity': key[0], 'flag': key[1], 'count': count}
                       for key, count in sorted(buckets.items())],
            'log_bytes': len(text.encode('utf-8'))}


def run_unit(item: dict, options, identity: str) -> dict:
    work = options.output / item['id']
    work.mkdir(exist_ok=True)
    receipt = work / 'result.json'
    source = Path(item['source'])
    signature = digest(source) if source.is_file() else 'missing'
    pin = hashlib.sha256(json.dumps([item, signature, identity, options.timeout,
                                   options.structural], sort_keys=True).encode()).hexdigest()
    if receipt.is_file():
        old = json.loads(receipt.read_text())
        if old['pin'] != pin:
            raise RuntimeError('Inputs changed; select a new output directory: ' + str(source))
        return old
    result = {'id': item['id'], 'source': str(source), 'group': item['group'],
              'project': item['project'], 'pin': pin, 'source_sha256': signature}
    if not source.is_file():
        result['status'] = 'missing-input'
        write_json(receipt, result)
        return result
    # -o also relocates the intermediate C selected by -r. Never emit beside
    # somebody else's BASIC source, and never copy whole project asset trees.
    command = [str(options.fbc), '-prefix', str(options.root), '-gen', 'gcc', '-r',
               '-maxerr', '10', *item.get('flags', []), '-i', str(options.root / 'inc'),
               str(source), '-o', str(work / 'unit.o')]
    result['emit'] = invoke(command, Path(item['cwd']), work / 'emit.log', options.timeout)
    cfile = work / 'unit.c'
    if result['emit']['exit'] or not cfile.is_file():
        result['status'] = 'emission-failed'
        cfile.unlink(missing_ok=True)
        write_json(receipt, result)
        return result
    result.update(status='emitted', c_sha256=digest(cfile), c_bytes=cfile.stat().st_size)
    for name in ('gcc', 'clang'):
        compiler = str(getattr(options, name))
        flags = (GCC_WARNINGS + ['-fno-diagnostics-show-caret', '-fmax-errors=20'] if name == 'gcc'
                 else ['-Weverything', '-Wwrite-strings', '-fno-caret-diagnostics', '-ferror-limit=20'])
        # Compile through optimization to assembly in the null sink: GCC's
        # uninitialized and bounds diagnostics need more than -fsyntax-only.
        command = [compiler, *SEMANTICS, '-O2', *flags, '-fdiagnostics-color=never',
                   '-S', str(cfile), '-o', os.devnull]
        result[name] = invoke(command, work, work / (name + '.log'), options.timeout)
    if options.structural:
        command = [str(options.cppcheck), '--enable=all', '--inconclusive', '--std=c11',
                   '--platform=win64', '--template={file}:{line}:{column}: {severity}: {message} [{id}]',
                   '--max-configs=1', str(cfile)]
        result['cppcheck'] = invoke(command, work, work / 'cppcheck.log', options.timeout)
    write_json(receipt, result)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--artifact-root', type=Path,
                        help='allow a separate artifact tree while retaining the compiler root')
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    parser.add_argument('--cppcheck', type=Path)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--jobs', type=int, choices=(1, 2), default=2)
    parser.add_argument('--timeout', type=int, default=30)
    parser.add_argument('--limit', type=int, default=0)
    parser.add_argument('--structural', action='store_true')
    parser.add_argument('--disk-budget-mib', type=int, default=2048)
    options = parser.parse_args()
    for name in ('root', 'output', 'artifact_root', 'plan', 'fbc', 'gcc', 'clang', 'cppcheck'):
        value = getattr(options, name)
        if value is not None:
            setattr(options, name, value.resolve())
    artifact_root = options.artifact_root or options.root / 'build'
    if artifact_root.is_relative_to(options.root) and not artifact_root.is_relative_to(options.root / 'build'):
        parser.error('artifact root inside the checkout must stay below build/')
    if not options.output.is_relative_to(artifact_root):
        parser.error('output must be below the artifact root')
    if options.timeout < 1 or options.disk_budget_mib < 1:
        parser.error('timeout and disk budget must be positive')
    for name in ('fbc', 'gcc', 'clang') + (('cppcheck',) if options.structural else ()):
        if getattr(options, name) is None or not getattr(options, name).is_file():
            parser.error('executable missing: ' + name)
    options.output.mkdir(parents=True, exist_ok=True)
    lock = options.output / '.running'
    # Exclusive creation prevents concurrent writers from corrupting a run.
    with lock.open('x') as stream:
        stream.write(str(os.getpid()))
    try:
        plan = json.loads(options.plan.read_text(encoding='utf-8'))
        items = plan['units'][:options.limit or None]
        ids = [item['id'] for item in items]
        if len(ids) != len(set(ids)) or any(not re.fullmatch(r'[A-Za-z0-9_-]+', value) for value in ids):
            raise ValueError('Unit ids must be unique directory names')
        tools = {}
        for name in ('fbc', 'gcc', 'clang', 'cppcheck'):
            path = getattr(options, name)
            if path:
                version = subprocess.run([str(path), '-version' if name == 'fbc' else '--version'],
                                         capture_output=True, text=True, errors='replace', timeout=10)
                tools[name] = {'path': str(path), 'sha256': digest(path),
                               'version': version.stdout + version.stderr}
        identity = hashlib.sha256(json.dumps([tools, SEMANTICS, GCC_WARNINGS, plan.get('input_identity'),
                                             digest(Path(__file__))], sort_keys=True).encode()).hexdigest()
        write_json(options.output / 'tools.json', tools)
        counts = Counter()
        def checked(item):
            if shutil.disk_usage(options.output).free < 2 * 1024**3:
                raise RuntimeError('Less than 2 GiB free; stopped before another unit')
            return run_unit(item, options, identity)
        with ThreadPoolExecutor(max_workers=options.jobs) as pool:
            # Small batches bound queued work and allow a real disk-budget check.
            for offset in range(0, len(items), options.jobs):
                for result in pool.map(checked, items[offset:offset + options.jobs]):
                    counts[result['status']] += 1
                if offset % 50 < options.jobs:
                    print('C_AUDIT_PROGRESS', sum(counts.values()), len(items), dict(counts), flush=True)
                if offset % 100 < options.jobs:
                    used = sum(p.stat().st_size for p in options.output.rglob('*') if p.is_file())
                    if used > options.disk_budget_mib * 1024**2:
                        raise RuntimeError('Audit disk budget reached; completed receipts remain resumable')
        write_json(options.output / 'summary.json', {'planned': len(items), 'counts': dict(counts),
                   'identity': identity, 'plan_sha256': digest(options.plan)})
        print('C_AUDIT_COMPLETE', len(items), dict(counts), flush=True)
        return 0
    finally:
        lock.unlink(missing_ok=True)


if __name__ == '__main__':
    raise SystemExit(main())

# end of audit-c-emitter.py
