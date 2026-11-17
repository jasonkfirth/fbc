"""Run actual Coreutils slot callbacks under unsuppressed diagnostics.

File test-coreutils-readonly-slots.py
Check full callback bodies, borrowed slot identity and the LP64 hash provider.
Source emission and whole-command execution have separate qualification.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', required=True, type=Path)
    parser.add_argument('--observer', type=Path,
        default=Path(__file__).resolve().parents[1] / 'tests/compiler-support/coreutils-readonly-slots.c')
    parser.add_argument('--cc', required=True)
    parser.add_argument('--bits', required=True, choices=('64',))
    parser.add_argument('--native-flags-json', required=True, type=Path)
    parser.add_argument('--optimization', choices=('0', '2'), default='2')
    parser.add_argument('--scratch-root', required=True, type=Path)
    parser.add_argument('--receipt', required=True, type=Path)
    args = parser.parse_args()
    sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
    cc = shutil.which(args.cc)
    assert cc and not args.receipt.exists()
    pins = {str(path.resolve()): sha(path) for path in (args.module, args.observer, args.native_flags_json)}
    flags = json.loads(args.native_flags_json.read_text())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not re.fullmatch(r'-O\w+', flag)]
    flags += ['-m' + args.bits, '-O' + args.optimization, '-ffunction-sections', '-fdata-sections']
    link_flags = ['-m' + args.bits] + [flag for flag in flags if flag.startswith(('--target=', '--sysroot='))]
    state = dict(checks_complete=False, input_pins=pins, compiler_sha256=sha(cc),
        commands=[], peak_scratch_bytes=0)

    def run(command):
        creationflags = 0
        if os.name == 'nt': creationflags = subprocess.CREATE_NO_WINDOW | subprocess.BELOW_NORMAL_PRIORITY_CLASS
        result = subprocess.run(list(map(str, command)), cwd=work,
            stdin=subprocess.DEVNULL, capture_output=True, text=True, creationflags=creationflags)
        output = result.stdout + result.stderr
        row = dict(command=list(map(str, command)), exit=result.returncode, output=output,
            warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        assert result.returncode == 0, output[-5000:]
        state['peak_scratch_bytes'] = max(state['peak_scratch_bytes'], sum(path.stat().st_size for path in work.iterdir() if path.is_file()))
        assert state['peak_scratch_bytes'] < 8 * 1024 * 1024
        return row

    try:
        with tempfile.TemporaryDirectory(prefix='coreutils-readonly-slots-', dir=args.scratch_root) as directory:
            work = Path(directory)
            run([cc, *flags, '-c', args.module.resolve(), '-o', work / 'module.o'])
            run([cc, *[flag for flag in flags if flag != '-nostdinc'], '-c', args.observer.resolve(), '-o', work / 'observer.o'])
            executable = work / ('probe.exe' if os.name == 'nt' else 'probe')
            run([cc, *link_flags, work / 'module.o', work / 'observer.o', '-Wl,--gc-sections', '-o', executable])
            state['execution'] = run([executable])
            match = re.fullmatch(r'COREUTILS_READONLY_SLOTS_PASS cases=(\d+) checks=(\d+) comparisons=(\d+) hashes=(\d+)\s*', state['execution']['output'])
            assert match, state['execution']['output']
            state['counts'] = dict(zip(('cases', 'checks', 'comparisons', 'hashes'), map(int, match.groups())))
            assert state['counts'] == dict(cases=17409, checks=51328, comparisons=16384, hashes=896)
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

# end of test-coreutils-readonly-slots.py
