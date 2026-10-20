"""FreeBASIC runtime; File: test-secure-random.py.
Purpose: Build live and controlled-failure secure-random tests with an existing SDK.
Responsibilities: Exact source/tool pins, fresh bounded outputs and actual status.
This helper does not rebuild the compiler, install runtime archives or run lint.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--compiler-prefix', type=Path)
    parser.add_argument('--runtime-headers', type=Path)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    compiler = args.compiler.resolve(strict=True)
    prefix = (args.compiler_prefix or compiler.parent.parent).resolve(strict=True)
    headers = (args.runtime_headers or prefix / 'src/rtlib').resolve(strict=True)
    cc = shutil.which(args.cc)
    assert cc and compiler.is_file() and (headers / 'fb.h').is_file()
    target = {'Windows': 'win32', 'Linux': 'unix', 'FreeBSD': 'unix', 'Haiku': 'haiku'}[platform.system()]
    provider = ROOT / 'src/rtlib/drivers' / target / 'sys_hsecure_random.c'
    assert provider.is_file(), 'the selected target has no provider source'
    output = args.output.resolve()
    assert not output.exists() and output.parent.is_dir() and not output.parent.is_symlink()
    assert not any(output.is_relative_to(prefix / name) for name in ('bin', 'lib', 'inc', 'src'))
    assert shutil.disk_usage(output.parent).free > 64 * 1024 * 1024
    output.mkdir()
    (output / '.owned').write_text('FreeBASIC secure-random test outputs\n')
    environment = os.environ.copy()
    environment['PATH'] = str(Path(cc).parent) + os.pathsep + environment.get('PATH', '')
    if platform.system() == 'Haiku':
        environment['LIBRARY_PATH'] = '/boot/system/lib'
    pins = {str(path): digest(path) for path in (compiler, Path(cc), headers / 'fb.h')}
    sources = [ROOT / 'src/rtlib/services/sys_secure_random.c', provider,
               ROOT / 'src/rtlib/fb_secure_random.h', ROOT / 'inc/fbrandom.bi',
               ROOT / 'tests/secure-random/live.bas', ROOT / 'tests/secure-random/failure-provider.c']
    source_pins = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    # Compile a tiny private copy so backend intermediates cannot land beside
    # the canonical test source or another session's application sources.
    test_source = output / 'live.bas'
    test_source.write_bytes((ROOT / 'tests/secure-random/live.bas').read_bytes())
    records = []

    def run(label, command, seconds=60):
        started = time.monotonic()
        result = subprocess.run([str(value) for value in command], cwd=output, env=environment,
                                capture_output=True, timeout=seconds)
        assert len(result.stdout) + len(result.stderr) < 128 * 1024
        log = output / (label + '.log')
        log.write_bytes(result.stdout + result.stderr)
        record = {'label': label, 'command': [str(value) for value in command],
                  'actual_exit': result.returncode, 'seconds': time.monotonic() - started,
                  'log': log.name, 'log_sha256': digest(log)}
        records.append(record)
        (output / 'progress.json').write_text(json.dumps(records, indent=2) + '\n')
        print('SECURE_RANDOM_STEP=' + label + ' actual_exit=' + str(result.returncode), flush=True)
        if result.returncode:
            print((result.stdout + result.stderr).decode(errors='replace'), flush=True)
            raise RuntimeError('secure-random test failed: ' + label)
        return result

    include_paths = [headers, headers / 'drivers' / target,
                     ROOT / 'src/rtlib', ROOT / 'src/rtlib/drivers' / target]
    flags = ['-O2', '-Wall', '-Wextra', '-Werror', '-DENABLE_MT']
    for path in include_paths:
        flags += ['-I', str(path)]
    for label, source in (('common', sources[0]), ('provider', provider), ('failure-provider', sources[-1])):
        run('compile-' + label, [cc, *flags, '-c', source, '-o', output / (label + '.o')])
    extension = '.exe' if os.name == 'nt' else ''
    for label, helper in (('live', 'provider.o'), ('failure', 'failure-provider.o')):
        image = output / (label + extension)
        command = [compiler, '-prefix', prefix, '-i', ROOT / 'inc', '-mt', '-w', 'all', '-exx', '-O', '2']
        if label == 'failure':
            command += ['-d', 'TEST_SECURE_RANDOM_FAILURE=1']
        command += [test_source, output / 'common.o', output / helper, '-x', image]
        run('build-' + label, command)
        result = run('run-' + label, [image], seconds=120)
        marker = b'SECURE_RANDOM_NATIVE=PASS' if label == 'live' else b'SECURE_RANDOM_CONTROLLED_FAILURES=PASS'
        assert marker in result.stdout
    assert all(digest(Path(name)) == wanted for name, wanted in pins.items())
    assert all(digest(ROOT / name) == wanted for name, wanted in source_pins.items())
    size = sum(path.stat().st_size for path in output.rglob('*') if path.is_file())
    assert size < 32 * 1024 * 1024
    receipt = {'format': 'fbc-secure-random-native-test-1', 'system': platform.system(),
               'machine': platform.machine(), 'tools': pins, 'inputs': source_pins, 'steps': records,
               'images': {label: digest(output / (label + extension)) for label in ('live', 'failure')},
               'output_bytes': size, 'installed_archives_modified': False,
               'compiler_rebuilt_or_replaced': False, 'lint_executed': False,
               'statistical_entropy_proof_claimed': False}
    (output / 'qualification.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('SECURE_RANDOM_NATIVE_QUALIFICATION=PASS system=' + platform.system(), flush=True)


if __name__ == '__main__':
    main()
# end of test-secure-random.py
