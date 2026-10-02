#!/usr/bin/env python3
"""
FreeBASIC OMA programs for classic AmigaOS
-----------------------------------------
File: amiga-build-oma-games.py
Purpose: Build and stage every maintained program present in the OMA tree.
Responsibilities:
    - freeze the compiler, libraries, sources, and runtime assets
    - compile native Hunk programs with the ordinary graphics and sound APIs
    - preserve failures and publish a per-program manifest
This file intentionally does NOT contain gameplay automation or SDK provisioning.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
import importlib.util
import json
import os
from pathlib import Path
import shutil
import signal


ROOT = Path(__file__).resolve().parents[1]
specification = importlib.util.spec_from_file_location('amiga_test', ROOT / 'build_scripts/amiga-test.py')
amiga = importlib.util.module_from_spec(specification)
specification.loader.exec_module(amiga)

# Paths name the maintained entry points, rather than historical DOS modules
# that are retained beside the ported games as reference material.
PROGRAMS = {
    'behold': ('Behold', ['Behold.bas'], []),
    'duel999': ('duel999', ['SD_Main.bas'], ['defaults.cfg', 'config.cfg', 'data']),
    'kinematics': ('kinematics', ['kinematic_man_two_bodies_self_collision_friction.bas'], []),
    'kinematics_joint_limits': ('kinematics', ['kinematic_man_two_bodies_joint_limits.bas'], []),
    'kinematics_floor_friction': ('kinematics', ['kinematic_man_two_bodies_floor_friction.bas'], []),
    'kinematics_impulse_capsules': ('kinematics', ['kinematic_man_two_bodies_impulse_capsules.bas'], []),
    'nietzsche': ('NietzscheSE-MSDOS-1.1/Nietzsche', ['src/win32/win11.bas'],
                  ['battles1.jss', 'chars.spr', 'jrpg.pal', 'maps.txt', 'script1', 'data', 'music', 'pics']),
    'qfak': ('QuestForAKing-Win32-1.5', ['src/win11.bas'],
             ['BATTLES1.JSS', 'CHARS.SPR', 'JRPG.PAL', 'MAPS.TXT', 'SCRIPT1', 'config.cfg', 'data', 'MUSIC', 'PICS']),
    'rambo': ('RamboVsKittyCat-Win32-0.1', ['killquest.bas'], ['config.cfg', 'input.cfg', 'map1.map', 'Images']),
    'starphalanx': ('StarPhalanx-win32-0.5', ['entryv2.bas'], ['config.cfg', 'data']),
    'openmarket': ('Tamper/tamper', ['src/openmarket_bootstrap.bas'], ['data/open_assets', 'data/tamper_port.cfg']),
    'openslicks': ('Slicks n Slide', ['src/' + name + '.bas' for name in
                    ('slicks', 'slicks_app', 'slicks_font', 'slicks_track', 'slicks_input',
                     'slicks_vehicle', 'slicks_game', 'slicks_render', 'slicks_menu')], []),
}


def copy_asset(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    if source.is_dir():
        shutil.copytree(source, destination, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('.git', '*.exe', '*.dll', '*.so'))
    else:
        shutil.copy2(source, destination)


def stage_assets(key, source, destination, assets, oma):
    destination.mkdir(parents=True, exist_ok=True)
    for name in assets:
        copy_asset(source / name, destination / name)
    if key in ('nietzsche', 'qfak'):
        (destination / 'save').mkdir(exist_ok=True)
    if key == 'openmarket':
        (destination / 'data/reports').mkdir(parents=True, exist_ok=True)
    if key == 'openslicks':
        # Use the supplied complete course and art, so reaching the race proves
        # the ordinary loader and renderer instead of a reduced track fixture.
        copy_asset(oma / 'Slicks n Slide/194 Tracks for Slicks 1.29/ASFALTTI.SS',
                   destination / 'TRACKS/ASFALTTI.SS')
        copy_asset(oma / 'Slicks n Slide/Slix151/SLICKS.DAT', destination / 'TRACKS/SLICKS.DAT')


def build_program(args, prefix, env, key):
    directory, modules, assets = PROGRAMS[key]
    oma = args.output / 'source/OMA'
    source = oma / directory
    binary = args.output / 'bin' / key
    resources = args.output / 'assets' / key
    command = [str(prefix / 'bin/fbc'), '-prefix', str(prefix), '-target', 'amiga-m68k',
               '-mt', '-O', '2', '-exx', '-e', '-i', str(prefix / 'inc'),
               '-i', str(prefix / 'inc/amiga'), '-i', str((source / modules[0]).parent),
               *[str(source / module) for module in modules], '-x', str(binary)]
    code = amiga.run_command(command, args.output / 'logs' / (key + '.compile.log'),
                             cwd=source, env=env, timeout=args.compile_timeout)
    if code == 0:
        stage_assets(key, source, resources, assets, oma)
    record = {'key': key, 'sources': [directory + '/' + module for module in modules],
              'compile_status': 'pass' if code == 0 else f'fail({code})',
              'binary': str(binary), 'resources': str(resources), 'gameplay_status': 'not-run'}
    print(key + ': ' + record['compile_status'], flush=True)
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'out/amiga/oma')
    parser.add_argument('--toolchain-root', type=Path, default=ROOT / 'out/amiga/toolchain')
    parser.add_argument('--oma', type=Path, default=ROOT / 'OMA')
    parser.add_argument('--games', default='')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--compile-timeout', type=int, default=600)
    args = parser.parse_args()
    if args.jobs <= 0 or args.compile_timeout <= 0:
        parser.error('Counts and timeouts must be positive')
    keys = args.games.split(',') if args.games else list(PROGRAMS)
    if any(key not in PROGRAMS for key in keys): parser.error('Unknown OMA program')
    args.output = args.output.resolve()
    args.toolchain_root = args.toolchain_root.resolve()
    if args.output.exists(): parser.error('Use a new output directory to preserve prior evidence')
    args.output.mkdir(parents=True)
    prefix = amiga.snapshot(args.output)
    amiga.copy_sources(args.oma.resolve(), args.output / 'source/OMA')
    (args.output / 'bin').mkdir()
    env = os.environ.copy()
    env.pop('DEBUG', None)
    env['PATH'] = str(args.toolchain_root / 'bin') + os.pathsep + env['PATH']
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        records = list(pool.map(lambda key: build_program(args, prefix, env, key), keys))
    hashes = {str(path.relative_to(args.output)): amiga.digest(path)
              for directory in ('source', 'assets', 'bin')
              for path in sorted((args.output / directory).rglob('*')) if path.is_file()}
    (args.output / 'game-input-hashes.json').write_text(json.dumps(hashes, indent=2) + '\n')
    (args.output / 'manifest.json').write_text(json.dumps(records, indent=2) + '\n')
    return int(any(record['compile_status'] != 'pass' for record in records))


if __name__ == '__main__':
    signal.signal(signal.SIGTERM, amiga.interrupt_run)
    raise SystemExit(main())

# end of amiga-build-oma-games.py
