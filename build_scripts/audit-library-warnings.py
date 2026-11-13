"""Project: FreeBASIC runtime maintenance
File: audit-library-warnings.py
Purpose: Compile the Make-selected C libraries with exhaustive diagnostics.
Responsibilities: Isolate products and save reproducible diagnostic counts.
This file does not install libraries or suppress source warnings.
"""

import argparse
from collections import Counter
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent
GCC_WARNINGS = """
-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
-Wcast-qual -Wcast-align=strict -Wformat=2 -Wundef -Wwrite-strings
-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
-Wold-style-definition -Wredundant-decls -Wnested-externs -Wpointer-arith
-Wdouble-promotion -Wfloat-equal -Wnull-dereference -Wduplicated-cond
-Wduplicated-branches -Wlogical-op -Wformat-overflow=2 -Wformat-truncation=2
-Wstringop-overflow=4 -Wstrict-overflow=5 -Warray-bounds=2 -Walloca -Wvla
-Wswitch-enum -Wswitch-default -Wbad-function-cast -Wjump-misses-init
-Wuninitialized -Winit-self -Wmissing-include-dirs -Wtrampolines -Wdate-time
-Wpacked -Wpadded
""".split()


def source_hashes(source_root=None):
    sources = {}
    source_root = source_root or ROOT / 'src'
    for library in ('rtlib', 'gfxlib2', 'gfxlib3', 'sfxlib'):
        for directory, children, files in os.walk(source_root / library):
            children[:] = [name for name in children if name != 'obj']
            for name in files:
                source = Path(directory) / name
                if source.suffix in ('.c', '.h', '.cpp', '.inc', '.s', '.S'):
                    sources['src/' + source.relative_to(source_root).as_posix()] = hashlib.sha256(source.read_bytes()).hexdigest()
    return sources


def normalized_path(path, base=ROOT):
    path = path.replace('\\', '/')
    if os.name == 'nt':
        match = re.match(r'^/([A-Za-z])/(.*)$', path)
        if match:
            path = match.group(1) + ':/' + match.group(2)
    resolved = Path(path)
    if not resolved.is_absolute():
        resolved = base / resolved
    return os.path.normcase(os.path.normpath(str(resolved.resolve()))).replace('\\', '/')


def selected_source_changes(changed, source_root, output):
    dependency_text = '\n'.join(
        path.read_text(encoding='utf-8', errors='replace')
        for path in (output / 'src').rglob('*.d'))
    dependency_text = re.sub(r'\\\r?\n', ' ', dependency_text)
    dependencies = set()
    for token in shlex.split(dependency_text):
        token = token.rstrip(':')
        if token:
            dependencies.add(normalized_path(token))
    selected = []
    for path in changed:
        relative_path = path[4:] if path.startswith('src/') else path
        source_path = normalized_path(relative_path, source_root)
        if source_path in dependencies:
            selected.append(path)
    return selected


def write_counts(output):
    """Keep one small table per compiler, avoiding concurrent writer races."""
    rows = []
    for receipt in sorted(output.glob('*.json'), key=lambda path: path.stat().st_mtime_ns):
        record = json.loads(receipt.read_text(encoding='utf-8'))
        census = record.get('census')
        accepted = bool(record.get('full_rebuild') and record['exit_code'] == 0
                        and record.get('diagnostic_capture') == 'per-object-v1'
                        and census and not census['missing_diagnostics']
                        and not census['unexpected_diagnostics']
                        and not record.get('changed_selected_inputs', record.get('changed_during_build', [])))
        rows.append(dict(run=receipt.stem, warnings=record['warnings'],
                         exit_code=record['exit_code'], accepted=accepted,
                         compiler=record['compiler']))
    with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', newline='',
                                     dir=output, suffix='.csv', delete=False) as stream:
        writer = csv.DictWriter(stream, fieldnames=('run', 'warnings', 'exit_code', 'accepted', 'compiler'))
        writer.writeheader()
        writer.writerows(rows)
        temporary = stream.name
    os.replace(temporary, output / 'counts.csv')


