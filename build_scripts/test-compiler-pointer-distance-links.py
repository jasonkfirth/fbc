#!/usr/bin/env python3
"""Project: FreeBASIC compiler qualification
File: test-compiler-pointer-distance-links.py
Purpose: Independently check pointer-distance execution links after normalization.
Responsibilities: Native GCC/GAS64 exports, fresh sources and exact typed roots.
This file intentionally does NOT infer links from coordinates or install compilers.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    options = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / 'tests/semantic-sidecar'))
    from sidecar import Model
    compiler = options.compiler.resolve(strict=True)
    source = root / 'tests/semantic-sidecar/pointer-distance-links.bas'
    with tempfile.TemporaryDirectory(prefix='fbc-pointer-distance-') as temporary:
        for backend in ('gcc', 'gas64'):
            prefix = Path(temporary) / backend
            subprocess.run([str(compiler), '-prefix', str(root), '-gen', backend,
                            '-semantic-bundle', str(prefix), '-r', '-o', str(prefix) + '.out',
                            str(source)], cwd=root, capture_output=True, text=True, timeout=30, check=True)
            model = Model.read(prefix.with_suffix('.fbcsem'))
            model.validate_source_revisions()
            expressions = {row[1]: row for row in model.records['E']}
            nodes = {row[1]: row for row in model.records['N']}
            sites = []
            for operation in model.records['EX']:
                if operation[2:4] != ['binary', 'subtract']:
                    continue
                left, right = expressions[operation[4]], expressions[operation[5]]
                # FBC datatype bits 5 through 8 encode pointer indirection.
                if not (int(left[12]) & 0x1e0 and int(right[12]) & 0x1e0):
                    continue
                result = expressions[operation[1]]
                matches = []
                for link in model.records['H']:
                    if (link[1] == 'node' and link[3] == 'expression' and
                            link[4] == result[1] and link[5] == 'source-expression'):
                        node = nodes[link[2]]
                        if node[4] == result[8] and node[8] == result[12]:
                            matches.append(node[1])
                assert len(set(matches)) == 1, (backend, operation, matches)
                sites.append(operation[1])
            assert len(sites) == 3, (backend, sites)
            print(f'POINTER_DISTANCE_LINKS_PASS backend={backend} sites={len(sites)}')


if __name__ == '__main__':
    main()

# end of test-compiler-pointer-distance-links.py
