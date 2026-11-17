#!/usr/bin/env python3
"""FreeBASIC Wii qualification; wii-run-program.py.

Run a DOL with private Dolphin configuration and a reusable FAT test image.
Require a fresh nonce-bound guest exit receipt; retain bounded guest artifacts.
This runner does not compile programs, select tests or supply firmware assets.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import subprocess
import sys
import time

IMAGE_BYTES = 128 * 1024 * 1024
OWNER = {'tool': 'FreeBASIC Wii qualification', 'image_bytes': IMAGE_BYTES}
RESERVED = {'fbqnonce.txt', 'fbqexit.txt', 'fbqexit.tmp', 'fbqargs.bin', 'fbqprog.dol'}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def own_directory(path: Path) -> None:
    owner = path / 'owner.json'
    if owner.exists():
        if json.loads(owner.read_text()) != OWNER:
            raise ValueError('work directory belongs to another task')
    elif path.exists() and any(path.iterdir()):
        raise ValueError('work directory must be empty or owned by this runner')
    path.mkdir(parents=True, exist_ok=True)
    owner.write_text(json.dumps(OWNER) + '\n')


def stage_image(image: Path, resources: Path | None, nonce: str, program: Path,
                program_arguments: list[str], fat, fatfs) -> str:
    if not image.exists():
        image.touch()
        volume = fat()
        volume.mkfs(str(image), fat_type=16, size=IMAGE_BYTES, label='FBQUAL')
        volume.close()
    if image.stat().st_size != IMAGE_BYTES:
        raise ValueError('unexpected size for owned SD image')
    files = sorted(path for path in resources.rglob('*') if path.is_file()) if resources else []
    # Cluster allocation and long-name entries need space beyond file contents.
    if program.stat().st_size + sum(((path.stat().st_size + 4095) // 4096) * 4096 for path in files) > IMAGE_BYTES * 3 // 4:
        raise ValueError('resource tree is too large for the qualification image')
    identity = hashlib.sha256()
    with fatfs(str(image), preserve_case=True) as filesystem:
        # Clear the virtual filesystem, not the host directory. This resets
        # files created by the preceding guest without rewriting a whole image.
        for name in filesystem.listdir('/'):
            if filesystem.isdir(name):
                filesystem.removetree(name)
            else:
                filesystem.remove(name)
        for path in files:
            relative = path.relative_to(resources).as_posix()
            if path.is_symlink() or relative.casefold() in RESERVED:
                raise ValueError('unsupported or reserved resource: ' + relative)
            parent = path.relative_to(resources).parent.as_posix()
            if parent != '.':
                filesystem.makedirs(parent, recreate=True)
            identity.update(relative.encode('utf-8') + b'\0')
            identity.update(sha256(path).encode('ascii') + b'\0')
            with path.open('rb') as source, filesystem.openbin(relative, 'w') as target:
                shutil.copyfileobj(source, target)
        # A standalone DOL's host pathname is not visible on the guest disk.
        # Supply a real guest argv[0], rather than inventing FileExists output.
        with program.open('rb') as source, filesystem.openbin('FBQPROG.DOL', 'w') as target:
            shutil.copyfileobj(source, target)
        arguments = b'FBQA1' + bytes([len(program_arguments)])
        for argument in program_arguments:
            data = argument.encode('utf-8')
            arguments += len(data).to_bytes(2, 'little') + data
        filesystem.writebytes('FBQARGS.BIN', arguments)
        filesystem.writebytes('FBQNONCE.TXT', nonce.encode('ascii'))
    return identity.hexdigest()


def read_status(image: Path, nonce: str, fatfs) -> int | None:
    with fatfs(str(image), read_only=True) as filesystem:
        if not filesystem.exists('FBQEXIT.TXT'):
            return None
        with filesystem.openbin('FBQEXIT.TXT') as stream:
            data = stream.read(80)
    match = re.fullmatch(rb'FBQUAL ([0-9a-f]{32}) (-?[0-9]{1,10}) DONE\r?\n', data)
    if match is None or match[1].decode('ascii') != nonce:
        return None
    return int(match[2])


def run(args) -> int:
    if args.fat_tools:
        sys.path.insert(0, str(args.fat_tools.resolve(strict=True)))
    from pyfatfs.PyFat import PyFat
    from pyfatfs.PyFatFS import PyFatFS

    emulator = args.dolphin.resolve(strict=True)
    program = args.program.resolve(strict=True)
    resources = args.resources.resolve(strict=True) if args.resources else None
    work = args.workdir.resolve()
    if program.suffix.lower() != '.dol' or args.timeout <= 0 or args.max_output_bytes <= 0:
        raise ValueError('a DOL and positive execution/output bounds are required')
    if resources and not resources.is_dir():
        raise ValueError('resources must be an immutable directory of guest inputs')
    own_directory(work)
    image = work / 'sd.raw'
    nonce = secrets.token_hex(16)
    if (len(args.program_arg) > 32 or any('\0' in arg or len(arg.encode('utf-8')) > 4095 for arg in args.program_arg)
            or sum(len(arg.encode('utf-8')) + 1 for arg in args.program_arg) > 8192):
        raise ValueError('program arguments exceed the guest adapter bounds')
    resource_identity = stage_image(image, resources, nonce, program, args.program_arg, PyFat, PyFatFS)
    user = work / 'dolphin-user'
    config = user / 'Config'
    config.mkdir(parents=True, exist_ok=True)
    (config / 'Dolphin.ini').write_text(
        '[Analytics]\nEnabled = False\nPermissionAsked = True\n'
        '[Interface]\nUsePanicHandlers = False\nConfirmStop = False\n'
        '[Core]\nCPUCore = ' + ('0' if args.core == 'interpreter' else '1') +
        '\nGFXBackend = Null\nWiiSDCard = True\n'
        'WiiSDCardAllowWrites = True\nWiiSDCardEnableFolderSync = False\n'
        '[General]\nWiiSDCardPath = ' + image.as_posix() + '\n')
    exports = work / 'exports'
    exports.mkdir(exist_ok=True)
    for name in args.watch_file:
        if not re.fullmatch(r'[A-Za-z0-9_-]+\.[A-Za-z0-9]+', name):
            raise ValueError('watched artifacts must be filenames at the SD root')
        (exports / name).unlink(missing_ok=True)

    command = [str(emulator), '--batch', '--exec', str(program), '--user', str(user)]
    options = {}
    if os.name == 'nt':
        startup = subprocess.STARTUPINFO()
        startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
        options['startupinfo'] = startup
    log = work / 'dolphin.log'
    started = time.monotonic()
    guest_status = None
    stop_reason = ''
    read_error = ''
    with log.open('wb') as output:
        process = subprocess.Popen(command, stdout=output, stderr=output, **options)
        try:
            while True:
                if os.name != 'nt':
                    try:
                        guest_status = read_status(image, nonce, PyFatFS)
                    except Exception as error:
                        # Metadata can be between guest sector writes.
                        read_error = str(error)[:300]
                if guest_status is not None:
                    try:
                        process.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        # Homebrew can remain in its loader after _exit. Its
                        # fresh exit receipt, not a forced emulator status,
                        # establishes the program's actual result.
                        stop_reason = 'stopped-after-guest-exit'
                    break
                if process.poll() is not None:
                    # Windows requires exclusive access when Dolphin opens
                    # the image. Even a read-only poll can prevent that open
                    # and abort SD initialization. Read only after it exits.
                    guest_status = read_status(image, nonce, PyFatFS)
                    if guest_status is None:
                        stop_reason = 'missing-guest-exit'
                    break
                if time.monotonic() - started > args.timeout:
                    stop_reason = 'timeout'
                    break
                if log.stat().st_size > args.max_output_bytes:
                    stop_reason = 'output-limit'
                    break
                time.sleep(0.25)
        finally:
            if process.poll() is None:
                process.kill()
            process.wait()

    artifacts = {}
    with PyFatFS(str(image), read_only=True) as filesystem:
        for name in args.watch_file:
            if filesystem.exists(name):
                if filesystem.getsize(name) > args.max_output_bytes:
                    stop_reason = 'output-limit'
                    continue
                data = filesystem.readbytes(name)
                (exports / name).write_bytes(data)
                artifacts[name] = hashlib.sha256(data).hexdigest()
    if log.stat().st_size > args.max_output_bytes:
        stop_reason = 'output-limit'
    passed = (guest_status == args.expect_exit
              and stop_reason in ('', 'stopped-after-guest-exit')
              and (process.returncode == 0 or stop_reason == 'stopped-after-guest-exit'))
    receipt = dict(passed=passed, program_sha256=sha256(program),
                   emulator_sha256=sha256(emulator), controller_sha256=sha256(Path(__file__)),
                   nonce=nonce, guest_exit=guest_status, expected_exit=args.expect_exit,
                   core=args.core,
                   program_arguments=args.program_arg,
                   emulator_exit=process.returncode, stop_reason=stop_reason,
                   elapsed_seconds=round(time.monotonic() - started, 3),
                   resources_sha256=resource_identity, artifacts=artifacts,
                   last_sd_read_error=read_error)
    (work / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt), flush=True)
    return 0 if passed else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dolphin', type=Path, required=True)
    parser.add_argument('--program', type=Path, required=True)
    parser.add_argument('--workdir', type=Path, required=True)
    parser.add_argument('--resources', type=Path)
    parser.add_argument('--program-arg', action='append', default=[])
    parser.add_argument('--fat-tools', type=Path)
    # Dolphin's CPUCore values are 0 for the interpreter and 1 for x64 JIT.
    parser.add_argument('--core', choices=('interpreter', 'jit'), default='interpreter')
    parser.add_argument('--watch-file', action='append', default=[])
    parser.add_argument('--expect-exit', type=int, default=0)
    parser.add_argument('--timeout', type=float, default=180)
    parser.add_argument('--max-output-bytes', type=int, default=2 * 1024 * 1024)
    raise SystemExit(run(parser.parse_args()))
# end of wii-run-program.py