def captured_diagnostics(output, log):
    """Count independent captures and retain a sequential, reproducible log."""
    output = output.resolve()
    expected = set()
    for members in (output / 'lib').glob('**/.archive-members/*.a'):
        expected.update(members.read_text(encoding='utf-8').split())
        for startup in members.parent.parent.glob('*.o'):
            expected.add(startup.relative_to(ROOT).as_posix())
    captured = {}
    counts = Counter()
    by_library = Counter()
    unique = set()
    errors = []
    # Only this runner's output roots are searched. A fresh run removes their
    # old sidecars before Make starts, so an omitted invocation stays visible.
    paths = sorted(path for directory in (output / 'src', output / 'lib')
                   for path in directory.rglob('*.o.diagnostics'))
    with log.with_suffix('.diagnostics.log').open('wb') as stream:
        for path in paths:
            target = path.relative_to(ROOT).as_posix().removesuffix('.diagnostics')
            payload = path.read_bytes()
            diagnostics = payload.decode('utf-8', errors='replace')
            warnings = diagnostics.count('warning:')
            captured[target] = dict(warnings=warnings, sha256=hashlib.sha256(payload).hexdigest())
            owner = re.search(r'/src/(rtlib|gfxlib2|gfxlib3|sfxlib)/', target)
            by_library[owner[1] if owner else 'rtlib'] += warnings
            stream.write(('\n=== ' + target + ' ===\n').encode('utf-8'))
            stream.write(payload)
            for line in diagnostics.splitlines():
                if 'warning:' in line:
                    match = re.search(r'\[(-W[^\]]+)\]', line)
                    counts[match[1] if match else 'unclassified'] += line.count('warning:')
                    unique.add(line)
                if 'error:' in line:
                    errors.append(line)
    census = dict(expected_objects=len(expected), captured_objects=len(captured),
                  missing_diagnostics=sorted(expected - captured.keys()),
                  unexpected_diagnostics=sorted(captured.keys() - expected),
                  warnings_by_library=dict(by_library))
    return census, captured, counts, unique, errors


