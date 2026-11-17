"""Project: FreeBASIC runtime regression tests
File: test-unix-console-warning-fixes.py
Purpose: Exercise console geometry, output and resize ownership on native Linux.
Responsibilities: Compile production entry points with deterministic OS callbacks,
record exact inputs and remove one bounded temporary build directory.
The full runtime build and real terminal control retain separate receipts.
"""
import argparse
import hashlib
import json
from pathlib import Path
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
    parser.add_argument('--receipt', type=Path)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--case', action='append', choices=('all', 'spans', 'wide',
        'resize', 'input', 'input-cursor', 'background-narrow', 'background-wide', 'long'))
    args = parser.parse_args()
    source = ROOT / 'tests/rtlib/unix-console-bounds.c'
    native = args.source_root / 'src/rtlib'
    sources = [source, *[native / name for name in (
        'drivers/unix/io_printbuff.c', 'drivers/unix/io_printbuff_wstr.c',
        'drivers/unix/io_cls.c', 'console/con_lineinp.c', 'console/con_lineinp_wstr.c',
        'strings/encoding/utf_convfrom_wchar.c', 'strings/encoding/utf_core.c',
        'strings/wide/strw_convto_str.c')]]
    init_path = native / 'drivers/unix/hinit.c'
    init = init_path.read_text()
    first = init.index('void fb_hRecheckConsoleSize( int requery_cursorpos )')
    last = init.index('\nstatic void sigwinch_handler', first)
    resize = init[first:last]
    assert resize.rstrip().endswith('}')
    pins = {str(path): sha(path) for path in
        [*sources, init_path, native / 'drivers/unix/fb_private_console.h', Path(__file__)]}
    flags = ['-Wall', '-Wextra', '-Werror']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    with tempfile.TemporaryDirectory(prefix='fbc-unix-console-', dir=args.scratch_root) as directory:
        work = Path(directory)
        fragment = work / 'console-resize.c'
        fragment.write_text('''/* FreeBASIC console resize production-body control.
 * The syscall and signal flag names connect to deterministic test callbacks.
 */
#define _GNU_SOURCE 1
#include "fb.h"
#include "drivers/unix/fb_private_console.h"
#include <signal.h>
#include <sys/ioctl.h>
extern volatile sig_atomic_t fb_test_console_resized;
int fb_test_console_ioctl(int fd, unsigned long request, ...);
int fb_hTermQuery(int code, int *first, int *second);
#define __fb_console_resized fb_test_console_resized
#define ioctl fb_test_console_ioctl
''' + resize + '\n/* end of console-resize.c */\n')
        executable = work / 'console-bounds'
        command = [args.cc, '-std=gnu11', '-O' + args.optimization,
            '-m' + args.bits, '-D_FILE_OFFSET_BITS=64', '-DENABLE_MT',
            '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
            *[flag for flag in flags if not flag.startswith('-O')],
            '-I' + str(native), *map(str, sources), str(fragment),
            '-Wl,--gc-sections', '-Wl,--wrap=malloc', '-Wl,--wrap=calloc',
            '-pthread', '-o', str(executable)]
        compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        row = dict(compile_command=command, compile_exit=compiled.returncode,
            compile_output=compiled.stdout + compiled.stderr)
        row['executions'] = []
        if compiled.returncode == 0:
            # Failure controls can exhaust a deliberately small native stack.
            # Keep resulting core files outside the test's resource footprint.
            resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
            for case in args.case or ['all']:
                observed = subprocess.run([str(executable), case], cwd=work,
                    capture_output=True, text=True)
                row['executions'].append(dict(case=case, exit=observed.returncode,
                    output=observed.stdout + observed.stderr))
        peak = sum(path.stat().st_size for path in work.iterdir() if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == pin for path, pin in pins.items())
    complete = (compiled.returncode == 0 and all(result['exit'] == 0 and
        'UNIX_CONSOLE_BOUNDS_PASS' in result['output'] for result in row['executions']))
    receipt = dict(checks_complete=complete, source_pins=pins, row=row,
        resize_body_sha256=hashlib.sha256(resize.encode()).hexdigest(),
        peak_owned_bytes=peak, scratch_directory_removed=not work.exists())
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(receipt, indent=2) + '\n')
    if not complete:
        print(row['compile_output'] + '\n'.join(result['output']
            for result in row['executions']), flush=True)
        return 1
    for result in row['executions']:
        print(result['output'].strip(), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-unix-console-warning-fixes.py
