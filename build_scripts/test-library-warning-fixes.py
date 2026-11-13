"""Project: FreeBASIC runtime maintenance
File: test-library-warning-fixes.py
Purpose: Run focused native regressions against the audited Windows archives.
Responsibilities: Check ownership, bounds, pixel values and existing C contracts.
This file does not install libraries or open graphics/audio devices.
"""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--libdir', required=True, type=Path)
    parser.add_argument('--mt', action='store_true')
    parser.add_argument('--archives-only', action='store_true')
    args = parser.parse_args()
    family = 'clang' if 'clang' in Path(args.cc).name else 'gcc'
    variant = 'mt' if args.mt else 'base'
    output = ROOT / 'build/c-warning-audit/checks' / family / variant
    output.mkdir(parents=True, exist_ok=True)
    library = args.libdir.resolve()
    suffix = 'mt' if args.mt else ''
    runtime = library / ('libfb' + suffix + '.a')
    graphics = library / ('libfbgfx' + suffix + '.a')
    startup = library / 'fbrt0.o'
    for archive in (runtime, graphics, startup):
        if not archive.is_file():
            parser.error('missing archive: ' + str(archive))
    tests = [] if args.mt or args.archives_only else [
        ('lpt-device-state', False), ('profile-filename', False),
        ('utf-word-values', False), ('console-view-resize', False),
        ('time-parse-bounds', False), ('win32-file-mode-bounds', True)]
    tests += [('rnd-state-runtime', True), ('time-parse-runtime', True),
              ('str-instrrev-runtime', True), ('tls-context-runtime', True),
              ('prototype-contracts', True),
              ('gfx-pixel-shifts', True)]
    results = []
    for name, linked in tests:
        executable = output / (name + '.exe')
        source = ROOT / 'tests/rtlib' / (name + '.c')
        command = [args.cc, '-O2', '-Wall', '-Wextra', '-fno-strict-aliasing',
                   '-I' + str(ROOT / 'src/rtlib'), str(source), '-o', str(executable)]
        if name != 'gfx-pixel-shifts':
            command.append('-Werror')
        if family == 'clang' and name in ('gfx-pixel-shifts', 'time-parse-bounds', 'win32-file-mode-bounds'):
            # Trap instrumentation needs no separately installed UBSan runtime.
            command += ['-fsanitize=undefined', '-fsanitize-trap=undefined']
        if args.mt:
            command.append('-DENABLE_MT')
        if linked:
            # fb_Init sets program arguments; fbrt0 performs native runtime
            # startup first, including the MT critical sections and TLS keys.
            command += [str(startup), '-Wl,--start-group', str(graphics), str(runtime),
                        '-Wl,--end-group', '-lwinmm', '-lgdi32', '-lmsimg32',
                        '-lopengl32', '-lole32', '-luuid', '-lshell32', '-lws2_32', '-lwinspool']
        built = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=120)
        (output / (name + '.log')).write_text(built.stdout + built.stderr, encoding='utf-8')
        record = dict(test=name, command=command, compile_exit=built.returncode,
                      source_sha256=hashlib.sha256(source.read_bytes()).hexdigest())
        if built.returncode == 0:
            ran = subprocess.run([str(executable)], cwd=output, capture_output=True,
                                 text=True, timeout=30)
            record.update(run_exit=ran.returncode, output=ran.stdout + ran.stderr)
        results.append(record)
        print(name, 'PASS' if record.get('run_exit') == 0 else 'FAIL', flush=True)
    failures = sum(record.get('run_exit') != 0 for record in results)
    receipt = dict(compiler=args.cc, variant=variant, failures=failures, results=results,
                   archives={str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                             for path in (runtime, graphics, startup)})
    (output / 'summary.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print('LIBRARY_WARNING_FIXES', family, variant, 'tests=' + str(len(results)),
          'failures=' + str(failures), flush=True)
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-library-warning-fixes.py
