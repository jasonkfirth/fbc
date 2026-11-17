"""Qualify complete emitted nl/csplit configured-format helper bodies.

File: test-coreutils-configured-formats.py
Use actual source helpers, independent libc references and retained diagnostics.
Application argument processing and filesystem actions retain their own tests.
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


def wrapper(text, program):
    def slot(name):
        match = re.search(r'\b(\w+)(?:\[\d+\])?\s+__asm__\("C2FB_' + name + r'\$"\)', text)
        assert match, name
        return match[1]

    if program == 'nl':
        body = '''
void c_audit_numbered_line(int, int, long, long, const char*, int, long*, int*);
void c_audit_numbered_line(int mode, int width, long first, long increment,
    const char* separator, int overflow, long* next, int* wrapped)
{
    if (mode < 0 || mode >= 3) __builtin_trap();
    const char* formats[3] = {(const char*)FORMAT_LEFT, (const char*)FORMAT_RIGHT_NOLZ,
        (const char*)FORMAT_RIGHT_LZ};
    LINENO_FORMAT = formats[mode];
    LINENO_WIDTH = width;
    LINE_NO = first;
    PAGE_INCR = increment;
    SEPARATOR_STR = separator;
    LINE_NO_OVERFLOW = (int8)overflow;
    PRINT_LINENO();
    *next = LINE_NO;
    *wrapped = LINE_NO_OVERFLOW;
}
'''
        names = ('FORMAT_LEFT', 'FORMAT_RIGHT_NOLZ', 'FORMAT_RIGHT_LZ', 'LINENO_FORMAT',
            'LINENO_WIDTH', 'LINE_NO', 'PAGE_INCR', 'SEPARATOR_STR', 'LINE_NO_OVERFLOW')
    else:
        body = '''
long c_audit_split_validate(char*);
char* c_audit_split_filename(char*, const char*, char*, int, int);
long c_audit_split_validate(char* format) { return MAX_OUT(format); }
char* c_audit_split_filename(char* destination, const char* prefix,
    char* suffix, int digits, int number)
{
    FILENAME_SPACE = destination;
    PREFIX = prefix;
    SUFFIX = suffix;
    DIGITS = digits;
    return MAKE_FILENAME(number);
}
'''
        names = ('FILENAME_SPACE', 'PREFIX', 'SUFFIX', 'DIGITS')
    for name in sorted(names, key=len, reverse=True):
        body = re.sub(r'\b' + name + r'\b', slot(name), body)
    return body


def controls(text, program, split_arguments):
    owners = ('PRINT_LINENO',) if program == 'nl' else ('MAKE_FILENAME', 'MAX_OUT')
    spans = []
    for name in owners:
        header = re.search(r'^static [^;\r\n]+\b' + name + r'\([^;\r\n]*\)\r?\n\{', text, re.M)
        assert header, name
        end = text.index('\n}', header.end())
        calls = list(re.finditer(r'\b(printf|sprintf|snprintf)\(\s*(.*?)\s*\);', text[header.end():end]))
        for call in calls:
            arguments = split_arguments(call[2])
            if name == 'MAKE_FILENAME' and len(arguments) != 3:
                continue
            position = {'printf': 0, 'sprintf': 1, 'snprintf': 2}[call[1]]
            good = '%*jd%s' if name == 'PRINT_LINENO' else '%d'
            bad = '%*f%s' if name == 'PRINT_LINENO' else '%s'
            spans.append((name, header.end() + call.start(), header.end() + call.end(),
                call[1], arguments, position, good, bad))
    assert len(spans) == len(owners)
    for fault in (None, *owners):
        current = text
        for name, first, last, callee, arguments, position, good, bad in sorted(spans, key=lambda row: row[1], reverse=True):
            arguments = list(arguments)
            arguments[position] = json.dumps(bad if fault == name else good)
            current = current[:first] + callee + '(' + ', '.join(arguments) + ');' + current[last:]
        yield fault or 'correct-types', current, fault is not None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-c', type=Path, required=True)
    parser.add_argument('--program', choices=('nl', 'csplit'), required=True)
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
    observer = ROOT / 'tests/compiler-support/coreutils-configured-formats.c'
    stat = Path(__file__).with_name('test-coreutils-stat-formats.py')
    formatter = Path(__file__).with_name('test-coreutils-vasnprintf.py')
    support = ROOT / 'tests/compiler-support/native_c_support.py'
    executable_sections = runpy.run_path(str(stat))['executable_sections']
    split_arguments = runpy.run_path(str(formatter))['split_arguments']
    native_command = runpy.run_path(str(support))['native_command']
    runtime = args.rtlib_dir / 'math/longdouble_ops.c'
    inputs = [args.source_c, observer, stat, formatter, support, Path(__file__)]
    if args.program == 'nl':
        inputs += [runtime, runtime.with_name('extended_math.h'), runtime.with_name('longdouble_parse.h')]
    pins = {str(path.resolve()): sha(path) for path in inputs}
    raw = args.source_c.read_bytes()
    text = raw.decode()
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')]
    flags += ['-O' + args.optimization, '-fno-strict-aliasing', '-fwrapv', '-ffunction-sections', '-fdata-sections']
    state = dict(checks_complete=False, source_pins=pins, program=args.program,
        optimization=args.optimization, commands=[], diagnostic_controls=[], peak_owned_bytes=0)

    def run(command, work, accepted=True):
        command = native_command(command)
        result = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL,
            capture_output=True, text=True)
        output = result.stdout + result.stderr
        assert len(output.encode()) < 4 * 1024 * 1024
        row = dict(command=command, exit=result.returncode, output=output,
            warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        state['peak_owned_bytes'] = max(state['peak_owned_bytes'],
            sum(path.stat().st_size for path in work.iterdir() if path.is_file()))
        assert state['peak_owned_bytes'] < 8 * 1024 * 1024
        if accepted:
            assert result.returncode == 0, output[-6000:]
        return row

    assert not args.receipt.exists()
    try:
        with tempfile.TemporaryDirectory(prefix='fbc-configured-formats-', dir=args.scratch_root) as directory:
            work = Path(directory)
            source, obj = work / 'module.c', work / 'module.o'
            source.write_bytes(raw + wrapper(text, args.program).encode())
            compiled = run([compiler, *flags, '-c', source, '-o', obj], work)
            state['production_warnings'] = compiled['warnings']
            sections = executable_sections(obj)
            run([objcopy, '--redefine-sym', 'main=c_audit_unused_configured_main', obj], work)
            assert executable_sections(obj) == sections
            state['executable_sections_unchanged_by_entry_symbol_rename'] = sections
            objects = [obj]
            header_flags = [flag for flag in flags if flag != '-nostdinc']
            if args.program == 'nl':
                helper = work / 'longdouble.o'
                run([compiler, *header_flags, '-c', runtime.resolve(), '-o', helper], work)
                objects.append(helper)
            observed = work / 'observer.o'
            defines = ['-DCONFIGURED_NL'] if args.program == 'nl' else []
            run([compiler, *header_flags, *defines, '-c', observer, '-o', observed], work)
            executable = work / 'values'
            wraps = ['error', 'dcgettext', 'vsnprintf']
            wraps += ['printf'] if args.program == 'nl' else ['sprintf', 'snprintf', 'vsprintf']
            run([compiler, '-Wl,--gc-sections', *['-Wl,--wrap=' + name for name in wraps],
                *objects, observed, '-lm', '-o', executable], work)
            executed = run([executable], work)
            match = re.fullmatch(r'CONFIGURED_FORMAT_PASS cases=(\d+) checks=(\d+) formats=(\d+) errors=(\d+)\s*', executed['output'])
            assert match, executed['output']
            state.update(cases=int(match[1]), checks=int(match[2]), formats=int(match[3]), errors=int(match[4]))
            for label, control, rejected in controls(text, args.program, split_arguments):
                source.write_text(control)
                result = run([compiler, *flags, '-Werror=format', '-c', source, '-o', work / 'control.o'],
                    work, accepted=False)
                assert (result['exit'] != 0) == rejected, (label, result['output'][-6000:])
                if rejected:
                    assert re.search(r'\[-W(?:error=|error,-W)format(?:=)?\]', result['output']), result['output'][-6000:]
                state['diagnostic_controls'].append(dict(label=label, expected_error=rejected, result=result))
        assert all(sha(path) == pin for path, pin in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists())
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('CONFIGURED_FORMAT_QUALIFIED', args.program, args.optimization, state['cases'], state['checks'])
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-coreutils-configured-formats.py
