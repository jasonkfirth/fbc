"""Project: FreeBASIC C backend regression tests
File: c-packed-descriptors.py
Purpose: Qualify packed descriptor layout and ordinary native library accesses.
Responsibilities: Check both C producers, optimizations, runtime modes and widths.
This file intentionally does NOT install libraries or execute Win32 programs.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import sqlite3
import zlib

ROOT = Path(__file__).resolve().parents[2]
MODULES = ('arrays/array_destructstr.c', 'arrays/array_erase.c', 'arrays/array_resetdesc.c',
    'strings/str_del.c', 'strings/str_assign.c', 'strings/str_core.c', 'strings/str_comp.c')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'prefix', 'gcc', 'clang', 'work-dir', 'diagnostic-profile', 'runtime-override'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--rtlib', type=Path, default=ROOT / 'src/rtlib')
    parser.add_argument('--tool-temp', type=Path, required=True)
    parser.add_argument('--descriptor-modules', type=Path,
        help='Optional relative source list for the complete affected native cohort')
    parser.add_argument('--win64-only', action='store_true')
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(exist_ok=True)
    assert not (work / 'receipt.json').exists()
    tools = {cc: getattr(args, cc).resolve() for cc in ('gcc', 'clang')}
    fixture = Path(__file__).with_suffix('.bas')
    counter = Path(__file__).with_suffix('.c')
    rtl = args.rtlib.resolve()
    modules = MODULES
    if args.descriptor_modules:
        modules = tuple(args.descriptor_modules.read_text().splitlines())
        assert modules and len(modules) == len(set(modules))
        assert all(not Path(n).is_absolute() and '..' not in Path(n).parts and
            (rtl / n).suffix == '.c' and 'png' not in n.lower() for n in modules)
    runtime = args.prefix.resolve() / 'lib/freebasic/win64'
    # The complete cohort also contains TCP and printer entry points. Match
    # the compiler's Windows platform/file-runtime import-library selection.
    native_libraries = ['-lws2_32', '-lgdi32', '-lwinspool'] if args.descriptor_modules and os.name == 'nt' else []
    files = [Path(__file__), fixture, counter, args.fbc, args.runtime_override,
        args.diagnostic_profile, *tools.values(), runtime / 'fbrt0.o', runtime / 'libfb.a',
        runtime / 'libfbmt.a', *[rtl / n for n in modules], *rtl.rglob('*.h')]
    if args.descriptor_modules:
        files.append(args.descriptor_modules)
    pins = {str(p.resolve()): digest(p) for p in files}
    with sqlite3.connect(args.diagnostic_profile) as db:
        flags = json.loads(db.execute("SELECT value FROM metadata WHERE name='inputs'").fetchone()[0])
    spec = importlib.util.spec_from_file_location('packed_descriptor_bounded', ROOT / 'build_scripts/audit-exampleageddon-c.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    env = dict(os.environ)
    env['PATH'] = str(tools['gcc'].parent) + os.pathsep + env.get('PATH', '')
    env['SOURCE_DATE_EPOCH'] = flags['source_date_epoch']
    args.tool_temp.mkdir(exist_ok=True)
    for key in ('TMP', 'TEMP', 'TMPDIR'):
        env[key] = str(args.tool_temp.resolve())
    db = sqlite3.connect(work / 'results.sqlite3')
    db.execute('CREATE TABLE IF NOT EXISTS checks(name TEXT PRIMARY KEY,result TEXT,log BLOB)')

    def run(name, command, execute=False):
        assert shutil.disk_usage(work.drive + '/').free >= 512 * 1024 * 1024
        assert sum(p.stat().st_size for p in work.rglob('*') if p.is_file()) < 8 * 1024 * 1024
        command = list(map(str, command))
        assert not any(f == '-w' or f.startswith('-Wno-') for f in command)
        row, output = helper.run_command(command, ROOT, env, 15 if execute else 90)
        db.execute('INSERT OR REPLACE INTO checks VALUES(?,?,?)',
            (name, json.dumps({'command': command, **row}), zlib.compress(output)))
        db.commit()
        assert row['exit'] == 0 and not row['timeout'], (name, row, output[-3000:])
        return row, output

    native32 = executions = main32 = 0
    for width in ((64,) if args.win64_only else (32, 64)):
        for mt in (False, True):
            mode = 'mt' if mt else 'st'
            native = []
            for source in (rtl / name for name in modules):
                obj = work / (source.stem + '.o')
                for cc in (('gcc', 'clang') if width == 32 else ('clang',)):
                    run(f'{width}-{mode}-{cc}-{source.stem}', [tools[cc],
                        *[f for f in flags[cc + '_flags'] if f != '-nostdinc'], '-m' + str(width),
                        # The native x86 runtime helpers use AT&T inline assembly.
                        '-masm=att',
                        *(['-DENABLE_MT'] if mt else []),
                        *(['-fsanitize=alignment', '-fsanitize-minimal-runtime'] if cc == 'clang' else []),
                        '-I', rtl, '-c', source, '-o', obj])
                    if width == 32:
                        assert int.from_bytes(obj.read_bytes()[:2], 'little') == 0x14c
                        native32 += 1
                native.append(obj)
            count_obj = work / 'counter.o'
            run(f'{width}-{mode}-counter', [tools['gcc'], *flags['gcc_flags'], '-m' + str(width),
                '-c', counter, '-o', count_obj])
            cpp, main_obj, exe = [work / n for n in ('unit.c', 'main.o', 'unit.exe')]
            for producer in ('gcc', 'clang'):
                tag = f'{width}-{mode}-{producer}'
                run(tag + '-emit', [args.fbc, '-prefix', args.prefix, '-gen', producer, '-exx',
                    *(['-arch', '686'] if width == 32 else []), *(['-mt'] if mt else []),
                    '-d', 'TEST_ALIGNMENT', '-m', fixture.stem, '-r', fixture, '-o', cpp])
                (work / (tag + '.c.zlib')).write_bytes(zlib.compress(cpp.read_bytes()))
                for cc in tools:
                    for opt in ((2,) if width == 32 else (0, 2)):
                        case = f'{tag}-{cc}-O{opt}'
                        run(case + '-program', [tools[cc], *flags[cc + '_flags'], '-m' + str(width),
                            '-O' + str(opt),
                            *(['-fsanitize=alignment', '-fsanitize-minimal-runtime'] if cc == 'clang' else []),
                            '-c', cpp, '-o', main_obj])
                        if width == 32:
                            assert int.from_bytes(main_obj.read_bytes()[:2], 'little') == 0x14c
                            main32 += 1
                            continue
                        run(case + '-link', [tools[cc], '-o', exe, main_obj, *native, count_obj,
                            args.runtime_override, runtime / 'fbrt0.o',
                            runtime / ('libfbmt.a' if mt else 'libfb.a'), '-lm', '-mthreads', *native_libraries])
                        _, output = run(case + '-execute', [exe], execute=True)
                        assert output.replace(b'\r', b'').strip() == b'PACKED_DESCRIPTORS_PASS', output
                        executions += 1
                        print('PACKED_DESCRIPTOR_PASS', case, flush=True)
            for path in (*native, count_obj, cpp, main_obj, exe):
                path.unlink(missing_ok=True)
    assert all(digest(Path(p)) == value for p, value in pins.items())
    db.close()
    (work / 'receipt.json').write_text(json.dumps({'checks_complete': True,
        'source_tool_runtime_sha256': pins, 'instrumented_Win64_executions': executions,
        'Win32_native_objects': native32, 'Win32_emitted_program_objects': main32,
        'alignment_reports': 0, 'MT_links_are_single_threaded': True,
        'native_descriptor_modules_per_mode': len(modules),
        'overall_goal_complete': False}, indent=2) + '\n')


if __name__ == '__main__':
    main()

# end of c-packed-descriptors.py