def capture_compiler(compiler, output):
    """Use a native launcher so Make can still identify the compiler family."""
    source = ROOT / 'build_scripts/capture-library-compiler.c'
    executable = output / ('clang-capture' if 'clang' in output.name else 'gcc-capture')
    if os.name == 'nt':
        executable = executable.with_suffix('.exe')
    signature = executable.with_suffix('.sha256')
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    if not executable.exists() or not signature.exists() or signature.read_text() != digest:
        subprocess.run([compiler, '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror',
                        str(source), '-o', str(executable)], cwd=ROOT, check=True)
        signature.write_text(digest)
    return executable


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--make', default='make')
    parser.add_argument('--target', help='Make target triplet (default: native)')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--label', default='current')
    parser.add_argument('--rebuild', action='store_true', help='Recompile all selected units in place')
    parser.add_argument('--source-root', type=Path, help='Use a pinned source snapshot instead of src/')
    args = parser.parse_args()
    if args.jobs < 1 or not re.fullmatch(r'[A-Za-z0-9_-]+', args.label):
        parser.error('jobs must be positive and label must be a simple name')
    clang = 'clang' in Path(args.cc).name
    family = 'clang' if clang else 'gcc'
    output = ROOT / 'build/c-warning-audit' / family
    output.mkdir(parents=True, exist_ok=True)
    flags = ['-O2', '-fdiagnostics-color=never']
    flags += ['-Weverything', '-fno-caret-diagnostics'] if clang else (
        GCC_WARNINGS + ['-fno-diagnostics-show-caret'])
    compiler = shutil.which(args.cc)
    if compiler is None:
        parser.error('compiler was not found: ' + args.cc)
    compiler = str(Path(compiler).resolve())
    launcher = capture_compiler(compiler, output)
    environment = os.environ.copy()
    environment['FBC_AUDIT_REAL_CC'] = compiler
    # Make's output synchronization improves the human-readable log. Exact
    # counts come from the launcher's separate file for every object.
    command = [args.make, '-k', '--output-sync=target', '-j' + str(args.jobs),
               'rtlib', 'gfxlib2', 'gfxlib3', 'sfxlib',
               'BUILD_ROOT=build/c-warning-audit/' + family,
               'CC=' + launcher.relative_to(ROOT).as_posix(), 'CFLAGS=' + ' '.join(flags)]
    if args.target:
        command.append('TARGET=' + args.target)
    if args.rebuild:
        command.append('-B')
    source_root = args.source_root.resolve() if args.source_root else ROOT / 'src'
    if args.source_root:
        command.append('srcdir=' + source_root.as_posix())
    log = output / (args.label + '.log')
    if args.label != 'current' and (log.exists() or log.with_suffix('.json').exists()):
        parser.error('named runs are retained; choose a new --label')
    version = subprocess.check_output([compiler, '--version'], text=True).splitlines()[0]
    for directory in (output / 'src', output / 'lib'):
        for sidecar in directory.rglob('*.o.diagnostics'):
            sidecar.unlink()
    sources = source_hashes(source_root)
    start = time.monotonic()
    # MSYS child processes must not inherit independently seekable log handles.
    # A pipe keeps one Python writer responsible for the log's file position.
    with log.open('wb') as stream:
        process = subprocess.Popen(command, cwd=ROOT, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, env=environment)
        with process.stdout:
            shutil.copyfileobj(process.stdout, stream)
        build_exit = process.wait()
    diagnostics = log.read_text(encoding='utf-8', errors='replace')
    census, captured, counts, unique, errors = captured_diagnostics(output, log)
    for line in diagnostics.splitlines():
        if 'Error ' in line:
            errors.append(line)
    after = source_hashes(source_root)
    changed = sorted(path for path in sources.keys() | after.keys()
                     if sources.get(path) != after.get(path))
    selected_changes = []
    if changed:
        # Other targets can be maintained concurrently. Only changes present
        # in this build's dependency closure invalidate it. Normalize both
        # relative source names and compiler paths, which may contain '..'.
        selected_changes = selected_source_changes(changed, source_root, output)
    archives = {}
    for archive in (output / 'lib').rglob('*.a'):
        if not any(part.startswith('.') for part in archive.relative_to(output).parts):
            archives[archive.relative_to(ROOT).as_posix()] = hashlib.sha256(archive.read_bytes()).hexdigest()
    compiled = sum(' -c ' in line and ' -o ' in line for line in diagnostics.splitlines())
    summary = dict(compiler=version, command=command, exit_code=build_exit,
                   seconds=round(time.monotonic() - start, 1),
                   warnings=sum(record['warnings'] for record in captured.values()),
                   unique_warning_lines=len(unique), diagnostic_capture='per-object-v1',
                   compiler_executable=compiler,
                   capture_launcher_sha256=hashlib.sha256(launcher.read_bytes()).hexdigest(),
                   capture_source_sha256=hashlib.sha256((ROOT / 'build_scripts/capture-library-compiler.c').read_bytes()).hexdigest(),
                   categories=dict(counts.most_common()), errors=errors,
                   compiled=compiled, full_rebuild=args.rebuild, archives=archives,
                   census=census, captured=captured,
                   changed_during_build=changed,
                   changed_selected_inputs=selected_changes, sources=sources)
    (output / (args.label + '.json')).write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
    write_counts(output)
    print(json.dumps({key: value for key, value in summary.items() if key not in ('sources', 'captured')}, indent=2))
    return build_exit or bool(selected_changes) or (args.rebuild and bool(
        census['missing_diagnostics'] or census['unexpected_diagnostics']))


if __name__ == '__main__':
    raise SystemExit(main())

# end of audit-library-warnings.py
