"""Project: FreeBASIC runtime qualification
File: test-string-header-conversions.py
Purpose: Exercise actual string header helper definitions under both C tools.
Responsibilities: Extract production helpers, test widths one/two/four, use real
    32-bit size types for object checks and keep one reusable binary scratch slot.
This file intentionally does NOT qualify another OS or rewrite the runtime headers.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[1]
FUNCTIONS = ('fb_wstr_AllocTemp', 'fb_wstr_Len', 'fb_wstr_Copy', 'fb_wstr_Move', 'fb_wstr_Compare')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def function(text, name):
    match = re.search(r'^static __inline__[^\n]*\b' + re.escape(name) + r'\s*\(', text, re.M)
    assert match, name
    start = text.index('{', match.start())
    depth = 1
    end = start + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


def fragments(directory):
    wide = (directory / 'fb_unicode.h').read_text(encoding='utf-8-sig')
    narrow = (directory / 'fb_string.h').read_text(encoding='utf-8-sig')
    flags = narrow[narrow.index('#ifdef HOST_64BIT'):narrow.index('/** Returns the string data.')]
    # The public descriptor typedef is the contract; its private C tag can change.
    descriptor_match = re.search(r'typedef struct\s+\w+\s*\{[^}]+\}\s*FBSTRING;', narrow)
    access_match = re.search(r'typedef FBSTRING FBSTRING_ACCESS[^;]+;', narrow)
    assert descriptor_match and access_match
    descriptor = narrow[descriptor_match.start():access_match.end()] + '\n'
    return ('/* Production fragments extracted by test-string-header-conversions.py. */\n' + flags + descriptor +
            '\n'.join(function(wide, name) for name in FUNCTIONS) + function(narrow, 'fb_hStrSetLength'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gcc', type=Path, required=True)
    parser.add_argument('--clang', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--control-headers', type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    # Reuse a dedicated scratch slot on later runs, while refusing a directory
    # containing unrelated work. No per-profile folders or backup copies grow.
    owned = {'string-header-fragments.h', 'string-header-test.exe', 'string-header-test.o',
             'results.json', 'logs.json.zlib'}
    assert all(path.is_file() and path.name in owned for path in output.iterdir()), 'Use a dedicated results directory.'
    headers = ROOT / 'src/rtlib'
    source = ROOT / 'tests/rtlib/string-header-conversions.c'
    pins = {str(path): digest(path) for path in (headers / 'fb_unicode.h', headers / 'fb_string.h', source, Path(__file__))}
    tools = {'gcc': args.gcc.resolve(strict=True), 'clang': args.clang.resolve(strict=True)}
    pins.update({str(path): digest(path) for path in tools.values()})
    rows, logs = [], []
    generated = output / 'string-header-fragments.h'
    binary = output / 'string-header-test.exe'
    obj = output / 'string-header-test.o'
    env = dict(os.environ)
    for name in ('TMP', 'TEMP', 'TMPDIR'):
        env[name] = str(output)

    def run(argv):
        command = list(map(str, argv))
        assert not any(arg == '-w' or arg.startswith('-Wno-') for arg in command)
        process = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, timeout=600)
        logs.append({'command': command, 'exit': process.returncode,
                     'stdout': process.stdout.decode(errors='replace'), 'stderr': process.stderr.decode(errors='replace')})
        return process

    cases = [('current', headers)]
    if args.control_headers:
        control = args.control_headers.resolve(strict=True)
        cases.append(('old-control', control))
        pins.update({str(control / name): digest(control / name) for name in ('fb_unicode.h', 'fb_string.h')})
    for provider, directory in cases:
        generated.write_text(fragments(directory), encoding='utf-8', newline='\n')
        for tool, executable in tools.items():
            for width in (1, 2, 4):
                for standard in (('gnu11', 'gnu23') if provider == 'current' else ('gnu23',)):
                    for optimization in (('O0', 'O2') if provider == 'current' else ('O2',)):
                        flags = ['-std=' + standard, '-' + optimization, '-DFB_TEST_WCHAR_WIDTH=' + str(width), '-I', output]
                        flags += (['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion', '-Wcast-align=strict']
                                  if tool == 'gcc' else ['-Weverything', '-ferror-limit=0'])
                        check = run([executable, *flags, source, '-o', binary])
                        assert check.returncode == 0, logs[-1]
                        executed = run([binary])
                        expected = 0 if provider == 'current' else 1
                        assert executed.returncode == expected, logs[-1]
                        assert ('STRING_HEADER_CONVERSIONS_PASS' in executed.stdout.decode()) == (provider == 'current')
                        rows.append({'provider': provider, 'tool': tool, 'width': width, 'standard': standard,
                                     'optimization': optimization, 'execution_exit': executed.returncode})
                        if provider == 'current' and standard == 'gnu23' and optimization == 'O2':
                            target = ['-m32'] if tool == 'gcc' else ['--target=i686-w64-windows-gnu']
                            check = run([executable, *target, *flags, '-c', source, '-o', obj])
                            assert check.returncode == 0, logs[-1]
                            rows.append({'provider': provider, 'tool': tool, 'width': width, 'target': 'win32-object', 'exit': check.returncode})
    generated.write_text(fragments(headers), encoding='utf-8', newline='\n')
    assert all(digest(Path(name)) == expected for name, expected in pins.items())
    result = {'checks_complete': True, 'pins': pins, 'cases': rows,
              'native_executions': sum('execution_exit' in row and row['provider'] == 'current' for row in rows),
              'old_controls': sum(row['provider'] == 'old-control' for row in rows),
              'win32_objects': sum(row.get('target') == 'win32-object' for row in rows),
              'widths_are_value_models': True, 'other_OS_qualification': False}
    (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    (output / 'logs.json.zlib').write_bytes(zlib.compress(json.dumps(logs).encode()))
    print('STRING_HEADER_MATRIX_PASS', result['native_executions'], result['old_controls'], result['win32_objects'])


if __name__ == '__main__':
    main()

# end of test-string-header-conversions.py
