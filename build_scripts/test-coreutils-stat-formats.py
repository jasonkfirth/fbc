"""FreeBASIC coreutils stat format-contract regression.
File: test-coreutils-stat-formats.py
Compile an emitted Linux x86-64 stat module, expose its production helper
definitions to the separate C observer and retain every native diagnostic.
The source module's ordinary application entry keeps a separate test role.
"""
from collections import Counter
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def executable_sections(path):
    raw = path.read_bytes()
    assert raw[:6] == b'\x7fELF\x02\x01', 'The stat source profile requires little-endian ELF64'
    first = struct.unpack_from('<Q', raw, 40)[0]
    width, count = struct.unpack_from('<HH', raw, 58)
    sections = []
    for index in range(count):
        offset = first + index * width
        flags = struct.unpack_from('<Q', raw, offset + 8)[0]
        if flags & 4:
            start, size = struct.unpack_from('<QQ', raw, offset + 24)
            sections.append(dict(bytes=size,
                sha256=hashlib.sha256(raw[start:start + size]).hexdigest()))
    return sections


def format_type_controls(raw):
    contracts = [('OUT_STRING', '%s', '%jd'), ('OUT_INT', '%jd', '%s'),
        ('OUT_UINT', '%ju', '%s'), ('OUT_UINT_O', '%jo', '%s'),
        ('OUT_UINT_X', '%jx', '%s'), ('OUT_MINUS_ZERO', '%.0f', '%s'),
        ('OUT_FILE_CONTEXT', '%s', '%jd')]
    spans = []
    for name, correct, incorrect in contracts:
        header = re.search(rb'^static [^;\r\n]+\b' + name.encode() +
            rb'\([^;\r\n]*\)\r?\n\{', raw, re.M)
        assert header, name
        end = raw.index(b'\n}', header.end()) + 2
        call = re.search(rb'printf\(\s*\(const char\*\)\w+,', raw[header.end():end])
        assert call, name
        spans.append((name, header.end() + call.start(), header.end() + call.end(), correct, incorrect))
    # Only the format operand changes. Declarations and every variadic payload
    # retain their actual production C types in all diagnostic controls.
    for fault in [None, *[name for name, *_ in spans]]:
        control = raw
        for name, first, last, correct, incorrect in sorted(spans, key=lambda item: item[1], reverse=True):
            literal = incorrect if name == fault else correct
            control = control[:first] + ('printf( "' + literal + '",').encode() + control[last:]
        yield fault or 'correct-types', control, fault is not None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-c', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--objcopy', default='objcopy')
    parser.add_argument('--optimization', choices=('0', '2'), default='2')
    parser.add_argument('--native-flags-json', type=Path)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    compiler = shutil.which(args.cc)
    objcopy = shutil.which(args.objcopy)
    assert compiler and objcopy, 'Both the native C compiler and objcopy must be available'
    compiler, objcopy = str(Path(compiler).absolute()), str(Path(objcopy).absolute())
    observer = ROOT / 'tests/compiler-support/coreutils-stat-formats.c'
    pins = {str(path.resolve()): sha(path) for path in [args.source_c, observer, Path(__file__)]}
    raw = args.source_c.read_bytes()
    text = raw.decode()
    assert 'c_audit_stat_call' not in text
    follow = re.search(r'^static int8 (\w+) __asm__\("C2FB_FOLLOW_LINKS\$"\);', text, re.M)
    assert follow, 'The production stat module must define its follow-links slot'
    for name in ('OUT_STRING', 'OUT_INT', 'OUT_UINT', 'OUT_UINT_O', 'OUT_UINT_X',
                 'OUT_MINUS_ZERO', 'OUT_FILE_CONTEXT'):
        assert re.search(r'^static [^;\r\n]+\b' + name + r'\([^;\r\n]*\)\r?\n\{', text, re.M), name
    flag_line = next(line for line in text.splitlines() if '__asm__("C2FB_PRINTF_FLAGS$") =' in line)
    assert list(map(int, re.findall(r'\(int8\)(\d+)', flag_line))) == [39, 45, 43, 32, 35, 48, 73, 0]
    wrapper = '''
/* Audit entry points call the unchanged production helper definitions. */
int32 c_audit_stat_call(int32, char*, unsigned long, long, unsigned long, const char*, int32);
int32 c_audit_stat_call(int32 kind, char* format, unsigned long length, long signed_value,
    unsigned long unsigned_value, const char* text, int32 follow)
{
    switch(kind) {
    case 0: OUT_STRING(format, length, text); return 0;
    case 1: return OUT_INT(format, length, signed_value);
    case 2: return OUT_UINT(format, length, unsigned_value);
    case 3: OUT_UINT_O(format, length, unsigned_value); return 0;
    case 4: OUT_UINT_X(format, length, unsigned_value); return 0;
    case 5: return OUT_MINUS_ZERO(format, length);
    case 6: FOLLOW = (int8)follow; return OUT_FILE_CONTEXT(format, length, text);
    default: return -777;
    }
}
'''.replace('FOLLOW', follow[1])
    flags = ['-std=gnu11', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion',
        '-Wsign-conversion', '-Wcast-qual', '-Wshadow', '-Wformat=2']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')]
    flags += ['-O' + args.optimization, '-fno-strict-aliasing', '-fwrapv',
        '-ffunction-sections', '-fdata-sections']
    commands = []

    def run(command, work, check=True):
        process = subprocess.run(list(map(str, command)), cwd=work,
            stdin=subprocess.DEVNULL, capture_output=True, text=True)
        output = process.stdout + process.stderr
        assert len(output.encode()) < 4 * 1024 * 1024
        result = dict(command=list(map(str, command)), exit=process.returncode, output=output,
            warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        commands.append(result)
        if check and process.returncode:
            raise RuntimeError(output[-6000:])
        return result

    with tempfile.TemporaryDirectory(prefix='fbc-stat-formats-', dir=args.scratch_root) as directory:
        work = Path(directory).resolve()
        module = work / 'stat.c'
        module.write_bytes(raw + wrapper.encode())
        obj, executable = work / 'stat.o', work / 'values'
        sections = None
        compared = 0
        failure = None
        diagnostics = []
        try:
            run([compiler, *flags, '-c', module, '-o', obj], work)
            sections = executable_sections(obj)
            run([objcopy, '--redefine-sym', 'main=c_audit_unused_stat_main', obj], work)
            assert executable_sections(obj) == sections
            run([compiler, *[flag for flag in flags if flag != '-nostdinc'],
                '-c', observer.resolve(), '-o', work / 'observer.o'], work)
            run([compiler, '-Wl,--gc-sections', '-Wl,--wrap=printf', '-Wl,--wrap=error',
                obj, work / 'observer.o', '-o', executable], work)
            observed = run([executable], work)
            match = re.search(r'STAT_FORMAT_VALUES_PASS (\d+)', observed['output'])
            if match:
                compared = int(match[1])
            else:
                failure = observed
            control_source = work / 'type-control.c'
            for name, control, expect_error in format_type_controls(raw):
                control_source.write_bytes(control)
                checked = run([compiler, *flags, '-Werror=format', '-c',
                    control_source, '-o', work / 'control.o'], work, check=False)
                tagged_format_error = re.search(r'\[-W(?:error=|error,-W)format(?:=)?\]', checked['output']) is not None
                accepted = checked['exit'] != 0 and tagged_format_error if expect_error else checked['exit'] == 0
                diagnostics.append(dict(name=name, expect_format_error=expect_error,
                    accepted=accepted, result=checked))
                if not accepted:
                    raise RuntimeError(checked['output'][-6000:])
        except RuntimeError:
            failure = commands[-1]
        peak = sum(path.stat().st_size for path in work.iterdir() if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == digest for path, digest in pins.items())
    complete = (compared == 82695 and failure is None and len(diagnostics) == 8 and
        all(control['accepted'] for control in diagnostics))
    result = dict(checks_complete=complete, source_pins=pins, commands=commands,
        value_comparisons=compared, diagnostic_controls=diagnostics,
        executable_sections_unchanged_by_entry_symbol_rename=sections,
        peak_owned_bytes=peak, scratch_directory_removed=not work.exists(), failure=failure)
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    if not result['checks_complete']:
        print(json.dumps(failure or result), flush=True)
        return 1
    print('STAT_FORMAT_CONTRACTS_PASS', compared, flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-coreutils-stat-formats.py
