#!/usr/bin/env python3
"""FreeBASIC DOS tests: run-dosbox-x.py.

Run a probe in a new isolated DOSBox-X directory with a bounded watchdog.
Optionally boot a disposable FreeDOS floppy (requires pyfatfs). Preserve the
configuration, guest results, emulator log, and optional audio capture. Only
guest completion and every requested PASS marker constitute success.
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import time


def launch(emulator, config, work, timeout, log_name):
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (work / log_name).open('wb') as log:
        process = subprocess.Popen([str(emulator), '-fastlaunch', '-conf', str(config)],
                                   cwd=work, env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            return process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            return 'TIMEOUT'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dosbox', required=True, type=Path)
    parser.add_argument('--cwsdpmi', required=True, type=Path)
    parser.add_argument('--program', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--argument', default='')
    parser.add_argument('--expect', action='append', required=True)
    parser.add_argument('--timeout', type=int, default=30)
    parser.add_argument('--boot-template', type=Path)
    parser.add_argument('--capture', action='store_true')
    parser.add_argument('--speaker-only', action='store_true',
                        help='disable Sound Blaster and require the PC speaker backend')
    args = parser.parse_args()
    if args.timeout <= 0 or args.timeout > 120:
        parser.error('timeout must be between 1 and 120 seconds')
    if args.capture and args.boot_template:
        parser.error('emulator WAV capture requires the built-in DOS shell')
    for path in [args.dosbox, args.cwsdpmi, args.program]:
        if not path.is_file():
            parser.error('Missing input: ' + str(path))
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=False)
    emulator = args.dosbox.resolve()
    config_base = ('[sdl]\nfullscreen=false\noutput=surface\n'
                   '[dosbox]\nmemsize=32\ncaptures=' + work.as_posix() + '\n'
                   '[cpu]\ncore=normal\ncputype=pentium\ncycles=fixed 50000\n'
                   '[midi]\nmididevice=none\n'
                   '[mixer]\nnosound=false\n'
                   '[sblaster]\nsbtype=sb16\nsbbase=220\nirq=7\ndma=1\nhdma=5\n')
    if args.speaker_only:
        config_base = config_base.replace('sbtype=sb16', 'sbtype=none')
        config_base += '[speaker]\npcspeaker=true\n'
    commands = ('@echo off\nset BLASTER=A220 I7 D1 H5 T6\n'
                'ver > HOST.TXT\nPROBE.EXE ' + args.argument + ' > RESULT.TXT\n'
                'if errorlevel 1 echo FAIL guest exit status >> RESULT.TXT\n'
                'echo completed > DONE.TXT\n')
    if args.speaker_only:
        commands = commands.replace('set BLASTER=A220 I7 D1 H5 T6',
                                    'set BLASTER=\nset SFXLIB_DRIVER=PCSpeaker')
    if args.boot_template:
        from pyfatfs.PyFatFS import PyFatFS
        # DOSBox-X supplies a redistributable real-mode APM shutdown utility.
        # Export it through the built-in DOS before booting the separate kernel.
        export = work / 'export.conf'
        export.write_text(config_base + '[autoexec]\nmount c "' + work.as_posix() +
                          '"\ncopy z:\\bin\\shutdown.com c:\\SHUTDOWN.COM\nexit\n')
        if launch(emulator, export, work, 10, 'export.log') != 0 or not (work / 'SHUTDOWN.COM').is_file():
            raise RuntimeError('Could not export the DOSBox-X APM shutdown utility')
        disk = work / 'freedos.img'
        shutil.copyfile(args.boot_template, disk)
        with PyFatFS(str(disk)) as fs:
            fs.writetext('/FDCONFIG.SYS', 'FILES=40\r\nBUFFERS=20\r\n'
                         'SHELL=A:\\FREEDOS\\BIN\\COMMAND.COM A:\\FREEDOS\\BIN /E:2048 /P=A:\\FDAUTO.BAT\r\n')
            fs.writetext('/FDAUTO.BAT', (commands + 'A:\\SHUTDOWN.COM /S\n').replace('\n', '\r\n'))
            for name, source in [('PROBE.EXE', args.program), ('CWSDPMI.EXE', args.cwsdpmi),
                                 ('SHUTDOWN.COM', work / 'SHUTDOWN.COM')]:
                fs.writebytes('/' + name, source.read_bytes())
        autoexec = 'boot "' + disk.as_posix() + '"\n'
    else:
        shutil.copyfile(args.program, work / 'PROBE.EXE')
        shutil.copyfile(args.cwsdpmi, work / 'CWSDPMI.EXE')
        (work / 'RUN.BAT').write_text(commands.replace('\n', '\r\n'))
        autoexec = 'mount c "' + work.as_posix() + '"\nc:\n'
        if args.capture:
            autoexec += 'DX-CAPTURE /A /-D COMMAND.COM /C RUN.BAT\n'
        else:
            autoexec += 'call RUN.BAT\n'
        autoexec += 'exit\n'
    config = work / 'dosbox.conf'
    config.write_text(config_base + '[autoexec]\n' + autoexec)
    started = time.monotonic()
    status = launch(emulator, config, work, args.timeout, 'dosbox.log')
    elapsed = time.monotonic() - started
    if args.boot_template:
        with PyFatFS(str(disk), read_only=True) as fs:
            for entry in fs.listdir('/'):
                if entry.upper().endswith(('.TXT', '.WAV')):
                    (work / entry).write_bytes(fs.readbytes('/' + entry))
    result = (work / 'RESULT.TXT').read_text(errors='replace') if (work / 'RESULT.TXT').exists() else ''
    passed = (status == 0 and (work / 'DONE.TXT').is_file() and 'FAIL' not in result and
              all(marker in result for marker in args.expect))
    (work / 'outcome.txt').write_text(('PASS' if passed else 'FAIL') + ' emulator=' + str(status) +
                                    f' seconds={elapsed:.3f}\n')
    print(result, end='')
    print(('PASS' if passed else 'FAIL') + ': ' + str(work) +
          ' (emulator=' + str(status) + f', seconds={elapsed:.3f})')
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())

# end of run-dosbox-x.py
