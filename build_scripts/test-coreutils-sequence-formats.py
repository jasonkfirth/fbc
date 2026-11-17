"""Qualify the complete emitted seq configured-format helpers.

File test-coreutils-sequence-formats.py
Preserve actual parser providers, native argument types and unsuppressed diagnostics.
Application argument handling and the integer fast path retain separate tests.
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


def wrapper(text):
    layout = re.search(r'static void PRINT_NUMBERS\( const char\*, (struct \w+), long double,', text)
    view = re.search(r'static const char\* LONG_DOUBLE_FORMAT\( const char\*, (\w+)\* \);', text)
    assert layout and view
    body = '''
const char* c_audit_sequence_format(const char*, unsigned long*, unsigned long*);
void c_audit_sequence_print(const char*, unsigned long, unsigned long,
    long double, long double, long double, const char*, int);
const char* c_audit_sequence_format(const char* source, unsigned long* prefix, unsigned long* suffix)
{
    LAYOUT layout = {0};
    const char* result = LONG_DOUBLE_FORMAT(source, (VIEW*)&layout);
    *prefix = layout.PREFIX_LEN;
    *suffix = layout.SUFFIX_LEN;
    return result;
}
void c_audit_sequence_print(const char* format, unsigned long prefix, unsigned long suffix,
    long double first, long double step, long double last, const char* separator, int locale)
{
    LAYOUT layout = { .PREFIX_LEN = prefix, .SUFFIX_LEN = suffix };
    SEPARATOR = separator;
    LOCALE_OK = (int8)locale;
    PRINT_NUMBERS(format, layout, first, step, last);
}
'''.replace('LAYOUT', layout[1]).replace('VIEW', view[1])
    for name in ('SEPARATOR', 'LOCALE_OK'):
        match = re.search(r'\b(\w+)\s+__asm__\("C2FB_' + name + r'\$"\)', text)
        assert match
        body = re.sub(r'\b' + name + r'\b', match[1], body)
    return body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-c', type=Path, required=True)
    parser.add_argument('--provider-c', type=Path, action='append', required=True)
    parser.add_argument('--rtlib-dir', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--objcopy', default='objcopy')
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    compiler, objcopy = shutil.which(args.cc), shutil.which(args.objcopy)
    assert compiler and objcopy
    observer = ROOT / 'tests/compiler-support/coreutils-sequence-formats.c'
    stat = Path(__file__).with_name('test-coreutils-stat-formats.py')
    formatter = Path(__file__).with_name('test-coreutils-vasnprintf.py')
    support = ROOT / 'tests/compiler-support/native_c_support.py'
    executable_sections = runpy.run_path(str(stat))['executable_sections']
    split_arguments = runpy.run_path(str(formatter))['split_arguments']
    native_command = runpy.run_path(str(support))['native_command']
    runtime = args.rtlib_dir / 'math/longdouble_ops.c'
    inputs = [args.source_c, *args.provider_c, observer, stat, formatter, support, Path(__file__),
        runtime, runtime.with_name('extended_math.h'), runtime.with_name('longdouble_parse.h')]
    pins = {str(path.resolve()): sha(path) for path in inputs}
    raw = args.source_c.read_bytes()
    text = raw.decode()
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')]
    flags += ['-O' + args.optimization, '-fno-strict-aliasing', '-fwrapv', '-ffunction-sections', '-fdata-sections']
    state = dict(checks_complete=False, source_pins=pins, optimization=args.optimization,
        commands=[], diagnostic_controls=[], peak_owned_bytes=0)

    def run(command, work, accepted=True):
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
        if accepted:
            assert result.returncode == 0, output[-4000:]
        return row

    assert not args.receipt.exists()
    try:
        with tempfile.TemporaryDirectory(prefix='fbc-sequence-formats-', dir=args.scratch_root) as directory:
            work = Path(directory)
            source, obj = work / 'module.c', work / 'module.o'
            source.write_bytes(raw + wrapper(text).encode())
            compiled = run([compiler, *flags, '-c', source, '-o', obj], work)
            state['production_warnings'] = compiled['warnings']
            sections = executable_sections(obj)
            run([objcopy, '--redefine-sym', 'main=c_audit_unused_sequence_main', obj], work)
            assert executable_sections(obj) == sections
            state['executable_sections_unchanged_by_entry_symbol_rename'] = sections
            objects = [obj]
            state['provider_warnings'] = {}
            for index, provider in enumerate(args.provider_c):
                helper = work / ('provider-' + str(index) + '.o')
                compiled = run([compiler, *flags, '-c', provider.resolve(), '-o', helper], work)
                state['provider_warnings'][provider.name] = compiled['warnings']
                sections = executable_sections(helper)
                run([objcopy, '--redefine-sym', 'main=c_audit_unused_sequence_provider_' + str(index), helper], work)
                assert executable_sections(helper) == sections
                objects.append(helper)
            header_flags = [flag for flag in flags if flag != '-nostdinc']
            helper = work / 'longdouble.o'
            run([compiler, *header_flags, '-c', runtime.resolve(), '-o', helper], work)
            objects.append(helper)
            observed = work / 'observer.o'
            run([compiler, *header_flags, '-c', observer, '-o', observed], work)
            executable = work / 'values'
            wraps = ['printf', 'asprintf', 'vasprintf', 'fputs_unlocked', 'setlocale', 'free']
            run([compiler, '-Wl,--gc-sections', *['-Wl,--wrap=' + name for name in wraps],
                *objects, observed, '-lm', '-o', executable], work)
            executed = run([executable], work)
            fields = ('cases', 'checks', 'formats', 'rounded', 'errors', 'parsers', 'separators', 'locales')
            pattern = 'SEQUENCE_FORMAT_PASS ' + ' '.join(name + r'=(\d+)' for name in fields) + r'\s*'
            match = re.fullmatch(pattern, executed['output'])
            assert match, executed['output']
            state.update({name: int(value) for name, value in zip(fields, match.groups())})
            header = re.search(r'^static void PRINT_NUMBERS\([^;\r\n]*\)\r?\n\{', text, re.M)
            assert header
            end = text.index('\n}', header.end())
            calls = list(re.finditer(r'\bprintf\(\s*(.*?)\s*\);', text[header.end():end]))
            assert len(calls) == 1
            call = calls[0]
            arguments = split_arguments(call[1])
            assert len(arguments) == 2
            for label, literal, rejected in (('correct-long-double', '%Lg', False), ('wrong-pointer', '%s', True)):
                arguments[0] = json.dumps(literal)
                controlled = text[:header.end() + call.start()] + 'printf(' + ', '.join(arguments) + ');' + text[header.end() + call.end():]
                source.write_text(controlled)
                result = run([compiler, *flags, '-Werror=format', '-c', source, '-o', work / 'control.o'], work, accepted=False)
                assert (result['exit'] != 0) == rejected, (label, result['output'][-4000:])
                if rejected:
                    assert re.search(r'\[-W(?:error=|error,-W)format(?:=)?\]', result['output'])
                state['diagnostic_controls'].append(dict(label=label, expected_error=rejected, result=result))
        assert all(sha(path) == pin for path, pin in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists())
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('SEQUENCE_FORMAT_QUALIFIED', args.optimization, state['cases'], state['checks'])
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-coreutils-sequence-formats.py
