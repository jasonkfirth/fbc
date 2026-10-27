"""Project: FreeBASIC C backend qualification
File: test-c-emitter-array-copies.py
Purpose: Check generated dynamic-array field copies and allocation failures.
Responsibilities: Retain unsuppressed diagnostics, guard proofs and native results.
This file intentionally does NOT install providers or qualify other runtime ABIs.
"""
import argparse
import ctypes
import gzip
import importlib.util
import os
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('c_audit', ROOT / 'build_scripts/audit-c-emitter.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
MAX_OUTPUT_BYTES = 16 * 1024 ** 2
CASES = {
    'c-array-copy-values': ('', None),
    'c-array-copy-failure-check': ('ARRAY_COPY_FAILURE_PASS', 26),
    'c-array-copy-object-failure': ('', 10),
}
SANITIZER = ['-fsanitize=undefined', '-fsanitize-undefined-trap-on-error']


def copy_guards(text):
    """Both storage checks must precede the source-end calculation."""
    proofs = []
    for call in re.finditer(r'^\tfb_ArrayRedimTo\([^\n]*\n', text, re.MULTILINE):
        tail = text[call.end():]
        stop = re.search(r'^\}|^\tfb_ArrayRedimTo\(', tail, re.MULTILINE)
        if stop is None:
            raise RuntimeError('unterminated generated copy procedure')
        body = tail[:stop.start()]
        pointers = list(re.finditer(r'^\t(\w+) = [^\n]*\.PTR;\s*$', body, re.MULTILINE))
        if len(pointers) < 2:
            raise RuntimeError('generated copy lacks its storage assignments')
        checks = []
        for pointer in pointers[:2]:
            variable = pointer.group(1)
            check = re.search(r'^\tif\(\s*' + re.escape(variable) +
                              r'\s*==\s*0[uUlL]*\)\s*goto\s*(\w+);', body, re.MULTILINE)
            if check is None:
                raise RuntimeError('generated copy lacks a null-storage guard')
            checks.append(check)
        end = re.search(r'^\t\w+ = [^\n]*\(uint8\*\)' +
                        re.escape(pointers[1].group(1)) + r' \+', body, re.MULTILINE)
        ordered = (pointers[0].start() < checks[0].start() <
                   pointers[1].start() < checks[1].start())
        if not ordered or end is None or checks[1].end() >= end.start():
            raise RuntimeError('storage is used before its null check')
        if checks[0].group(1) != checks[1].group(1):
            raise RuntimeError('copy guards do not share the loop exit')
        label = '\t' + checks[0].group(1) + ':;'
        if label not in body or body.index(label) <= end.start():
            raise RuntimeError('copy guard does not skip the loop')
        proofs.append(dict(destination=pointers[0].group(1), source=pointers[1].group(1),
                           exit_label=checks[0].group(1)))
    if not proofs:
        raise RuntimeError('fixture did not emit a dynamic-array field copy')
    return proofs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'gcc', 'clang', 'runtime-dir', 'output'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--baseline-runtime-source', type=Path,
                        help='optional older array_redimto.c for failing controls')
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('this gate checks Windows objects and Win64 native code')
    output = args.output.resolve()
    if output.exists():
        parser.error('output already exists; preserve it and select a new directory')
    tools = dict(gcc=args.gcc.resolve(), clang=args.clang.resolve())
    runtime = [args.runtime_dir.resolve() / name for name in ('fbrt0.o', 'libfb.a')]
    runtime_source = ROOT / 'src/rtlib/arrays/array_redimto.c'
    hooks = ROOT / 'tests/rtlib/array-copy-hooks.c'
    hook_header = hooks.with_suffix('.h')
    sources = [ROOT / 'tests/quirk' / (name + '.bas') for name in CASES]
    inputs = [args.fbc.resolve(), *tools.values(), *runtime, runtime_source, hooks,
              hook_header, *sources, Path(__file__).resolve(),
              ROOT / 'build_scripts/audit-c-emitter.py', *sorted((ROOT / 'src/rtlib').rglob('*.h'))]
    if args.baseline_runtime_source:
        inputs.append(args.baseline_runtime_source.resolve())
    if not all(path.is_file() for path in inputs):
        parser.error('a provider, runtime input or fixture is missing')
    pins = {str(path): audit.digest(path) for path in inputs}
    output.mkdir(parents=True)
    temporary = output / 'tmp'
    temporary.mkdir()
    for name in ('TEMP', 'TMP', 'TMPDIR'):
        os.environ[name] = str(temporary)
    ctypes.windll.kernel32.SetErrorMode(3)
    result = dict(complete=False, overall_complete=False, installed=False, pins=pins,
                  scope='Win32/Win64 objects and Win64 generated-copy sanitizer controls',
                  records=[], ordinary_objects=0, native_after=0, native_before_controls=0)
    support = {}

    def guard(extra=0):
        owned = sum(p.stat().st_size for p in output.rglob('*') if p.is_file())
        if owned + extra > MAX_OUTPUT_BYTES or shutil.disk_usage(output).free < 5 * 1024 ** 3:
            raise RuntimeError('array-copy gate resource limit reached')

    def publish():
        audit.write_json(output / 'results.json', result)

    def invoke(command, folder, label, expected=0):
        guard()
        stage = audit.invoke(list(map(str, command)), ROOT, folder / (label + '.log'), 120)
        stage['expected_exit'] = expected
        if stage['exit'] != expected:
            result['failure'] = stage
            publish()
            raise RuntimeError((label, stage['exit'], expected, stage['log']))
        return stage

    def archive(path):
        raw = path.read_bytes()
        packed = gzip.compress(raw, mtime=0)
        guard(len(packed))
        target = Path(str(path) + '.gz')
        target.write_bytes(packed)
        if gzip.decompress(target.read_bytes()) != raw:
            raise RuntimeError('artifact verification failed: ' + str(path))
        saved = dict(path=str(path), sha256=audit.digest(path), gzip_path=str(target),
                     gzip_sha256=audit.digest(target))
        path.unlink()
        return saved

    def support_object(backend, optimization, fault, version, warnings):
        key = backend, optimization, fault, version
        if key in support:
            return support[key]['object']
        folder = output / 'support'
        folder.mkdir(exist_ok=True)
        name = '-'.join((backend, optimization, 'fault' if fault else 'normal', version))
        obj = folder / (name + '.o')
        source = args.baseline_runtime_source if version == 'before' else runtime_source
        # Runtime headers require the real CRT include paths. Generated C is
        # self-contained and retains -nostdinc in its own compilation below.
        semantics = [flag for flag in audit.SEMANTICS if flag != '-nostdinc']
        command = [tools[backend], *semantics, '-pipe', '-' + optimization, '-m64',
                   *warnings, *SANITIZER, '-I' + str(ROOT / 'src/rtlib')]
        if fault:
            command += ['-DFB_COPY_ALLOC_INSTRUMENT', '-include', hook_header]
        command += ['-c', source, '-o', obj]
        support[key] = dict(object=obj, stage=invoke(command, folder, name))
        return obj

    try:
        for source in sources:
            expected_stdout, old_exit = CASES[source.stem]
            fault = old_exit is not None
            for bits in (32, 64):
                for backend in ('gcc', 'clang'):
                    folder = output / (source.stem + '-' + str(bits) + '-' + backend)
                    folder.mkdir()
                    emit = invoke([args.fbc, '-prefix', ROOT, '-gen', backend, '-r', '-O', '2',
                                   '-arch', '686' if bits == 32 else 'x86_64', '-m', source.stem,
                                   source, '-o', folder / 'unit.o'], folder, 'emit')
                    cfile = folder / 'unit.c'
                    row = dict(source=str(source), bits=bits, backend=backend, emit=emit, checks=[],
                               c_sha256=audit.digest(cfile), guards=copy_guards(cfile.read_text()))
                    result['records'].append(row)
                    warnings = (audit.GCC_WARNINGS + ['-fno-diagnostics-show-caret'] if backend == 'gcc'
                                else ['-Weverything', '-Wwrite-strings', '-fno-caret-diagnostics'])
                    for optimization in ('O0', 'O2'):
                        obj = folder / (optimization + '.o')
                        command = [tools[backend], *audit.SEMANTICS, '-pipe', '-' + optimization,
                                   '-m' + str(bits), *warnings, '-c', cfile, '-o', obj]
                        check = dict(optimization=optimization,
                                     object=invoke(command, folder, optimization + '-object'))
                        row['checks'].append(check)
                        result['ordinary_objects'] += 1
                        if bits == 64:
                            sanitized = folder / (optimization + '-san.o')
                            check['sanitizer'] = invoke([*command[:-4], *SANITIZER, '-c', cfile,
                                                        '-o', sanitized], folder, optimization + '-sanitizer')
                            hook_obj = folder / (optimization + '-hooks.o')
                            if fault:
                                check['hooks'] = invoke([tools[backend], '-std=gnu11', '-pipe',
                                    '-' + optimization, *warnings, '-I' + str(ROOT / 'src/rtlib'),
                                    '-c', hooks, '-o', hook_obj], folder, optimization + '-hooks')
                            check['native'] = []
                            versions = ['before', 'after'] if fault and args.baseline_runtime_source else ['after']
                            for version in versions:
                                rtobj = support_object(backend, optimization, fault, version, warnings)
                                image = folder / (optimization + '-' + version + '.exe')
                                objects = [sanitized, rtobj] + ([hook_obj] if fault else [])
                                link = invoke([tools[backend], '-s', *objects, *runtime, '-lm',
                                               '-mthreads', '-o', image], folder, optimization + '-' + version + '-link')
                                run = invoke([image], folder, optimization + '-' + version + '-run',
                                             old_exit if version == 'before' else 0)
                                text = gzip.decompress(Path(run['log']).read_bytes()).decode().strip()
                                if text != ('' if version == 'before' else expected_stdout):
                                    raise RuntimeError('unexpected native output: ' + text)
                                result['native_before_controls' if version == 'before' else 'native_after'] += 1
                                check['native'].append(dict(version=version, link=link, run=run,
                                                           stdout=text, executable_storage=archive(image)))
                            check['sanitizer_storage'] = archive(sanitized)
                            if fault:
                                check['hook_storage'] = archive(hook_obj)
                        check['object_storage'] = archive(obj)
                        publish()
                    row['c_storage'] = archive(cfile)
        result['support'] = [dict(key=list(key), stage=value['stage'], storage=archive(value['object']))
                             for key, value in support.items()]
        if result['ordinary_objects'] != 24 or result['native_after'] != 12:
            raise RuntimeError('array-copy gate did not complete its matrix')
        if args.baseline_runtime_source and result['native_before_controls'] != 8:
            raise RuntimeError('array-copy gate did not preserve all failing controls')
        for path, expected in pins.items():
            if audit.digest(Path(path)) != expected:
                raise RuntimeError('input changed during qualification: ' + path)
        result['complete'] = True
        publish()
        print('C_ARRAY_COPIES_PASS objects=24 native=12 controls=' + str(result['native_before_controls']))
    except BaseException:
        publish()
        raise


if __name__ == '__main__':
    main()
# end of test-c-emitter-array-copies.py
