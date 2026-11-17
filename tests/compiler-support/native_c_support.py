"""Project: FreeBASIC native C regression support
File: native_c_support.py
Purpose: Select native C dialects, diagnostic spellings and ABI machine modes.
Responsibilities: Probe capabilities without assuming that a command named gcc
is GNU GCC, match BASIC pointer widths and assembler identities, and keep
GNU LTO checks separate. Honor native options published in emitted C.
This file intentionally does NOT suppress warnings or weaken language modes.
"""
from functools import lru_cache
import os
from pathlib import Path
import re
import shutil
import subprocess


def native_path(filename):
    if os.name != 'nt' and re.match(r'^[A-Za-z]:[\\/]', filename):
        filename = subprocess.check_output(['cygpath', '-u', filename], text=True).strip()
    return Path(filename)


def resolve_executable(command):
    # An environment variable such as GCC can contain a native drive path
    # even when shutil/pathlib are running inside MSYS. Normalize before both
    # executable discovery and strict resolution so hashes use the real file.
    resolved = shutil.which(str(native_path(str(command))))
    if resolved is None:
        raise FileNotFoundError('Cannot resolve executable: ' + str(command))
    return native_path(resolved).resolve(strict=True)


@lru_cache(maxsize=None)
def native_machine_flags(fbc):
    result = subprocess.run([str(fbc), '-print', 'target'], text=True,
                            capture_output=True, timeout=30, check=True)
    target = result.stdout.strip()
    # MSYS may expose a 64-bit Clang alongside a 32-bit BASIC compiler.
    # ARM targets use their native toolchain without x86 machine switches.
    if target in ('win32', 'win32-x86', 'linux-x86'):
        return ('-m32',)
    if target in ('win64', 'win64-x86_64', 'linux-x86_64'):
        return ('-m64',)
    return ()


@lru_cache(maxsize=None)
def is_gnu_compiler(compiler):
    result = subprocess.run([str(compiler), '-dM', '-E', '-x', 'c', '-'],
                            input='', text=True, capture_output=True, timeout=30, check=False)
    if result.returncode:
        raise RuntimeError('Cannot query native C compiler: ' + str(compiler) + '\n' + result.stderr)
    return '__GNUC__' in result.stdout and '__clang__' not in result.stdout


@lru_cache(maxsize=None)
def c23_standard(compiler):
    # GCC 12/13 and older Clang call the same evolving C23 mode gnu2x.
    for standard in ('gnu23', 'gnu2x'):
        result = subprocess.run([str(compiler), '-std=' + standard, '-x', 'c', '-fsyntax-only', '-'],
            input='_Static_assert(__STDC_VERSION__ >= 202000L, "C23 mode required");\n',
            text=True, capture_output=True, timeout=30, check=False)
        if result.returncode == 0:
            return standard
    raise RuntimeError('Native C compiler has no supported C23 mode: ' + str(compiler))


def native_source_flags(source):
    # The C emitter and normal driver share this stack contract. Read bounded
    # line fragments so a large generated initializer needs no extra full copy.
    marker = b'/* Compile x86-64 inline ASM with -mno-red-zone. */'
    continued = False
    with native_path(str(source)).open('rb') as stream:
        while True:
            line = stream.readline(65536)
            if not line:
                return ()
            if not continued and line.strip() == marker:
                return ('-mno-red-zone',)
            continued = not line.endswith(b'\n')


def native_command(command):
    command = list(map(str, command))
    source_flags = []
    compiling = any(flag.startswith('-std=') or flag in ('-c', '-S', '-E', '-fsyntax-only') for flag in command)
    if compiling:
        for index, argument in enumerate(command[1:], 1):
            if argument.startswith('-') or command[index - 1] == '-o' or Path(argument).suffix.lower() != '.c':
                continue
            source = native_path(argument)
            if source.is_file():
                for flag in native_source_flags(source):
                    if flag not in source_flags and flag not in command:
                        source_flags.append(flag)
    # Explicit caller options retain their normal last-option precedence.
    command = command[:1] + source_flags + command[1:]
    if '-std=gnu23' in command:
        command = ['-std=' + c23_standard(command[0]) if flag == '-std=gnu23' else flag
                   for flag in command]
    if any(flag in command for flag in ('-Wcast-align=strict', '-Warray-bounds=2')):
        if not is_gnu_compiler(command[0]):
            equivalent = {'-Wcast-align=strict': '-Wcast-align', '-Warray-bounds=2': '-Warray-bounds'}
            command = [equivalent.get(flag, flag) for flag in command]
    return command


def has_assembly_function(generated, assembly, name):
    # FBC can give a C identifier an explicit assembler identity. In
    # particular, Win32 stdcall adds both an underscore and an argument-byte
    # suffix. Inspect that identity, not debug strings containing the name.
    alias = re.search(r'\b' + re.escape(name) +
                      r'\([^;\n]*\)\s+__asm__\("([^"\n]+)"\);', generated)
    label = re.escape(alias.group(1)) if alias else '_?' + re.escape(name)
    return re.search(r'^\s*(?:' + label + '|"' + label + r'"):\s*(?:[#;].*)?$',
                     assembly, re.MULTILINE) is not None


# end of native_c_support.py
