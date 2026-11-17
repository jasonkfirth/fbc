"""FreeBASIC Unix transient-buffer regression runner.

File: test-unix-transient-warning-fixes.py
Compile actual process and printer entry points with deterministic printer
callbacks, record input identities and retire one bounded temporary directory.
The normal full-runtime build retains its independent qualification.
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
    parser.add_argument('--case', action='append', choices=(
        'all', 'exec', 'printer', 'failure', 'long-exec', 'long-print'))
    args = parser.parse_args()
    native = args.source_root / 'src/rtlib'
    source = ROOT / 'tests/rtlib/unix-transient-buffers.c'
    sources = [source, *[native / name for name in (
        'drivers/unix/sys_execex.c', 'drivers/unix/io_printer.c',
        'system/sys_parseargs.c', 'files/file_hconvpath.c',
        'strings/encoding/utf_convfrom_wchar.c', 'strings/encoding/utf_core.c')]]
    pins = {str(path): sha(path) for path in [*sources, Path(__file__)]}
    flags = ['-Wall', '-Wextra', '-Werror']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    with tempfile.TemporaryDirectory(prefix='fbc-unix-transient-', dir=args.scratch_root) as directory:
        work = Path(directory)
        executable = work / 'transient-buffers'
        command = [args.cc, '-std=gnu11', '-m' + args.bits, '-O' + args.optimization,
            '-D_FILE_OFFSET_BITS=64', '-DENABLE_MT', '-fno-strict-aliasing', '-fwrapv',
            '-ffunction-sections', '-fdata-sections',
            *[flag for flag in flags if not flag.startswith('-O')],
            '-I' + str(native), *map(str, sources),
            '-Wl,--gc-sections', '-Wl,--wrap=malloc', '-Wl,--wrap=free',
            '-Wl,--wrap=execvp', '-Wl,--wrap=popen', '-Wl,--wrap=pclose',
            '-Wl,--wrap=fopen', '-Wl,--wrap=fopen64', '-pthread', '-o', str(executable)]
        built = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        row = dict(compile_command=command, compile_exit=built.returncode,
            compile_output=built.stdout + built.stderr, executions=[])
        if built.returncode == 0:
            # Original-body controls deliberately use a small worker stack.
            resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
            for case in args.case or ['all']:
                ran = subprocess.run([str(executable), case], cwd=work,
                    capture_output=True, text=True)
                row['executions'].append(dict(case=case, exit=ran.returncode,
                    output=ran.stdout + ran.stderr))
        peak = sum(path.stat().st_size for path in work.iterdir() if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == digest for path, digest in pins.items())
    complete = built.returncode == 0 and all(result['exit'] == 0 and
        'UNIX_TRANSIENT_BUFFERS_PASS' in result['output'] for result in row['executions'])
    result = dict(checks_complete=complete, source_pins=pins, row=row,
        peak_owned_bytes=peak, scratch_directory_removed=not work.exists())
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    if not complete:
        print(row['compile_output'] + '\n'.join(item['output'] for item in row['executions']), flush=True)
        return 1
    for item in row['executions']:
        print(item['output'].strip(), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-unix-transient-warning-fixes.py
