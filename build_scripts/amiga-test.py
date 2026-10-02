#!/usr/bin/env python3
"""
FreeBASIC classic AmigaOS qualification
--------------------------------------

File: amiga-test.py

Purpose:
    Build and run fbctests and Exampleageddon through a native FS-UAE controller.

Responsibilities:
    - snapshot the compiler and target libraries before compiling a matrix
    - preserve compile failures and native process return codes per program
    - stage actual resources in disposable host-directory boot volumes
    - bound guest hangs and preserve reports across interrupted runs

This file intentionally does NOT contain:
    - SDK provisioning, ROM distribution, or interactive example automation
"""

import argparse
import csv
from contextlib import contextmanager
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import subprocess
import struct
import time
import wave
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
REQUIRED_GROUPS = ('self-contained', 'fbctests', 'native-probe')


@contextmanager
def virtual_display(output):
    """Keep emulation and screenshots away from the user's desktop and GPU."""
    fallback = ROOT / 'out/amiga/virtual-display/usr/bin/Xvfb'
    executable = os.environ.get('AMIGA_XVFB_BIN') or shutil.which('Xvfb')
    if executable is None and fallback.is_file(): executable = str(fallback)
    if executable is None:
        raise SystemExit('Headless tests require Xvfb; install xvfb or set AMIGA_XVFB_BIN')
    env = os.environ.copy()
    env.pop('WAYLAND_DISPLAY', None)
    env.update({'LIBGL_ALWAYS_SOFTWARE': '1', 'GALLIUM_DRIVER': 'llvmpipe',
                'LP_NUM_THREADS': '2', 'SDL_VIDEODRIVER': 'x11', 'ALSOFT_DRIVERS': 'null'})
    authority = output / 'xauthority'
    cookie = os.urandom(16)
    server = None
    log = (output / 'xvfb.log').open('w')
    try:
        for number in range(100, 1000):
            socket = Path('/tmp/.X11-unix') / ('X' + str(number))
            if socket.exists() or Path(f'/tmp/.X{number}-lock').exists(): continue
            # Xauthority records use network-order length fields. FamilyWild
            # accepts the local transport; the display number and random
            # cookie still restrict access to this isolated server.
            fields = (b'', str(number).encode(), b'MIT-MAGIC-COOKIE-1', cookie)
            data = struct.pack('!H', 65535)
            for field in fields: data += struct.pack('!H', len(field)) + field
            authority.write_bytes(data)
            authority.chmod(0o600)
            env['DISPLAY'] = f':{number}'
            env['XAUTHORITY'] = str(authority)
            server = subprocess.Popen([executable, env['DISPLAY'], '-screen', '0', '800x600x24',
                                       '-nolisten', 'tcp', '-auth', str(authority), '-noreset'],
                                      env=env, stdout=log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 5
            while server.poll() is None and not socket.exists() and time.monotonic() < deadline:
                time.sleep(0.05)
            if server.poll() is None and socket.exists(): break
            if server.poll() is None:
                server.terminate(); server.wait(timeout=5)
            server = None
        if server is None: raise SystemExit('Could not start isolated Xvfb; see ' + str(output / 'xvfb.log'))
        yield env
    finally:
        if server is not None and server.poll() is None:
            server.terminate()
            try: server.wait(timeout=5)
            except subprocess.TimeoutExpired: server.kill(); server.wait()
        log.close()
        authority.unlink(missing_ok=True)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_command(command, log, cwd=None, env=None, timeout=1800):
    log.parent.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ if env is None else env)
    temporary = log.parent.parent / 'tmp'
    temporary.mkdir(parents=True, exist_ok=True)
    env['TMPDIR'] = str(temporary)
    with log.open('w') as output:
        try:
            return subprocess.run(command, cwd=cwd, env=env, stdout=output,
                                  stderr=subprocess.STDOUT, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            output.write('\nHost build timeout\n')
            return 124


def copy_sources(source, destination):
    ignored = shutil.ignore_patterns('*.o', '*.a', '*.asm', '*.exe', '*.log', '*.failed',
                                     'fbc-tests', 'unit-tests.inc', 'unit-tests-obj.lst',
                                     '.git', '__pycache__')
    shutil.copytree(source, destination, dirs_exist_ok=True, ignore=ignored)


def snapshot(output):
    destination = output / 'compiler'
    destination.mkdir(parents=True, exist_ok=True)
    copy_sources(ROOT / 'inc', destination / 'inc')
    (destination / 'bin').mkdir(exist_ok=True)
    shutil.copy2(ROOT / 'bin/fbc', destination / 'bin/fbc')
    libraries = destination / 'lib/freebasic/amiga-m68k'
    libraries.mkdir(parents=True, exist_ok=True)
    shutil.copytree(ROOT / 'lib/freebasic/amiga-m68k', libraries, dirs_exist_ok=True)
    hashes = {str(path.relative_to(destination)): digest(path)
              for path in destination.rglob('*') if path.is_file()}
    (output / 'compiler-hashes.json').write_text(json.dumps(hashes, indent=2) + '\n')
    return destination


def verify_snapshot(output):
    """A resumed matrix must retain the exact compiler and libraries it used."""
    prefix = output / 'compiler'
    hashes = json.loads((output / 'compiler-hashes.json').read_text())
    actual = {str(path.relative_to(prefix)): digest(path)
              for path in prefix.rglob('*') if path.is_file()}
    if hashes != actual:
        raise SystemExit('Compiler snapshot changed; use a new output directory')
    inputs = output / 'input-hashes.json'
    if inputs.is_file():
        saved = json.loads(inputs.read_text())
        if saved['hashes'] != input_hashes(output, saved['binaries']):
            raise SystemExit('Qualification inputs changed; use a new output directory')


def input_hashes(output, binaries):
    files = set(output / name for name in binaries)
    for name in ('source', 'work'):
        files.update(path for path in (output / name).rglob('*') if path.is_file())
    return {str(path.relative_to(output)): digest(path) for path in sorted(files)}


def freeze_inputs(output, records):
    binaries = [str(Path(record['binary']).relative_to(output)) for record in records
                if record['compile_status'] == 'pass']
    data = {'binaries': binaries, 'hashes': input_hashes(output, binaries)}
    (output / 'input-hashes.json').write_text(json.dumps(data, indent=2) + '\n')


def load_example_module():
    specification = importlib.util.spec_from_file_location('amiga_examples',
                                                          ROOT / 'build_scripts/exampleageddon-freebasic.py')
    module = importlib.util.module_from_spec(specification)
    import sys
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def compile_unit_tests(args, prefix, env):
    work = args.output / 'work'
    tests = work / 'tests'
    copy_sources(ROOT / 'tests', tests)
    (work / 'inc').symlink_to(prefix / 'inc', target_is_directory=True)
    copy_sources(ROOT / 'mk', work / 'mk')
    text = (ROOT / 'tests/dirlist.mk').read_text().split('DIRLIST_FB :=', 1)[1]
    available = text.split('DIRLIST_FBLITE', 1)[0].replace('\\', '').split()
    selected = args.dirs.split(',') if args.dirs else available
    if not selected or any(name not in available for name in selected):
        raise SystemExit('Unknown fbctests directory; consult tests/dirlist.mk')
    compiler = shlex.join([str(prefix / 'bin/fbc'), '-prefix', str(prefix),
                          '-i', str(prefix / 'inc'), '-i', str(prefix / 'inc/amiga')])
    common = ['make', '-s', '-f', 'unit-tests.mk',
              'FBC=' + compiler, 'TARGET=amiga-m68k', 'TARGET_TRIPLET=m68k-amigaos',
              'CC=' + str(args.toolchain_root / 'bin/m68k-amigaos-gcc'),
              'AR=' + str(args.toolchain_root / 'bin/m68k-amigaos-ar'),
              'DIRLIST_INC=amiga-dirlist.mk']
    records = []
    binaries = args.output / 'bin'
    binaries.mkdir(exist_ok=True)
    for index, name in enumerate(selected):
        (tests / 'amiga-dirlist.mk').write_text('DIRLIST_FB := ' + name + '\n')
        if run_command([*common, 'clean'], args.output / 'logs' / (name + '.clean.log'), tests, env) != 0:
            raise SystemExit('Staged test cleanup failed; see logs/' + name + '.clean.log')
        code = run_command([*common, '-j' + str(args.jobs), 'build_tests'],
                           args.output / 'logs' / (name + '.compile.log'), tests, env)
        binary = binaries / name
        if code == 0 and (tests / 'fbc-tests').is_file():
            shutil.copy2(tests / 'fbc-tests', binary)
        else:
            code = code or 1
        records.append({'id': f'{index:06d}', 'path': name, 'group': 'fbctests',
                        'compile_status': 'pass' if code == 0 else f'fail({code})',
                        'run_status': 'not-run', 'binary': str(binary), 'resources': str(tests),
                        'arguments': '--verbose --brief-summary --xml fbc-results.xml',
                        'expected': 0})
        print(f'fbctests {name}: {records[-1]["compile_status"]}', flush=True)
    return records


def compile_examples(args, prefix, env):
    inventory = args.output / 'inventory'
    source_root = args.output / 'source'
    copy_sources(ROOT / 'examples', source_root / 'examples')
    command = ['python3', str(ROOT / 'build_scripts/exampleageddon-freebasic.py'),
               '--root', str(source_root), '--outdir', str(inventory), '--prefix', str(prefix),
               '--include-dir', str(prefix / 'inc'),
               '--fbc', str(prefix / 'bin/fbc'), '--fbc-arg=-target', '--fbc-arg=amiga-m68k',
               '--fbc-arg=-i', '--fbc-arg=' + str(prefix / 'inc/amiga'),
               '--target-os', 'amiga', '--jobs', str(args.jobs), '--no-run', '--keep-work',
               '--main-module-from-source', '--compile-timeout', str(args.compile_timeout)]
    code = run_command(command, args.output / 'logs/inventory.log', env=env, timeout=7200)
    if code not in (0, 1) or not (inventory / 'results.csv').is_file():
        raise SystemExit('Example inventory did not complete; see logs/inventory.log')
    records = []
    with (inventory / 'results.csv').open() as stream:
        for index, row in enumerate(csv.DictReader(stream)):
            source = source_root / row['path']
            records.append({'id': f'{index:06d}', 'path': row['path'], 'group': row['group'],
                            'compile_status': row['compile_status'], 'run_status': 'not-run',
                            'binary': str(inventory / row['output']), 'resources': str(source.parent),
                            'arguments': '', 'expected': 0})
    return records


def compile_probes(args, prefix, env):
    source = args.output / 'source'
    copy_sources(ROOT / 'tests/amiga', source / 'tests/amiga')
    for subsystem in ('rtlib', 'gfxlib2', 'sfxlib'):
        for header in (ROOT / 'src' / subsystem).rglob('*.h'):
            destination = source / header.relative_to(ROOT)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(header, destination)
    conversion = source / 'build_scripts/amiga/softfloat-convert.c'
    conversion.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / 'build_scripts/amiga/softfloat-convert.c', conversion)
    resources = source / 'tests/amiga'
    binaries = args.output / 'bin'
    binaries.mkdir(exist_ok=True)
    cc = str(args.toolchain_root / 'bin/m68k-amigaos-gcc')
    cflags = ['-m68020', '-msoft-float', '-fno-common', '-O2']
    fbc = [str(prefix / 'bin/fbc'), '-prefix', str(prefix), '-target', 'amiga-m68k',
           '-i', str(prefix / 'inc'), '-i', str(prefix / 'inc/amiga')]
    records = []
    for name in ('native-io', 'graphics', 'sound', 'softfloat-convert'):
        binary = binaries / name
        if name == 'softfloat-convert':
            oracle = binaries / 'host-float-oracle'
            command = [os.environ.get('HOST_CC', 'cc'), '-O2', '-fno-builtin',
                       str(resources / (name + '.c')), str(conversion), '-o', str(oracle)]
            code = run_command(command, args.output / 'logs/oracle-build.log', env=env)
            if code == 0:
                code = run_command([str(oracle), str(resources / 'float-reference.bin')],
                                   args.output / 'logs/oracle.log', env=env)
            if code != 0: raise SystemExit('Host floating-point oracle failed; see logs/oracle.log')
            command = [cc, *cflags, str(resources / (name + '.c')),
                       '-L' + str(prefix / 'lib/freebasic/amiga-m68k'),
                       '-Wl,--whole-archive', '-lfbsoftfloat', '-Wl,--no-whole-archive',
                       '-o', str(binary)]
            code = run_command(command, args.output / 'logs' / (name + '.compile.log'), env=env)
        else:
            objects = []
            code = 0
            if name in ('graphics', 'sound'):
                bridge = binaries / (name + '-bridge.o')
                code = run_command([cc, *cflags, '-c', str(resources / (name + '-bridge.c')),
                                    '-o', str(bridge)], args.output / 'logs' / (name + '-bridge.log'), env=env)
                objects.append(str(bridge))
            if code == 0:
                code = run_command([*fbc, str(resources / (name + '.bas')), *objects,
                                    '-x', str(binary)], args.output / 'logs' / (name + '.compile.log'), env=env)
        record = {'id': f'{len(records):06d}', 'path': 'tests/amiga/' + name,
                  'group': 'native-probe', 'compile_status': 'pass' if code == 0 else f'fail({code})',
                  'run_status': 'not-run', 'binary': str(binary), 'resources': str(resources),
                  'arguments': '', 'expected': 0}
        if name == 'sound': record['audio_capture'] = 'native-sound.wav'
        records.append(record)
        print(f'Amiga probe {name}: {record["compile_status"]}', flush=True)
    if args.native_compiler is not None:
        binary = binaries / 'native-fbc'
        shutil.copy2(args.native_compiler, binary)
        copy_sources(prefix / 'inc', resources / 'inc')
        (resources / 'native-source.bas').write_text(
            '#include once "crt/stdio.bi"\n'
            '#include once "crt/time.bi"\n'
            '#include once "crt/wchar.bi"\n'
            'if command(1) <> "native-argument" then end 1\n'
            'print "Compiled by native Amiga FreeBASIC"\nend 0\n')
        records.append({'id': f'{len(records):06d}', 'path': 'native compiler C emission',
                        'group': 'native-probe', 'compile_status': 'pass', 'run_status': 'not-run',
                        'binary': str(binary), 'resources': str(resources), 'expected': 0,
                        'arguments': '-i inc -r -m native-source native-source.bas',
                        'native_c_output': 'native-source.c'})
    return records


