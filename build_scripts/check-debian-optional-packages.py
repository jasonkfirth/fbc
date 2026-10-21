#!/usr/bin/env python3
"""FreeBASIC Debian optional SDK coverage guard.
File: check-debian-optional-packages.py
Purpose: Keep every verified optional development SDK reachable from its packages.
Responsibilities: Check coverage, optional dependency strength and newer SDK placement.
This file does not install packages or replace APT's dependency resolver.
"""
import argparse
import json
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    metadata = json.loads((args.root / 'debian/optional-dev-packages.json').read_text())
    current = set(metadata['packages'])
    newer = set(metadata['newer_suggestions'])
    if len(current) != len(metadata['packages']) or current & newer:
        raise RuntimeError('Optional SDK manifest has duplicate package names')
    packages = {}
    for paragraph in (args.root / 'debian/control').read_text().strip().split('\n\n'):
        fields = {}
        key = None
        for line in paragraph.splitlines():
            if line.startswith(' '):
                fields[key] += '\n' + line
            else:
                key, value = line.split(':', 1)
                if key in fields:
                    raise RuntimeError('Duplicate control field: ' + key)
                fields[key] = value.strip()
        if 'Package' in fields:
            packages[fields['Package']] = fields

    def names(field):
        return re.findall(r'(?:^|[,|])\s*([a-z0-9][a-z0-9+.-]*)', field)

    for package in metadata['consumers']:
        fields = packages[package]
        recommended = names(fields.get('Recommends', ''))
        suggested = names(fields.get('Suggests', ''))
        optional = set(recommended + suggested)
        missing = (current | newer) - optional
        if missing:
            raise RuntimeError(f'{package} omits optional SDKs: {sorted(missing)}')
        if len(recommended + suggested) != len(optional):
            raise RuntimeError(package + ' duplicates an optional dependency')
        if newer - set(suggested):
            raise RuntimeError(package + ' must keep newer SDKs in Suggests')
        groups = [set(names(group)) for field in ('Recommends', 'Suggests')
                  for group in fields.get(field, '').split(',')]
        for preferred, fallbacks in metadata.get('alternatives', {}).items():
            if not any({preferred, *fallbacks} <= group for group in groups):
                raise RuntimeError(package + ' separated compatible SDK alternatives: ' + preferred)
        if (current | newer) & set(names(fields.get('Depends', ''))):
            raise RuntimeError(package + ' made an optional SDK mandatory')
    print(f'DEBIAN_OPTIONAL_SDK_COVERAGE_PASS current={len(current)} newer={len(newer)} packages={len(metadata["consumers"])}')


if __name__ == '__main__':
    main()
# end of check-debian-optional-packages.py
