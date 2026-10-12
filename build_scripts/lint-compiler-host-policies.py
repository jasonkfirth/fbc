"""Project: FreeBASIC compiler qualification
File: lint-compiler-host-policies.py
Purpose: Semantically lint compiler modules replaced on non-Windows hosts.
Responsibilities: Use canonical DOS and RISC OS build contexts and fresh facts.
This file intentionally does NOT claim native execution or a cross-target build.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import shlex


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--linter', type=Path, required=True)
    options = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    controller = Path(__file__).with_name('lint-compiler-sources.py')
    spec = importlib.util.spec_from_file_location('compiler_lint', controller)
    lint = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lint)
    compiler = options.compiler.resolve(strict=True)
    linter = options.linter.resolve(strict=True)
    output = root / 'build/compiler-lint/host-policies'
    output.mkdir(exist_ok=True)
    pins = {str(path): lint.digest(path) for path in (compiler, linter, controller, Path(__file__))}
    records = {}
    for host, unit in (('dos', 'platform/dos/fp-policy.bas'),
                       ('riscos', 'platform/riscos/fp-bits.bas')):
        configuration = root / f'build/compiler-lint/configuration-{host}.log'
        pins[str(configuration)] = lint.digest(configuration)
        settings = dict(line[5:].split('=', 1) for line in configuration.read_text(
            encoding='utf-8-sig').splitlines() if line.startswith('LINT_'))
        assert settings['HOST'] == host, settings
        source = root / 'src/compiler' / unit
        assert source in [lint.native_path(path) for path in shlex.split(settings['UNITS'])], unit
        prefix = output / 'context'
        for path in output.glob('context.*'):
            path.unlink()
        flags = shlex.split(settings['FLAGS']) + shlex.split(settings['INCLUDES'])
        command = [str(compiler), '-prefix', str(root), '-target', settings['TARGET'],
                   '-gen', 'gcc', *flags, '-i', str(root / 'src/compiler'), '-r',
                   '-semantic-bundle', str(prefix), '-o', str(prefix.with_suffix('.c')), str(source)]
        result = lint.run(command, root=root, timeout=120)
        assert result.returncode == 0, result.stdout
        sources = lint.model_sources(prefix.with_suffix('.fbcsem'))
        assert lint.fresh(sources), 'Host policy changed during export'
        log = output / f'{host}.jsonl'
        command = [str(linter), '--no-config', '--profile', 'strict', '--honor-suppressions',
                   '--require-semantic', '--show-semantic', '--format', 'jsonl', '--jobs', '1',
                   '--compiler', str(compiler), '--compiler-prefix', str(root),
                   '--compiler-target', settings['TARGET'], '--compiler-backend', 'gcc',
                   '--compiler-include', str(root / 'src/compiler'),
                   *lint.arguments(settings['FLAGS'], settings['INCLUDES']),
                   '--semantic-root', str(source), '--semantic-bundle', str(prefix), str(source)]
        with log.open('w', encoding='utf-8') as stream:
            result = lint.run(command, root=root, timeout=180, output=stream)
        rows = [json.loads(line) for line in log.read_text().splitlines() if line.startswith('{')]
        summaries = [row for row in rows if row['type'] == 'summary']
        assert len(summaries) == 1, rows
        summary = summaries[0]
        passed = (result.returncode == 0 and summary['errors'] == 0 and
                  summary['operational_errors'] == 0 and summary['semantic_accepted'] == 1 and
                  summary['semantic_unavailable'] == 0 and summary['semantic_skipped'] == 0 and
                  lint.fresh(sources))
        records[unit] = {'passed': passed, 'summary': summary, 'sources': sources,
                         'target': settings['TARGET'], 'configuration': settings}
        print(f'HOST_POLICY_SEMANTIC_LINT host={host} passed={passed}', flush=True)
        lint.save(output / 'results.json', {'pins': pins, 'records': records, 'complete': False})
    assert all(lint.digest(Path(path)) == expected for path, expected in pins.items()), 'Tool drift'
    complete = all(row['passed'] and lint.fresh(row['sources']) for row in records.values())
    lint.save(output / 'results.json', {'pins': pins, 'records': records, 'complete': complete})
    return 0 if complete else 1


if __name__ == '__main__':
    raise SystemExit(main())

# end of lint-compiler-host-policies.py
