#!/usr/bin/env python3
"""Project: FreeBASIC compiler qualification
File: lint-compiler-sources.py
Purpose: Complete resumable semantic lint of Make-selected compiler modules.
Responsibilities: Preserve real build context, source freshness and bounded scratch.
This file intentionally does NOT build/install tools or treat source-only lint as proof.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tests/semantic-sidecar'))
from semantic_binary import read_rows
from sidecar import unescape


def digest(path: Path) -> str:
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def native_path(value: str) -> Path:
    # Make was queried in MSYS. FBC and fblint are native Windows programs.
    if len(value) > 3 and value[0] == '/' and value[2] == '/' and value[1].isalpha():
        value = value[1].upper() + ':' + value[2:]
    return Path(value).resolve()


def save(path: Path, value: dict) -> None:
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')
    temporary.replace(path)


def model_sources(model: Path) -> dict[str, str]:
    sources = {}
    complete = False
    for line in read_rows(model):
        if complete:
            raise ValueError('Compiler model has records after its footer')
        fields = line.decode('ascii').split('\t')
        if fields[0] == 'FILE':
            if len(fields) != 7 or fields[6] != 'regular':
                raise ValueError('Compiler model contains an unsupported source revision')
            # Compiler paths use byte-safe percent escapes in both transports.
            path = native_path(unescape(fields[2]))
            sources[str(path)] = fields[4]
        elif fields[0] == 'END':
            complete = True
    if not complete:
        raise ValueError('Compiler model has no completion footer')
    if not sources:
        raise ValueError('Compiler model has no physical source revisions')
    return sources


def fresh(sources: dict[str, str]) -> bool:
    return bool(sources) and all(Path(path).is_file() and digest(Path(path)) == expected
               for path, expected in sources.items())


def run(command: list[str], *, root: Path, timeout: int, output=None):
    # Kill only this owned process tree, before disposing of the parent PID.
    # A linter query can have a child compiler when its timeout expires.
    with subprocess.Popen(command, cwd=root, stdout=output or subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True) as process:
        try:
            stdout, _ = process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'],
                           capture_output=True, timeout=30, check=False)
            process.kill()
            process.communicate()
            raise
        return subprocess.CompletedProcess(command, process.returncode, stdout or '', '')


def arguments(flags: str, includes: str) -> list[str]:
    tokens = shlex.split(flags)
    result = []
    index = 0
    while index < len(tokens):
        flag = tokens[index]
        if flag == '-e':
            result.append('--compiler-error-check')
            index += 1
        elif flag in ('-m', '-d'):
            result.extend(['--compiler-main' if flag == '-m' else '--compiler-define', tokens[index + 1]])
            index += 2
        elif flag in ('-w', '-maxerr', '-Wc'):
            # These are diagnostic/backend-C options. The external primary
            # export still uses them exactly; they do not select FB syntax.
            index += 2
        else:
            raise ValueError('Unrepresented frontend build option: ' + flag)
    extra = shlex.split(includes)
    if len(extra) % 2 or any(extra[i] != '-i' for i in range(0, len(extra), 2)):
        raise ValueError('Unrepresented compiler include flags')
    for index in range(1, len(extra), 2):
        result.extend(['--compiler-include', str(native_path(extra[index]))])
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--linter', type=Path, required=True)
    parser.add_argument('--configuration', type=Path, required=True,
                        help='Output from the canonical compiler-lint-configuration Make query')
    parser.add_argument('--output', type=Path, default=Path('build/compiler-lint/pass'))
    parser.add_argument('--unit', action='append', help='Compiler-relative module; repeatable')
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('--retry-failed', action='store_true')
    options = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    compiler = options.compiler.resolve(strict=True)
    linter = options.linter.resolve(strict=True)
    configuration = options.configuration.resolve(strict=True)
    output = options.output.resolve()
    if not output.is_relative_to(root / 'build') or options.timeout <= 0:
        parser.error('Use a positive timeout and an output directory inside this checkout build/')
    output.mkdir(parents=True, exist_ok=True)
    scratch = output / 'scratch'
    scratch.mkdir(exist_ok=True)
    settings = dict(line[5:].split('=', 1) for line in configuration.read_text(
        encoding='utf-8-sig').splitlines() if line.startswith('LINT_'))
    if settings['TARGET'] != 'win64' or settings['HOST'] != 'win32':
        raise ValueError('This qualification currently requires the native Windows x64 graph')
    units = [native_path(name) for name in shlex.split(settings['UNITS'])]
    if not units or len(units) != len(set(units)):
        raise ValueError('Make selected an empty or ambiguous source graph')
    names = {str(path.relative_to(root / 'src/compiler')).replace('\\', '/'): path for path in units}
    selected = list(dict.fromkeys(options.unit or names))
    if set(selected) - set(names):
        raise ValueError('Requested modules are outside the Make-selected graph')
    frontend = arguments(settings['FLAGS'], settings['INCLUDES'])
    pins = {'compiler': digest(compiler), 'linter': digest(linter),
            'configuration': digest(configuration), 'controller': digest(Path(__file__))}
    receipt_path = output / 'results.json'
    receipt = json.loads(receipt_path.read_text()) if receipt_path.exists() else {}
    if receipt and receipt['pins'] != pins:
        raise ValueError('Qualification tools/configuration changed; use a separate result directory')
    receipt.update(pins=pins, graph=list(names), configuration=settings)
    records = receipt.setdefault('records', {})
    common = [str(linter), '--no-config', '--profile', 'strict', '--honor-suppressions',
              '--timings', '--progress',
              '--require-semantic', '--show-semantic', '--format', 'jsonl', '--jobs', '1',
              '--compiler', str(compiler), '--compiler-prefix', str(root),
              '--compiler-target', 'win64', '--compiler-backend', 'gcc',
              '--compiler-include', str(root / 'src/compiler'),
              '--compiler-include', str(root / 'inc'), *frontend]
    for ordinal, name in enumerate(selected, 1):
        previous = records.get(name)
        if previous and fresh(previous.get('sources', {})) and (
                previous['passed'] or not options.retry_failed):
            print(f'{ordinal}/{len(selected)} {name}: retained {previous["passed"]}', flush=True)
            continue
        if shutil.disk_usage(output).free < 10 * 1024 ** 3:
            raise ValueError('Compiler lint scratch must retain 10 GiB of disk headroom')
        # One bundle slot is reused for the whole graph, so large models never
        # accumulate. Evidence consists of findings, dependency hashes and status.
        for path in scratch.iterdir():
            if path.is_file() and path.name.startswith('context.'):
                path.unlink()
        source = names[name]
        prefix = scratch / 'context'
        log = output / (name.replace('/', '__').removesuffix('.bas') + '.jsonl')
        started = time.monotonic()
        record = {'passed': False, 'sources': {}, 'summary': None}
        print(f'{ordinal}/{len(selected)} {name}: start', flush=True)
        try:
            flags = shlex.split(settings['FLAGS']) + shlex.split(settings['INCLUDES'])
            command = [str(compiler), '-prefix', str(root), '-target', 'win64', '-gen', 'gcc',
                       *flags, '-i', str(root / 'src/compiler'), '-i', str(root / 'inc'),
                       '-r', '-semantic-bundle',
                       str(prefix), '-o', str(prefix.with_suffix('.c')), str(source)]
            result = run(command, root=root, timeout=options.timeout)
            record['compiler_exit'] = result.returncode
            record['compiler_diagnostics'] = result.stdout + result.stderr
            if result.returncode:
                raise ValueError('Native compiler export failed')
            record['sources'] = model_sources(prefix.with_suffix('.fbcsem'))
            if not fresh(record['sources']):
                raise ValueError('Source changed during compiler export')
            command = common + ['--semantic-root', str(source), '--semantic-bundle',
                                str(prefix), str(source)]
            with log.open('w', encoding='utf-8') as stream:
                result = run(command, root=root, output=stream, timeout=options.timeout)
            record['linter_exit'] = result.returncode
            rows = [json.loads(line) for line in log.read_text().splitlines() if line.startswith('{')]
            summaries = [row for row in rows if row['type'] == 'summary']
            if len(summaries) != 1:
                raise ValueError('Linter did not publish exactly one completion summary')
            summary = record['summary'] = summaries[0]
            record['timings'] = [row for row in rows if row['type'] == 'timings']
            record['passed'] = (result.returncode == 0 and summary['errors'] == 0 and
                summary['operational_errors'] == 0 and summary['semantic_accepted'] == 1 and
                summary['semantic_unavailable'] == 0 and summary['semantic_skipped'] == 0 and
                fresh(record['sources']))
        except (ValueError, OSError, subprocess.TimeoutExpired) as error:
            record['failure'] = str(error)
        record['seconds'] = round(time.monotonic() - started, 3)
        records[name] = record
        receipt['complete'] = (set(records) == set(names) and
            all(item['passed'] and fresh(item['sources']) for item in records.values()))
        save(receipt_path, receipt)
        summary = record.get('summary') or {}
        print(f'{ordinal}/{len(selected)} {name}: passed={record["passed"]} '
              f'warnings={summary.get("warnings")} operational_errors={summary.get("operational_errors")} '
              f'seconds={record["seconds"]}', flush=True)
        if digest(compiler) != pins['compiler'] or digest(linter) != pins['linter']:
            raise ValueError('A qualification tool changed during the pass')
    # Deduplicate shared-header findings while preserving the per-context logs.
    findings = {}
    for name, record in records.items():
        if not record['passed']:
            continue
        log = output / (name.replace('/', '__').removesuffix('.bas') + '.jsonl')
        for line in log.read_text().splitlines():
            if not line.startswith('{'):
                continue
            row = json.loads(line)
            if row['type'] == 'finding':
                key = (str(native_path(row['path'])).lower(), row['line'], row['column'], row['rule'], row['message'])
                findings[key] = row
    save(output / 'findings.json', {'findings': list(findings.values()),
        'rules': dict(Counter(row['rule'] for row in findings.values()))})
    print(f'COMPILER_SEMANTIC_LINT_COMPLETE={receipt.get("complete", False)} '
          f'accepted={sum(item["passed"] for item in records.values())}/{len(names)} '
          f'unique_findings={len(findings)}', flush=True)
    return 0 if receipt.get('complete') else 1


if __name__ == '__main__':
    raise SystemExit(main())

# end of lint-compiler-sources.py
