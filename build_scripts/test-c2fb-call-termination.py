"""Compare emitted terminating calls with their independent native C source.
File test-c2fb-call-termination.py
Own complete module compiles, real runtime linkage and native entry outcomes.
Converter shape checks and production-census refresh have separate owners.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CASES = [(0, 0), (1, 0), (2, 0), (3, 0), (5, 0), (6, 0), (8, 0), (9, 0), (9, 1)]
CASES += [(which, status) for which in (4, 7) for status in (0, 1, -1, 7, 37, 255)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module-c', type=Path, required=True)
    parser.add_argument('--reference-c', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--runtime-dir', type=Path, required=True)
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--revision', choices=('before', 'after'), required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    digest = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
    driver = ROOT / 'tests/compiler-support/c2fb-call-termination.c'
    support = ROOT / 'tests/compiler-support/native_c_support.py'
    native_command = runpy.run_path(str(support))['native_command']
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not re.fullmatch(r'-O\w+', flag)] + ['-O' + args.optimization]
    header_flags = [flag for flag in flags if flag != '-nostdinc' and not flag.startswith('-masm=')]
    aliases = ['-Derror=audit_error_provider', '-Derror_at_line=audit_error_at_line_provider']
    runtime_files = [args.runtime_dir / name for name in ('fbrt0.o', 'libfb.a')]
    pins = {str(path.resolve()): digest(path) for path in
            [args.module_c, args.reference_c, driver, support, Path(__file__), *runtime_files]}
    state = dict(checks_complete=False, source_pins=pins, commands=[], comparisons=[], peak_scratch_bytes=0)
    assert not args.receipt.exists()

    def run(command, work):
        command = native_command(command)
        options = dict(cwd=work, stdin=subprocess.DEVNULL, capture_output=True, text=True)
        if os.name == 'nt':
            options['creationflags'] = subprocess.CREATE_NO_WINDOW | subprocess.BELOW_NORMAL_PRIORITY_CLASS
        result = subprocess.run(command, **options)
        output = result.stdout + result.stderr
        row = dict(command=list(map(str, command)), exit=result.returncode, stdout=result.stdout, stderr=result.stderr,
                   warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        return row

    try:
        with tempfile.TemporaryDirectory(prefix='c2fb-call-termination-', dir=args.scratch_root) as directory:
            work = Path(directory)
            module_object, reference_object, driver_object = [work / name for name in ('module.o', 'reference.o', 'driver.o')]
            binary = work / ('module.exe' if os.name == 'nt' else 'module')
            reference = work / ('reference.exe' if os.name == 'nt' else 'reference')
            libraries = ['-lm', '-mthreads'] if os.name == 'nt' else ['-lm', '-lpthread', '-ldl', '-lncurses', '-ltinfo', '-lrt']
            commands = ([args.cc, *flags, *aliases, '-c', args.module_c, '-o', module_object],
                        [args.cc, *header_flags, *aliases, '-c', args.reference_c, '-o', reference_object],
                        [args.cc, *header_flags, '-c', driver, '-o', driver_object],
                        [args.cc, module_object, driver_object, *runtime_files, *libraries, '-o', binary],
                        [args.cc, reference_object, driver_object, '-o', reference])
            for command in commands:
                compiled = run(command, work)
                assert compiled['exit'] == 0, compiled
            for which, status in CASES:
                expected = run([reference, str(which), str(status)], work)
                actual = run([binary, str(which), str(status)], work)
                assert (actual['exit'], actual['stdout'], actual['stderr']) == (expected['exit'], expected['stdout'], expected['stderr']), (which, status, expected, actual)
                state['comparisons'].append(dict(which=which, status=status, exit=actual['exit'],
                                                 stdout=actual['stdout'], stderr=actual['stderr']))
            guard = run([binary, '1', '0', 'violate'], work)
            if args.revision == 'after':
                assert guard['exit'] == 5 and 'runtime error 5' in guard['stderr'].lower(), guard
                assert 'CALL_RETURN' not in guard['stdout'], guard
            else:
                assert guard['exit'] == 0 and guard['stdout'] == 'CALL_RETURN 19\n', guard
            state['guard_control'] = guard
            state['cases'] = len(state['comparisons'])
            state['peak_scratch_bytes'] = sum(path.stat().st_size for path in work.iterdir())
            assert state['peak_scratch_bytes'] < 8 * 1024 * 1024
        assert all(digest(path) == pin for path, pin in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists() if 'work' in locals() else True)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('C2FB_CALL_TERMINATION_NATIVE', state['cases'], 'cases and provider-return control')


if __name__ == '__main__':
    main()

# end of test-c2fb-call-termination.py
