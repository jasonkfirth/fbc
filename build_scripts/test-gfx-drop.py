"""FreeBASIC graphics; File: test-gfx-drop.py.
Purpose: Build private graphics archives from the Make-selected native source graph.
Responsibilities: Two compiler jobs, public drop fixtures and bounded receipts.
This helper does not rebuild fbc, install libraries or claim physical user input.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import threading

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--prefix', type=Path)
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--ar', default='ar')
    parser.add_argument('--make', default='make')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    compiler = args.compiler.resolve(strict=True)
    prefix = (args.prefix or compiler.parent.parent).resolve(strict=True)
    cc, ar, make = [shutil.which(item) for item in (args.cc, args.ar, args.make)]
    assert cc and ar and make and not output.exists() and output.parent.is_dir()
    assert shutil.disk_usage(output.parent).free > 512 * 1024 * 1024
    output.mkdir()
    (output / '.owned').write_text('FreeBASIC native drop graphics tests\n')
    target = {'Windows': 'win32', 'Linux': 'linux', 'FreeBSD': 'freebsd'}[platform.system()]
    extension = '.exe' if os.name == 'nt' else ''
    environment = os.environ.copy()
    environment['PATH'] = str(Path(cc).parent) + os.pathsep + environment.get('PATH', '')
    records, pins = [], {}
    guard = threading.Lock()

    def run(label, command, timeout=90):
        result = subprocess.run([str(item) for item in command], cwd=output,
                                env=environment, capture_output=True, timeout=timeout)
        assert len(result.stdout) + len(result.stderr) < 1024 * 1024
        log = output / (label + '.log')
        log.write_bytes(result.stdout + result.stderr)
        with guard:
            records.append({'label': label, 'command': [str(item) for item in command],
                            'exit': result.returncode, 'log': log.name, 'log_sha256': digest(log)})
            (output / 'progress.json').write_text(json.dumps(records, indent=2) + '\n')
        if result.returncode:
            raise RuntimeError(label + ':\n' + log.read_text(errors='replace'))
        return result

    graph = output / 'graph.mk'
    graph.write_text('rootdir := ' + ROOT.as_posix() + '\n'
                     'srcdir := ' + (ROOT / 'src').as_posix() + '\n'
                     'mkpath := ' + (ROOT / 'mk').as_posix() + '\n'
                     'TARGET_OS := ' + target + '\nTARGET_ARCH := x86_64\n'
                     'include ' + (ROOT / 'mk/source-graph.mk').as_posix() + '\n'
                     '.PHONY: graph\ngraph:\n'
                     '\t$(info GFX2=$(GFX_SRC))\n\t$(info GFX3=$(GFX3_SRC))\n')
    selected = run('source-graph', [make, '--no-print-directory', '-rR', '-f', graph, 'graph'])
    rows = dict(line.split('=', 1) for line in selected.stdout.decode().splitlines()
                if line.startswith(('GFX2=', 'GFX3=')))
    assert set(rows) == {'GFX2', 'GFX3'}
    flags = ['-O2', '-Wall', '-Wextra', '-Wno-unused-parameter',
             '-Werror=implicit-function-declaration', '-fno-strict-aliasing', '-DENABLE_MT']
    if os.name == 'nt':
        flags += ['-DDISABLE_D3D10']
    elif target == 'freebsd':
        flags += ['-I/usr/local/include']
    for path in (ROOT / 'src/rtlib', ROOT / 'src/rtlib' / ('win32' if os.name == 'nt' else 'unix')):
        flags += ['-I', str(path)]
    if target == 'linux':
        drm = subprocess.run(['pkg-config', '--cflags', 'libdrm', 'gbm'], capture_output=True, text=True)
        if drm.returncode == 0:
            flags += drm.stdout.split()
        else:
            flags += ['-DDISABLE_DRM']
    libraries = ['GFX2', 'GFX3'] if target in ('win32', 'linux') else ['GFX2']
    input_paths = {Path(item) for group in libraries for item in rows[group].split()}
    for folder in ('src/gfxlib2', 'src/gfxlib3', 'src/rtlib'):
        input_paths.update((ROOT / folder).rglob('*.h'))
    input_paths.update((ROOT / 'inc').glob('fb*.bi'))
    input_paths.update((ROOT / 'inc/crt').rglob('*.bi'))
    input_paths.update(ROOT / name for name in (
        'mk/source-graph.mk', 'mk/compiler-sources.mk',
        'src/rtlib/sys_drop.c', 'src/rtlib/sys_service_utf8.c',
        'src/rtlib/win32/sys_hidle.c' if os.name == 'nt' else 'src/rtlib/unix/sys_hidle.c',
        'tests/services/drop-receiver.bas', 'tests/services/win32-drop-source.c',
        'examples/graphics/drag-drop.bas', 'build_scripts/test-gfx-drop.py'))
    pins = {str(path.relative_to(ROOT)): digest(path) for path in input_paths}
    for group in libraries:
        paths = [Path(item) for item in rows[group].split()]
        assert paths and all(item.is_file() for item in paths)
        assert len({item.stem for item in paths}) == len(paths)
        objects = []

        def compile_one(path):
            obj = output / (group + '-' + path.stem + '.o')
            extra = ['-x', 'assembler-with-cpp'] if path.suffix == '.s' else []
            run('compile-' + group + '-' + path.stem,
                [cc, *flags, *extra, '-c', path, '-o', obj])
            return obj

        with ThreadPoolExecutor(max_workers=2) as pool:
            objects = list(pool.map(compile_one, paths))
        archive = output / ('libfbgfxmt.a' if group == 'GFX2' else 'libfbgfx3mt.a')
        run('archive-' + group, [ar, 'rcs', archive, *objects])
        print(group + '_ARCHIVE=PASS sources=' + str(len(paths)), flush=True)
    helpers = []
    runtime_names = ['sys_drop.c', 'sys_service_utf8.c']
    runtime_names += ['win32/sys_hidle.c' if os.name == 'nt' else 'unix/sys_hidle.c']
    for name in runtime_names:
        path = ROOT / 'src/rtlib' / name
        obj = output / ('runtime-' + path.stem + '.o')
        run('runtime-' + path.stem, [cc, *flags, '-c', path, '-o', obj])
        helpers.append(obj)
    if os.name == 'nt':
        obj = output / 'win32-drop-source.o'
        run('native-source', [cc, *flags, '-c', ROOT / 'tests/services/win32-drop-source.c', '-o', obj])
        helpers.append(obj)
    for group in libraries:
        option = ['-gfx3'] if group == 'GFX3' else []
        for stem, original in [('receiver', 'tests/services/drop-receiver.bas'),
                               ('example', 'examples/graphics/drag-drop.bas')]:
            sample = output / (group + '-' + stem + '.bas')
            sample.write_bytes((ROOT / original).read_bytes())
            image = output / (group + '-' + stem + extension)
            run('build-' + group + '-' + stem,
                [compiler, '-prefix', prefix, '-mt', '-w', 'all', '-exx', *option,
                 '-i', ROOT / 'inc', '-p', output, sample, *helpers, '-x', image])
        if os.name == 'nt':
            result = run('run-' + group, [output / (group + '-receiver' + extension)], 20)
            assert b'DROP_NATIVE=PASS' in result.stdout
    assert all(digest(ROOT / path) == value for path, value in pins.items())
    size = sum(path.stat().st_size for path in output.rglob('*') if path.is_file())
    assert size < 96 * 1024 * 1024
    receipt = {'format': 'fbc-native-drop-1', 'system': platform.system(),
               'source_graph': {group: rows[group].split() for group in libraries},
               'compiler': str(compiler), 'compiler_sha256': digest(compiler),
               'inputs': pins, 'steps': records, 'output_bytes': size,
               'native_protocol_tested': os.name == 'nt', 'physical_drag_tested': False,
               'installed_archives_modified': False}
    (output / 'qualification.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('GFX_DROP_BUILD=PASS system=' + platform.system(), flush=True)


if __name__ == '__main__':
    main()
# end of test-gfx-drop.py
