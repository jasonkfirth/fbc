"""Exercise complete emitted volatile updates and observe their access events.

File test-c2fb-volatile-updates.py
Own native reference comparison and insertion-only helper observation hooks.
Converter translation and complete application execution retain separate checks.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def instrument(text):
    declarations = ('void audit_volatile_before_read(const void *, unsigned int);\n'
                    'void audit_volatile_after_write(void *, unsigned int);\n')
    pattern = r'(?m)^static (?:uint64|void\*|void) (C2FB_VOLATILE_(SCALAR|POINTER)_(LOAD|STORE)_\w+)\( ([^\n]+) \)\n\{'
    edits, hooks = [], []
    for match in re.finditer(pattern, text):
        name, storage, direction, parameters = match.groups()
        address = re.search(r'(\w+)\s*$', parameters.split(',')[0])[1]
        width = 64 if storage == 'POINTER' else int(re.search(r'_(\d+)$', name)[1])
        assert width in (8, 16, 32, 64)
        end = text.index('\n}', match.end())
        if direction == 'LOAD':
            access = re.search(r'(?m)^[\t ]*__asm__ __volatile__\(\s*"mov rax,', text[match.end():end])
            assert access, name
            position = match.end() + access.start()
            inserted = '\n    audit_volatile_before_read(' + address + ', ' + str(width) + 'u);'
        else:
            assert not re.search(r'\breturn\b', text[match.end():end]), name
            position = end
            inserted = '\n    audit_volatile_after_write(' + address + ', ' + str(width) + 'u);'
        edits.append((position, inserted))
        hooks.append(dict(function=name, storage=storage, direction=direction, width=width))
    result = text
    for position, inserted in sorted(edits, reverse=True):
        result = result[:position] + inserted + result[position:]
    recovered = result
    for position, inserted in sorted(edits):
        assert recovered[position:position + len(inserted)] == inserted
        recovered = recovered[:position] + recovered[position + len(inserted):]
    assert recovered == text
    return declarations + result, hooks


def reference_source(text):
    names = re.findall(r'(?m)^(?:unsigned long long|void \*|const void \*)\s*(audit_\w+)\(', text)
    names = sorted(set(names))
    assert len(names) == 75, len(names)
    aliases = '\n'.join('#define ' + name + ' reference_' + name for name in names)
    return '/* Independent native C reference aliases. */\n' + aliases + '\n' + text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module-c', type=Path, required=True)
    parser.add_argument('--reference-c', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--tracked', action='store_true')
    parser.add_argument('--expect-access-failure', action='store_true')
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    assert not args.expect_access_failure or args.tracked
    digest = lambda raw: hashlib.sha256(raw).hexdigest()
    observer = ROOT / 'tests/compiler-support/c2fb-volatile-updates.c'
    support = ROOT / 'tests/compiler-support/native_c_support.py'
    pins = {str(path.resolve()): digest(path.read_bytes()) for path in
            (args.module_c, args.reference_c, observer, support, Path(__file__))}
    native_command = runpy.run_path(str(support))['native_command']
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not re.fullmatch(r'-O\w+', flag)]
    flags += ['-O' + args.optimization, '-fwrapv', '-fno-strict-aliasing']
    header_flags = [flag for flag in flags if flag != '-nostdinc' and not flag.startswith('-masm=')]
    state = dict(checks_complete=False, tracked=args.tracked, source_pins=pins, commands=[], peak_scratch_bytes=0)

    def run(command, work):
        command = native_command(command)
        creationflags = 0 if os.name != 'nt' else subprocess.CREATE_NO_WINDOW | subprocess.BELOW_NORMAL_PRIORITY_CLASS
        result = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL,
                                capture_output=True, text=True, creationflags=creationflags)
        output = result.stdout + result.stderr
        row = dict(command=command, exit=result.returncode, output=output,
                   warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        return row

    assert not args.receipt.exists()
    try:
        with tempfile.TemporaryDirectory(prefix='c2fb-volatile-updates-', dir=args.scratch_root) as directory:
            work = Path(directory)
            raw = args.module_c.read_bytes()
            source, reference = work / 'module.c', work / 'reference.c'
            if args.tracked:
                text = raw.decode().replace('\r\n', '\n').replace('\r', '')
                instrumented, hooks = instrument(text)
                source.write_text(instrumented)
                state.update(instrumentation_insertions_only=True, observed_helpers=hooks,
                             normalized_module_sha256=digest(text.encode()), observed_module_sha256=digest(source.read_bytes()))
            else:
                source.write_bytes(raw)
            reference.write_text(reference_source(args.reference_c.read_text()))
            module_object, reference_object, observer_object = work / 'module.o', work / 'reference.o', work / 'observer.o'
            binary = work / ('values.exe' if os.name == 'nt' else 'values')
            commands = ([args.cc, *flags, '-c', source, '-o', module_object],
                        [args.cc, *header_flags, '-c', reference, '-o', reference_object],
                        [args.cc, *header_flags, '-DAUDIT_TRACKED=' + str(int(args.tracked)), '-c', observer, '-o', observer_object],
                        [args.cc, module_object, reference_object, observer_object, '-o', binary])
            for command in commands:
                row = run(command, work)
                assert row['exit'] == 0, row['output'][-6000:]
            executed = run([binary], work)
            if args.expect_access_failure:
                assert executed['exit'] != 0 and 'VOLATILE_UPDATE_FAIL' in executed['output'], executed
                state['expected_access_failure'] = True
                state.update(cases=0, checks=0)
            else:
                assert executed['exit'] == 0, executed['output'][-6000:]
                match = re.fullmatch(r'VOLATILE_UPDATE_PASS cases=(\d+) checks=(\d+)\s*', executed['output'])
                assert match, executed['output']
                state.update(cases=int(match[1]), checks=int(match[2]))
            state['peak_scratch_bytes'] = sum(path.stat().st_size for path in work.iterdir())
            assert state['peak_scratch_bytes'] < 8 * 1024 * 1024
        assert all(digest(Path(path).read_bytes()) == expected for path, expected in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists() if 'work' in locals() else True)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('VOLATILE_UPDATE_QUALIFIED', state['cases'], state['checks'], 'tracked', int(args.tracked))


if __name__ == '__main__':
    main()

# end of test-c2fb-volatile-updates.py
