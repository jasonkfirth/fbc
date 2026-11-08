"""Project: FreeBASIC native C regression support
File: native_c_support.py
Purpose: Select equivalent C dialect and diagnostic spellings for native tools.
Responsibilities: Probe capabilities without assuming that a command named gcc
is GNU GCC, and keep GCC-only LTO checks separate from native ABI value checks.
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


def native_command(command):
    command = list(map(str, command))
    if '-std=gnu23' in command:
        command = ['-std=' + c23_standard(command[0]) if flag == '-std=gnu23' else flag
                   for flag in command]
    if any(flag in command for flag in ('-Wcast-align=strict', '-Warray-bounds=2')):
        if not is_gnu_compiler(command[0]):
            equivalent = {'-Wcast-align=strict': '-Wcast-align', '-Warray-bounds=2': '-Warray-bounds'}
            command = [equivalent.get(flag, flag) for flag in command]
    return command

# end of native_c_support.py
