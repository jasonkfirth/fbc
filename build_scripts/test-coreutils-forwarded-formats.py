"""Qualify the complete copy helper and both forwarded-format caller contracts.

File test-coreutils-forwarded-formats.py
Check exhaustive private callers, original native call types and helper output.
Complete filesystem behavior retains its project qualification.
"""
import argparse
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


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def call_arguments(line, name, split_arguments, complete=False):
    match = re.search(r'\b' + name + r'\s*\(', line, re.I)
    assert match
    start = match.end() - 1
    depth, quoted, index = 0, False, start
    while index < len(line):
        value = line[index]
        if quoted and value == '\\':
            index += 2
            continue
        if value == '"':
            quoted = not quoted
        elif not quoted:
            if value == '(':
                depth += 1
            elif value == ')':
                depth -= 1
                if depth == 0:
                    if complete:
                        assert not line[index + 1:].strip()
                    return split_arguments(line[start + 1:index])
        index += 1
    raise AssertionError('Unterminated source call')


def resolved_format_choices(expression, split_arguments):
    expression = expression.strip()
    while expression.startswith('('):
        group = call_arguments('group' + expression, 'group', split_arguments, complete=True)
        assert len(group) == 1
        expression = group[0].strip()
    if re.fullmatch(r'(?:&O)?0ULL', expression, re.I):
        return [None]
    match = re.match(r'(\w+)\s*\(', expression)
    assert match, expression
    name = match[1].lower()
    arguments = call_arguments(expression, name, split_arguments, complete=True)
    if name == 'strptr':
        assert len(arguments) == 1
        literal = re.fullmatch(r'!"([^"\r\n]*)"', arguments[0].strip())
        assert literal
        return [literal[1]]
    if name in ('cptr', 'cast'):
        assert len(arguments) == 2
        choices = resolved_format_choices(arguments[1], split_arguments)
        datatype = re.sub(r'\s+', '', arguments[0]).lower()
        if name == 'cptr':
            assert datatype in ('constzstringptr', 'zstringptr', 'constanyptr', 'anyptr')
        else:
            assert choices == [None] and datatype in ('c2fb_size_t', 'c2fb_c_int')
        return choices
    if name == 'iif':
        assert len(arguments) == 3 and re.fullmatch(r'\w+->(?:verbose|exchange)', arguments[0].strip(), re.I)
        return resolved_format_choices(arguments[1], split_arguments) + resolved_format_choices(arguments[2], split_arguments)
    assert name == 'c2fb_external_dcgettext' and len(arguments) == 3
    return resolved_format_choices(arguments[1], split_arguments)


