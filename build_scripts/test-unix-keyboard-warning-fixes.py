"""FreeBASIC keyboard and console input regression runner.
File: test-unix-keyboard-warning-fixes.py
Compile actual production mapping, keyboard and line-input implementations.
One native invocation owns and removes bounded component executables.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import resource
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
    parser.add_argument('--original', action='store_true')
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    native = args.source_root / 'src/rtlib'
    fixtures = {
        'x11-keycode-values': (['normal', 'null', 'zero-count', 'range', 'stale'], []),
        'linux-keyboard-values': (['console-bytes', 'ioctl-failure', 'X11-bits'],
            ['-Wl,--wrap=read', '-Wl,--wrap=ioctl', '-Wl,--wrap=kill']),
        'unix-readstr-values': (['capacity', 'bytes', 'negative', 'editing'],
            ['-Wl,--wrap=fgets', '-Wl,--wrap=fputc'])}
    inputs = [Path(__file__), *[ROOT / 'tests/rtlib' / (name + '.c') for name in fixtures],
        *[native / name for name in ('drivers/unix/scancodes_x11.c',
            'drivers/unix/fb_private_scancodes_x11.h', 'drivers/linux/io_multikey.c',
            'drivers/unix/io_readstr.c', 'console/scancodes.c')]]
    pins = {str(path.resolve()): sha(path) for path in inputs}
    flags = ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion',
        '-Wcast-qual', '-Wmissing-prototypes', '-Wstrict-prototypes']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    rows = []
    with tempfile.TemporaryDirectory(prefix='fbc-keyboard-values-', dir=args.scratch_root) as directory:
        work = Path(directory).resolve()
        common = [args.cc, '-std=gnu11', '-m' + args.bits, '-O' + args.optimization,
            '-D_FILE_OFFSET_BITS=64', '-fno-strict-aliasing', '-fwrapv',
            '-ffunction-sections', '-fdata-sections', *flags, '-I' + str(native)]
        if args.original:
            common.append('-DFB_TEST_ORIGINAL')
        for name, (cases, link_flags) in fixtures.items():
            executable = work / name
            command = [*common, ROOT / 'tests/rtlib' / (name + '.c'),
                *link_flags, '-Wl,--gc-sections', '-o', executable]
            compiled = subprocess.run(list(map(str, command)), cwd=ROOT,
                stdin=subprocess.DEVNULL, capture_output=True, text=True)
            result = dict(command=list(map(str, command)), exit=compiled.returncode,
                output=compiled.stdout + compiled.stderr)
            executions = []
            if compiled.returncode == 0:
                for case in cases:
                    process = subprocess.run([str(executable), case], cwd=work,
                        stdin=subprocess.DEVNULL, capture_output=True, text=True)
                    executions.append(dict(case=case, exit=process.returncode,
                        output=process.stdout + process.stderr))
            rows.append(dict(fixture=name, compiled=result, executions=executions))
        peak = sum(path.stat().st_size for path in work.iterdir() if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == digest for path, digest in pins.items())
    complete = all(row['compiled']['exit'] == 0 and
        len(row['executions']) == len(fixtures[row['fixture']][0]) and
        all(execution['exit'] == 0 and '_PASS checks=' in execution['output']
            for execution in row['executions']) for row in rows)
    result = dict(checks_complete=complete, source_pins=pins, rows=rows,
        value_checks=sum(int(count) for row in rows for execution in row['executions']
            for count in re.findall(r'_PASS checks=(\d+)', execution['output'])),
        peak_owned_bytes=peak, scratch_directory_removed=not work.exists())
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    print('KEYBOARD_VALUES', 'PASS' if complete else 'FAIL', result['value_checks'], flush=True)
    if not complete:
        print(json.dumps(rows), flush=True)
    return 0 if complete else 1


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-unix-keyboard-warning-fixes.py
