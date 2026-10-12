#!/usr/bin/env python3
"""FreeBASIC native regex example qualification
File: test-regex.py
Purpose: Execute the original PCRE/TRE examples with their real Windows SDKs.
Responsibilities: Matching C headers, ABI checks, allocator balance, empty-match
progress, copy rejection, private console input and source/SDK fingerprints.
This file contains no replacement regex engine or public-network test.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import re
import shutil
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--bits', type=int, choices=(32, 64), default=64)
    parser.add_argument('--tool-dir', type=Path, required=True)
    parser.add_argument('--runtime-prefix', type=Path, required=True)
    parser.add_argument('--pcre-header', type=Path, required=True)
    parser.add_argument('--pcre-lib', type=Path, required=True)
    parser.add_argument('--pcre-static', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('This qualification requires Windows SDKs and private consoles')
    root = Path(__file__).resolve().parents[1]
    work = args.output.resolve()
    work.mkdir(parents=True, exist_ok=True)
    compiler, tools = args.fbc.resolve(), args.tool_dir.resolve()
    sdk = tools.parent
    header = args.pcre_header.resolve()
    env = os.environ.copy()
    env['PATH'] = str(tools)+';'+env['PATH']
    for label in ('gcc', 'as', 'ld', 'ar'):
        env[label.upper()] = str(tools/(label+'.exe'))
    pcre_lib = args.pcre_lib.resolve()
    sdk_files = [header, sdk/'include/tre/regex.h', sdk/'bin/libtre-5.dll', sdk/'lib/libtre.dll.a']
    if args.pcre_static:
        # A private archive-only search directory prevents accidental DLL linkage.
        native_lib = work/'static-lib'
        native_lib.mkdir(exist_ok=True)
        for name in ('pcre', 'pcre16', 'pcre32'):
            source = pcre_lib/('lib'+name+'.a')
            sdk_files.append(source)
            shutil.copyfile(source, native_lib/source.name)
        pcre_lib = native_lib
    else:
        sdk_files += [sdk/'bin'/name for name in ('libpcre-1.dll', 'libpcre16-0.dll', 'libpcre32-0.dll')]
        sdk_files += [pcre_lib/('lib'+name+'.dll.a') for name in ('pcre', 'pcre16', 'pcre32')]
    examples = {
        'pcredemo': 'examples/regex/PCRE/pcredemo.bas',
        'preg': 'examples/regex/PCRE/preg_class.bas',
        'cregex': 'examples/regex/PCRE/CRegex/test.bas',
        'tre-match': 'examples/regex/TRE/match.bas',
        'tre-replace': 'examples/regex/TRE/replace.bas',
        'tre-subexpr': 'examples/regex/TRE/subexpr.bas',
    }
    names = list(examples.values())+[
        'examples/regex/PCRE/CRegex/CRegex.bi', 'examples/regex/PCRE/CRegex/CRegex.bas',
        'examples/regex/PCRE/CRegex/test.txt', 'examples/regex/PCRE/match-progress.bi',
        'inc/pcre-common.bi', 'inc/pcre.bi', 'inc/pcre16.bi', 'inc/pcre32.bi',
        'inc/regex.bi', 'inc/tre/tre.bi', 'inc/tre/regex.bi',
        'tests/bindings/regex-abi.c', 'tests/bindings/regex-abi.bas', 'tests/bindings/regex-runtime.bas',
        'doc/manual/cache/ExtLibpcre.wakka', 'doc/manual/cache/ExtLibtre.wakka',
        'build_scripts/test-regex.py', 'build_scripts/windows-test-console.py',
    ]
    fingerprint = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    records = []
    receipt = {
        'status': 'pending', 'bits': args.bits, 'pcre_static': args.pcre_static,
        'compiler_sha256': fingerprint(compiler),
        'source_sha256': {name: fingerprint(root/name) for name in names},
        'sdk_sha256': {str(path): fingerprint(path) for path in sdk_files},
        'records': records,
    }
    result_file = work/'results.json'

    def save():
        result_file.write_text(json.dumps(receipt, indent=2)+'\n')

    def run(command, directory, label, expected=0):
        result = subprocess.run(list(map(str, command)), cwd=directory, env=env,
                                stdin=subprocess.DEVNULL, capture_output=True, timeout=30)
        (directory/(label+'.log')).write_bytes(repr(list(map(str, command))).encode()+b'\n'+result.stdout+result.stderr)
        assert result.returncode == expected, (label, result.returncode, result.stdout[:1200], result.stderr[:1200])
        return result.stdout

    def record(backend, example, case, **evidence):
        records.append({'backend': backend, 'example': examples.get(example, example),
                        'case': case, 'passed': True, **evidence})
        save()

    def machine(binary, uses_pcre=True):
        data = binary.read_bytes()
        offset = struct.unpack_from('<I', data, 0x3c)[0]
        assert struct.unpack_from('<H', data, offset+4)[0] == (0x14c if args.bits == 32 else 0x8664)
        imports = run([tools/'objdump.exe', '-p', binary], binary.parent, binary.stem+'-imports')
        has_pcre_dll = bool(re.search(rb'DLL Name:\s+libpcre(?:16|32)?-', imports))
        assert has_pcre_dll == (uses_pcre and not args.pcre_static), ('PCRE linkage', binary, args.pcre_static)

    def fb_flags(backend, directory):
        flags = [compiler, '-i', root/'inc', '-i', root/'examples/regex/PCRE/CRegex',
                 '-p', directory, '-p', pcre_lib, '-p', sdk/'lib', '-gen', backend, '-w', 'all']
        if args.pcre_static:
            flags += ['-d', 'PCRE_STATIC']
        if args.bits == 32:
            flags += ['-target', 'win32', '-prefix', args.runtime_prefix.resolve(), '-buildprefix', str(tools)+'/']
        return flags

    def compile_fb(source, backend, label, directory):
        binary = directory/(label+'.exe')
        run(fb_flags(backend, directory)+[root/source, '-x', binary], directory, label+'-compile')
        machine(binary, uses_pcre=not label.startswith('tre-'))
        return binary

    def abi(data):
        return {line.split()[0]: list(map(int, line.split()[1:]))
                for line in data.decode().splitlines() if line.split() and line.split()[0] in ('PCRE', 'TRE')}

    save()
    spec = importlib.util.spec_from_file_location('private_console', root/'build_scripts/windows-test-console.py')
    console = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(console)
    native = work/'abi-c.exe'
    flags = [tools/'gcc.exe', '-Wall', '-Wextra', '-Werror', '-I'+str(header.parent),
             '-I'+str(sdk/'include'), root/'tests/bindings/regex-abi.c', '-L'+str(pcre_lib),
             '-L'+str(sdk/'lib'), '-lpcre', '-lpcre16', '-lpcre32', '-ltre', '-o', native]
    if args.pcre_static:
        flags += ['-DPCRE_STATIC']
    run(flags, work, 'c-compile')
    machine(native)
    reference = run([native], work, 'c-abi')
    assert b'PCRE_NATIVE_WIDTHS=PASS 8 16 32' in reference
    receipt['native_reference'] = reference.decode().strip()
    for backend in ('gcc', 'gas' if args.bits == 32 else 'gas64'):
        directory = work/backend
        directory.mkdir(exist_ok=True)
        run(fb_flags(backend, directory)+['-lib', root/'examples/regex/PCRE/CRegex/CRegex.bas',
            '-x', directory/'libCRegex.a'], directory, 'cregex-library')
        binary = compile_fb('tests/bindings/regex-abi.bas', backend, 'abi', directory)
        actual = run([binary], directory, 'abi-run')
        assert abi(actual) == abi(reference), (actual, reference)
        assert b'PCRE_NATIVE_WIDTHS=PASS 8 16 32' in actual
        record(backend, 'ABI', 'C layouts and actual 8/16/32 allocator exports', values=abi(actual))
        binary = compile_fb('tests/bindings/regex-runtime.bas', backend, 'runtime', directory)
        actual = run([binary], directory, 'runtime-run')
        assert b'REGEX_RUNTIME=PASS cycles=30 allocator_balance=0 TRE_lifetimes=10000' in actual
        record(backend, 'wrappers', 'bounds, captures, empty matches, owned subjects and cleanup',
               actual=actual.decode().strip())
        for typename in ('CRegex', 'preg_t'):
            for operation in ('copy', 'assignment'):
                source = directory/(typename+'-'+operation+'.bas')
                includes = '#include once "CRegex.bi"\n' if typename == 'CRegex' else (
                    '#define REGEX_EXAMPLE_NO_MAIN\n#include once "'+str(root/'examples/regex/PCRE/preg_class.bas')+'"\n')
                initialization = ' = CRegex("a")' if typename == 'CRegex' else ''
                text = includes+'dim first as '+typename+initialization+'\n'
                text += ('dim second as '+typename+' = first\n' if operation == 'copy' else
                         'dim second as '+typename+initialization+'\nsecond = first\n')
                source.write_text(text)
                result = subprocess.run(list(map(str, fb_flags(backend, directory)+['-r', source])),
                                        cwd=directory, env=env, capture_output=True, timeout=30)
                diagnostic = result.stdout+result.stderr
                (directory/(source.stem+'.log')).write_bytes(diagnostic)
                assert result.returncode != 0 and any(message in diagnostic.lower() for message in
                    (b'has no public access', b'illegal member access')), diagnostic
                record(backend, typename, 'reject '+operation, diagnostic=diagnostic.decode().strip())
        for name, source in examples.items():
            binary = compile_fb(source, backend, name, directory)
            if name == 'tre-match':
                outcome = console.run_console([binary], directory, env, timeout=10)
                assert outcome['exit'] == 0
                screen = outcome['output']
                assert all(token in screen for token in ('<foo>', '<_bar>', '<foo123>', '<BAR>', '<Foo__>'))
                record(backend, name, 'original real console and final SLEEP', **outcome)
                continue
            if name == 'cregex':
                shutil.copyfile(root/'examples/regex/PCRE/CRegex/test.txt', directory/'test.txt')
            actual = run([binary], directory, name+'-run')
            normalized = actual.replace(b'\r\n', b'\n')
            if name == 'pcredemo':
                assert b'0: foo' in normalized and b'rc = 1' in normalized
            elif name == 'cregex':
                assert normalized.count(b'Symbol found:') == 6
                assert normalized.count(b'Number found:') == 3
                assert normalized.count(b'Operator found:') == 8
            elif name == 'tre-replace':
                assert normalized.strip() == b'Hi *you* strange *user* :D'
            elif name == 'tre-subexpr':
                assert normalized.count(b'Full:') == 13 and b'name.ext1.ext2' in normalized
            else:
                assert b'match:foobar' in normalized and b'foo--//$-//bar//--foo' in normalized
            record(backend, name, 'original example output', stdout=actual.decode())
            if name == 'pcredemo':
                for label, pattern, subject, expected, marker in (
                    ('invalid', '(', 'abc', 1, b'compilation failed'),
                    ('no-match', 'z+', 'abc', 0, b'no match'),
                    ('unset', '(a)?(b)(c)?(d)', 'bd', 0, b'<unset>'),
                ):
                    output = run([binary, pattern, subject], directory, name+'-'+label, expected)
                    assert marker in output
                    record(backend, name, label, exit=expected, stdout=output.decode())
    assert all(fingerprint(root/name) == value for name, value in receipt['source_sha256'].items())
    assert all(fingerprint(Path(name)) == value for name, value in receipt['sdk_sha256'].items())
    receipt['status'] = 'passed'
    save()
    print('NATIVE_REGEX=PASS bits='+str(args.bits)+' static='+str(args.pcre_static)+' records='+str(len(records)))


if __name__ == '__main__':
    main()
# end of test-regex.py