def compile_native_output(args, record, env):
    """Link guest-generated C through the cross SDK, then run its native Hunk."""
    prefix = args.output / 'compiler'
    source = args.output / 'source/native-generated.c'
    guest = Path(record['guest_log']).parent.parent / 'jobs' / record['id'] / record['native_c_output']
    binary = args.output / 'bin/native-generated'
    objects = binary.with_suffix('.o')
    code = 1
    if guest.is_file():
        shutil.copy2(guest, source)
        command = [str(args.toolchain_root / 'bin/m68k-amigaos-gcc'), '-m68020', '-msoft-float',
                   '-fno-common', '-O2', '-c', str(source), '-o', str(objects)]
        code = run_command(command, args.output / 'logs/native-generated.compile.log', env=env)
        if code == 0:
            command = [str(prefix / 'bin/fbc'), '-prefix', str(prefix), '-target', 'amiga-m68k',
                       str(objects), '-x', str(binary)]
            code = run_command(command, args.output / 'logs/native-generated.link.log', env=env)
    return {'id': f'{int(record["id"]) + 1:06d}', 'path': 'native compiler generated program',
            'group': 'native-probe', 'compile_status': 'pass' if code == 0 else f'fail({code})',
            'run_status': 'not-run', 'binary': str(binary), 'resources': record['resources'],
            'arguments': 'native-argument', 'expected': 0, 'native_generated': True}


