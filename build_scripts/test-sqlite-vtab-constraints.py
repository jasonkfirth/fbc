"""Run emitted constraint fixtures against their canonical-record observer.

File test-sqlite-vtab-constraints.py
Preserve aggressive native flags, execution counts and diagnostic receipts.
The caller emits the BASIC fixture with its verbatim production helper body.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', required=True, type=Path)
    parser.add_argument('--kind', required=True, choices=('rhs', 'where'))
    parser.add_argument('--observer', type=Path,
        default=Path(__file__).resolve().parents[1] / 'tests/compiler-support/sqlite-vtab-constraints.c')
    parser.add_argument('--cc', required=True)
    parser.add_argument('--native-flags-json', required=True, type=Path)
    parser.add_argument('--optimization', choices=('0', '2'), default='2')
    parser.add_argument('--before', action='store_true')
    parser.add_argument('--scratch-root', required=True, type=Path)
    parser.add_argument('--receipt', required=True, type=Path)
    args = parser.parse_args()
    sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
    cc = shutil.which(args.cc)
    assert cc and not args.receipt.exists()
    inputs = (args.module, args.observer, args.native_flags_json)
    pins = {str(path.resolve()): sha(path) for path in inputs}
    flags = json.loads(args.native_flags_json.read_text())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not re.fullmatch(r'-O\w+', flag)]
    flags += ['-O' + args.optimization, '-ffunction-sections', '-fdata-sections']
    state = dict(checks_complete=False, input_pins=pins, compiler_sha256=sha(cc),
        kind=args.kind, before=args.before, commands=[], peak_scratch_bytes=0)

    def run(command):
        result = subprocess.run(list(map(str, command)), cwd=work,
            stdin=subprocess.DEVNULL, capture_output=True, text=True)
        output = result.stdout + result.stderr
        row = dict(command=list(map(str, command)), exit=result.returncode, output=output,
            warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        assert result.returncode == 0, output[-5000:]
        state['peak_scratch_bytes'] = max(state['peak_scratch_bytes'],
            sum(path.stat().st_size for path in work.iterdir() if path.is_file()))
        assert state['peak_scratch_bytes'] < 8 * 1024 * 1024
        return row

    try:
        with tempfile.TemporaryDirectory(prefix='sqlite-vtab-constraints-', dir=args.scratch_root) as directory:
            work = Path(directory)
            run([cc, *flags, '-c', args.module.resolve(), '-o', work / 'module.o'])
            observer_flags = [flag for flag in flags if flag != '-nostdinc']
            if args.kind == 'where': observer_flags += ['-DSQLITE_CONSTRAINT_WHERE']
            if args.before: observer_flags += ['-DSQLITE_TEST_BEFORE']
            run([cc, *observer_flags, '-c', args.observer.resolve(), '-o', work / 'observer.o'])
            run([cc, work / 'module.o', work / 'observer.o', '-Wl,--gc-sections', '-o', work / 'probe'])
            state['execution'] = run([work / 'probe'])
            match = re.fullmatch(r'SQLITE_CONSTRAINT_PASS common_cases=(\d+) common_checks=(\d+) common_hash=(\d+) '
                r'extra_cases=(\d+) extra_checks=(\d+) extra_hash=(\d+)\s*', state['execution']['output'])
            assert match, state['execution']['output']
            state['counts'] = dict(zip(('common_cases', 'common_checks', 'common_hash',
                'extra_cases', 'extra_checks', 'extra_hash'), map(int, match.groups())))
            assert (state['counts']['extra_cases'] > 0) == (not args.before)
        assert all(sha(path) == pin for path, pin in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists() if 'work' in locals() else True)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print(state['execution']['output'].strip())


if __name__ == '__main__':
    main()

# end of test-sqlite-vtab-constraints.py
