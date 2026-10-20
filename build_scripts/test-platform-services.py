"""FreeBASIC runtime; File: test-platform-services.py.
Purpose: Check service contracts, native adapters and public BASIC examples.
Responsibilities: Bounded private objects, durable logs and exact input hashes.
This helper does not rebuild a compiler or replace installed runtime archives.
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
    parser.add_argument('--prefix', type=Path)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--live-notifications', action='store_true')
    args = parser.parse_args()
    compiler = args.compiler.resolve(strict=True)
    prefix = (args.prefix or compiler.parent.parent).resolve(strict=True)
    cc = shutil.which(args.cc)
    assert cc and compiler.is_file()
    output = args.output.resolve()
    assert not output.exists() and output.parent.is_dir()
    assert not any(output.is_relative_to(prefix / part) for part in ('bin', 'lib', 'inc', 'src'))
    assert shutil.disk_usage(output.parent).free >= 512 * 1024 * 1024
    output.mkdir()
    (output / '.owned').write_text('FreeBASIC platform service tests\n')
    target = 'win32' if os.name == 'nt' else 'unix'
    extension = '.exe' if os.name == 'nt' else ''
    env = os.environ.copy()
    env['PATH'] = str(Path(cc).parent) + os.pathsep + env.get('PATH', '')
    records = []
    inputs = {}
    flags = ['-O2', '-Wall', '-Wextra', '-Werror', '-DENABLE_MT',
             '-I', str(ROOT / 'src/rtlib'), '-I', str(ROOT / 'src/rtlib/drivers' / target)]

    def run(label, command, seconds=60):
        result = subprocess.run([str(item) for item in command], cwd=output, env=env,
                                capture_output=True, timeout=seconds)
        log = output / (label + '.log')
        assert len(result.stdout) + len(result.stderr) < 1024 * 1024
        log.write_bytes(result.stdout + result.stderr)
        records.append({'label': label, 'command': [str(item) for item in command],
                        'exit': result.returncode, 'log': log.name, 'log_sha256': digest(log)})
        (output / 'progress.json').write_text(json.dumps(records, indent=2) + '\n')
        print(label + ' exit=' + str(result.returncode), flush=True)
        if result.returncode:
            print(log.read_text(errors='replace'), flush=True)
            raise RuntimeError(label)
        return result

    def source(relative):
        path = ROOT / relative
        inputs[relative] = digest(path)
        return path

    # A deterministic clock/provider test is separate from the native objects.
    common = ['src/rtlib/services/sys_idle.c', 'src/rtlib/services/sys_notify.c',
              'src/rtlib/services/sys_service_utf8.c', 'src/rtlib/services/sys_drop.c']
    run('build-contracts', [cc, *flags, *[source(name) for name in common],
                           source('tests/services/contracts.c'), '-lm',
                           '-o', output / ('contracts' + extension)])
    result = run('run-contracts', [output / ('contracts' + extension)])
    assert b'PLATFORM_SERVICE_CONTRACTS=PASS' in result.stdout

    objects = []
    native = common + ['src/rtlib/drivers/' + target + '/sys_hidle.c',
                       'src/rtlib/drivers/' + target + '/sys_hnotify.c',
                       'src/rtlib/services/sys_secure_random.c',
                       'src/rtlib/drivers/' + target + '/sys_hsecure_random.c']
    for name in native:
        obj = output / (Path(name).stem + '.o')
        run('compile-' + obj.stem, [cc, *flags, '-c', source(name), '-o', obj])
        objects.append(obj)
    examples = ['idle', 'idle-worker', 'notify', 'secure-random', 'secure-range']
    for name in examples:
        sample = source('examples/services/' + name + '.bas')
        private = output / sample.name
        private.write_bytes(sample.read_bytes())
        image = output / (name + extension)
        run('build-' + name, [compiler, '-prefix', prefix, '-mt', '-w', 'all', '-exx',
                             '-i', ROOT / 'inc', private, *objects, '-x', image])
        # A notification can disturb the current desktop. Its explicit live
        # fixture is run separately; unattended qualification builds it only.
        if name != 'notify':
            run('run-' + name, [image], 15)
    sample = source('tests/services/notify-native.bas')
    private = output / sample.name
    private.write_bytes(sample.read_bytes())
    image = output / ('notify-native' + extension)
    run('build-notify-native', [compiler, '-prefix', prefix, '-mt', '-w', 'all', '-exx',
                               '-i', ROOT / 'inc', private, *objects, '-x', image])
    if args.live_notifications:
        result = run('run-notify-native', [image], 30)
        assert b'NOTIFY_NATIVE=PASS' in result.stdout
    # Header dependencies must also link a simple caller without -mt. The
    # threaded fixture above cannot prove this: THREADCREATE selects MT itself.
    for name in ('idle', 'notify'):
        run('build-single-thread-' + name,
            [compiler, '-prefix', prefix, '-w', 'all', '-i', ROOT / 'inc',
             output / (name + '.bas'), *objects,
             '-x', output / (name + '-single-thread' + extension)])
    run('run-single-thread-idle', [output / ('idle-single-thread' + extension)], 15)
    for path in (ROOT / 'src/rtlib').glob('fb_*.h'):
        inputs[str(path.relative_to(ROOT))] = digest(path)
    for name in ('fbservices.bi', 'fbnotify.bi', 'fbidle.bi', 'fbrandom.bi', 'fbdrop.bi', 'fbgfx.bi'):
        source('inc/' + name)
    assert all(digest(ROOT / name) == value for name, value in inputs.items())
    size = sum(item.stat().st_size for item in output.rglob('*') if item.is_file())
    assert size < 64 * 1024 * 1024
    receipt = {'format': 'fbc-platform-services-1', 'system': platform.system(),
               'compiler': {'path': str(compiler), 'sha256': digest(compiler)},
               'cc': {'path': cc, 'sha256': digest(cc)}, 'inputs': inputs,
               'steps': records, 'output_bytes': size,
               'installed_toolchain_modified': False, 'notification_display_tested': False,
               'native_notification_delivery_tested': args.live_notifications,
               'graphics_protocol_tested': False}
    (output / 'qualification.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('PLATFORM_SERVICES_NATIVE=PASS system=' + platform.system(), flush=True)


if __name__ == '__main__':
    main()
# end of test-platform-services.py
