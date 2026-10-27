"""Project: FreeBASIC runtime qualification
File: test-array-allocation-runtime.py
Purpose: Qualify array initialization after successful allocation only.
Responsibilities: Preserve an old failing control, check guarded native behavior,
    compare both-width public symbols, and retain unsuppressed diagnostics.
This file intentionally does NOT install a runtime or qualify array indexing.
"""
import argparse
import ctypes
import gzip
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]
WORK_ROOT = Path('E:/fbc-c-audit-01a166ce')
RESERVE = 5 * 1024 ** 3
# Artifact allowance includes retained evidence and one temporary module.
# Disk and RAM reserves remain independent of this bounded work allowance.
CAP = 160 * 1024 ** 2
spec = importlib.util.spec_from_file_location('audit_c', ROOT / 'build_scripts/audit-c-emitter.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class MemoryStatus(ctypes.Structure):
    """Win32 MEMORYSTATUSEX: two DWORDs followed by seven 64-bit byte counts."""
    _fields_ = [('length', ctypes.c_uint32), ('load', ctypes.c_uint32)] + [
        (name, ctypes.c_uint64) for name in
        ('physical', 'available', 'pagefile', 'available_pagefile', 'virtual',
         'available_virtual', 'available_extended_virtual')]


def guard(admission=False):
    free = shutil.disk_usage('E:/').free
    owned = sum(p.stat().st_size for p in WORK_ROOT.rglob('*') if p.is_file()) if WORK_ROOT.exists() else 0
    memory = MemoryStatus()
    memory.length = ctypes.sizeof(memory)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(memory)):
        raise OSError('GlobalMemoryStatusEx failed')
    if free < RESERVE + (CAP if admission else 0) or owned > CAP or memory.available < 2 * 1024 ** 3:
        raise RuntimeError(f'resource guard: E free={free}, owned={owned}, RAM available={memory.available}')
    return dict(free_bytes=free, owned_bytes=owned, available_memory_bytes=memory.available)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('gcc', 'clang', 'nm', 'baseline-source', 'runtime-dir', 'output'):
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if os.name != 'nt' or not output.is_relative_to(WORK_ROOT) or output == WORK_ROOT:
        parser.error('use a new cohort directory below E:/fbc-c-audit-01a166ce on Windows')
    if output.exists():
        parser.error('output already exists; preserve its receipts and select a new cohort')
    initial = guard(admission=True)
    sources = {'before': args.baseline_source.resolve(), 'after': ROOT / 'src/rtlib/arrays/array_redim.c'}
    fixture = ROOT / 'tests/rtlib/array-allocation.c'
    hooks = ROOT / 'tests/rtlib/array-allocation-hooks.h'
    headers = sources['before'].parent.parent
    runtime = [args.runtime_dir.resolve() / name for name in ('fbrt0.o', 'libfb.a')]
    tools = {name: getattr(args, name).resolve() for name in ('gcc', 'clang', 'nm')}
    inputs = [*sources.values(), fixture, hooks, *runtime, *tools.values(),
              Path(__file__).resolve(), ROOT / 'build_scripts/audit-c-emitter.py', *headers.rglob('*.h')]
    pins = {str(path): audit.digest(path) for path in inputs}
    output.mkdir(parents=True)
    temporary = output / 'tmp'
    temporary.mkdir()
    for name in ('TEMP', 'TMP', 'TMPDIR'):
        os.environ[name] = str(temporary)
    os.environ['PYTHONDONTWRITEBYTECODE'] = '1'
    os.environ['PATH'] = os.pathsep.join(dict.fromkeys([
        str(tools['gcc'].parent), str(tools['clang'].parent), os.environ['PATH']]))
    result = dict(complete=False, overall_complete=False, installed=False, pins=pins,
                  initial_resources=initial, output=str(output), stages=[], native=[], symbols=[])

    def publish():
        audit.write_json(output / 'results.json', result)

    def stage(command, label, timeout=45):
        guard()
        row = audit.invoke(list(map(str, command)), ROOT, output / (label + '.log'), timeout)
        row['label'] = label
        result['stages'].append(row)
        publish()
        return row

    def checked(command, label, timeout=45):
        row = stage(command, label, timeout)
        if row['exit'] != 0:
            raise RuntimeError(f'{label} failed: {row["exit"]}; see {row["log"]}')
        return row

    semantics = [flag for flag in audit.SEMANTICS if flag != '-nostdinc']
    summary_pattern = re.compile(r'^ARRAY_ALLOCATION_CASES=(\d+) NULL_FILL_ATTEMPTS=(\d+) FAILURES=(\d+)$', re.M)
    for backend in ('gcc', 'clang'):
        warnings = audit.GCC_WARNINGS if backend == 'gcc' else ['-Weverything', '-Wwrite-strings']
        common = [tools[backend], *semantics, *warnings, '-I' + str(headers)]
        for optimization in ('O0', 'O2'):
            fixture_obj = output / f'{backend}-{optimization}-fixture.o'
            checked([*common, '-' + optimization, '-m64', '-c', fixture, '-o', fixture_obj],
                    f'{backend}-{optimization}-fixture')
            for version, source in sources.items():
                label = f'{backend}-{optimization}-{version}'
                obj = output / (label + '-instrumented.o')
                checked([*common, '-' + optimization, '-m64', '-DFB_ARRAY_ALLOC_INSTRUMENT',
                         '-include', hooks, '-c', source, '-o', obj], label + '-instrumented')
                exe = output / (label + '.exe')
                checked([tools['gcc'], fixture_obj, obj, *runtime, '-o', exe, '-lm', '-mthreads'], label + '-link')
                run = stage([exe], label + '-run', 10)
                text = gzip.decompress(Path(run['log']).read_bytes()).decode(errors='replace')
                matches = summary_pattern.findall(text.replace('\r\n', '\n'))
                expected = (16, 1, 1) if version == 'before' else (16, 0, 0)
                observed = tuple(map(int, matches[0])) if len(matches) == 1 else None
                expected_exit = 1 if version == 'before' else 0
                row = dict(backend=backend, optimization=optimization, version=version,
                           executable=str(exe), executable_sha256=audit.digest(exe),
                           allocator_object_sha256=audit.digest(obj), fixture_object_sha256=audit.digest(fixture_obj),
                           observed=observed, expected=expected, expected_exit=expected_exit,
                           stage=run, passed=run['exit'] == expected_exit and observed == expected)
                result['native'].append(row)
                publish()
                print('ARRAY_ALLOCATION', label, 'exit=' + str(run['exit']), 'observed=' + str(observed), flush=True)
                if not row['passed']:
                    raise RuntimeError('native control failed: ' + label)

        for width in ('32', '64'):
            definitions = {}
            for version, source in sources.items():
                label = f'{backend}-O2-{width}-{version}-ordinary'
                obj = output / (label + '.o')
                checked([*common, '-O2', '-m' + width, '-c', source, '-o', obj], label)
                machine = int.from_bytes(obj.read_bytes()[:2], 'little')
                if machine != {'32': 0x14c, '64': 0x8664}[width]:
                    raise RuntimeError('unexpected COFF machine: ' + label)
                nm = checked([tools['nm'], '--extern-only', '--defined-only', obj], label + '-symbols')
                text = gzip.decompress(Path(nm['log']).read_bytes()).decode(errors='replace')
                definitions[version] = sorted(line.split()[-1] for line in text.splitlines() if line.strip())
            row = dict(backend=backend, width=width, definitions=definitions,
                       passed=definitions['before'] == definitions['after'])
            result['symbols'].append(row)
            publish()
            if not row['passed']:
                raise RuntimeError(f'public symbols differ: {backend} {width}')
            print('ARRAY_SYMBOLS', backend, width, 'identical', flush=True)

    if any(audit.digest(Path(path)) != pin for path, pin in pins.items()):
        raise RuntimeError('input changed during qualification')
    result['final_resources'] = guard()
    result['complete'] = True
    publish()
    print('ARRAY_ALLOCATION_RUNTIME_PASS native=8 symbol_pairs=4', flush=True)


if __name__ == '__main__':
    main()

# end of test-array-allocation-runtime.py
