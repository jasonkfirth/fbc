"""FreeBASIC native I/O count regression runner.
File: test-native-io-counts.py
Compile the actual TCP adapters against bounded native callbacks and retain
every compiler diagnostic. Real transfers use the existing BASIC TCP test.
"""
import argparse
import hashlib
import json
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
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    native = args.source_root / 'src/rtlib'
    source = ROOT / 'tests/rtlib/native-io-counts.c'
    inputs = [source, Path(__file__), native / 'devices/dev_tcp.c', native / 'devices/dev_tcp_private.h']
    pins = {str(path.resolve()): sha(path) for path in inputs}
    flags = ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion', '-Wcast-qual', '-Wformat=2']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    wrappers = ['recv', 'send', 'getaddrinfo', 'freeaddrinfo', 'socket', 'connect',
        'bind', 'listen', 'setsockopt', 'ioctl']
    with tempfile.TemporaryDirectory(prefix='fbc-native-io-', dir=args.scratch_root) as directory:
        work = Path(directory).resolve()
        executable = work / 'counts'
        command = [args.cc, '-std=gnu11', '-m' + args.bits, '-O' + args.optimization,
            '-D_FILE_OFFSET_BITS=64', '-fno-strict-aliasing', '-fwrapv', '-ffunction-sections',
            '-fdata-sections', *flags, '-I' + str(native), str(source),
            *['-Wl,--wrap=' + name for name in wrappers], '-Wl,--gc-sections', '-o', str(executable)]
        compiled = subprocess.run(command, cwd=ROOT, stdin=subprocess.DEVNULL,
            capture_output=True, text=True)
        result = dict(command=command, exit=compiled.returncode, output=compiled.stdout + compiled.stderr)
        executed = None
        if compiled.returncode == 0:
            process = subprocess.run([str(executable)], cwd=work, stdin=subprocess.DEVNULL,
                capture_output=True, text=True)
            executed = dict(exit=process.returncode, output=process.stdout + process.stderr)
        peak = sum(path.stat().st_size for path in work.iterdir() if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == digest for path, digest in pins.items())
    match = re.search(r'NATIVE_IO_COUNTS_PASS checks=(\d+)', executed['output']) if executed else None
    complete = compiled.returncode == 0 and executed['exit'] == 0 and match is not None
    receipt = dict(checks_complete=complete, source_pins=pins, compiled=result, executed=executed,
        value_checks=int(match[1]) if match else 0, peak_owned_bytes=peak,
        scratch_directory_removed=not work.exists())
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(receipt, indent=2) + '\n')
    print(executed['output'] if complete else json.dumps(receipt), flush=True)
    return 0 if complete else 1


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-native-io-counts.py
