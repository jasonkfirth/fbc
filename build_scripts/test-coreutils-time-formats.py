"""Qualify complete emitted Coreutils time-format modules with native libc.
File: test-coreutils-time-formats.py
Original helper bodies supply format construction and timestamp dispatch.
The observer supplies callback controls and an independent libc reference.
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


def wrapper(text, kind):
    if kind == 'underlying':
        assert re.search(r'^static unsigned long UNDERLYING_STRFTIME\(', text, re.M)
        return '''
/* The audit entry delegates to the unchanged production helper. */
unsigned long c_audit_time_call(void*, char*, unsigned long, signed char, signed char, const void*);
unsigned long c_audit_time_call(void* zone, char* output, unsigned long capacity,
    signed char modifier, signed char conversion, const void* value)
{
    return UNDERLYING_STRFTIME(zone, output, capacity, modifier, conversion, value);
}
'''
    record = re.search(r'^struct (fb_type_\w+GL_5FUTMP) \{', text, re.M)
    slot = re.search(r'^static const char\* (\w+) __asm__\("C2FB_TIME_FORMAT\$"\);', text, re.M)
    assert record and slot and 'TIME_STRING(' in text
    return '''
/* Construct the declared record, then observe the existing timestamp helper. */
const char* c_audit_time_string(long, const char*);
const char* c_audit_time_string(long seconds, const char* format)
{
    struct RECORD value = {0};
    _Static_assert(sizeof(value.UT_TS.TV_SEC) == sizeof(seconds), "Timestamp width");
    value.UT_TS.TV_SEC = seconds;
    FORMAT_SLOT = format;
    return TIME_STRING(&value);
}
'''.replace('RECORD', record[1]).replace('FORMAT_SLOT', slot[1])


def diagnostic_controls(text, kind, split_arguments):
    name = 'UNDERLYING_STRFTIME' if kind == 'underlying' else 'TIME_STRING'
    function = re.search(r'^static [^;\r\n]+\b' + name + r'\([^;\r\n]*\)\r?\n\{', text, re.M)
    assert function, name
    end = text.index('\n}', function.end())
    body = text[function.end():end]
    call = re.search(r'\bstrftime\(\s*(.*)\s*\);', body)
    assert call
    arguments = split_arguments(call[1])
    assert len(arguments) == 4
    for label, literal in (('valid-year', ' %Y'), ('invalid-conversion', ' %Q')):
        current = list(arguments)
        current[2] = json.dumps(literal)
        first, last = function.end() + call.start(), function.end() + call.end()
        yield label, text[:first] + 'strftime(' + ', '.join(current) + ');' + text[last:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-c', type=Path, required=True)
    parser.add_argument('--kind', choices=('underlying', 'string'), required=True)
    parser.add_argument('--imaxtostr-c', type=Path)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--objcopy', default='objcopy')
    parser.add_argument('--optimization', choices=('0', '2'), default='2')
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    assert args.kind != 'string' or args.imaxtostr_c is not None
    compiler, objcopy = shutil.which(args.cc), shutil.which(args.objcopy)
    assert compiler and objcopy
    stat_path = Path(__file__).with_name('test-coreutils-stat-formats.py')
    formatter_path = Path(__file__).with_name('test-coreutils-vasnprintf.py')
    executable_sections = runpy.run_path(str(stat_path))['executable_sections']
    split_arguments = runpy.run_path(str(formatter_path))['split_arguments']
    observer = ROOT / 'tests/compiler-support/coreutils-time-formats.c'
    inputs = [args.source_c, observer, Path(__file__), stat_path, formatter_path]
    if args.imaxtostr_c:
        inputs.append(args.imaxtostr_c)
    pins = {str(path.resolve()): sha(path) for path in inputs}
    raw = args.source_c.read_bytes()
    text = raw.decode()
    assert 'c_audit_time_' not in text
    flags = json.loads(args.native_flags_json.read_text())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')]
    flags += ['-O' + args.optimization, '-fno-strict-aliasing', '-fwrapv',
        '-ffunction-sections', '-fdata-sections']
    state = dict(checks_complete=False, source_pins=pins, commands=[], diagnostic_controls=[],
        kind=args.kind, optimization=args.optimization, peak_owned_bytes=0)

    def run(command, work, accepted=True):
        command = list(map(str, command))
        process = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL,
            capture_output=True, text=True)
        output = process.stdout + process.stderr
        assert len(output.encode()) < 4 * 1024 * 1024
        row = dict(command=command, exit=process.returncode, output=output,
            warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        state['peak_owned_bytes'] = max(state['peak_owned_bytes'],
            sum(path.stat().st_size for path in work.iterdir() if path.is_file()))
        assert state['peak_owned_bytes'] < 8 * 1024 * 1024
        if accepted:
            assert process.returncode == 0, output[-6000:]
        return row

    assert not args.receipt.exists()
    try:
        with tempfile.TemporaryDirectory(prefix='fbc-time-formats-', dir=args.scratch_root) as directory:
            work = Path(directory)
            module, obj = work / 'module.c', work / 'module.o'
            module.write_bytes(raw + wrapper(text, args.kind).encode())
            compiled = run([compiler, *flags, '-c', module, '-o', obj], work)
            state['production_warnings'] = compiled['warnings']
            sections = executable_sections(obj)
            run([objcopy, '--redefine-sym', 'main=c_audit_unused_time_main', obj], work)
            assert executable_sections(obj) == sections
            state['executable_sections_unchanged_by_entry_symbol_rename'] = sections
            objects = [obj]
            wraps = ['strftime', 'set_tz', 'revert_tz']
            if args.kind == 'string':
                helper = work / 'imaxtostr.o'
                run([compiler, *flags, '-c', args.imaxtostr_c.resolve(), '-o', helper], work)
                run([objcopy, '--redefine-sym', 'main=c_audit_unused_imaxtostr_main', helper], work)
                objects.append(helper)
                wraps = ['strftime', 'localtime', 'imaxtostr']
            observed = work / 'observer.o'
            mode = ['-DTIME_UNDERLYING'] if args.kind == 'underlying' else []
            run([compiler, *[flag for flag in flags if flag != '-nostdinc'], *mode,
                '-c', observer, '-o', observed], work)
            executable = work / 'values'
            run([compiler, '-Wl,--gc-sections', *['-Wl,--wrap=' + name for name in wraps],
                *objects, observed, '-o', executable], work)
            executed = run([executable], work)
            match = re.fullmatch(r'TIME_FORMAT_PASS cases=(\d+) checks=(\d+) locales=(\d+)\s*', executed['output'])
            assert match, executed['output']
            state.update(cases=int(match[1]), checks=int(match[2]), locales=int(match[3]))
            # Probe the native compiler independently of fbc declarations.
            # Some Clang versions accept strftime attributes but only parse
            # printf/scanf conversion grammars. The printf control must fail.
            probe = work / 'native-capability.c'
            declarations = '''
struct c_audit_tm;
extern __SIZE_TYPE__ c_audit_strftime(char*, __SIZE_TYPE__, const char*, const struct c_audit_tm*)
    __attribute__((format(strftime, 3, 0)));
extern int c_audit_printf(const char*, ...) __attribute__((format(printf, 1, 2)));
void c_audit_probe(char*, const struct c_audit_tm*);
'''
            probe.write_text(declarations + '''
void c_audit_probe(char* output, const struct c_audit_tm* value)
{ c_audit_strftime(output, 64, "%Q", value); }
''')
            calendar = run([compiler, *flags, '-Werror=format', '-c', probe,
                '-o', work / 'capability.o'], work, accepted=False)
            format_error = lambda result: re.search(r'\[-W(?:error=|error,-W)format(?:=)?\]', result['output']) is not None
            calendar_supported = calendar['exit'] != 0 and format_error(calendar)
            assert calendar_supported or calendar['exit'] == 0, calendar['output'][-6000:]
            probe.write_text(declarations + '''
void c_audit_probe(char* output, const struct c_audit_tm* value)
{ (void)output; (void)value; c_audit_printf("%d", "text"); }
''')
            printf = run([compiler, *flags, '-Werror=format', '-c', probe,
                '-o', work / 'capability.o'], work, accepted=False)
            assert printf['exit'] != 0 and format_error(printf), printf['output'][-6000:]
            state['native_calendar_diagnostics'] = dict(available=calendar_supported,
                calendar_probe=calendar, printf_probe=printf)
            control = work / 'format-control.c'
            for name, content in diagnostic_controls(text, args.kind, split_arguments):
                control.write_bytes(content.encode())
                checked = run([compiler, *flags, '-Werror=format', '-c', control,
                    '-o', work / 'control.o'], work, accepted=False)
                expected_error = name == 'invalid-conversion' and calendar_supported
                tagged = format_error(checked)
                assert (checked['exit'] != 0 and tagged) if expected_error else checked['exit'] == 0, checked['output'][-6000:]
                state['diagnostic_controls'].append(dict(name=name, expected_error=expected_error,
                    native_calendar_diagnostics_available=calendar_supported, result=checked))
        assert all(sha(path) == pin for path, pin in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists())
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')) + '\n')
    print('TIME_FORMAT_CONTRACT_PASS', args.kind, state['cases'], state['checks'], flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-coreutils-time-formats.py
