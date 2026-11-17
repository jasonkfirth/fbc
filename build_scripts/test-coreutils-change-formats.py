"""Compare complete emitted chmod/chown reporters with original C functions.

File test-coreutils-change-formats.py
Own native execution, original-site type controls and bounded scratch receipts.
Production conversion and application filesystem operations have separate scope.
"""
import argparse
import ast
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import runpy
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def reporting_body(text):
    header = re.search(r'^static void DESCRIBE_CHANGE\([^\n]*\)\n\{', text, re.M)
    assert header
    end = text.index('\n}', header.end()) + 2
    return header.start(), end, text[header.start():end]


def final_printf(text):
    start, _, body = reporting_body(text)
    calls = list(re.finditer(r'\bprintf\(\s*([^\n]+)\s*\);', body))
    assert calls
    call = calls[-1]
    return start + call.start(), start + call.end(), call[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--chmod-c', type=Path, required=True)
    parser.add_argument('--chown-c', type=Path, required=True)
    parser.add_argument('--chmod-reference', type=Path, required=True)
    parser.add_argument('--chown-reference', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--objcopy', default='objcopy')
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    compiler, objcopy = shutil.which(args.cc), shutil.which(args.objcopy)
    assert compiler and objcopy
    observer = ROOT / 'tests/compiler-support/coreutils-change-formats.c'
    support = ROOT / 'tests/compiler-support/native_c_support.py'
    sections_script = ROOT / 'build_scripts/test-coreutils-stat-formats.py'
    formatter = ROOT / 'build_scripts/test-coreutils-vasnprintf.py'
    native_command = runpy.run_path(str(support))['native_command']
    executable_sections = runpy.run_path(str(sections_script))['executable_sections']
    split_arguments = runpy.run_path(str(formatter))['split_arguments']
    paths = (args.chmod_c, args.chown_c, args.chmod_reference, args.chown_reference,
             observer, support, sections_script, formatter, Path(__file__))
    pins = {str(path.resolve()): digest(path) for path in paths}
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')]
    flags += ['-O' + args.optimization, '-ffunction-sections', '-fdata-sections']
    header_flags = [flag for flag in flags if flag != '-nostdinc' and not flag.startswith('-masm=')]
    state = dict(checks_complete=False, source_pins=pins, commands=[], diagnostic_controls=[],
                 production_warnings={}, reference_warnings={}, peak_owned_bytes=0)

    def run(command, work):
        command = native_command(command)
        result = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL, capture_output=True, text=True)
        output = result.stdout + result.stderr
        assert len(output.encode()) < 4 * 1024 * 1024
        row = dict(command=command, exit=result.returncode, output=output,
                   warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        state['peak_owned_bytes'] = max(state['peak_owned_bytes'],
            sum(path.stat().st_size for path in work.iterdir() if path.is_file()))
        assert state['peak_owned_bytes'] < 8 * 1024 * 1024
        assert result.returncode == 0, output[-6000:]
        return row

    assert not args.receipt.exists()
    try:
        with tempfile.TemporaryDirectory(prefix='coreutils-change-formats-', dir=args.scratch_root) as directory:
            work = Path(directory)
            objects = []
            for name, module, reference in (('chmod', args.chmod_c, args.chmod_reference),
                                             ('chown', args.chown_c, args.chown_reference)):
                text = '\n'.join(module.read_text().splitlines()) + '\n'
                if name == 'chmod':
                    record = re.search(r'^struct (\w+) \{\s*int32 STATUS;\s*uint32 OLD_MODE;\s*uint32 NEW_MODE;\s*\};', text, re.M)
                    assert record
                    wrapper = '\nvoid audit_actual_chmod(const char *, int32, uint32, uint32);\n'
                    wrapper += 'void audit_actual_chmod(const char *file, int32 status, uint32 old_mode, uint32 new_mode)\n'
                    wrapper += '{ struct ' + record[1] + ' change = {status, old_mode, new_mode}; DESCRIBE_CHANGE(file, &change); }\n'
                else:
                    wrapper = '\nvoid audit_actual_chown(const char *, int32, const char *, const char *, const char *, const char *);\n'
                    wrapper += 'void audit_actual_chown(const char *file, int32 status, const char *old_user, const char *old_group, const char *user, const char *group)\n'
                    wrapper += '{ DESCRIBE_CHANGE(file, status, old_user, old_group, user, group); }\n'
                source, obj = work / (name + '.c'), work / (name + '.o')
                # Keep the complete emitted module. The appended wrapper exposes
                # its private reporter and all real helper bodies to the observer.
                source.write_text(text + wrapper)
                extra = ['-mno-red-zone'] if name == 'chmod' else []
                compiled = run([compiler, *flags, *extra, '-c', source, '-o', obj], work)
                state['production_warnings'][name] = compiled['warnings']
                if name == 'chmod':
                    before = executable_sections(obj)
                    run([objcopy, '--redefine-sym', 'main=audit_unused_chmod_main', obj], work)
                    assert executable_sections(obj) == before
                    state['entry_rename_executable_sections'] = before
                objects.append(obj)
                reference_object = work / (name + '-reference.o')
                compiled = run([compiler, *header_flags, '-c', reference, '-o', reference_object], work)
                state['reference_warnings'][name] = compiled['warnings']
                objects.append(reference_object)
                first, last, expression = final_printf(text)
                arguments = split_arguments(expression)
                assert len(arguments) == (6 if name == 'chmod' else 4)
                literals = [ast.literal_eval('"' + value + '"')
                            for value in re.findall(r'_\("([^"\n]*)"\)', reference.read_text())]
                if name == 'chmod':
                    literals = [value for value in literals if value.count('%') == 5]
                formats = [(False, value) for value in dict.fromkeys(literals)]
                formats.append((True, '%s %s %s %04lo %s' if name == 'chmod' else '%s %d %s'))
                for fault, format_text in formats:
                    arguments[0] = json.dumps(format_text)
                    controlled = text[:first] + 'printf(' + ', '.join(arguments) + ');' + text[last:]
                    source.write_text(controlled)
                    compiled = run([compiler, *flags, *extra, '-c', source, '-o', work / 'control.o'], work)
                    line = text.count('\n', 0, first) + 1
                    diagnostics = [value for value in compiled['output'].splitlines()
                                   if re.search(r':' + str(line) + r':(?:\d+:)? warning:.*\[-Wformat=?\]', value)]
                    assert bool(diagnostics) == bool(fault), (name, fault, diagnostics)
                    state['diagnostic_controls'].append(dict(owner=name, incorrect=bool(fault),
                        line=line, diagnostics=diagnostics, compilation=compiled))
                source.unlink()
                (work / 'control.o').unlink()
            observed = work / 'observer.o'
            run([compiler, *header_flags, '-c', observer, '-o', observed], work)
            executable = work / 'reporting'
            run([compiler, '-Wl,--gc-sections', '-Wl,--wrap=printf', '-Wl,--wrap=free',
                 '-Wl,--wrap=__assert_fail', *objects, observed, '-o', executable], work)
            executed = run([executable], work)
            match = re.fullmatch(r'CHANGE_FORMAT_PASS cases=(\d+) checks=(\d+)\s*', executed['output'])
            assert match, executed['output']
            state.update(cases=int(match[1]), checks=int(match[2]))
        assert all(digest(path) == expected for path, expected in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists() if 'work' in locals() else True)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('CHANGE_FORMAT_QUALIFIED', state['cases'], state['checks'])


if __name__ == '__main__':
    main()

# end of test-coreutils-change-formats.py
