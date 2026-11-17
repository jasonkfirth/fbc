"""FreeBASIC production word-count format-contract regression.
File: test-coreutils-wc-formats.py
Compile the original emitted module and call its unchanged output helper.
File reading and complete command execution retain their application tests.
"""
from collections import Counter
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def wrapper(raw):
    symbols = []
    for name in ('PRINT_LINES', 'PRINT_WORDS', 'PRINT_CHARS', 'PRINT_BYTES',
            'PRINT_LINELENGTH', 'NUMBER_WIDTH'):
        symbol = re.search(rb'^static (?:int8|int32) (\w+) __asm__\("C2FB_' +
            name.encode() + rb'\$"\);', raw, re.M)
        assert symbol, name
        symbols.append(symbol[1].decode())
    body = '''
/* This entry calls the unchanged production helper with its actual globals. */
void c_audit_wc_call(int32, int32, unsigned long, unsigned long, unsigned long,
    unsigned long, long, const char*);
void c_audit_wc_call(int32 mask, int32 width, unsigned long lines, unsigned long words,
    unsigned long chars, unsigned long bytes, long longest, const char* name)
{
FLAGS
    WIDTH = width;
    WRITE_COUNTS(lines, words, chars, bytes, longest, name);
}
'''
    flags = '\n'.join('    ' + symbol + ' = (int8)((mask >> ' + str(index) + ') & 1);'
        for index, symbol in enumerate(symbols[:5]))
    return body.replace('FLAGS', flags).replace('WIDTH', symbols[5]).encode()


def type_controls(raw):
    header = re.search(rb'^static void WRITE_COUNTS\([^;\r\n]*\)\r?\n\{', raw, re.M)
    assert header
    end = raw.index(b'\n}', header.end()) + 2
    calls = list(re.finditer(rb'printf\(\s*\w+,', raw[header.end():end]))
    assert len(calls) == 5
    # Each variant changes one format operand. Actual width and string argument
    # expressions retain their production types, including pointer qualifiers.
    for bad_call, bad_width in [(None, False), *[(index, False) for index in range(5)],
            *[(index, True) for index in range(5)]]:
        control = raw
        for index in reversed(range(5)):
            literal = '%*s'
            if index == bad_call:
                literal = '%s%s' if bad_width else '%*ld'
            first = header.end() + calls[index].start()
            last = header.end() + calls[index].end()
            control = control[:first] + ('printf( "' + literal + '",').encode() + control[last:]
        name = 'correct-types' if bad_call is None else ('width-' if bad_width else 'value-') + str(bad_call)
        yield name, control, bad_call is not None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-c', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--objcopy', default='objcopy')
    parser.add_argument('--optimization', choices=('0', '2'), default='2')
    parser.add_argument('--native-flags-json', type=Path)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    compiler, objcopy = shutil.which(args.cc), shutil.which(args.objcopy)
    assert compiler and objcopy
    observer = ROOT / 'tests/compiler-support/coreutils-wc-formats.c'
    support = ROOT / 'build_scripts/test-coreutils-stat-formats.py'
    spec = importlib.util.spec_from_file_location('stat_format_support', support)
    stat_support = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(stat_support)
    pins = {str(path.resolve()): sha(path) for path in (args.source_c, observer, support, Path(__file__))}
    raw = args.source_c.read_bytes()
    assert b'c_audit_wc_call' not in raw
    flags = ['-std=gnu11', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wformat=2']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')]
    flags += ['-O' + args.optimization, '-fno-strict-aliasing', '-fwrapv',
        '-ffunction-sections', '-fdata-sections']
    commands, diagnostics = [], []

    def run(command, directory, check=True):
        process = subprocess.run(list(map(str, command)), cwd=directory,
            stdin=subprocess.DEVNULL, capture_output=True, text=True)
        output = process.stdout + process.stderr
        assert len(output.encode()) < 4 * 1024 * 1024
        result = dict(command=list(map(str, command)), exit=process.returncode, output=output,
            warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        commands.append(result)
        if check and process.returncode:
            raise RuntimeError(output[-6000:])
        return result

    compared = checks = 0
    failure = None
    sections = None
    with tempfile.TemporaryDirectory(prefix='fbc-wc-formats-', dir=args.scratch_root) as directory:
        work = Path(directory).resolve()
        module, obj = work / 'wc.c', work / 'wc.o'
        module.write_bytes(raw + wrapper(raw))
        try:
            run([compiler, *flags, '-c', module, '-o', obj], work)
            sections = stat_support.executable_sections(obj)
            run([objcopy, '--redefine-sym', 'main=c_audit_unused_wc_main', obj], work)
            assert stat_support.executable_sections(obj) == sections
            run([compiler, *[flag for flag in flags if flag != '-nostdinc'],
                '-c', observer, '-o', work / 'observer.o'], work)
            wrapping = ['printf', 'putchar_unlocked', 'ferror_unlocked', 'fflush_unlocked',
                'clearerr_unlocked', 'error']
            run([compiler, '-Wl,--gc-sections',
                *['-Wl,--wrap=' + name for name in wrapping], obj, work / 'observer.o',
                '-o', work / 'values'], work)
            observed = run([work / 'values'], work)
            match = re.search(r'WC_FORMAT_VALUES_PASS cases=(\d+) checks=(\d+)', observed['output'])
            assert match, observed
            compared, checks = map(int, match.groups())
            control_source = work / 'type-control.c'
            for name, control, expect_error in type_controls(raw):
                control_source.write_bytes(control)
                checked = run([compiler, *flags, '-Werror=format', '-c', control_source,
                    '-o', work / 'control.o'], work, check=False)
                format_error = re.search(r'\[-W(?:error=|error,-W)format(?:=)?\]', checked['output']) is not None
                accepted = checked['exit'] != 0 and format_error if expect_error else checked['exit'] == 0
                diagnostics.append(dict(name=name, expect_format_error=expect_error,
                    accepted=accepted, result=checked))
                if not accepted:
                    raise RuntimeError(checked['output'][-6000:])
        except RuntimeError:
            failure = commands[-1]
        peak = sum(path.stat().st_size for path in work.iterdir() if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == digest for path, digest in pins.items())
    complete = compared == 9472 and failure is None and len(diagnostics) == 11 and all(
        control['accepted'] for control in diagnostics)
    result = dict(checks_complete=complete, source_pins=pins, commands=commands,
        value_cases=compared, value_checks=checks, diagnostic_controls=diagnostics,
        executable_sections_unchanged_by_entry_symbol_rename=sections,
        peak_owned_bytes=peak, scratch_directory_removed=not work.exists(), failure=failure)
    assert not args.receipt.exists()
    args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    if not complete:
        print(json.dumps(failure or result), flush=True)
        return 1
    print('WC_FORMAT_CONTRACTS_PASS', compared, checks, flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-coreutils-wc-formats.py
