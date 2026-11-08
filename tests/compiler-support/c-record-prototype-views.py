"""Project: FreeBASIC C backend regression tests
File: c-record-prototype-views.py
Purpose: Check forward prototype aliases and completed packed receiver accesses.
Responsibilities: Preserve strict type checks, method declarations and alignment views.
This file intentionally does NOT suppress type diagnostics or install runtime libraries.
"""
import argparse
import base64
import hashlib
import json
import os
import re
from pathlib import Path
import signal
import subprocess
import tempfile
import zlib
from native_c_support import native_command, native_path, resolve_executable


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
    parser.add_argument('--baseline', action='store_true')
    parser.add_argument('--instrument', action='store_true')
    parser.add_argument('--native-control', action='store_true')
    args = parser.parse_args()
    args.fbc = str(resolve_executable(args.fbc))
    args.gcc = str(resolve_executable(args.gcc))
    if args.clang:
        args.clang = str(resolve_executable(args.clang))
    prefix = native_path(args.prefix).resolve(strict=True)
    source = Path(__file__).with_suffix('.bas')
    files = [Path(__file__), source, Path(args.fbc), Path(args.gcc)]
    if args.clang:
        files.append(Path(args.clang))
    pins = {str(p.resolve()): sha(p) for p in files}
    tools = [('gcc', args.gcc)] + ([('clang', args.clang)] if args.clang else [])
    standards = ('gnu11', 'gnu23') if args.matrix else ('gnu23',)
    optimizations = ('0', '2') if args.matrix else ('2',)
    common = ['-fno-strict-aliasing', '-fwrapv', '-frounding-math', '-fno-math-errno',
        '-Werror=incompatible-pointer-types', '-Werror=redundant-decls']
    diagnostics = {
        'gcc': ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion',
            '-Wcast-qual', '-Wcast-align=strict', '-Wpointer-arith', '-Wstrict-prototypes',
            '-Wmissing-prototypes', '-Wmissing-declarations', '-Wformat=2',
            '-Warray-bounds=2', '-Wpadded'],
        'clang': ['-Weverything', '-ferror-limit=0'],
    }
    instruments = ['-fsanitize=alignment,bounds,pointer-overflow',
        '-fsanitize-undefined-trap-on-error'] if args.instrument else []
    checks, emissions = [], []
    runs, controls = 0, 0
    parent = args.work_dir.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix='record-prototype-views-', dir=parent) as directory:
        work = Path(directory).resolve(strict=True)
        assert work != parent and work.is_relative_to(parent)
        cpp, obj = work / 'unit.c', work / 'unit.o'
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
                raise RuntimeError('record declaration scope failed: ' + repr(command) + '\n' + output[-5000:].decode(errors='replace'))
            if sum(p.stat().st_size for p in work.iterdir() if p.is_file()) > 8 * 1024 * 1024:
                raise RuntimeError('record scope scratch exceeded 8 MiB')
            return output

        def execute():
            output = run([exe], execute=True)
            if output.replace(b'\r', b'').strip() != b'C_RECORD_PROTOTYPE_VIEWS_PASS':
                raise RuntimeError('record prototype value checks did not finish: ' + repr(output))

        for mode in ('unchecked', 'checked'):
            flags = ['-exx'] if mode == 'checked' else []
            for producer in ('gcc', 'clang'):
                run([args.fbc, '-prefix', prefix, '-gen', producer, *flags,
                    '-m', source.stem, '-r', source, '-o', cpp])
                text = cpp.read_text()
                record = re.search(r'\bstruct\s+(fb_type_\w*PHASERECORD)\s*\{', text)
                if record is None:
                    raise RuntimeError('completed receiver record is missing')
                pattern = r'typedef\s+struct\s+' + re.escape(record.group(1)) + r'\s+(fb_c_pointer_view_\d+)\s+__attribute__'
                forward = re.findall(pattern, text[:record.start()])
                completed = re.findall(pattern, text[record.end():])
                if args.baseline:
                    if forward:
                        raise RuntimeError('baseline unexpectedly has a forward record view')
                else:
                    if not forward or not completed or set(forward) & set(completed):
                        raise RuntimeError('declaration and access views must be separate')
                    definitions = [line for line in text.splitlines()
                        if 'PHASERECORD' in line and 'fb_c_sym_THIS' in line and line.rstrip().endswith(')')]
                    if len(definitions) < 3 or any(not any(alias + '*' in line for alias in completed) for line in definitions):
                        raise RuntimeError('method definitions must use completed record views')
                emissions.append({'mode': mode, 'producer': producer, 'C_sha256': sha(cpp),
                    'C_zlib_base64': base64.b64encode(zlib.compress(cpp.read_bytes())).decode('ascii')})
                for cc, executable in tools:
                    for standard in standards:
                        for optimization in optimizations:
                            run([executable, '-std=' + standard, '-O' + optimization,
                                *common, *diagnostics[cc], *instruments, '-c', cpp, '-o', obj])
                            run([args.fbc, '-prefix', prefix, obj, '-x', exe])
                            execute()
                            runs += 1
            if args.native_control:
                run([args.fbc, '-prefix', prefix, '-gen', 'gas64', *flags, source, '-x', exe])
                execute()
                controls += 1
    assert all(sha(path) == digest for path, digest in pins.items())
    if args.receipt:
        args.receipt.write_text(json.dumps({'checks_complete': True, 'input_sha256': pins,
            'checks': checks, 'emissions': emissions, 'successful_C_executions': runs,
            'native_control_executions': controls, 'instrumented': args.instrument,
            'runtime_prefix': str(prefix), 'whole_runtime_instrumentation_claimed': False}, indent=2) + '\n')
    print('C_RECORD_PROTOTYPE_VIEWS_PASS runs=' + str(runs) + ' native_controls=' + str(controls))


if __name__ == '__main__':
    main()

# end of c-record-prototype-views.py
