"""Project: FreeBASIC C backend qualification
File: test-c-emitter-pointer-expressions.py
Purpose: Check pointer-result metadata through real BASIC source reductions.
Responsibilities: Retain strict C diagnostics and execute native pointer controls.
This file intentionally does NOT install providers or qualify other target ABIs.
"""
import argparse
import ctypes
import gzip
import importlib.util
import os
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('c_audit', ROOT / 'build_scripts/audit-c-emitter.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
CASES = {
    'c-pointer-offsets': 'POINTER_OFFSETS_PASS',
    'c-pointer-expression-types': 'POINTER_EXPRESSION_TYPES_PASS',
}
# These reductions normally retain less than 1 MiB, including native artifacts.
MAX_OUTPUT_BYTES = 16 * 1024 ** 2


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'gcc', 'clang', 'runtime-dir', 'output'):
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('this gate currently checks Win32/Win64 objects and Win64 native code')
    output = args.output.resolve()
    if output.exists():
        parser.error('output already exists; preserve it and select a new directory')
    compiler = args.fbc.resolve()
    tools = dict(gcc=args.gcc.resolve(), clang=args.clang.resolve())
    runtime = [args.runtime_dir.resolve() / name for name in ('fbrt0.o', 'libfb.a')]
    sources = [ROOT / 'tests/quirk' / (name + '.bas') for name in CASES]
    inputs = [compiler, *tools.values(), *runtime, *sources, Path(__file__).resolve(),
              ROOT / 'build_scripts/audit-c-emitter.py']
    if not all(path.is_file() for path in inputs):
        parser.error('a compiler, runtime object or fixture is missing')
    pins = {str(path): audit.digest(path) for path in inputs}
    output.mkdir(parents=True)
    temporary = output / 'tmp'
    temporary.mkdir()
    for name in ('TEMP', 'TMP', 'TMPDIR'):
        os.environ[name] = str(temporary)
    ctypes.windll.kernel32.SetErrorMode(3)
    result = dict(complete=False, overall_complete=False, installed=False, pins=pins,
                  scope='Win32/Win64 objects and Win64 pointer-expression sanitizer controls',
                  records=[], ordinary_objects=0, native_passes=0)

    def owned_bytes():
        return sum(path.stat().st_size for path in output.rglob('*') if path.is_file())

    def guard(extra=0):
        if owned_bytes() + extra > MAX_OUTPUT_BYTES or shutil.disk_usage(output).free < 5 * 1024 ** 3:
            raise RuntimeError('pointer-expression gate resource limit reached')

    def publish():
        audit.write_json(output / 'results.json', result)

    def invoke(command, folder, label):
        guard()
        stage = audit.invoke(list(map(str, command)), ROOT, folder / (label + '.log'), 120)
        if stage['exit'] != 0:
            result['failure'] = stage
            publish()
            raise RuntimeError((label, stage['exit'], stage['log']))
        return stage

    def archive(path):
        raw = path.read_bytes()
        packed = gzip.compress(raw, mtime=0)
        guard(len(packed))
        target = Path(str(path) + '.gz')
        target.write_bytes(packed)
        if gzip.decompress(target.read_bytes()) != raw:
            raise RuntimeError('artifact verification failed: ' + str(path))
        saved = dict(path=str(path), sha256=audit.digest(path), gzip_path=str(target),
                     gzip_sha256=audit.digest(target))
        path.unlink()
        return saved

    try:
        for source in sources:
            for bits in (32, 64):
                for backend in ('gcc', 'clang'):
                    folder = output / (source.stem + '-' + str(bits) + '-' + backend)
                    folder.mkdir()
                    row = dict(source=str(source), bits=bits, backend=backend, checks=[],
                               emit=invoke([compiler, '-prefix', ROOT, '-gen', backend, '-r',
                                            '-O', '2', '-arch', '686' if bits == 32 else 'x86_64',
                                            '-m', source.stem, source, '-o', folder / 'unit.o'], folder, 'emit'))
                    result['records'].append(row)
                    cfile = folder / 'unit.c'
                    row['c_sha256'] = audit.digest(cfile)
                    # GCC has no selectable warning name for this C constraint.
                    # The reductions use standard C, so pedantic errors catch it.
                    warnings = (audit.GCC_WARNINGS + ['-pedantic-errors', '-fno-diagnostics-show-caret']
                                if backend == 'gcc' else ['-Weverything', '-Wwrite-strings',
                                                          '-Werror=pointer-integer-compare', '-fno-caret-diagnostics'])
                    for optimization in ('O0', 'O2'):
                        obj = folder / (optimization + '.o')
                        command = [tools[backend], *audit.SEMANTICS, '-pipe', '-' + optimization,
                                   '-m' + str(bits), *warnings, '-c', cfile, '-o', obj]
                        check = dict(optimization=optimization, object=invoke(command, folder, optimization + '-object'))
                        row['checks'].append(check)
                        check['object_sha256'] = audit.digest(obj)
                        result['ordinary_objects'] += 1
                        if bits == 64:
                            sanitized = folder / (optimization + '-san.o')
                            check['sanitizer_object'] = invoke([*command[:-4], '-fsanitize=undefined',
                                '-fsanitize-undefined-trap-on-error', '-c', cfile, '-o', sanitized], folder, optimization + '-sanitizer')
                            image = folder / (optimization + '.exe')
                            check['link'] = invoke([tools[backend], '-s', sanitized, *runtime, '-lm',
                                                   '-mthreads', '-o', image], folder, optimization + '-link')
                            check['run'] = invoke([image], folder, optimization + '-run')
                            marker = gzip.decompress(Path(check['run']['log']).read_bytes()).decode().strip()
                            if marker != CASES[source.stem]:
                                raise RuntimeError('unexpected native output: ' + marker)
                            result['native_passes'] += 1
                            check['executable_storage'] = archive(image)
                            check['sanitizer_object_storage'] = archive(sanitized)
                        check['object_storage'] = archive(obj)
                        publish()
                    row['c_storage'] = archive(cfile)
        if result['ordinary_objects'] != 16 or result['native_passes'] != 8:
            raise RuntimeError('pointer-expression gate did not complete its matrix')
        for path, expected in pins.items():
            if audit.digest(Path(path)) != expected:
                raise RuntimeError('input changed during qualification: ' + path)
        result['complete'] = True
        publish()
        print('C_POINTER_EXPRESSIONS_PASS objects=16 native=8')
    except BaseException:
        publish()
        raise


if __name__ == '__main__':
    main()
# end of test-c-emitter-pointer-expressions.py
