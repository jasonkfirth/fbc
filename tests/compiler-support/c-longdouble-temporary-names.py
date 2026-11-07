"""Project: FreeBASIC C backend regression tests
File: c-longdouble-temporary-names.py
Purpose: Keep generated long-double temporary names reproducible and valid in C.
Responsibilities: Compare independent emissions and inspect each exercised naming path.
This file intentionally does NOT link programs or require a private runtime archive.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--work-dir', required=True, type=Path)
    args = parser.parse_args()
    compiler = args.fbc.resolve(strict=True)
    root = args.root.resolve(strict=True)
    prefix = args.prefix.resolve(strict=True)
    parent = args.work_dir.resolve(strict=True)
    source = root / 'tests/compiler-support/c-readonly-longdouble-guard.bas'
    if not source.is_file():
        raise RuntimeError('long-double value fixture is unavailable: ' + str(source))
    profiles, roles = 0, set()
    with tempfile.TemporaryDirectory(prefix='clongdouble-names-', dir=parent) as directory:
        work = Path(directory).resolve(strict=True)
        if work == parent or not work.is_relative_to(parent):
            raise RuntimeError('long-double scratch escaped the selected directory')
        output = work / 'unit.c'
        for producer in ('gcc', 'clang'):
            for mode in ('e', 'exx'):
                command = [str(compiler), '-prefix', str(prefix), '-i', str(root / 'inc'),
                    '-gen', producer, '-' + mode, '-m', 'c-readonly-longdouble-guard',
                    '-r', str(source), '-o', str(output)]
                first = None
                for _ in range(2):
                    result = subprocess.run(command, cwd=work, capture_output=True,
                        text=True, timeout=600, check=False)
                    if result.returncode:
                        raise RuntimeError('long-double C emission failed\n' + result.stdout + result.stderr)
                    data = output.read_bytes()
                    if len(data) > 8 * 1024 * 1024:
                        raise RuntimeError('long-double emission exceeded its scratch byte cap')
                    if first is not None and data != first:
                        raise RuntimeError(producer + '/' + mode + ': long-double C is not reproducible')
                    first = data
                names = re.findall(rb'\b(fb_c_clongdouble_(native_result|native_argument|return)_[A-Za-z0-9_$]+)', first)
                for identifier, role in names:
                    if not re.fullmatch(rb'[A-Za-z_][A-Za-z0-9_]*', identifier):
                        raise RuntimeError('long-double temporary is not a portable C identifier: ' + identifier.decode())
                    roles.add(role.decode())
                profiles += 1
    # C ABI lowering can differ by target. Report the exercised roles rather
    # than claiming a conversion path that this target did not emit.
    print('C_LONGDOUBLE_NAMES_PASS profiles=' + str(profiles) + ' paths=' + ','.join(sorted(roles)))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of c-longdouble-temporary-names.py
