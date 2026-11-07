"""Project: FreeBASIC C backend regression tests
File: reserved-abi-check.py
Purpose: Check C identifier encoding and external object names.
Responsibilities: Exercise mixed backends, CRT names and compiler intrinsics.
This script does not install a compiler or suppress C diagnostics.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = next(path for path in HERE.parents if (path / 'mk/compiler-sources.mk').is_file())


def run(command, cwd, environment):
    result = subprocess.run([str(part) for part in command], cwd=cwd, env=environment,
                            capture_output=True, text=True, errors='replace', timeout=120)
    if result.returncode:
        raise RuntimeError('command failed: ' + ' '.join(str(part) for part in command) +
                           '\n' + result.stdout + result.stderr)
    return result.stdout + result.stderr


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fbc', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--gcc', required=True, type=Path)
    parser.add_argument('--clang', required=True, type=Path)
    parser.add_argument('--gas64', action='store_true')
    parser.add_argument('--win32-gcc', action='store_true', help='also check GCC Win32 object generation')
    parser.add_argument('--wasm', action='store_true', help='also check JavaScript-target WebAssembly output')
    args = parser.parse_args()
    compiler, prefix = args.fbc.resolve(), args.prefix.resolve()
    gcc, clang = args.gcc.resolve(), args.clang.resolve()
    environment = os.environ.copy()
    environment['PATH'] = str(gcc.parent) + os.pathsep + environment.get('PATH', '')
    source = HERE / 'reserved-abi.bas'
    provider = HERE / 'reserved-abi-provider.bas'
    helper = HERE / 'reserved-abi-helper.c'
    marker = 'RESERVED_ABI_TEST_PASS'

    with tempfile.TemporaryDirectory(prefix='fbc-reserved-abi-') as directory:
        work = Path(directory)

        def invoke(command):
            return run(command, work, environment)

        def emit(backend, path, options=()):
            invoke([compiler, '-prefix', prefix, '-i', ROOT / 'inc', '-m', source.stem, '-gen', backend,
                    *options, '-r', source, '-o', path])

        def link(backend, path, *objects):
            invoke([compiler, '-prefix', prefix, '-gen', backend, '-x', path, *objects])
            if marker not in invoke([path]):
                raise RuntimeError('identifier fixture returned an unexpected result')

        helper_objects = {}
        for name, cc in (('gcc', gcc), ('clang', clang)):
            for variant, defines in (('standalone', ()),
                                     ('external', ('-DFB_TEST_EXTERNAL_PROVIDER',)),
                                     ('native', ('-DFB_TEST_EXTERNAL_PROVIDER',
                                                 '-DFB_TEST_EMULATE_BUILTIN'))):
                obj = work / (name + '-' + variant + '.o')
                invoke([cc, '-std=gnu23', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                        *defines, '-c', helper, '-o', obj])
                helper_objects[(name, variant)] = obj

        main_objects = {}
        for backend in ('gcc', 'clang'):
            c_file = work / (backend + '.c')
            emit(backend, c_file)
            generated = c_file.read_text(encoding='utf-8')
            if '__builtin_popcount(' not in generated or '_fb_test_external_add(' not in generated:
                raise RuntimeError('compiler intrinsic or unknown external function was renamed')
            if '__asm__("__fb_test_external_value")' not in generated:
                raise RuntimeError('the external data object name was changed')
            if re.search(r'\b__PRIVATE_VALUE\b', generated):
                raise RuntimeError('a reserved structure field was not encoded')
            if sys.platform == 'win32':
                for name in ('_strtoi64', '_strtoui64', '_wcstoi64', '_wcstoui64'):
                    pattern = r'\bfb_c_sym_\w+\([^;\n]*\)\s+__asm__\("' + name + r'"\)'
                    if not re.search(pattern, generated):
                        raise RuntimeError('the CRT object name was not preserved: ' + name)
            for name, cc in (('gcc', gcc), ('clang', clang)):
                for standard in ('gnu11', 'gnu23'):
                    obj = work / (backend + '-' + name + '-' + standard + '.o')
                    warnings = (['-Weverything', '-ferror-limit=0'] if name == 'clang'
                                else ['-Wall', '-Wextra', '-Wpedantic'])
                    output = invoke([cc, '-std=' + standard, '-nostdinc', '-O2',
                                     *warnings, '-c', c_file, '-o', obj])
                    if name == 'clang':
                        reserved = [line for line in output.splitlines()
                                    if '[-Wreserved-identifier]' in line]
                        if len(reserved) != 1 or '_fb_test_external_add' not in reserved[0]:
                            raise RuntimeError('unexpected reserved-identifier diagnostics:\n' + output)
                    exe = work / (backend + '-' + name + '-' + standard + '.exe')
                    link(backend, exe, obj, helper_objects[(name, 'standalone')])
                    main_objects[(backend, name, standard)] = obj

        if args.gas64:
            gas_main = work / 'gas-main.o'
            gas_provider = work / 'gas-provider.o'
            invoke([compiler, '-prefix', prefix, '-m', source.stem, '-gen', 'gas64', '-c', source, '-o', gas_main])
            invoke([compiler, '-prefix', prefix, '-gen', 'gas64', '-c', provider, '-o', gas_provider])
            for backend in ('gcc', 'clang'):
                c_provider = work / (backend + '-provider.o')
                invoke([compiler, '-prefix', prefix, '-gen', backend, '-c', provider, '-o', c_provider])
                link(backend, work / (backend + '-gas-provider.exe'),
                     main_objects[(backend, 'gcc', 'gnu23')], gas_provider,
                     helper_objects[('gcc', 'external')])
                link('gas64', work / ('gas-' + backend + '-provider.exe'), gas_main,
                     c_provider, helper_objects[('gcc', 'native')])

        if sys.platform == 'win32':
            # Address-only references qualify ordinary CRT spellings without
            # calling APIs which change the process state or produce sound.
            crt_names = ('_access', '_strdup', '_stricmp', '_strnicmp', '_errno',
                         '_i64toa', '_itoa', '_ultoa', '_ui64toa', '_itow',
                         '_ultow', '_i64tow', '_ui64tow', '_chdir', '_putenv',
                         '_rmdir', '_beep')
            crt_c, crt_obj = work / 'crt-names.c', work / 'crt-names.o'
            for target in ('win64', 'win32'):
                names = (*crt_names, '__acrt_iob_func') if target == 'win64' else crt_names
                for backend in ('gcc', 'clang'):
                    invoke([compiler, '-prefix', prefix, '-i', ROOT / 'inc',
                            '-target', target, '-gen', backend, '-r',
                            HERE / 'crt-name-addresses.bas', '-o', crt_c])
                    generated = crt_c.read_text(encoding='utf-8')
                    for name in names:
                        symbol = '_' + name if target == 'win32' else name
                        pattern = r'\bfb_c_sym_\w+\([^;\n]*\)\s+__asm__\("' + re.escape(symbol) + r'"\)'
                        if not re.search(pattern, generated):
                            raise RuntimeError('ordinary CRT name lost its assembler alias: ' + name)
                    for cc_name, cc in (('gcc', gcc), ('clang', clang)):
                        flags = ['-Weverything', '-ferror-limit=0'] if cc_name == 'clang' else ['-Wall', '-Wextra', '-Wpedantic']
                        for standard in ('gnu11', 'gnu23'):
                            output = invoke([cc, '-m32' if target == 'win32' else '-m64',
                                             '-std=' + standard, '-nostdinc', '-O2', *flags,
                                             '-Werror=incompatible-pointer-types', '-c', crt_c, '-o', crt_obj])
                            if '[-Wreserved-identifier]' in output:
                                raise RuntimeError('ordinary CRT name warning returned:\n' + output)
                            symbols = invoke([gcc.parent / 'nm.exe', '-u', crt_obj])
                            for name in names:
                                symbol = '_' + name if target == 'win32' else name
                                if not re.search(r'\bU\s+' + re.escape(symbol) + r'\s*$', symbols, re.M):
                                    raise RuntimeError('ordinary CRT linker name changed: ' + symbol)

            exe = work / 'clang-intrinsics.exe'
            invoke([compiler, '-prefix', prefix, '-gen', 'clang', '-x', exe,
                    HERE / 'intrinsic-probe.bas'])
            if 'INTRINSIC_PROBE_PASS' not in invoke([exe]):
                raise RuntimeError('Clang intrinsic fixture failed')

            # The Windows GCC TLS relocation uses the original C token.
            # Exercise both definitions and external references in real threads.
            tls_objects = {}
            for backend in ('gcc', 'clang'):
                for part in ('provider', 'consumer'):
                    path = HERE / ('tls-reserved-' + part + '.bas')
                    obj = work / ('tls-' + backend + '-' + part + '.o')
                    command = [compiler, '-prefix', prefix, '-mt', '-gen', backend,
                               '-Wc', '-std=gnu23', '-c', path, '-o', obj]
                    if part == 'consumer':
                        command[1:1] = ['-m', path.stem]
                    invoke(command)
                    tls_objects[(backend, part)] = obj
            for consumer in ('gcc', 'clang'):
                for provider_name in ('gcc', 'clang'):
                    exe = work / ('tls-' + consumer + '-' + provider_name + '.exe')
                    invoke([compiler, '-prefix', prefix, '-mt', '-gen', consumer, '-x', exe,
                            tls_objects[(consumer, 'consumer')], tls_objects[(provider_name, 'provider')]])
                    if 'RESERVED_TLS_TEST_PASS' not in invoke([exe]):
                        raise RuntimeError('reserved thread-local name failed across modules')

            tls_c = work / 'tls.c'
            invoke([compiler, '-prefix', prefix, '-i', ROOT / 'inc', '-mt',
                    '-m', 'tls-reserved-consumer', '-gen', 'gcc', '-r',
                    HERE / 'tls-reserved-consumer.bas', '-o', tls_c])
            tls_text = tls_c.read_text(encoding='utf-8')
            if '__asm__("__fb_test_tls_value")' in tls_text:
                raise RuntimeError('a Windows TLS declaration received an asm label')
            invoke([gcc, '-std=gnu23', '-nostdinc', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                    '-c', tls_c, '-o', work / 'tls-direct-gcc.o'])
            diagnostics = invoke([clang, '-std=gnu23', '-nostdinc', '-O2', '-Weverything',
                                  '-ferror-limit=0', '-c', tls_c, '-o', work / 'tls-direct-clang.o'])
            if '__fb_test_tls_value' not in diagnostics or '[-Wreserved-identifier]' not in diagnostics:
                raise RuntimeError('the necessary TLS name warning was not reported')

            c_file, obj = work / 'win32.c', work / 'win32.o'
            emit('clang', c_file, ('-target', 'win32'))
            invoke([clang, '--target=i686-w64-windows-gnu', '-std=gnu23', '-nostdinc',
                    '-O2', '-Weverything', '-ferror-limit=0', '-c', c_file, '-o', obj])
            symbols = invoke([gcc.parent / 'nm.exe', '-u', obj])
            for symbol in ('___fb_test_external_value', '__fb_test_external_add',
                           '__strtoi64', '__strtoui64', '__wcstoi64', '__wcstoui64'):
                if not re.search(r'\bU\s+' + re.escape(symbol) + r'\s*$', symbols, re.MULTILINE):
                    raise RuntimeError('wrong Win32 object name: ' + symbol + '\n' + symbols)

            tls32_c = work / 'tls-win32.c'
            invoke([compiler, '-prefix', prefix, '-i', ROOT / 'inc', '-mt', '-target', 'win32',
                    '-m', 'tls-reserved-consumer', '-gen', 'clang', '-r',
                    HERE / 'tls-reserved-consumer.bas', '-o', tls32_c])
            if '__asm__("__fb_test_tls_value")' in tls32_c.read_text(encoding='utf-8'):
                raise RuntimeError('a Win32 TLS declaration received an asm label')
            cross = [('clang', clang, ['--target=i686-w64-windows-gnu', '-Weverything', '-ferror-limit=0'])]
            if args.win32_gcc:
                cross.append(('gcc', gcc, ['-m32', '-Wall', '-Wextra', '-Wpedantic']))
            for name, cc, flags in cross:
                obj = work / ('tls-win32-' + name + '.o')
                invoke([cc, '-std=gnu23', '-nostdinc', '-O2', *flags, '-c', tls32_c, '-o', obj])
                symbols = invoke([gcc.parent / 'nm.exe', '-u', obj])
                if not re.search(r'\bU\s+___fb_test_tls_value\s*$', symbols, re.MULTILINE):
                    raise RuntimeError('wrong Win32 TLS object name:\n' + symbols)

        # These names are also usable by non-Windows libraries that export
        # them. Check the actual ELF references even when running on Windows.
        linux_c, linux_obj = work / 'linux-crt.c', work / 'linux-crt.o'
        invoke([compiler, '-prefix', prefix, '-i', ROOT / 'inc', '-m', 'crt-alias-linux', '-gen', 'clang',
                '-target', 'linux', '-r', HERE / 'crt-alias-linux.bas', '-o', linux_c])
        invoke([clang, '--target=x86_64-unknown-linux-gnu', '-std=gnu23', '-nostdinc',
                '-O2', '-Weverything', '-ferror-limit=0', '-c', linux_c, '-o', linux_obj])
        nm = gcc.parent / ('nm.exe' if sys.platform == 'win32' else 'nm')
        symbols = invoke([nm, '-u', linux_obj])
        for symbol in ('_strtoi64', '_strtoui64', '_wcstoi64', '_wcstoui64'):
            if not re.search(r'\bU\s+' + re.escape(symbol) + r'\s*$', symbols, re.MULTILINE):
                raise RuntimeError('wrong Linux object name: ' + symbol + '\n' + symbols)
        if sys.platform.startswith('linux'):
            helper_obj = work / 'linux-crt-helper.o'
            invoke([gcc, '-std=gnu23', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-c',
                    HERE / 'crt-alias-linux-helper.c', '-o', helper_obj])
            exe = work / 'linux-crt'
            invoke([compiler, '-prefix', prefix, '-gen', 'clang', '-x', exe, linux_obj, helper_obj])
            if 'CRT_ALIAS_LINUX_PASS' not in invoke([exe]):
                raise RuntimeError('Linux CRT-style aliases failed')

        if args.wasm:
            c_file, obj = work / 'reserved-abi-js.c', work / 'reserved-abi-js.o'
            emit('clang', c_file, ('-target', 'js-asmjs'))
            invoke([clang, '--target=wasm32-unknown-unknown', '-std=gnu23', '-nostdinc',
                    '-O2', '-Weverything', '-ferror-limit=0', '-c', c_file, '-o', obj])

    print('RESERVED_ABI_CHECK_PASS', flush=True)


if __name__ == '__main__':
    main()

# end of reserved-abi-check.py
