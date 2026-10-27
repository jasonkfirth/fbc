"""Project: FreeBASIC C error-frame qualification
File: test-error-frame-bindings.py
Purpose: Check typed frame APIs through BASIC worker errors and callbacks.
Responsibilities: Emit Win32/Win64 C, retain unsuppressed diagnostics, and run
    retry/next cases against private production runtime fragments on Windows.
This file intentionally does NOT qualify automatic ON ERROR outlining.
"""
import argparse
import gzip
import importlib.util
import os
from pathlib import Path


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'gcc', 'clang', 'runtime-dir', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    options = parser.parse_args()
    output = options.output.resolve()
    if os.name != 'nt' or not output.is_relative_to(root / 'build'):
        parser.error('use a Windows host and an output below build/')
    output.mkdir(parents=True, exist_ok=False)
    spec = importlib.util.spec_from_file_location('audit', root / 'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    tools = {name: getattr(options, name).resolve() for name in ('fbc', 'gcc', 'clang')}
    os.environ['PATH'] = str(tools['gcc'].parent) + os.pathsep + os.environ['PATH']
    source = root / 'tests/rtlib/error-frame-bindings.bas'
    runtime_sources = [root / 'src/rtlib' / path for path in
                       ('errors/error_frame.c', 'errors/error.c', 'threads/thread_ctx.c')]
    libraries = [options.runtime_dir.resolve() / name for name in ('fbrt0.o', 'libfb.a')]
    inputs = [source, Path(__file__).resolve(), root / 'build_scripts/audit-c-emitter.py',
              *runtime_sources, *(root / 'src/rtlib').rglob('*.h'), *libraries, *tools.values()]
    pins = {str(path): audit.digest(path) for path in inputs}
    result = dict(pins=pins, runtime_objects=[], records=[], complete=False)
    semantics = [flag for flag in audit.SEMANTICS if flag != '-nostdinc']

    def publish():
        audit.write_json(output / 'results.json', result)

    runtime_objects = []
    for path in runtime_sources:
        obj = output / (path.stem + '.o')
        stage = audit.invoke([str(tools['gcc']), *semantics, *audit.GCC_WARNINGS,
            '-O2', '-m64', '-I' + str(root / 'src/rtlib'), '-c', str(path), '-o', str(obj)],
            root, output / (path.stem + '.log'), 45)
        result['runtime_objects'].append(stage)
        publish()
        assert stage['exit'] == 0
        runtime_objects.append(obj)

    for target, width in (('win64', '64'), ('win32', '32')):
        for backend in ('gcc', 'clang'):
            folder = output / (target + '-' + backend)
            folder.mkdir()
            cfile = folder / 'fixture.c'
            emit = audit.invoke([str(tools['fbc']), '-prefix', str(root), '-gen', backend,
                '-target', target, '-m', source.stem, '-ex', '-r', str(source),
                '-o', str(cfile.with_suffix('.o'))], root, folder / 'emit.log', 30)
            row = dict(target=target, backend=backend, emit=emit, checks=[])
            result['records'].append(row)
            publish()
            assert emit['exit'] == 0
            row['c_sha256'] = audit.digest(cfile)
            for compiler in ('gcc', 'clang'):
                for optimization in ('O0', 'O2'):
                    stem = compiler + '-' + optimization
                    obj = folder / (stem + '.o')
                    warnings = audit.GCC_WARNINGS if compiler == 'gcc' else ['-Weverything']
                    stage = audit.invoke([str(tools[compiler]), *semantics, *warnings,
                        '-' + optimization, '-m' + width, '-c', str(cfile), '-o', str(obj)],
                        root, folder / (stem + '.log'), 45)
                    check = dict(compiler=compiler, optimization=optimization, object=stage)
                    row['checks'].append(check)
                    publish()
                    assert stage['exit'] == 0
                    assert int.from_bytes(obj.read_bytes()[:2], 'little') == {'32': 0x14c, '64': 0x8664}[width]
                    check['object_sha256'] = audit.digest(obj)
                    if width == '64':
                        exe = folder / (stem + '.exe')
                        check['link'] = audit.invoke([str(tools['gcc']), str(obj),
                            *map(str, runtime_objects), *map(str, libraries), '-o', str(exe),
                            '-lm', '-mthreads'], root, folder / (stem + '-link.log'), 30)
                        publish()
                        assert check['link']['exit'] == 0
                        check['run'] = audit.invoke([str(exe)], root, folder / (stem + '-run.log'), 8)
                        publish()
                        assert check['run']['exit'] == 0
                        assert gzip.decompress(Path(check['run']['log']).read_bytes()).decode().strip() == 'error-frame-bindings-ok'
                        exe.unlink()
                    obj.unlink()
    assert all(audit.digest(Path(path)) == pin for path, pin in pins.items())
    for obj in runtime_objects:
        obj.unlink()
    result['complete'] = True
    publish()
    print('BASIC_TYPED_ERROR_FRAME_PASS emissions=4 BASIC_objects=16 runtime_objects=3 native_runs=8')


if __name__ == '__main__':
    main()

# end of test-error-frame-bindings.py
