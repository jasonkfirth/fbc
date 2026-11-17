"""Qualify both complete numfmt runtime-built floating-format paths.

File test-coreutils-numfmt-formats.py
Use the actual current runtime, independent numeric references and native diagnostics.
Application option handling and localized grouping retain separate tests.
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
    body = '''
int c_audit_numfmt(long double, int, char*, unsigned long, int, int, int, int, long,
    const char*, const char*);
int c_audit_numfmt(long double number, int decimals, char* output, unsigned long capacity,
    int scale, int grouping, int rounding, int width, long precision,
    const char* suffix, const char* separator)
{
    ZERO_PADDING_WIDTH = width;
    USER_PRECISION = precision;
    SUFFIX = suffix;
    UNIT_SEPARATOR = separator;
    DEV_DEBUG = 0;
    return DOUBLE_TO_HUMAN(number, decimals, output, (long)capacity, scale, grouping, rounding);
}
'''
    for name in ('ZERO_PADDING_WIDTH', 'USER_PRECISION', 'SUFFIX', 'UNIT_SEPARATOR', 'DEV_DEBUG'):
        match = re.search(r'\b(\w+)\s+__asm__\("C2FB_' + name + r'\$"\)', text)
        assert match, name
        body = re.sub(r'\b' + name + r'\b', match[1], body)
    return body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-c', type=Path, required=True)
    parser.add_argument('--rtlib-dir', type=Path, required=True)
    parser.add_argument('--runtime-archive', type=Path, required=True)
    parser.add_argument('--startup-object', type=Path, required=True)
    parser.add_argument('--enable-mt', action='store_true')
    parser.add_argument('--cc', required=True)
    parser.add_argument('--objcopy', default='objcopy')
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    compiler, objcopy = shutil.which(args.cc), shutil.which(args.objcopy)
    assert compiler and objcopy
    observer = ROOT / 'tests/compiler-support/coreutils-numfmt-formats.c'
    stat = Path(__file__).with_name('test-coreutils-stat-formats.py')
    formatter = Path(__file__).with_name('test-coreutils-vasnprintf.py')
    support = ROOT / 'tests/compiler-support/native_c_support.py'
    executable_sections = runpy.run_path(str(stat))['executable_sections']
    split_arguments = runpy.run_path(str(formatter))['split_arguments']
    native_command = runpy.run_path(str(support))['native_command']
    inputs = [args.source_c, args.runtime_archive, args.startup_object, observer, stat, formatter,
        support, Path(__file__), *args.rtlib_dir.rglob('*.h')]
    pins = {str(path.resolve()): sha(path) for path in inputs}
    raw = args.source_c.read_bytes()
    text = raw.decode()
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')]
    flags += ['-O' + args.optimization, '-fno-strict-aliasing', '-fwrapv', '-ffunction-sections', '-fdata-sections']
    state = dict(checks_complete=False, source_pins=pins, optimization=args.optimization,
        multithread_runtime=args.enable_mt, commands=[], diagnostic_controls=[], peak_owned_bytes=0)

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
        with tempfile.TemporaryDirectory(prefix='fbc-numfmt-formats-', dir=args.scratch_root) as directory:
            work = Path(directory)
            source, obj = work / 'module.c', work / 'module.o'
            source.write_bytes(raw + wrapper(text).encode())
            compiled = run([compiler, *flags, '-c', source, '-o', obj], work)
            state['production_warnings'] = compiled['warnings']
            sections = executable_sections(obj)
            run([objcopy, '--redefine-sym', 'main=c_audit_unused_numfmt_main', obj], work)
            assert executable_sections(obj) == sections
            state['executable_sections_unchanged_by_entry_symbol_rename'] = sections
            header_flags = [flag for flag in flags if flag != '-nostdinc']
            header_flags += ['-I' + str(args.rtlib_dir.resolve())]
            if args.enable_mt:
                header_flags += ['-DENABLE_MT']
            observed = work / 'observer.o'
            run([compiler, *header_flags, '-c', observer, '-o', observed], work)
            executable = work / 'values'
            wraps = ['snprintf', 'sprintf', 'vsnprintf', 'vsprintf', 'fb_ArrayRedimEx', 'fb_ArrayErase']
            run([compiler, '-Wl,--gc-sections', *['-Wl,--wrap=' + name for name in wraps], obj, observed,
                args.startup_object.resolve(), args.runtime_archive.resolve(),
                '-ltinfo', '-lm', '-ldl', '-pthread', '-o', executable], work)
            executed = run([executable], work)
            fields = ('cases', 'checks', 'unscaled', 'scaled', 'widths', 'retired')
            match = re.fullmatch('NUMFMT_FORMAT_PASS ' + ' '.join(name + r'=(\d+)' for name in fields) + r'\s*', executed['output'])
            assert match, executed['output']
            state.update({name: int(value) for name, value in zip(fields, match.groups())})
            header = re.search(r'^static int32 DOUBLE_TO_HUMAN\([^;\r\n]*\)\r?\n\{', text, re.M)
            assert header
            end = text.index('\n}', header.end())
            calls = list(re.finditer(r'\bsnprintf\(\s*(.*?)\s*\);', text[header.end():end]))
            assert len(calls) == 2
            for fault in (None, 0, 1):
                current = text
                targets = []
                for index, call in reversed(list(enumerate(calls))):
                    arguments = split_arguments(call[1])
                    assert len(arguments) == (6 if index == 0 else 9)
                    literal = '%.*Lf%s' if index == 0 else '%.*Lf%s%s%s%s'
                    if index == fault:
                        literal = literal.replace('%s', '%d', 1)
                    arguments[2] = json.dumps(literal)
                    first, last = header.end() + call.start(), header.end() + call.end()
                    targets.append(text.count('\n', 0, first) + 1)
                    current = current[:first] + 'snprintf(' + ', '.join(arguments) + ');' + current[last:]
                source.write_text(current)
                result = run([compiler, *flags, '-c', source, '-o', work / 'control.o'], work)
                diagnostics = []
                for line in result['output'].splitlines():
                    diagnostic = re.search(r':(\d+)(?::\d+)?: warning:.*\[(-W[^\]]+)\]', line)
                    if diagnostic and int(diagnostic[1]) in targets and diagnostic[2] in ('-Wformat', '-Wformat='):
                        diagnostics.append(line)
                # The existing GNU apostrophe-extension warning retains its own
                # location. Inspect type warnings at these two calls directly.
                assert bool(diagnostics) == (fault is not None), (fault, result['output'][-4000:])
                state['diagnostic_controls'].append(dict(fault=fault, expected_type_warning=fault is not None,
                    target_lines=targets, type_diagnostics=diagnostics, result=result))
        assert all(sha(path) == pin for path, pin in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists())
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('NUMFMT_FORMAT_QUALIFIED', args.optimization, state['cases'], state['checks'])
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-coreutils-numfmt-formats.py
