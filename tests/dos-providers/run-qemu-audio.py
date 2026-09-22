#!/usr/bin/env python3
"""FreeBASIC DOS audio tests: run-qemu-audio.py.

Boot an isolated FreeDOS floppy with explicit emulated audio devices, capture
hardware output, and require guest completion and expected result markers.
Does not mount host disks, install software, or replace an existing test run.
Requires pyfatfs and an existing FreeDOS boot image with FreeCOM.
"""

import argparse
import json
from pathlib import Path
import queue
import shutil
import subprocess
import threading
import time

from pyfatfs.PyFatFS import PyFatFS


def run_machine(command, work, devices, timeout):
    """Capture through QEMU's monitor and close capture before quitting.

    QEMU 11.0.3 leaves its WAV backend length fields zero even after APM
    shutdown. The monitor capture has an explicit stop operation that writes
    the header. It observes the same emulated hardware output, without any
    changes to the recorded PCM or the guest driver.
    """
    messages = queue.Queue()
    events = []
    serial = 0
    shutdown = False
    error = ''
    with (work / 'qemu.log').open('w') as errors, (work / 'monitor.log').open('w') as log:
        process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=errors, text=True, encoding='utf-8')

        def reader():
            for line in process.stdout:
                log.write(line)
                log.flush()
                messages.put(json.loads(line))
            messages.put(None)

        thread = threading.Thread(target=reader, daemon=True)
        thread.start()

        def receive(wait):
            reply = messages.get(timeout=max(0.01, wait))
            if reply is None:
                raise RuntimeError('QEMU closed its control channel')
            if 'event' in reply:
                events.append(reply['event'])
            return reply

        def request(name, arguments=None):
            nonlocal serial
            serial += 1
            message = {'execute': name, 'id': serial}
            if arguments:
                message['arguments'] = arguments
            process.stdin.write(json.dumps(message) + '\n')
            process.stdin.flush()
            limit = time.monotonic() + 5
            while True:
                reply = receive(limit - time.monotonic())
                if reply.get('id') == serial:
                    if 'error' in reply:
                        raise RuntimeError(str(reply['error']))
                    return reply.get('return')

        def monitor(line):
            return request('human-monitor-command', {'command-line': line})

        try:
            if 'QMP' not in receive(5):
                raise RuntimeError('Missing QMP greeting')
            request('qmp_capabilities')
            for index, device in enumerate(devices):
                path = (work / (device + '.wav')).as_posix()
                reply = monitor(f'wavcapture "{path}" audio{index} 48000 16 2')
                if reply and any(not line.startswith("warning: 'wavcapture' is deprecated")
                                 for line in reply.splitlines() if line):
                    raise RuntimeError('Cannot start hardware capture: ' + reply)
            request('cont')
            deadline = time.monotonic() + timeout
            while 'SHUTDOWN' not in events:
                receive(deadline - time.monotonic())
            shutdown = True
            for _ in devices:
                # Removing index zero shifts the next capture into its place.
                monitor('stopcapture 0')
            request('quit')
        except (queue.Empty, RuntimeError, OSError) as exc:
            error = str(exc) or 'Timed out waiting for guest shutdown'
            try:
                monitor('stop')
                for query in ['info registers', 'info pci', 'info pic']:
                    log.write(str(monitor(query)) + '\n')
                request('quit')
            except (queue.Empty, RuntimeError, OSError):
                pass
        finally:
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            thread.join(timeout=5)
            process.stdin.close()
            process.stdout.close()
    return process.returncode, shutdown and not error, error


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qemu', required=True, type=Path)
    parser.add_argument('--boot-template', required=True, type=Path)
    parser.add_argument('--cwsdpmi', required=True, type=Path)
    parser.add_argument('--program', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--devices', choices=['none', 'ac97', 'both', 'sb', 'hda', 'es1370'], default='ac97')
    parser.add_argument('--blaster', choices=['unset', 'valid', 'invalid'], default='unset')
    parser.add_argument('--argument', default='direct')
    parser.add_argument('--expect', action='append', required=True)
    parser.add_argument('--timeout', type=int, default=90)
    args = parser.parse_args()
    if not 1 <= args.timeout <= 120:
        parser.error('timeout must be between 1 and 120 seconds')
    if not args.argument.isalnum():
        parser.error('argument must be a single alphanumeric probe mode')
    for path in [args.qemu, args.boot_template, args.cwsdpmi, args.program]:
        if not path.is_file():
            parser.error('missing file: ' + str(path))
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=False)
    disk = work / 'boot.img'
    shutil.copyfile(args.boot_template, disk)
    blaster = {'unset': '', 'valid': 'A220 I5 D1 H5 T6',
               'invalid': 'A240 I5 D1 H5 T6'}[args.blaster]
    with PyFatFS(str(disk)) as fs:
        fs.writetext('/FDCONFIG.SYS',
                     'FILES=40\r\nBUFFERS=20\r\n'
                     'SHELL=A:\\FREEDOS\\BIN\\COMMAND.COM A:\\FREEDOS\\BIN /E:2048 /P=A:\\FDAUTO.BAT\r\n')
        fs.writetext('/FDAUTO.BAT',
                     '@echo off\r\nPATH A:\\;A:\\FREEDOS\\BIN\r\n'
                     'SET BLASTER=' + blaster + '\r\nSET SFXLIB_DRIVER=\r\n'
                     'VER /R > A:\\HOST.TXT\r\n'
                     'A:\\PROBE.EXE ' + args.argument + ' > A:\\RESULT.TXT\r\n'
                     'IF ERRORLEVEL 1 GOTO FAILED\r\n'
                     'ECHO DONE > A:\\DONE.TXT\r\nGOTO FINISH\r\n'
                     ':FAILED\r\nECHO FAIL > A:\\FAIL.TXT\r\n'
                     ':FINISH\r\nA:\\DONE.COM\r\n')
        # Connect to the real-mode APM BIOS, select version 1.2 and power off
        # all devices (INT 15h/5307h, BX=1, CX=3). The host sees a SHUTDOWN
        # event, stops its capture and then quits. isa-debug-exit is only a
        # failing escape if the BIOS cannot power off.
        fs.writebytes('/DONE.COM', bytes.fromhex(
            'B8 01 53 31 DB CD 15 B8 0E 53 31 DB B9 02 01 CD 15 '
            'B8 07 53 BB 01 00 B9 03 00 CD 15 BA F4 00 B8 10 00 EF EB FE'))
        fs.writebytes('/CWSDPMI.EXE', args.cwsdpmi.read_bytes())
        fs.writebytes('/PROBE.EXE', args.program.read_bytes())
    command = [str(args.qemu), '-machine', 'pc', '-cpu', 'qemu32', '-accel', 'tcg',
               '-m', '32', '-smp', '1', '-nic', 'none', '-display', 'none',
               '-serial', 'none', '-monitor', 'none', '-qmp', 'stdio', '-S',
               '-no-shutdown', '-no-reboot', '-boot', 'a',
               '-drive', 'file=' + disk.as_posix() + ',format=raw,if=floppy',
               '-device', 'isa-debug-exit,iobase=0xf4,iosize=0x04']
    devices = {'none': [], 'ac97': ['AC97'], 'both': ['AC97', 'sb16'],
               'sb': ['sb16'], 'hda': ['hda-duplex'], 'es1370': ['ES1370']}[args.devices]
    if args.devices == 'hda':
        command += ['-device', 'intel-hda']
    for index, device in enumerate(devices):
        name = 'audio' + str(index)
        command += ['-audiodev', 'none,id=' + name + ',out.frequency=48000',
                    '-device', device + ',audiodev=' + name]
    (work / 'command.json').write_text(json.dumps(command, indent=2))
    start = time.monotonic()
    returncode, shutdown, error = run_machine(command, work, devices, args.timeout)
    with PyFatFS(str(disk), read_only=True) as fs:
        for name in fs.listdir('/'):
            if name.upper().endswith(('.TXT', '.WAV')):
                (work / name.upper()).write_bytes(fs.readbytes('/' + name))
    result = (work / 'RESULT.TXT').read_text(errors='replace') if (work / 'RESULT.TXT').exists() else ''
    passed = (shutdown and returncode == 0 and
              (work / 'DONE.TXT').exists() and not (work / 'FAIL.TXT').exists() and
              'FAIL' not in result and all(marker in result for marker in args.expect))
    outcome = {'passed': passed, 'shutdown': shutdown, 'error': error, 'returncode': returncode,
               'elapsed_seconds': round(time.monotonic() - start, 3),
               'devices': args.devices, 'blaster': args.blaster}
    (work / 'outcome.json').write_text(json.dumps(outcome, indent=2))
    print(result, end='')
    print(json.dumps(outcome))
    raise SystemExit(0 if passed else 1)


if __name__ == '__main__':
    main()

# end of run-qemu-audio.py