def build_controller(args):
    destination = args.output / 'guest-runner'
    command = [str(args.toolchain_root / 'bin/m68k-amigaos-gcc'), '-m68020', '-msoft-float',
               '-O2', '-ffreestanding', '-fno-builtin', '-fno-tree-loop-distribute-patterns',
               '-nostdlib', '-nostartfiles', str(ROOT / 'build_scripts/amiga/guest-entry.s'),
               str(ROOT / 'build_scripts/amiga/guest-runner.c'), '-lgcc', '-o', str(destination)]
    if run_command(command, args.output / 'logs/controller-build.log') != 0:
        raise SystemExit('Guest controller build failed; see logs/controller-build.log')
    for name in ('list', 'cat'):
        command = [str(args.toolchain_root / 'bin/m68k-amigaos-gcc'), '-m68020', '-msoft-float',
                   '-O2', str(ROOT / 'build_scripts/amiga' / (name + '.c')),
                   '-o', str(args.output / ('fb-' + name))]
        if run_command(command, args.output / 'logs' / (name + '-build.log')) != 0:
            raise SystemExit('Native test command build failed: ' + name)
    return destination


def stage_batch(args, controller, records, batch):
    boot = args.output / 'guests' / f'{batch:04d}'
    boot.mkdir(parents=True, exist_ok=True)
    for name in ('C', 'S', 'T', 'ENV', 'LIBS', 'DEVS', 'results', 'jobs'):
        (boot / name).mkdir(exist_ok=True)
    if args.backend_debug:
        (boot / 'ENV/FB_GFX_AMIGA_DEBUG').write_text('1')
        (boot / 'ENV/SFXLIB_DEBUG').write_text('1')
    shutil.copy2(controller, boot / 'C/fb-runner')
    for name in ('list', 'cat'):
        shutil.copy2(args.output / ('fb-' + name), boot / ('C/fb-' + name))
    (boot / 'S/Startup-Sequence').write_text('C:fb-runner\n')
    lines = []
    for record in records:
        directory = boot / 'jobs' / record['id']
        source = Path(record['resources'])
        if record['group'] == 'fbctests': copy_sources(source, directory)
        else:
            module = load_example_module()
            module.prepare_run_directory(source, directory,
                                         source != args.output / 'source/examples' and source != ROOT / 'examples')
        shutil.copy2(record['binary'], directory / 'program')
        fields = [record['id'], 'SYS:jobs/' + record['id'] + '/program',
                  'SYS:jobs/' + record['id'], record['arguments'], str(record['expected'])]
        if any(any(char in field for char in '\n\r\t\0') for field in fields):
            raise SystemExit('Invalid guest manifest field')
        lines.append('\t'.join(fields))
    (boot / 'jobs.tsv').write_text('\n'.join(lines) + '\n', encoding='latin-1')
    config = boot.with_suffix('.fs-uae')
    config.write_text('[fs-uae]\n'
                      'amiga_model = A1200/020\nchip_memory = 2048\nfast_memory = 8192\n'
                      'zorro_iii_memory = 262144\nuae_cpu_speed = max\nvideo_sync = off\n'
                      f'accuracy = {getattr(args, "accuracy", 0)}\n'
                      f'jit_compiler = {int(getattr(args, "jit", False))}\n'
                      f'bsdsocket_library = {int(getattr(args, "bsdsocket", False))}\n'
                      'fullscreen = 0\nwindow_width = 800\nwindow_height = 600\n'
                      'uae_sound_output = interrupts\n'
                      f'kickstart_file = {args.kickstart}\n'
                      f'hard_drive_0 = {boot}\nhard_drive_0_label = FBCTest\n'
                      'hard_drive_0_priority = 10\n')
    return boot, config


