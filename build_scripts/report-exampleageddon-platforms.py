#!/usr/bin/env python3
"""FreeBASIC Exampleageddon platform comparison.
File: report-exampleageddon-platforms.py
Purpose: Compare observed compile and runtime outcomes across six targets.
Responsibilities: Preserve skipped checks, source freshness and evidence paths.
This file contains no compiler invocation or assumed runtime successes.
"""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path

TARGETS = ('dos', 'win32', 'win64', 'linux', 'haiku', 'freebsd')


def pairs(values):
    result = {}
    for value in values:
        name, separator, path = value.partition('=')
        if not separator or name not in TARGETS or name in result:
            raise ValueError('Expected one TARGET=PATH entry per target: ' + value)
        result[name] = Path(path).resolve()
    return result


def outcome(row):
    if row is None:
        return 'not-tested'
    if row['compile_status'].startswith('blocked'):
        return 'blocked'
    if row['compile_status'] != 'pass':
        return 'compile-failed'
    if row['run_status'] == 'pass':
        return 'run-pass'
    if row['run_status'].startswith(('fail', 'timeout')):
        return 'runtime-failed'
    return 'compiled-runtime-unverified'


def write_csv(path, rows, fields):
    with path.open('w', newline='', encoding='utf-8') as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--sweep', action='append', default=[], metavar='TARGET=CSV')
    parser.add_argument('--pins', action='append', default=[], metavar='TARGET=JSON')
    parser.add_argument('--overlay', action='append', default=[], metavar='TARGET=CSV',
                        help='Replace named example rows with a later focused result')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root, output = args.root.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    sweeps, pin_files = pairs(args.sweep), pairs(args.pins)
    data, pins, identities, origins, evidence = {}, {}, {}, {}, {}
    for target, path in sweeps.items():
        with path.open(newline='', encoding='utf-8-sig') as stream:
            records = list(csv.DictReader(stream))
        mapping = {r['path'].replace('\\', '/'): r for r in records}
        if len(mapping) != len(records):
            raise ValueError('Duplicate example paths in ' + str(path))
        data[target] = mapping
        origins[target] = {name: path.parent for name in mapping}
        evidence[target] = {'csv': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                            'source_basis': 'No source pins supplied'}
    for target, path in pairs(args.overlay).items():
        if target not in data:
            raise ValueError('A focused overlay needs its full sweep: ' + target)
        with path.open(newline='', encoding='utf-8-sig') as stream:
            for row in csv.DictReader(stream):
                name = row['path'].replace('\\', '/')
                if name not in data[target]:
                    raise ValueError('Overlay example is absent from the full sweep: ' + name)
                data[target][name] = row
                origins[target][name] = path.parent
        evidence[target]['overlay'] = {'csv': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    for target, path in pin_files.items():
        value = json.loads(path.read_text(encoding='utf-8-sig'))
        pins[target] = value.get('source_sha256', value)
        identities[target] = value.get('source_identity', {})
        if target in evidence:
            evidence[target]['source_basis'] = value.get('source_basis', 'Recorded source pins')
            evidence[target]['pins'] = str(path)
            evidence[target]['pins_sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()

    rows, details, totals = [], [], {t: collections.Counter() for t in TARGETS}
    fields = ['path', 'ran_on', 'compiled_on', 'failed_on_all_six']
    for target in TARGETS:
        fields += [target + '_outcome', target + '_compile', target + '_run', target + '_source',
                   target + '_group', target + '_reason', target + '_compile_log']
    sources = sorted((root / 'examples').rglob('*.bas'))
    for source in sources:
        name = source.relative_to(root).as_posix()
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        row, observations, ran, compiled = {'path': name}, {}, [], []
        for target in TARGETS:
            result = data.get(target, {}).get(name)
            state = outcome(result)
            recorded_hash = pins.get(target, {}).get(name)
            logical_source = identities.get(target, {}).get(name, name)
            expected_digest = digest if logical_source == name else hashlib.sha256((root / logical_source).read_bytes()).hexdigest()
            freshness = 'unverified' if recorded_hash is None else 'match' if recorded_hash == expected_digest else 'changed'
            if freshness == 'changed':
                state = 'stale-evidence'
            totals[target][state] += 1
            row[target + '_outcome'] = state
            row[target + '_compile'] = result['compile_status'] if result else ''
            row[target + '_run'] = result['run_status'] if result else ''
            row[target + '_source'] = freshness
            row[target + '_group'] = result.get('group', '') if result else ''
            row[target + '_reason'] = result.get('reason', '') if result else ''
            row[target + '_compile_log'] = str(origins[target][name] / result['compile_log']) if result and result.get('compile_log') else ''
            if state == 'run-pass':
                ran.append(target)
            if state in ('run-pass', 'runtime-failed', 'compiled-runtime-unverified'):
                compiled.append(target)
            observations[target] = {'outcome': state, 'source_match': freshness, 'compiled_source': logical_source, 'record': result}
        # Missing, skipped and security-blocked checks cannot prove failure.
        all_failed = all(row[t + '_outcome'] in ('compile-failed', 'runtime-failed') for t in TARGETS)
        row.update(ran_on=','.join(ran), compiled_on=','.join(compiled), failed_on_all_six=str(all_failed).lower())
        rows.append(row)
        details.append({'path': name, 'source_sha256': digest, 'observations': observations})
    write_csv(output / 'platform-matrix.csv', rows, fields)
    selections = {
        'failed-all-six.csv': [r for r in rows if r['failed_on_all_six'] == 'true'],
        'no-recorded-runtime-pass.csv': [r for r in rows if not r['ran_on']],
        'no-compile-pass-on-tested-targets.csv': [r for r in rows if not r['compiled_on']],
        'fails-on-some-target.csv': [r for r in rows if any(r[t + '_outcome'] in ('compile-failed', 'runtime-failed') for t in TARGETS)],
    }
    for name, selection in selections.items():
        write_csv(output / name, selection, fields)
    receipt = {'total_examples': len(rows), 'targets': TARGETS, 'evidence': evidence,
               'totals': {t: dict(v) for t, v in totals.items()},
               'list_counts': {name: len(selection) for name, selection in selections.items()},
               'complete_six_target_coverage': all(t in data and len(data[t]) == len(rows) for t in TARGETS),
               'details': details}
    (output / 'platform-matrix.json').write_text(json.dumps(receipt, indent=2) + '\n')
    lines = ['# Exampleageddon platform comparison', '',
             'This compares observed sweep results. A compile pass is not a runtime pass.',
             'Skipped, blocked, stale and untested checks are not failures. Source freshness',
             'and the input evidence path are preserved in the CSV and JSON.', '',
             '| Target | Run pass | Compiled, runtime unverified | Compile failure | Runtime failure | Blocked | Not tested | Stale |',
             '| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |']
    for target in TARGETS:
        counts = totals[target]
        names = ('run-pass', 'compiled-runtime-unverified', 'compile-failed', 'runtime-failed', 'blocked', 'not-tested', 'stale-evidence')
        lines.append('| ' + target + ' | ' + ' | '.join(str(counts[n]) for n in names) + ' |')
    lines += ['', 'Lists:', '',
              '- `failed-all-six.csv`: demonstrated failures on every requested target.',
              '- `fails-on-some-target.csv`: failures observed on at least one target.',
              '- `no-compile-pass-on-tested-targets.csv`: candidates with no recorded compile success; untested targets remain unresolved.',
              '- `no-recorded-runtime-pass.csv`: no unattended runtime success in these sweeps, including interactive and external-library checks that were skipped.', '',
              'Focused native SDK tests are additional evidence and are not implicitly counted',
              'as passes here. Read their receipts before concluding that a skipped example fails.', '',
              'Six-target sweep coverage: ' + ('complete' if receipt['complete_six_target_coverage'] else 'incomplete'), '',
              '<!-- end of platform-report.md -->', '']
    (output / 'platform-report.md').write_text('\n'.join(lines))
    print('PLATFORM_MATRIX_WRITTEN examples=' + str(len(rows)) + ' targets=' + str(len(data)))


if __name__ == '__main__':
    main()
# end of report-exampleageddon-platforms.py