def source_contract(text, native, name, split_arguments):
    calls = [line.strip() for line in text.splitlines() if re.search(r'\b' + name + r'\s*\(', line, re.I)]
    expected_calls = 4 if name == 'emit_verbose' else 1
    assert len(calls) == expected_calls
    # Declaration, private definition and all direct calls account for every
    # use. A new address escape or caller requires a fresh contract review.
    assert len(re.findall(r'\b' + name + r'\b', text, re.I)) == expected_calls + 2
    native_code = re.sub(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*', '', native, flags=re.S)
    assert len(re.findall(r'\b' + name.upper() + r'\b', native_code)) == expected_calls + 2
    assert re.search(r'^static [^;\r\n]+\b' + name.upper() + r'\(', native, re.M)
    expressions, literal_sets, choices = [], [], []
    for line in calls:
        arguments = call_arguments(line, name, split_arguments)
        assert len(arguments) == (4 if name == 'emit_verbose' else 7)
        expression = arguments[0 if name == 'emit_verbose' else 3]
        expressions.append(expression)
        literal_sets.append(re.findall(r'StrPtr\(!"([^"\r\n]*)"\)', expression, re.I))
        choices.append(resolved_format_choices(expression, split_arguments))
    if name == 'emit_verbose':
        assert literal_sets == [['%s -> %s'], ['exchanged %s <-> %s', 'renamed %s -> %s'],
            ['copied %s -> %s'], ['%s -> %s']]
        assert choices == literal_sets
    else:
        assert literal_sets == [[r'%s -> %s\010']]
        assert choices == [[r'%s -> %s\010', None]]
    contract = dict(private=True, all_uses_accounted=True, direct_callers=len(calls),
        format_expressions=expressions, source_literal_sets=literal_sets, resolved_choices=choices)
    if name == 'make_dir_parents_private':
        header = re.search(r'^static [^;\r\n]+\bMAKE_DIR_PARENTS_PRIVATE\([^;\r\n]*\)\r?\n\{', native, re.M)
        assert header
        body = native[header.end():native.index('\n}', header.end())]
        printed = list(re.finditer(r'\bprintf\(\s*(.*?)\s*\);', body))
        assert len(printed) == 1
        arguments = split_arguments(printed[0][1])
        prefix = re.sub(r'\s+', '', body[:printed[0].start()])
        guard = re.search(r'if\(-\(int64\)\(int32\)\(int64\)-\(\(uint64\)(\w+)!=0ull\)==0ll\)goto(\w+);\{$', prefix)
        assert guard and arguments[0] == guard[1]
        suffix = re.sub(r'\s+', '', body[printed[0].end():])
        assert suffix.startswith('}' + guard[2] + ':;')
        contract['native_null_guard'] = dict(format_parameter=guard[1], skip_target=guard[2],
            proof='The normalized null predicate branches past the sole printf call')
    return contract


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--copy-c', type=Path, required=True)
    parser.add_argument('--cp-c', type=Path, required=True)
    parser.add_argument('--copy-bas', type=Path, required=True)
    parser.add_argument('--cp-bas', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--objcopy', default='objcopy')
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    compiler, objcopy = shutil.which(args.cc), shutil.which(args.objcopy)
    assert compiler and objcopy
    observer = ROOT / 'tests/compiler-support/coreutils-forwarded-formats.c'
    stat = Path(__file__).with_name('test-coreutils-stat-formats.py')
    formatter = Path(__file__).with_name('test-coreutils-vasnprintf.py')
    support = ROOT / 'tests/compiler-support/native_c_support.py'
    executable_sections = runpy.run_path(str(stat))['executable_sections']
    split_arguments = runpy.run_path(str(formatter))['split_arguments']
    native_command = runpy.run_path(str(support))['native_command']
    inputs = [args.copy_c, args.cp_c, args.copy_bas, args.cp_bas, observer, stat, formatter, support, Path(__file__)]
    pins = {str(path.resolve()): sha(path) for path in inputs}
    texts = {'copy': args.copy_c.read_text(), 'cp': args.cp_c.read_text()}
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')]
    flags += ['-O' + args.optimization, '-fno-strict-aliasing', '-fwrapv', '-ffunction-sections', '-fdata-sections']
    state = dict(checks_complete=False, source_pins=pins, optimization=args.optimization,
        commands=[], diagnostic_controls=[], peak_owned_bytes=0, source_contracts={})

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
        assert result.returncode == 0, output[-4000:]
        return row

    assert not args.receipt.exists()
    try:
        for name, source, function in (('copy', args.copy_bas, 'emit_verbose'),
            ('cp', args.cp_bas, 'make_dir_parents_private')):
            state['source_contracts'][name] = source_contract(source.read_text(), texts[name], function, split_arguments)
        with tempfile.TemporaryDirectory(prefix='fbc-forwarded-formats-', dir=args.scratch_root) as directory:
            work = Path(directory)
            objects = {}
            state['production_warnings'] = {}
            for name, original in (('copy', args.copy_c), ('cp', args.cp_c)):
                source, obj = work / (name + '.c'), work / (name + '.o')
                raw = original.read_bytes()
                if name == 'copy':
                    raw += b'\nvoid c_audit_forwarded_copy(const char*, const char*, const char*, const char*);\n'
                    raw += b'void c_audit_forwarded_copy(const char* format, const char* source, const char* destination, const char* backup)\n'
                    raw += b'{ EMIT_VERBOSE(format, source, destination, backup); }\n'
                source.write_bytes(raw)
                compiled = run([compiler, *flags, '-c', source, '-o', obj], work)
                state['production_warnings'][name] = compiled['warnings']
                objects[name] = obj
            sections = executable_sections(objects['copy'])
            run([objcopy, '--redefine-sym', 'main=c_audit_unused_forwarded_main', objects['copy']], work)
            assert executable_sections(objects['copy']) == sections
            state['copy_executable_sections_unchanged_by_entry_symbol_rename'] = sections
            observed = work / 'observer.o'
            header_flags = [flag for flag in flags if flag != '-nostdinc' and not flag.startswith('-masm=')]
            run([compiler, *header_flags, '-c', observer, '-o', observed], work)
            executable = work / 'values'
            run([compiler, '-Wl,--gc-sections', '-Wl,--wrap=printf', '-Wl,--wrap=putchar_unlocked',
                '-Wl,--wrap=vsnprintf', objects['copy'], observed, '-o', executable], work)
            executed = run([executable], work)
            fields = ('cases', 'checks', 'formats', 'backups', 'lines')
            match = re.fullmatch('FORWARDED_FORMAT_PASS ' + ' '.join(name + r'=(\d+)' for name in fields) + r'\s*', executed['output'])
            assert match, executed['output']
            state.update({name: int(value) for name, value in zip(fields, match.groups())})
            for name, function in (('copy', 'EMIT_VERBOSE'), ('cp', 'MAKE_DIR_PARENTS_PRIVATE')):
                text = texts[name]
                header = re.search(r'^static [^;\r\n]+\b' + function + r'\([^;\r\n]*\)\r?\n\{', text, re.M)
                assert header
                end = text.index('\n}', header.end())
                calls = list(re.finditer(r'\bprintf\(\s*(.*?)\s*\);', text[header.end():end]))
                # The copy helper also prints a fixed backup suffix.
                call = calls[0]
                arguments = split_arguments(call[1])
                assert len(arguments) == 3
                target_line = text.count('\n', 0, header.end() + call.start()) + 1
                for fault in (False, True):
                    arguments[0] = json.dumps('%s -> %d' if fault else '%s -> %s')
                    first, last = header.end() + call.start(), header.end() + call.end()
                    controlled = text[:first] + 'printf(' + ', '.join(arguments) + ');' + text[last:]
                    source = work / 'control.c'
                    source.write_text(controlled)
                    result = run([compiler, *flags, '-c', source, '-o', work / 'control.o'], work)
                    diagnostics = []
                    for line in result['output'].splitlines():
                        diagnostic = re.search(r':(\d+)(?::\d+)?: warning:.*\[(-W[^\]]+)\]', line)
                        if diagnostic and int(diagnostic[1]) == target_line and diagnostic[2] in ('-Wformat', '-Wformat='):
                            diagnostics.append(line)
                    assert bool(diagnostics) == fault, (name, fault, result['output'][-4000:])
                    state['diagnostic_controls'].append(dict(module=name, expected_type_warning=fault,
                        target_line=target_line, type_diagnostics=diagnostics, result=result))
        assert all(sha(path) == pin for path, pin in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists(),
            cp_format_resolution='Complete private-caller proof and native correct/wrong-type controls at the original call')
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists() if 'work' in locals() else True)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('FORWARDED_FORMAT_QUALIFIED', args.optimization, state['cases'], state['checks'])
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-coreutils-forwarded-formats.py