def execute_batch(args, controller, records, batch):
    with virtual_display(args.output) as env:
        return execute_batch_on_display(args, controller, records, batch, env)


def execute_batch_on_display(args, controller, records, batch, env):
    boot, config = stage_batch(args, controller, records, batch)
    log = args.output / 'logs' / f'guest-{batch:04d}.log'
    started = time.monotonic()
    progress_time = started
    observed = ''
    codes = {}
    active = None
    with log.open('w') as output:
        process = subprocess.Popen([args.emulator, str(config)], env=env,
                                   stdout=output, stderr=subprocess.STDOUT)
        try:
            while process.poll() is None:
                marker = boot / 'guest-results.tsv'
                text = marker.read_text(errors='replace') if marker.exists() else ''
                if text != observed:
                    observed = text
                    progress_time = time.monotonic()
                    # The controller writes fields separately. A visible
                    # partial DONE record has no return code yet; wait for
                    # its newline before interpreting any protocol record.
                    for line in text.splitlines(keepends=True):
                        if not line.endswith('\n'): continue
                        fields = line.rstrip('\r\n').split('\t')
                        if len(fields) == 2 and fields[0] == 'BEGIN': active = fields[1]
                        if len(fields) == 3 and fields[0] == 'DONE':
                            codes[fields[1]] = int(fields[2])
                    if text.endswith('COMPLETE\n') or text.endswith('INVALID\n'): break
                if time.monotonic() - progress_time > args.timeout: break
                time.sleep(0.25)
        finally:
            if not observed.endswith('COMPLETE\n'):
                try:
                    windows = subprocess.check_output(['xdotool', 'search', '--pid', str(process.pid),
                                                       '--name', 'FS-UAE'], text=True,
                                                      env=env, stderr=subprocess.DEVNULL).splitlines()
                    if windows:
                        subprocess.run(['import', '-window', windows[-1], str(boot / 'timeout.png')],
                                       env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
                except (OSError, subprocess.SubprocessError):
                    pass
            if process.poll() is None:
                process.terminate()
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired: process.kill(); process.wait()
    for record in records:
        identifier = record['id']
        record['guest_log'] = str(boot / 'results' / (identifier + '.log'))
        if identifier in codes:
            record['return_code'] = codes[identifier]
            record['run_status'] = 'pass' if codes[identifier] == record['expected'] else f'fail({codes[identifier]})'
        elif 'BOOT\n' not in observed: record['run_status'] = 'boot-failed'
        elif observed.endswith('INVALID\n') or observed.endswith('COMPLETE\n'):
            record['run_status'] = 'controller-failed'
        elif identifier == active: record['run_status'] = 'timeout-or-crash'
        else: record['run_status'] = 'not-reached'
        if record['run_status'] == 'pass' and record['group'] == 'fbctests':
            report = boot / 'jobs' / identifier / 'fbc-results.xml'
            validate_unit_report(record, report)
        if record['run_status'] == 'pass' and record.get('audio_capture'):
            validate_sound_capture(record, boot / 'jobs' / identifier / record['audio_capture'])
        print(f'{record["path"]}: {record["run_status"]}', flush=True)
    return [record for record in records if record['run_status'] == 'not-reached']


def validate_sound_capture(record, path):
    """Check duration, amplitude, stereo ordering, and known tone frequencies."""
    try:
        with wave.open(str(path), 'rb') as stream:
            rate, frames = stream.getframerate(), stream.getnframes()
            if stream.getnchannels() != 2 or stream.getsampwidth() != 2 or frames != rate // 4:
                raise ValueError('Unexpected PCM format or duration')
            values = [sample[0] for sample in struct.iter_unpack('<h', stream.readframes(frames))]
        metrics = []
        for channel, frequency in enumerate((440, 880)):
            samples = values[channel::2]
            rms = math.sqrt(sum(sample * sample for sample in samples) / frames)
            peak = max(abs(sample) for sample in samples)
            strengths = []
            for tone in (frequency, 880 if frequency == 440 else 440):
                strengths.append(abs(sum(sample * complex(math.cos(2 * math.pi * tone * index / rate),
                                                          math.sin(2 * math.pi * tone * index / rate))
                                         for index, sample in enumerate(samples))) / frames)
            if not (11000 < rms < 12000 and 16000 < peak < 17000 and
                    7000 < strengths[0] < 9000 and strengths[1] < strengths[0] / 20):
                raise ValueError('PCM amplitude or stereo frequency check failed')
            metrics.append({'frequency': frequency, 'rms': rms, 'peak': peak,
                            'strength': strengths[0], 'other_channel_tone': strengths[1]})
        record['audio_metrics'] = {'rate': rate, 'frames': frames, 'channels': metrics}
    except (OSError, wave.Error, ValueError, ZeroDivisionError) as error:
        record['run_status'] = 'invalid-audio-capture'
        record['report_error'] = str(error)


def validate_unit_report(record, report):
    """A return code alone cannot prove that every registered test ran."""
    if not report.is_file() or report.stat().st_size == 0:
        record['run_status'] = 'missing-test-report'
        return
    try:
        root = ET.parse(report).getroot()
        if root.tag != 'testsuites': raise ValueError('Unexpected XML root')
        totals = {'tests': 0, 'assertions': 0, 'failures': 0, 'errors': 0}
        for suite in root:
            if suite.tag != 'testsuite': raise ValueError('Unexpected XML suite')
            for name in totals:
                value = int(suite.attrib[name])
                if value < 0: raise ValueError('Negative test count')
                totals[name] += value
            cases = list(suite)
            if any(case.tag != 'testcase' for case in cases):
                raise ValueError('Unexpected XML case')
            if len(cases) != int(suite.attrib['tests']):
                raise ValueError('Test-case count differs from suite count')
        record.update(totals)
        if totals['failures'] or totals['errors']:
            record['run_status'] = 'failed-test-report'
    except (OSError, ET.ParseError, KeyError, ValueError) as error:
        record['run_status'] = 'invalid-test-report'
        record['report_error'] = str(error)


def save_results(args, records):
    temporary = args.output / 'results.json.new'
    temporary.write_text(json.dumps(records, indent=2) + '\n')
    temporary.replace(args.output / 'results.json')
    fields = ['id', 'path', 'group', 'compile_status', 'run_status', 'return_code', 'guest_log']
    with (args.output / 'results.csv').open('w', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction='ignore')
        writer.writeheader(); writer.writerows(records)
    lines = ['# AmigaOS qualification', '', '| Program | Compile | Guest |', '| --- | --- | --- |']
    lines.extend(f'| {record["path"]} | {record["compile_status"]} | {record["run_status"]} |' for record in records)
    lines.extend(['', '<!-- end of report.md -->', ''])
    (args.output / 'report.md').write_text('\n'.join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('fbctests', 'exampleageddon', 'probes'))
    parser.add_argument('--output', type=Path)
    parser.add_argument('--toolchain-root', type=Path, default=ROOT / 'out/amiga/toolchain')
    parser.add_argument('--kickstart', type=Path, default=os.environ.get('AMIGA_KICKSTART'))
    parser.add_argument('--emulator', default=os.environ.get('FS_UAE_BIN', 'fs-uae'))
    parser.add_argument('--dirs', default='')
    parser.add_argument('--jobs', type=int, default=8)
    parser.add_argument('--batch-size', type=int, default=16)
    parser.add_argument('--timeout', type=int, default=120)
    parser.add_argument('--compile-timeout', type=int, default=120)
    parser.add_argument('--compile-only', action='store_true')
    parser.add_argument('--resume', action='store_true')
    parser.add_argument('--backend-debug', action='store_true')
    parser.add_argument('--native-compiler', type=Path,
                        help='Also qualify this native compiler in probes mode')
    parser.add_argument('--accuracy', type=int, choices=(-1, 0, 1), default=0,
                        help='FS-UAE accuracy; 0 permits accelerated CPU execution')
    parser.add_argument('--jit', action='store_true', help='Enable FS-UAE integer CPU JIT')
    args = parser.parse_args()
    if min(args.jobs, args.batch_size, args.timeout, args.compile_timeout) <= 0:
        parser.error('Counts and timeouts must be positive')
    args.output = (args.output or ROOT / 'out/amiga' / args.mode).resolve()
    args.toolchain_root = args.toolchain_root.resolve()
    if args.native_compiler is not None:
        if args.mode != 'probes' or not args.native_compiler.is_file():
            parser.error('--native-compiler requires probes mode and an existing native binary')
        args.native_compiler = args.native_compiler.resolve()
    if not args.compile_only:
        if args.kickstart is None or not args.kickstart.is_file(): parser.error('Provide --kickstart or AMIGA_KICKSTART')
        args.kickstart = args.kickstart.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.pop('DEBUG', None)
    env['PATH'] = str(args.toolchain_root / 'bin') + os.pathsep + env['PATH']
    result_file = args.output / 'results.json'
    if args.resume and result_file.is_file():
        verify_snapshot(args.output)
        records = json.loads(result_file.read_text())
    else:
        if result_file.exists(): parser.error('Use --resume or a new output directory')
        prefix = snapshot(args.output)
        if args.mode == 'fbctests': records = compile_unit_tests(args, prefix, env)
        elif args.mode == 'exampleageddon': records = compile_examples(args, prefix, env)
        else: records = compile_probes(args, prefix, env)
        freeze_inputs(args.output, records)
        save_results(args, records)
    if not args.compile_only:
        controller = build_controller(args)
        pending = [record for record in records if record['compile_status'] == 'pass' and
                   record['run_status'] != 'pass' and record['group'] in REQUIRED_GROUPS]
        batch = len(list((args.output / 'guests').glob('*.fs-uae'))) if (args.output / 'guests').exists() else 0
        while pending:
            current, pending = pending[:args.batch_size], pending[args.batch_size:]
            pending = execute_batch(args, controller, current, batch) + pending
            batch += 1
            save_results(args, records)
        native = next((record for record in records if record.get('native_c_output')), None)
        if native is not None and native['run_status'] == 'pass' and not any(
                record.get('native_generated') for record in records):
            generated = compile_native_output(args, native, env)
            records.append(generated)
            freeze_inputs(args.output, records)
            save_results(args, records)
            if generated['compile_status'] == 'pass':
                execute_batch(args, controller, [generated], batch)
                save_results(args, records)
    failed = [record for record in records if record['group'] in REQUIRED_GROUPS and
              (record['compile_status'] != 'pass' or (not args.compile_only and record['run_status'] != 'pass'))]
    print(f'{len(records)} records, {len(failed)} required failures. Report: {args.output / "report.md"}')
    return 1 if failed else 0


def interrupt_run(signum, frame):
    """Unwind emulator/display ownership when the runner receives SIGTERM."""
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    raise KeyboardInterrupt


if __name__ == '__main__':
    signal.signal(signal.SIGTERM, interrupt_run)
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print('Interrupted; resume the saved matrix with --resume', flush=True)
        raise SystemExit(130)

# end of amiga-test.py
