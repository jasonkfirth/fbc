"""FreeBASIC Linux filesystem/count regression runner.
File: test-linux-filesystem-warning-fixes.py
Compile production entry points and the existing file-copy alias regression,
record exact source identities and remove one bounded temporary build tree.
Full runtime archives and ordinary BASIC execution retain separate controls.
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
        'all', 'paths', 'null', 'cwd', 'memory', 'locale', 'directory'))
    args = parser.parse_args()
    native = args.source_root / 'src/rtlib'
    source = ROOT / 'tests/rtlib/linux-filesystem-sizes.c'
    copy = ROOT / 'tests/file/copy-alias-posix.c'
    sources = [native / name for name in (
        'drivers/unix/file_dir.c', 'drivers/unix/sys_getcwd.c',
        'drivers/unix/drv_intl_getmonthname.c', 'drivers/unix/drv_intl_getweekdayname.c',
        'drivers/linux/sys_getexename.c', 'drivers/linux/sys_getexepath.c',
        'drivers/linux/sys_fmem.c')]
    copy_source = native / 'drivers/unix/file_copy.c'
    pins = {str(path): sha(path) for path in [source, copy, copy_source, *sources, Path(__file__)]}
    flags = ['-Wall', '-Wextra', '-Werror']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    with tempfile.TemporaryDirectory(prefix='fbc-linux-filesystem-', dir=args.scratch_root) as directory:
        work = Path(directory)
        executable, copy_executable = work / 'filesystem-sizes', work / 'copy-alias'
        common = [args.cc, '-std=gnu11', '-m' + args.bits, '-O' + args.optimization,
            '-D_FILE_OFFSET_BITS=64', '-DENABLE_MT', '-fno-strict-aliasing', '-fwrapv',
            *[flag for flag in flags if not flag.startswith('-O')], '-I' + str(native)]
        command = [*common, '-fno-builtin-strlen', str(source), *map(str, sources),
            '-Wl,--wrap=strlen', '-Wl,--wrap=readlink', '-Wl,--wrap=getcwd',
            '-Wl,--wrap=get_avphys_pages', '-Wl,--wrap=sysconf', '-Wl,--wrap=nl_langinfo',
            '-o', str(executable)]
        built = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        row = dict(compile_command=command, compile_exit=built.returncode,
            compile_output=built.stdout + built.stderr, executions=[])
        if built.returncode == 0:
            resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
            for case in args.case or ['all']:
                process = subprocess.run([str(executable), case], cwd=work,
                    capture_output=True, text=True)
                row['executions'].append(dict(case=case, exit=process.returncode,
                    output=process.stdout + process.stderr))
        copy_command = [*common, str(copy), str(copy_source), '-o', str(copy_executable)]
        compiled_copy = subprocess.run(copy_command, cwd=ROOT, capture_output=True, text=True)
        copy_row = dict(compile_command=copy_command, compile_exit=compiled_copy.returncode,
            compile_output=compiled_copy.stdout + compiled_copy.stderr)
        if compiled_copy.returncode == 0:
            process = subprocess.run([str(copy_executable)], cwd=work, capture_output=True, text=True)
            copy_row.update(exit=process.returncode, output=process.stdout + process.stderr)
        peak = sum(path.stat().st_size for path in work.rglob('*') if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == digest for path, digest in pins.items())
    complete = built.returncode == 0 and compiled_copy.returncode == 0 and copy_row.get('exit') == 0 and all(
        item['exit'] == 0 and 'LINUX_FILESYSTEM_SIZES_PASS' in item['output'] for item in row['executions'])
    result = dict(checks_complete=complete, source_pins=pins, row=row, copy=copy_row,
        peak_owned_bytes=peak, scratch_directory_removed=not work.exists())
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    if not complete:
        print(row['compile_output'] + '\n'.join(item['output'] for item in row['executions']) +
            copy_row['compile_output'] + copy_row.get('output', ''), flush=True)
        return 1
    for item in row['executions']:
        print(item['output'].strip(), flush=True)
    print(copy_row['output'].strip(), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-linux-filesystem-warning-fixes.py
