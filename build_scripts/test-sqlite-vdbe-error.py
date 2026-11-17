"""Project: FreeBASIC SQLite regression
File: test-sqlite-vdbe-error.py
Purpose: Run the generated error-helper fixture against its C callback observer.
Responsibilities: Preserve native flags and retain diagnostic and execution proof.
The caller emits the BASIC fixture with the exact production helper body.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', required=True, type=Path)
    parser.add_argument('--observer', type=Path,
        default=Path(__file__).resolve().parents[1] / 'tests/compiler-support/sqlite-vdbe-error-context.c')
    parser.add_argument('--cc', required=True)
    parser.add_argument('--native-flags-json', required=True, type=Path)
    parser.add_argument('--optimization', choices=('0', '2'), default='2')
    parser.add_argument('--scratch-root', required=True, type=Path)
    parser.add_argument('--receipt', required=True, type=Path)
    args = parser.parse_args()
    inputs = (args.module, args.observer, args.native_flags_json)
    pins = {str(path.resolve()): hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs}
    flags = json.loads(args.native_flags_json.read_text())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not re.fullmatch(r'-O\w+', flag)]
    flags += ['-O' + args.optimization, '-ffunction-sections', '-fdata-sections']
    state = dict(checks_complete=False, input_pins=pins, builds=[], execution=None)
    try:
        with tempfile.TemporaryDirectory(prefix='sqlite-vdbe-error-', dir=args.scratch_root) as directory:
            work = Path(directory)
            for label, source in (('module', args.module), ('observer', args.observer)):
                selected_flags = flags if label == 'module' else [flag for flag in flags if flag != '-nostdinc']
                command = [args.cc, *selected_flags, '-c', str(source.resolve()), '-o', str(work / (label + '.o'))]
                result = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL, capture_output=True, text=True)
                output = result.stdout + result.stderr
                state['builds'].append(dict(command=command, exit=result.returncode, output=output))
                assert result.returncode == 0, output[-5000:]
            command = [args.cc, str(work / 'module.o'), str(work / 'observer.o'),
                '-Wl,--gc-sections', '-o', str(work / 'probe')]
            result = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL, capture_output=True, text=True)
            state['link'] = dict(command=command, exit=result.returncode, output=result.stdout + result.stderr)
            assert result.returncode == 0, state['link']['output'][-5000:]
            result = subprocess.run([str(work / 'probe')], cwd=work,
                stdin=subprocess.DEVNULL, capture_output=True, text=True)
            state['execution'] = dict(exit=result.returncode, output=result.stdout + result.stderr)
            assert result.returncode == 0, state['execution']
            assert result.stdout.strip() == 'SQLITE_VDBE_ERROR_PASS cases=23040 checks=149760', state['execution']
            state['scratch_bytes'] = sum(path.stat().st_size for path in work.iterdir())
            assert state['scratch_bytes'] < 32 * 1024 * 1024
        assert all(hashlib.sha256(Path(path).read_bytes()).hexdigest() == pin for path, pin in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state['failure'] = repr(error)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, indent=2) + '\n')
    print(state['execution']['output'].strip())


if __name__ == '__main__':
    main()

# end of test-sqlite-vdbe-error.py
