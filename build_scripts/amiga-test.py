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
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import time


ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_command(command, log, cwd=None, env=None, timeout=1800):
    log.parent.mkdir(parents=True, exist_ok=True)
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
    for source in (ROOT / 'lib/freebasic/amiga-m68k').iterdir():
        if source.is_file(): shutil.copy2(source, libraries / source.name)
    hashes = {str(path.relative_to(destination)): digest(path)
              for path in destination.rglob('*') if path.is_file()}
    (output / 'compiler-hashes.json').write_text(json.dumps(hashes, indent=2) + '\n')
    return destination


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
    for name in ('inc', 'mk'):
        target = work / name
        if not target.exists(): target.symlink_to(ROOT / name, target_is_directory=True)
    text = (ROOT / 'tests/dirlist.mk').read_text().split('DIRLIST_FB :=', 1)[1]
    available = text.split('DIRLIST_FBLITE', 1)[0].replace('\\', '').split()
    selected = args.dirs.split(',') if args.dirs else available
    if not selected or any(name not in available for name in selected):
        raise SystemExit('Unknown fbctests directory; consult tests/dirlist.mk')
    compiler = shlex.join([str(prefix / 'bin/fbc'), '-prefix', str(prefix), '-i', str(prefix / 'inc/amiga')])
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
                        'arguments': '--brief-summary --hide-cases --xml fbc-results.xml',
                        'expected': 0})
        print(f'fbctests {name}: {records[-1]["compile_status"]}', flush=True)
    return records


def compile_examples(args, prefix, env):
    inventory = args.output / 'inventory'
    command = ['python3', str(ROOT / 'build_scripts/exampleageddon-freebasic.py'),
               '--root', str(ROOT), '--outdir', str(inventory), '--prefix', str(prefix),
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
            source = ROOT / row['path']
            records.append({'id': f'{index:06d}', 'path': row['path'], 'group': row['group'],
                            'compile_status': row['compile_status'], 'run_status': 'not-run',
                            'binary': str(inventory / row['output']), 'resources': str(source.parent),
                            'arguments': '', 'expected': 0})
    return records


def build_controller(args):
    destination = args.output / 'guest-runner'
    command = [str(args.toolchain_root / 'bin/m68k-amigaos-gcc'), '-m68020', '-msoft-float',
               '-O2', '-ffreestanding', '-fno-builtin', '-fno-tree-loop-distribute-patterns',
               '-nostdlib', '-nostartfiles', str(ROOT / 'build_scripts/amiga/guest-entry.s'),
               str(ROOT / 'build_scripts/amiga/guest-runner.c'), '-lgcc', '-o', str(destination)]
    if run_command(command, args.output / 'logs/controller-build.log') != 0:
        raise SystemExit('Guest controller build failed; see logs/controller-build.log')
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
    (boot / 'S/Startup-Sequence').write_text('C:fb-runner\n')
    lines = []
    for record in records:
        directory = boot / 'jobs' / record['id']
        source = Path(record['resources'])
        if record['group'] == 'fbctests': copy_sources(source, directory)
        else:
            module = load_example_module()
            module.prepare_run_directory(source, directory, source != ROOT / 'examples')
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
                      'fullscreen = 0\nwindow_width = 800\nwindow_height = 600\n'
                      'sound_output = none\n'
                      f'kickstart_file = {args.kickstart}\n'
                      f'hard_drive_0 = {boot}\nhard_drive_0_label = FBCTest\n'
                      'hard_drive_0_priority = 10\n')
    return boot, config


def execute_batch(args, controller, records, batch):
    boot, config = stage_batch(args, controller, records, batch)
    log = args.output / 'logs' / f'guest-{batch:04d}.log'
    started = time.monotonic()
    progress_time = started
    observed = ''
    codes = {}
    active = None
    with log.open('w') as output:
        process = subprocess.Popen([args.emulator, str(config)], stdout=output, stderr=subprocess.STDOUT)
        try:
            while process.poll() is None:
                marker = boot / 'guest-results.tsv'
                text = marker.read_text(errors='replace') if marker.exists() else ''
                if text != observed:
                    observed = text
                    progress_time = time.monotonic()
                    for line in text.splitlines():
                        fields = line.split('\t')
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
                                                       '--name', 'FS-UAE'], text=True).splitlines()
                    if windows:
                        subprocess.run(['import', '-window', windows[-1], str(boot / 'timeout.png')],
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
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
            if not report.is_file() or report.stat().st_size == 0:
                record['run_status'] = 'missing-test-report'
        print(f'{record["path"]}: {record["run_status"]}', flush=True)
    return [record for record in records if record['run_status'] == 'not-reached']


def save_results(args, records):
    (args.output / 'results.json').write_text(json.dumps(records, indent=2) + '\n')
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
    parser.add_argument('mode', choices=('fbctests', 'exampleageddon'))
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
    args = parser.parse_args()
    if min(args.jobs, args.batch_size, args.timeout, args.compile_timeout) <= 0:
        parser.error('Counts and timeouts must be positive')
    args.output = (args.output or ROOT / 'out/amiga' / args.mode).resolve()
    args.toolchain_root = args.toolchain_root.resolve()
    if not args.compile_only:
        if args.kickstart is None or not args.kickstart.is_file(): parser.error('Provide --kickstart or AMIGA_KICKSTART')
        args.kickstart = args.kickstart.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.pop('DEBUG', None)
    env['PATH'] = str(args.toolchain_root / 'bin') + os.pathsep + env['PATH']
    result_file = args.output / 'results.json'
    if args.resume and result_file.is_file():
        records = json.loads(result_file.read_text())
    else:
        prefix = snapshot(args.output)
        records = compile_unit_tests(args, prefix, env) if args.mode == 'fbctests' else compile_examples(args, prefix, env)
        save_results(args, records)
    if not args.compile_only:
        controller = build_controller(args)
        pending = [record for record in records if record['compile_status'] == 'pass' and
                   record['run_status'] != 'pass' and record['group'] in ('self-contained', 'fbctests')]
        batch = len(list((args.output / 'guests').glob('*.fs-uae'))) if (args.output / 'guests').exists() else 0
        while pending:
            current, pending = pending[:args.batch_size], pending[args.batch_size:]
            pending = execute_batch(args, controller, current, batch) + pending
            batch += 1
            save_results(args, records)
    failed = [record for record in records if record['group'] in ('self-contained', 'fbctests') and
              (record['compile_status'] != 'pass' or (not args.compile_only and record['run_status'] != 'pass'))]
    print(f'{len(records)} records, {len(failed)} required failures. Report: {args.output / "report.md"}')
    return 1 if failed else 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of amiga-test.py
