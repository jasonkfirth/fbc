"""FreeBASIC unsigned string-conversion regression.
File: test-string-scalar-warning-fixes.py
Compile the production helpers with a bounded native value observer and retain
all diagnostics. Complete runtime archives have their own build qualification.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--bits', choices=('32', '64'), default='64')
    parser.add_argument('--optimization', choices=('0', '2'), default='2')
    parser.add_argument('--native-flags-json', type=Path)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    native = ROOT / 'src/rtlib'
    observer = ROOT / 'tests/rtlib/string-scalar-values.c'
    inputs = [observer, Path(__file__), *[native / name for name in (
        'strings/str_convfrom_uint.c', 'strings/str_hskip.c',
        'strings/wide/strw_convfrom_uint.c', 'strings/wide/strw_asc.c',
        'strings/unicode/ustr_core.c')]]
    pins = {str(path.resolve()): sha(path) for path in inputs}
    flags = ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion',
        '-Wshadow', '-Wcast-qual', '-Wformat=2']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    environment = dict(os.environ)
    environment['PATH'] = str(Path(args.cc).resolve().parent) + os.pathsep + environment.get('PATH', '')
    with tempfile.TemporaryDirectory(prefix='fbc-string-scalars-', dir=args.scratch_root) as directory:
        work = Path(directory).resolve()
        executable = work / 'values.exe'
        command = [args.cc, '-std=gnu11', '-m' + args.bits, '-O' + args.optimization,
            '-fno-strict-aliasing', '-fwrapv', '-ffunction-sections', '-fdata-sections',
            *flags, '-I' + str(native), str(observer), '-Wl,--gc-sections', '-o', str(executable)]
        compiled = subprocess.run(command, cwd=ROOT, stdin=subprocess.DEVNULL,
            capture_output=True, text=True, env=environment)
        compile_result = dict(command=command, exit=compiled.returncode,
            output=compiled.stdout + compiled.stderr)
        executed = None
        if compiled.returncode == 0:
            process = subprocess.run([str(executable)], cwd=work, stdin=subprocess.DEVNULL,
                capture_output=True, text=True, env=environment)
            executed = dict(exit=process.returncode, output=process.stdout + process.stderr)
        peak = sum(path.stat().st_size for path in work.iterdir() if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == digest for path, digest in pins.items())
    match = re.search(r'STRING_SCALAR_VALUES_PASS checks=(\d+) wchar=(\d+) ulong=(\d+)',
        executed['output']) if executed else None
    complete = compiled.returncode == 0 and executed['exit'] == 0 and match is not None
    result = dict(checks_complete=complete, source_pins=pins, compiled=compile_result,
        executed=executed, value_checks=int(match[1]) if match else 0,
        wchar_bytes=int(match[2]) if match else None,
        native_ulong_bytes=int(match[3]) if match else None,
        peak_owned_bytes=peak, scratch_directory_removed=not work.exists())
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    print(executed['output'] if complete else json.dumps(result), flush=True)
    return 0 if complete else 1


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-string-scalar-warning-fixes.py
