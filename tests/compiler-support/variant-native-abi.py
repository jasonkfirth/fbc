"""Project: FreeBASIC compiler and library regression tests
File: variant-native-abi.py
Purpose: Check the Windows variant wrapper against its actual library implementation.
Responsibilities: Build the Make-selected modules and test native values, text and LTO types.
This file intentionally does NOT use installed variant archives or modify examples.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import tempfile
import zlib
from native_c_support import is_gnu_compiler, native_command, native_path


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
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    owner = root / 'examples/win32/variants'
    library = owner / 'libvariant'
    prefix = Path(args.prefix).resolve(strict=True)
    tools = [('gcc', shutil.which(args.gcc))]
    if args.clang:
        tools.append(('clang', shutil.which(args.clang)))
    if any(not tool for _, tool in tools):
        raise RuntimeError('a requested C compiler is unavailable')
    gcc = tools[0][1]
    gcc_lto = is_gnu_compiler(gcc)
    names = re.findall(r'\bvariant-[\w-]+\.bas\b', (library / 'GNUmakefile').read_text())
    if len(names) != len(set(names)) or set(names) != {p.name for p in library.glob('*.bas')}:
        raise RuntimeError('variant Make source list differs from the library sources')
    fixtures = [root / ('tests/compiler-support/' + name + '.bas')
        for name in ('variant-native-abi', 'variant-string-overloads')]
    files = [Path(__file__), owner / 'variant.bi', library / 'intern.bi', library / 'GNUmakefile',
        *[library / name for name in names], *fixtures, Path(args.fbc), *[Path(p) for _, p in tools]]
    pins = {str(p.resolve()): sha(p) for p in files}
    checks, emissions = [], []
    counts = {'ordinary_executions': 0, 'mixed_LTO_executions': 0}
    common = ['-std=gnu23', '-O2', '-fno-strict-aliasing', '-fwrapv',
        '-frounding-math', '-fno-math-errno']
    diagnostics = {
        'gcc': ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion',
            '-Wcast-qual', '-Wcast-align=strict', '-Wpointer-arith', '-Wstrict-prototypes',
            '-Wmissing-prototypes', '-Wmissing-declarations', '-Wformat=2',
            '-Warray-bounds=2', '-Wpadded'],
        'clang': ['-Weverything', '-ferror-limit=0'],
    }
    parent = args.work_dir.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix='variant-native-', dir=parent) as directory:
        work = Path(directory).resolve(strict=True)
        assert work != parent and work.is_relative_to(parent)
        cpp, obj, combined, native, exe = [work / name
            for name in ('unit.c', 'unit.o', 'combined.o', 'library.o', 'unit.exe')]

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
                raise RuntimeError('variant check failed: ' + repr(command) + '\n' + output[-5000:].decode(errors='replace'))
            if sum(p.stat().st_size for p in work.iterdir() if p.is_file()) > 16 * 1024 * 1024:
                raise RuntimeError('variant scratch exceeded 16 MiB')
            return output

        target = run([args.fbc, '-print', 'target']).decode().strip()
        if target not in ('win64', 'win64-x86_64', 'win32', 'win32-x86'):
            raise RuntimeError('variant native execution requires a Windows x86 target: ' + target)
        system = target.split('-')[0]
        architecture = target.partition('-')[2]
        machine = (['-m64'] if system == 'win64' or architecture == 'x86_64' else
                   ['-m32'] if architecture in ('', 'x86', '686') else [])
        runtime = native_path(run([args.fbc, '-prefix', prefix, '-print', 'fblibdir']).decode().strip())
        if not (runtime / 'fbrt0.o').is_file() or not (runtime / 'libfb.a').is_file():
            raise RuntimeError('the selected prefix has no matching Windows runtime')
        for p in (runtime / 'fbrt0.o', runtime / 'libfb.a'):
            pins[str(p)] = sha(p)
        objects = []
        for name in names:
            source = library / name
            run([args.fbc, '-prefix', prefix, '-i', root / 'inc', '-i', owner,
                '-d', 'VARIANT_ISBUILDING', '-gen', 'gcc', '-r', source, '-o', cpp])
            emissions.append({'source': str(source), 'producer': 'gcc', 'C_sha256': sha(cpp)})
            output = work / (source.stem + '.o')
            run([gcc, *common, *machine, *diagnostics['gcc'], *(['-flto'] if gcc_lto else []), '-c', cpp, '-o', output])
            objects.append(output)
        # Keep GIMPLE in the per-module objects for mixed caller checks, and
        # materialize one ordinary library object for both C compiler clients.
        combine = [gcc, *machine, '-flto', '-O2', '-nostdlib', '-r',
            '-flinker-output=nolto-rel', '-Werror=lto-type-mismatch']
        if gcc_lto:
            run([*combine, '-o', native, *objects])

        def execute(client, implementation, marker):
            # The generated C is compiled externally. Link the exact objects
            # directly so a pre-existing libvariant cannot satisfy this test.
            run([gcc, *machine, '-o', exe, client, *(implementation or []),
                runtime / 'fbrt0.o', runtime / 'libfb.a', '-loleaut32', '-lm', '-mthreads'])
            output = run([exe], execute=True)
            if output.replace(b'\r', b'').strip() != marker:
                raise RuntimeError('variant value checks did not finish: ' + repr(output))

        for source in fixtures:
            marker = (source.stem.replace('-', '_').upper() + '_PASS').encode()
            for producer in ('gcc', 'clang'):
                run([args.fbc, '-prefix', prefix, '-i', owner, '-i', root / 'inc',
                    '-gen', producer, '-m', source.stem, '-r', source, '-o', cpp])
                emissions.append({'source': str(source), 'producer': producer, 'C_sha256': sha(cpp)})
                for cc, executable in tools:
                    run([executable, *common, *machine, *diagnostics[cc], '-c', cpp, '-o', obj])
                    execute(obj, [native] if gcc_lto else objects, marker)
                    counts['ordinary_executions'] += 1
                if gcc_lto:
                    run([gcc, *common, *machine, *diagnostics['gcc'], '-flto', '-c', cpp, '-o', obj])
                    run([*combine, '-o', combined, obj, *objects])
                    execute(combined, None, marker)
                    counts['mixed_LTO_executions'] += 1
    assert all(sha(p) == digest for p, digest in pins.items())
    if args.receipt:
        args.receipt.write_text(json.dumps({'checks_complete': True, 'input_sha256': pins,
            'counts': counts, 'checks': checks, 'emissions': emissions,
            'installed_archives_replaced': False, 'consumer_programs_modified': False}, indent=2) + '\n')
    print('VARIANT_LIBRARY_ABI_PASS ' + json.dumps(counts, sort_keys=True) +
          ' gcc_lto=' + ('checked' if gcc_lto else 'unavailable; native ABI checked'))


if __name__ == '__main__':
    main()

# end of variant-native-abi.py
