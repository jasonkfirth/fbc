"""FreeBASIC Unix driver-interface regression runner.
File: test-unix-driver-warning-fixes.py
Compile actual focus/stub entry points and both termcap prototype variants.
Each invocation owns and removes one bounded native build directory.
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
    source = ROOT / 'tests/rtlib/unix-driver-contracts.c'
    termcap = ROOT / 'tests/rtlib/termcap-capabilities.c'
    names = ['drivers/unix/io_pageset.c', 'drivers/unix/io_pcopy.c',
        'drivers/unix/sys_dylib.c', 'drivers/linux/io_mouse.c']
    sources = [native / name for name in names]
    inputs = [source, termcap, Path(__file__), *sources,
        native / 'drivers/unix/io_xfocus.c', native / 'fb_private_xfocus.h',
        native / 'fb_private_termcap.h']
    pins = {str(path.resolve()): sha(path) for path in inputs}
    flags = ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion',
        '-Wcast-qual', '-Wmissing-prototypes', '-Wstrict-prototypes']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    commands, executions = [], []

    def run(command, work):
        process = subprocess.run(list(map(str, command)), cwd=work,
            stdin=subprocess.DEVNULL, capture_output=True, text=True)
        result = dict(command=list(map(str, command)), exit=process.returncode,
            output=process.stdout + process.stderr)
        commands.append(result)
        return result

    with tempfile.TemporaryDirectory(prefix='fbc-unix-drivers-', dir=args.scratch_root) as directory:
        work = Path(directory).resolve()
        common = [args.cc, '-std=gnu11', '-m' + args.bits, '-O' + args.optimization,
            '-D_FILE_OFFSET_BITS=64', '-DDISABLE_GPM', '-fno-strict-aliasing', '-fwrapv',
            '-ffunction-sections', '-fdata-sections', *flags, '-I' + str(native)]
        if args.original:
            common.append('-DFB_TEST_OLD_XOPEN')
        for mode in ('focus', 'no-X'):
            executable = work / mode
            defines = ['-DDISABLE_X11'] if mode == 'no-X' else []
            compiled = run([*common, *defines, source, *sources,
                '-Wl,--gc-sections', '-o', executable], ROOT)
            cases = ['uninitialized', 'load-failure', 'open-failure',
                'retry-load', 'retry-open', 'shared'] if mode == 'focus' else ['no-X']
            if compiled['exit'] == 0:
                for case in cases:
                    executed = run([executable, case], work)
                    executions.append(dict(kind=case, compiled=compiled, executed=executed))
            else:
                executions.append(dict(kind=mode, compiled=compiled, executed=None))
        fake_header = work / 'termcap.h'
        fake_header.write_text('#ifdef FB_TEST_MUTABLE_TERMCAP\n'
            'char *tgetstr(char *, char **);\nint tgetflag(char *);\n'
            '#else\nchar *tgetstr(const char *, char **);\nint tgetflag(const char *);\n#endif\n')
        for mode in ('const', 'mutable'):
            executable = work / ('termcap-' + mode)
            defines = ['-DFB_TEST_MUTABLE_TERMCAP'] if mode == 'mutable' else []
            compiled = run([*common, *defines, '-I' + str(work), termcap,
                '-o', executable], ROOT)
            executed = run([executable], work) if compiled['exit'] == 0 else None
            executions.append(dict(kind='termcap-' + mode, compiled=compiled, executed=executed))
        peak = sum(path.stat().st_size for path in work.iterdir() if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == digest for path, digest in pins.items())
    complete = len(executions) == 9 and all(row['compiled']['exit'] == 0 and
        row['executed'] is not None and row['executed']['exit'] == 0 and
        '_PASS checks=' in row['executed']['output'] for row in executions)
    result = dict(checks_complete=complete, source_pins=pins, rows=executions,
        value_checks=sum(int(value) for row in executions if row['executed'] is not None
            for value in re.findall(r'_PASS checks=(\d+)', row['executed']['output'])),
        peak_owned_bytes=peak, scratch_directory_removed=not work.exists())
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    print('UNIX_DRIVER_RUNNER', 'PASS' if complete else 'FAIL', result['value_checks'], flush=True)
    if not complete:
        print(json.dumps(executions), flush=True)
    return 0 if complete else 1


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-unix-driver-warning-fixes.py
