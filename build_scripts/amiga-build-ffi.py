#!/usr/bin/env python3
"""
FreeBASIC classic AmigaOS foreign-call provider
----------------------------------------------

File: amiga-build-ffi.py

Purpose:
    Build an immutable libffi 3.4.8 source revision for the Amiga Hunk ABI.

Responsibilities:
    - verify and safely extract the upstream source archive
    - select Amiga symbol names, pointer returns, and native cache maintenance
    - remove ELF-only assembler metadata and publish headers with the archive

This file intentionally does NOT contain:
    - FreeBASIC argument packing or a new foreign-call implementation
"""

import argparse
import hashlib
from pathlib import Path, PurePosixPath
import os
import shutil
import subprocess
import tarfile
import urllib.request


REVISION = '6a99edb8082f75e523e0d6ebaba42218b80e10c8'
SOURCE_URL = f'https://codeload.github.com/libffi/libffi/tar.gz/{REVISION}'
SOURCE_SHA256 = 'f2fea2db7b517f60ed64aadd06827fb047a0fe14e31acf4493835d11aa9a04c0'


def replace_once(source, old, new):
    if source.count(old) != 1:
        raise SystemExit('Unsupported libffi source at patch: ' + old[:80])
    return source.replace(old, new, 1)


def patch_sources(source):
    path = source / 'src/m68k/ffi.c'
    text = path.read_text()
    text = replace_once(text, '#ifdef __rtems__\nvoid rtems_cache_flush_multiple_data_lines',
                        '#ifdef __amigaos__\n#include <proto/exec.h>\n#include <exec/execbase.h>\n'
                        '#elif defined(__rtems__)\nvoid rtems_cache_flush_multiple_data_lines')
    text = replace_once(text, '#ifdef __rtems__\n  rtems_cache_flush_multiple_data_lines',
                        '#ifdef __amigaos__\n  CacheClearE(codeloc, FFI_TRAMPOLINE_SIZE, CACRF_ClearI | CACRF_ClearD);\n'
                        '#elif defined(__rtems__)\n  rtems_cache_flush_multiple_data_lines')
    path.write_text(text)
    path = source / 'src/m68k/sysv.S'
    text = path.read_text()
    for suffix in ('#define CALLFUNC(funcname)', '\tmove.l\t%d0,%a1', '\tmove.l\t%d0,(%a1)'):
        text = replace_once(text, '#ifdef __MINT__\n' + suffix,
                            '#if defined(__MINT__) || defined(__amigaos__)\n' + suffix)
    text = '\n'.join(line for line in text.splitlines()
                     if not line.lstrip().startswith(('.type', '.size', '.section .note.GNU-stack'))) + '\n'
    path.write_text(text)


def build(prefix, work, output, jobs):
    work.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    archive = work / 'source.tar.gz'
    if not archive.exists():
        with urllib.request.urlopen(SOURCE_URL, timeout=30) as response:
            data = response.read()
        if hashlib.sha256(data).hexdigest() != SOURCE_SHA256:
            raise SystemExit('libffi source hash mismatch')
        archive.write_bytes(data)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise SystemExit('Cached libffi source hash mismatch')

    recipe = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    stamp = work / 'built-recipe.txt'
    source, build_directory = work / 'source', work / 'build'
    if not stamp.is_file() or stamp.read_text().strip() != recipe:
        for directory in (source, build_directory):
            if directory.exists(): shutil.rmtree(directory)
            directory.mkdir()
        with tarfile.open(archive) as bundle:
            for member in bundle:
                parts = PurePosixPath(member.name).parts
                if not parts or parts[0] != 'libffi-' + REVISION or '..' in parts:
                    raise SystemExit('Invalid libffi source member')
                destination = source.joinpath(*parts[1:])
                if member.isdir(): destination.mkdir(parents=True, exist_ok=True)
                elif member.isfile():
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    with bundle.extractfile(member) as stream:
                        destination.write_bytes(stream.read())
                    destination.chmod(member.mode & 0o777)
                else: raise SystemExit('Unexpected libffi archive member type')
        patch_sources(source)
        env = os.environ.copy()
        temporary = work / 'tmp'
        temporary.mkdir(exist_ok=True)
        env['TMPDIR'] = str(temporary)
        env['PATH'] = str(prefix / 'bin') + os.pathsep + env['PATH']
        env['CC'] = str(prefix / 'bin/m68k-amigaos-gcc')
        env['AR'] = str(prefix / 'bin/m68k-amigaos-ar')
        env['RANLIB'] = str(prefix / 'bin/m68k-amigaos-ranlib')
        env['CFLAGS'] = '-m68020 -msoft-float -O2 -fno-strict-aliasing'
        env['ac_cv_func_mmap'] = 'no'
        subprocess.run(['autoreconf', '-fi'], cwd=source, env=env, check=True)
        subprocess.run([str(source / 'configure'), '--host=m68k-amigaos', '--disable-shared',
                        '--disable-docs', '--disable-multi-os-directory'], cwd=build_directory,
                       env=env, check=True)
        configuration = build_directory / 'fficonfig.h'
        configuration.write_text(configuration.read_text().replace(
            '#define HAVE_AS_CFI_PSEUDO_OP 1', '/* Hunk objects have no ELF CFI sections. */'))
        subprocess.run(['make', '-s', '-j' + str(jobs)], cwd=build_directory, env=env, check=True)
        stamp.write_text(recipe + '\n')
    temporary = output.with_suffix('.a.new')
    shutil.copy2(build_directory / '.libs/libffi.a', temporary)
    temporary.replace(output)
    headers = output.parent / 'include'
    headers.mkdir(exist_ok=True)
    for name in ('ffi.h', 'ffitarget.h'):
        shutil.copy2(build_directory / 'include' / name, headers / name)
    shutil.copy2(source / 'LICENSE', output.parent / 'libffi-license.txt')
    print('Built Amiga libffi provider:', output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--toolchain-root', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=8)
    args = parser.parse_args()
    if args.jobs <= 0: parser.error('Job count must be positive')
    build(args.toolchain_root.resolve(), args.work.resolve(), args.output.resolve(), args.jobs)

# end of amiga-build-ffi.py
