"""Project: FreeBASIC C backend regression tests
File: c-record-pointer-dependencies.py
Purpose: Qualify opaque dependencies, real layouts and mixed C record contracts.
Responsibilities: Check C structure, execute both producers and enforce GCC LTO types.
This file intentionally does NOT suppress diagnostics or install runtime libraries.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import zlib
from native_c_support import is_gnu_compiler, native_command, native_path, resolve_executable


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'prefix', 'gcc'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--clang')
    parser.add_argument('--work-dir', type=Path, required=True)
    parser.add_argument('--receipt', type=Path)
    parser.add_argument('--matrix', action='store_true')
    parser.add_argument('--instrument', action='store_true')
    parser.add_argument('--baseline', action='store_true', help='Check the old complete pointer dependency for comparison.')
    args = parser.parse_args()
    # Parent runners may pass PATH names such as "gcc". Hash the executable
    # that subprocess will actually run, independent of the working directory.
    for name in ('fbc', 'gcc', 'clang'):
        command = getattr(args, name)
        if command:
            try:
                resolved = resolve_executable(command)
            except FileNotFoundError:
                parser.error('Cannot resolve ' + name + ' executable: ' + command)
            setattr(args, name, str(resolved))
    args.prefix = str(native_path(args.prefix).resolve(strict=True))
    source = Path(__file__).resolve().parent
    parent = args.work_dir.resolve(strict=True)
    files = [Path(__file__), source / 'c-record-pointer-dependencies.bas',
        source / 'c-record-pointer-dependencies.c', Path(args.fbc), Path(args.gcc)]
    if args.clang:
        files.append(Path(args.clang))
    pins = {str(p.resolve()): sha(p) for p in files}
    checks, emissions = [], []
    runs = 0
    standards = ('gnu11', 'gnu23') if args.matrix else ('gnu23',)
    optimizations = ('0', '2') if args.matrix else ('2',)
    tools = [('gcc', args.gcc)] + ([('clang', args.clang)] if args.clang else [])
    common = ['-fno-strict-aliasing', '-fwrapv', '-frounding-math', '-fno-math-errno']
    diagnostics = {
        'gcc': ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion',
            '-Wcast-qual', '-Wcast-align=strict', '-Wpointer-arith', '-Wstrict-prototypes',
            '-Wmissing-prototypes', '-Wmissing-declarations', '-Wformat=2',
            '-Warray-bounds=2', '-Wpadded'],
        'clang': ['-Weverything', '-ferror-limit=0'],
    }
    instruments = ['-fsanitize=alignment,bounds,pointer-overflow',
        '-fsanitize-undefined-trap-on-error'] if args.instrument else []

    with tempfile.TemporaryDirectory(prefix='record-pointers-', dir=parent) as directory:
        work = Path(directory).resolve(strict=True)
        assert work != parent and work.is_relative_to(parent)
        cpp, obj, observer, combined = [work / p for p in ('unit.c', 'unit.o', 'observer.o', 'combined.o')]
        exe = work / ('unit.exe' if os.name == 'nt' else 'unit')

        def run(command, execute=False):
            command = native_command(command)
            assert not any(v == '-w' or v.startswith('-Wno-') for v in command)
            process = subprocess.Popen(command, cwd=work, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name == 'nt' else 0,
                start_new_session=os.name != 'nt')
            timed_out = False
            try:
                output, _ = process.communicate(timeout=15 if execute else 90)
            except subprocess.TimeoutExpired:
                timed_out = True
                if process.poll() is None:
                    if os.name == 'nt':
                        subprocess.run([str(Path(os.environ['SystemRoot']) / 'System32/taskkill.exe'),
                            '/PID', str(process.pid), '/T', '/F'], capture_output=True, timeout=15, check=False)
                    else:
                        os.killpg(process.pid, signal.SIGKILL)
                output, _ = process.communicate(timeout=15)
            checks.append({'command': command, 'exit': process.returncode, 'timeout': timed_out,
                'output_zlib_base64': base64.b64encode(zlib.compress(output)).decode('ascii')})
            if process.returncode or timed_out:
                raise RuntimeError('record dependency check failed: ' + repr(command) + '\n' + output[-5000:].decode(errors='replace'))
            if sum(p.stat().st_size for p in work.iterdir() if p.is_file()) > 8 * 1024 * 1024:
                raise RuntimeError('record dependency scratch exceeded 8 MiB')
            return output

        def execute():
            nonlocal runs
            output = run([exe], execute=True)
            if output.replace(b'\r', b'').strip() != b'C_RECORD_POINTER_DEPENDENCIES_PASS':
                raise RuntimeError('record dependency observers did not finish: ' + repr(output))
            runs += 1

        for producer in ('gcc', 'clang'):
            run([args.fbc, '-prefix', args.prefix, '-gen', producer,
                '-m', 'c-record-pointer-dependencies', '-r', source / 'c-record-pointer-dependencies.bas', '-o', cpp])
            text = cpp.read_text()
            tags = {}
            for role, suffix in (('OPAQUE', 'OPAQUEDEPENDENCY'), ('POINTER_HEADER', 'POINTERHEADER'), ('COMPLETED', 'COMPLETEDLATER')):
                match = re.search(r'\bstruct\s+(fb_type_\w*' + suffix + r')\s*[{;]', text)
                if not match:
                    raise RuntimeError('record tag missing: ' + suffix)
                tags[role] = match.group(1)
            complete = re.search(r'\bstruct\s+' + re.escape(tags['OPAQUE']) + r'\s*\{', text) is not None
            if complete != args.baseline:
                raise RuntimeError('pointer-only dependency completion differs from the selected contract')
            for role in ('POINTER_HEADER', 'COMPLETED'):
                if not re.search(r'\bstruct\s+' + re.escape(tags[role]) + r'\s*\{', text):
                    raise RuntimeError('required record body missing: ' + role)
            definitions = ['-DFB_TEST_' + role + '=' + tag for role, tag in tags.items()]
            emissions.append({'producer': producer, 'C_sha256': sha(cpp), 'tags': tags,
                'C_zlib_base64': base64.b64encode(zlib.compress(cpp.read_bytes())).decode('ascii')})
            for cc, executable in tools:
                for standard in standards:
                    for optimization in optimizations:
                        flags = ['-std=' + standard, '-O' + optimization, *common, *diagnostics[cc], *instruments]
                        run([executable, *flags, '-nostdinc', '-c', cpp, '-o', obj])
                        run([executable, *flags, *definitions, '-c', source / 'c-record-pointer-dependencies.c', '-o', observer])
                        run([args.fbc, '-prefix', args.prefix, obj, observer, '-x', exe])
                        execute()
            for standard in (standards if is_gnu_compiler(args.gcc) else ()):
                flags = ['-std=' + standard, '-O2', '-flto', *common, *diagnostics['gcc'], *instruments]
                run([args.gcc, *flags, '-nostdinc', '-c', cpp, '-o', obj])
                run([args.gcc, *flags, *definitions, '-c', source / 'c-record-pointer-dependencies.c', '-o', observer])
                # Materialize the mixed declarations before the ordinary BASIC
                # link, so platform library selection remains the driver's job.
                run([args.gcc, '-flto', '-O2', '-nostdlib', '-r', '-flinker-output=nolto-rel',
                    '-Werror=lto-type-mismatch', '-o', combined, obj, observer])
                run([args.fbc, '-prefix', args.prefix, combined, '-x', exe])
                execute()
    assert all(sha(p) == digest for p, digest in pins.items())
    if args.receipt:
        args.receipt.write_text(json.dumps({'checks_complete': True, 'input_sha256': pins,
            'checks': checks, 'emissions': emissions, 'successful_executions': runs,
            'instrumented': args.instrument, 'baseline': args.baseline,
            'runtime_prefix': args.prefix, 'whole_runtime_instrumentation_claimed': False}, indent=2) + '\n')
    print('C_RECORD_POINTER_DEPENDENCIES_PASS runs=' + str(runs) +
          ' gcc_lto=' + ('checked' if is_gnu_compiler(args.gcc) else 'unavailable; native ABI checked'))


if __name__ == '__main__':
    main()

# end of c-record-pointer-dependencies.py
