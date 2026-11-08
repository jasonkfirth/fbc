#!/usr/bin/env python3
"""Project: FreeBASIC compiler smoke tests
File: check-semantic-smoke-expressions.py
Purpose: Validate the compact model and the smoke fixture's expression probes.
Responsibilities: Use the independent schema reader and retain fixture coverage.
This file intentionally does NOT maintain a second semantic record whitelist.
"""
import argparse
from pathlib import Path
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--recovery', action='store_true')
    parser.add_argument('artifact', type=Path)
    args = parser.parse_args()
    sys.path.insert(0, str(args.root / 'tests/semantic-sidecar'))
    from sidecar import Model
    model = Model.read(args.artifact, expressions_only=True, allow_recovery=args.recovery)
    dependencies = model.records['D']
    expressions = model.records['E']
    if args.recovery:
        if len(model.records['M']) != 1 or len(dependencies) != 1 or len(model.recovered_modules) != 1:
            raise ValueError('Recovery smoke module/dependency coverage changed')
        if not any(row[2] == '1' and row[4] == '3' and row[15].lower() == 'double' for row in expressions):
            raise ValueError('Recovery smoke omitted the later typed expression')
        print('Semantic recovery smoke passed')
        return 0
    if len(model.records['M']) != 2 or len(dependencies) != 3:
        raise ValueError('Expression smoke module/dependency coverage changed')
    if not any(row[1].replace('\\', '/').endswith('/semantic-model-header.bi') for row in dependencies):
        raise ValueError('Expression smoke omitted its included header')
    if len(expressions) < 2 or any(row[13:15] != ['0', '0'] for row in expressions):
        raise ValueError('Expression smoke contains invalid expression references')
    operations = {row[10] for row in expressions if row[11] == 'builtin'}
    required = {'subtract', 'power', 'concatenate', 'unary-plus', 'identity-test'}
    if not required <= operations:
        raise ValueError('Expression smoke omitted operators: ' + repr(required - operations))
    print('Semantic expression smoke passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of check-semantic-smoke-expressions.py
