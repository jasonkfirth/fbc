"""FreeBASIC runtime; File: stage-secure-random.py.
Purpose: Stage a private MT runtime containing the qualified randomness API.
Responsibilities: Preserve SDK/member bytes, reproduce objects and test linking.
This helper does not install libraries, replace a compiler or run lint.
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


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    controller = Path(__file__).resolve()
    controller_sha256 = digest(controller)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--sdk-library', type=Path, required=True)
    parser.add_argument('--ar', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--dos-runtime-qualification', type=Path)
    args = parser.parse_args()
    qpath = args.qualification.resolve(strict=True)
    q = json.loads(qpath.read_text())
    dos = q['format'] == 'fbc-secure-random-dos-build-1'
    assert dos or q['format'] == 'fbc-secure-random-native-test-1'
    if not dos:
        assert q['system'] == platform.system()
        assert any(row['label'] == 'run-live' and row['actual_exit'] == 0 for row in q['steps'])
    else:
        assert platform.system() == 'Windows' and args.dos_runtime_qualification
        runtime = json.loads(args.dos_runtime_qualification.read_text())
        assert runtime['emulator_kind'] == 'candidate' and runtime['actual_host_exit'] == 0
        assert {row['label'] for row in runtime['cases']} == {'device', 'live', 'scratch'}
        assert all(row['actual_DOS_exit_zero'] for row in runtime['cases'])
        for name, wanted in runtime['pins'].items():
            assert digest(name) == wanted, name
    assert all(row['actual_exit'] == 0 for row in q['steps'])
    steps = {row['label']: row for row in q['steps']}
    compile_common = steps['compile-common']['command']
    source_root = Path(compile_common[compile_common.index('-c') + 1]).parents[2]
    source_pins = {name.replace('\\', '/'): wanted for name, wanted in q['inputs'].items()}
    for name, wanted in source_pins.items():
        assert digest(source_root / name) == wanted, name
    for name, wanted in q['tools'].items():
        assert digest(name) == wanted, name
    sdk_library = args.sdk_library.resolve(strict=True)
    assert sdk_library.name in ('libfbmt.a', 'libfbmtpic.a') and sdk_library.stat().st_size <= 32 * 1024**2
    pic = sdk_library.name == 'libfbmtpic.a'
    assert not dos or not pic
    ar = Path(shutil.which(args.ar)).resolve(strict=True)
    output = args.output.resolve()
    assert not output.exists() and output.parent.is_dir() and not output.parent.is_symlink()
    assert not output.is_relative_to(sdk_library.parent)
    assert shutil.disk_usage(output.parent).free > 128 * 1024**2
    pins = {str(qpath): digest(qpath), str(sdk_library): digest(sdk_library), str(ar): digest(ar)}
    output.mkdir()
    (output / '.owned').write_text('FreeBASIC secure-random MT runtime overlay\n')
    (output / 'lib').mkdir()
    (output / 'inc').mkdir()
    header = output / 'inc/fbrandom.bi'
    header.write_bytes((source_root / 'inc/fbrandom.bi').read_bytes())
    test_source = output / 'live.bas'
    test_source.write_bytes((source_root / 'tests/secure-random/live.bas').read_bytes())
    environment = os.environ.copy()
    cc = Path(compile_common[0])
    environment['PATH'] = str(cc.parent) + os.pathsep + environment.get('PATH', '')
    if platform.system() == 'Haiku':
        environment['LIBRARY_PATH'] = '/boot/system/lib'
    if dos:
        for name in ('AS', 'AR', 'LD', 'GCC'):
            environment[name] = str(cc.parent / ('i586-pc-msdosdjgpp-' + name.lower() + '.exe'))
        environment['TEMP'] = environment['TMP'] = str(output)
    records = []

    def run(label, command, binary=False):
        started = time.monotonic()
        result = subprocess.run([str(value) for value in command], cwd=output, env=environment,
                                capture_output=True, timeout=120)
        bound = 34 * 1024**2 if binary else 128 * 1024
        assert len(result.stdout) <= bound and len(result.stderr) < 128 * 1024
        record = {'label': label, 'command': [str(value) for value in command],
                  'actual_exit': result.returncode, 'seconds': time.monotonic() - started,
                  'stdout_sha256': hashlib.sha256(result.stdout).hexdigest(),
                  'stderr_sha256': hashlib.sha256(result.stderr).hexdigest()}
        if not binary:
            log = output / (label + '.log')
            log.write_bytes(result.stdout + result.stderr)
            record.update(log=log.name, log_sha256=digest(log))
        records.append(record)
        (output / 'progress.json').write_text(json.dumps(records, indent=2) + '\n')
        print('SECURE_RANDOM_RUNTIME_STAGE=' + label + ' actual_exit=' + str(result.returncode), flush=True)
        if result.returncode:
            print(result.stderr.decode(errors='replace'), flush=True)
        result.check_returncode()
        return result.stdout

    old_names = run('base-members', [ar, 't', sdk_library]).decode().splitlines()
    assert old_names and len(old_names) < 4096
    new_names = ['sys_secure_random.o', 'sys_hsecure_random.o']
    assert not set(old_names).intersection(new_names)
    old_contents = run('base-member-bytes', [ar, 'p', sdk_library], binary=True)
    objects = []
    for label, filename in zip(('common', 'provider'), new_names):
        command = list(steps['compile-' + label]['command'])
        index = command.index('-o') + 1
        previous = Path(command[index])
        path = output / 'lib' / filename
        command[index] = str(path)
        if pic:
            command.insert(command.index('-c'), '-fPIC')
        run('compile-' + label, command)
        # Standard MT reproduces the object used by the accepted live test.
        # A PIC archive adds the required code-generation flag; the linked
        # program below qualifies that variant through the same public API.
        if not pic:
            assert digest(path) == digest(previous), label
        objects.append(path)
    library = output / 'lib' / sdk_library.name
    shutil.copyfile(sdk_library, library)
    run('add-api-members', [ar, 'rcs', library, *objects])
    assert run('overlay-members', [ar, 't', library]).decode().splitlines() == old_names + new_names
    contents = run('overlay-member-bytes', [ar, 'p', library], binary=True)
    printed = run('object-print-mode', [ar, 'p', library, objects[0].name], binary=True)
    raw = objects[0].read_bytes()
    expanded = False
    if printed != raw:
        # Some native Windows ar builds print through CRT text-mode stdout.
        # Its LF expansion is injective: compare the identical representation
        # for the base prefix and the new raw objects. No file is normalized.
        assert os.name == 'nt' and printed == raw.replace(b'\n', b'\r\n')
        expanded = True
    added = b''.join(path.read_bytes() for path in objects)
    if expanded:
        added = added.replace(b'\n', b'\r\n')
    assert contents == old_contents + added
    link = list(steps['link-live' if dos else 'build-live']['command'])
    old_source = next(value for value in link if value.endswith('live.bas'))
    for label in ('common', 'provider'):
        previous = steps['compile-' + label]['command']
        link.remove(previous[previous.index('-o') + 1])
    link[link.index(old_source)] = str(test_source)
    image = output / ('SRLIB.EXE' if dos else ('from-archive.exe' if os.name == 'nt' else 'from-archive'))
    link[link.index('-x') + 1] = str(image)
    link[1:1] = ['-i', str(output / 'inc'), '-p', str(output / 'lib')]
    run('link-from-archive', link)
    if not dos:
        result = run('run-from-archive', [image])
        assert b'SECURE_RANDOM_NATIVE=PASS' in result
    for name, wanted in pins.items():
        assert digest(name) == wanted, name
    for name, wanted in q['tools'].items():
        assert digest(name) == wanted, name
    for name, wanted in source_pins.items():
        assert digest(source_root / name) == wanted, name
    size = sum(path.stat().st_size for path in output.rglob('*') if path.is_file())
    assert size < 64 * 1024**2
    assert digest(controller) == controller_sha256
    receipt = {'format': 'fbc-secure-random-mt-overlay-1', 'system': 'DOS' if dos else platform.system(),
               'controller': str(controller), 'controller_sha256': controller_sha256,
               'base_library': str(sdk_library), 'base_library_sha256': pins[str(sdk_library)],
               'base_member_count': len(old_names), 'base_member_bytes_preserved': True,
               'ar_stdout_LF_expansion_checked': expanded,
               'qualified_source_receipt': str(qpath), 'qualified_source_receipt_sha256': pins[str(qpath)],
               'inputs': source_pins, 'tools': pins, 'steps': records, 'output_bytes': size,
               'library': str(library), 'library_sha256': digest(library), 'header_sha256': digest(header),
               'image': str(image), 'image_sha256': digest(image),
               'api_object_sha256': {path.name: digest(path) for path in objects},
               'runtime_test_passed': not dos, 'DOS_overlay_runtime_test_still_required': dos,
               'multithreaded_archive_only': True, 'installed_sdk_modified': False,
               'position_independent_archive': pic,
               'compiler_rebuilt_or_replaced': False, 'lint_executed': False}
    (output / 'qualification.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('SECURE_RANDOM_MT_OVERLAY=PASS system=' + receipt['system'] + ' runtime_tested=' + str(not dos), flush=True)


if __name__ == '__main__':
    main()
# end of stage-secure-random.py
