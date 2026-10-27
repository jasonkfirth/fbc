"""Project: FreeBASIC C backend qualification
File: test-c-emitter-alignment.py
Purpose: Check emitted Win64 scalar/record accesses with alignment sanitizers.
Responsibilities: Pin inputs, retain unsuppressed diagnostics and execute controls.
This file intentionally does NOT install providers or qualify other targets.
"""
import argparse
import ctypes
import gzip
import importlib.util
import os
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('c_audit', ROOT / 'build_scripts/audit-c-emitter.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
CASES = {
    'c-array-index-unaligned': 'c-array-index-unaligned-ok',
    'c-scalar-alignment': 'c-scalar-alignment-ok',
    'c-alignment-views': 'c-alignment-views-ok',
    'c-alignment-aggregate': 'aggregate-view-ok',
    'c-alignment-nested': 'aggregate-nested-ok',
    'c-alignment-pointer-fields': 'c-alignment-pointer-fields-ok',
}
TRAP_EXIT = 3221225501  # Windows STATUS_ILLEGAL_INSTRUCTION from sanitizer traps.


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'gcc', 'clang', 'runtime-dir', 'output'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--before-fbc', type=Path,
                        help='optional known failing pre-repair compiler')
    parser.add_argument('--allocator-object', type=Path)
    parser.add_argument('--case', action='append', choices=CASES, dest='cases')
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('this native gate currently qualifies Win64 only')
    output = args.output.resolve()
    if output.exists():
        parser.error('output already exists; preserve it and select a new directory')
    sources = [ROOT / 'tests/quirk' / (name + '.bas') for name in (args.cases or CASES)]
    runtime = [args.runtime_dir.resolve() / name for name in ('fbrt0.o', 'libfb.a')]
    versions = [('after', args.fbc.resolve())]
    if args.before_fbc:
        versions.insert(0, ('before', args.before_fbc.resolve()))
    tools = {'gcc': args.gcc.resolve(), 'clang': args.clang.resolve()}
    inputs = [*sources, *runtime, *tools.values(), *[p for _, p in versions],
              Path(__file__).resolve(), ROOT / 'build_scripts/audit-c-emitter.py']
    if args.allocator_object:
        inputs.append(args.allocator_object.resolve())
    if not all(path.is_file() for path in inputs):
        parser.error('a compiler, runtime object or fixture is missing')
    pins = {str(path): audit.digest(path) for path in inputs}
    output.mkdir(parents=True)
    temporary = output / 'tmp'
    temporary.mkdir()
    for name in ('TEMP', 'TMP', 'TMPDIR'):
        os.environ[name] = str(temporary)
    ctypes.windll.kernel32.SetErrorMode(3)
    result = dict(complete=False, overall_complete=False, installed=False, pins=pins,
                  scope='Win64 alignment sanitizer controls', records=[],
                  native_passes=0, negative_traps=0)

    def guard():
        # Bound this explicit output directory; callers may impose a shared cap.
        owned = sum(p.stat().st_size for p in output.rglob('*') if p.is_file())
        if owned > 128 * 1024 ** 2 or shutil.disk_usage(output).free < 5 * 1024 ** 3:
            raise RuntimeError('alignment gate resource limit reached')

    def publish():
        audit.write_json(output / 'results.json', result)

    def invoke(command, folder, label, expected=0):
        guard()
        stage = audit.invoke(list(map(str, command)), ROOT, folder / (label + '.log'), 120)
        if stage['exit'] != expected:
            result['failure'] = stage
            publish()
            raise RuntimeError((label, stage['exit'], expected, stage['log']))
        return stage

    try:
        for source in sources:
            for backend in ('gcc', 'clang'):
                for version, compiler in versions:
                    folder = output / (source.stem + '-' + backend + '-' + version)
                    folder.mkdir()
                    row = dict(source=str(source), backend=backend, version=version, checks=[],
                               emit=invoke([compiler, '-prefix', ROOT, '-gen', backend, '-r',
                                            '-arch', 'x86_64', '-O', '2', '-m', source.stem,
                                            source, '-o', folder / 'unit.o'], folder, 'emit'))
                    result['records'].append(row)
                    for optimization in ('O0', 'O2'):
                        obj = folder / (optimization + '.o')
                        executable = folder / (optimization + '.exe')
                        warnings = audit.GCC_WARNINGS if backend == 'gcc' else ['-Weverything', '-Wwrite-strings']
                        if version == 'after':
                            warnings = [*warnings, '-Werror=discarded-qualifiers' if backend == 'gcc'
                                        else '-Werror=incompatible-pointer-types-discards-qualifiers']
                        compiled = invoke([tools[backend], *audit.SEMANTICS, '-' + optimization, *warnings,
                            '-fno-diagnostics-show-caret' if backend == 'gcc' else '-fno-caret-diagnostics',
                            '-m64', '-fsanitize=alignment', '-fsanitize-undefined-trap-on-error',
                            '-c', folder / 'unit.c', '-o', obj], folder, optimization + '-object')
                        override = [args.allocator_object.resolve()] if args.allocator_object else []
                        linked = invoke([tools[backend], obj, *override, *runtime, '-lm', '-mthreads',
                                         '-o', executable], folder, optimization + '-link')
                        executed = invoke([executable], folder, optimization + '-run',
                                          TRAP_EXIT if version == 'before' else 0)
                        row['checks'].append(dict(optimization=optimization, object=compiled,
                                                  link=linked, run=executed))
                        if version == 'after':
                            text = gzip.decompress(Path(executed['log']).read_bytes()).decode(errors='replace')
                            assert CASES[source.stem] in text, (source, executed['log'])
                            result['native_passes'] += 1
                        else:
                            result['negative_traps'] += 1
                    publish()
        assert all(audit.digest(Path(p)) == h for p, h in pins.items())
        result['complete'] = True
        publish()
        print('C_ALIGNMENT_PASS native_passes=' + str(result['native_passes']) +
              ' negative_traps=' + str(result['negative_traps']))
    except BaseException:
        publish()
        raise


if __name__ == '__main__':
    main()
# end of test-c-emitter-alignment.py
