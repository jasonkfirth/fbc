"""FreeBASIC graphics tests; File: test-x11-drop.py.
Purpose: Exercise both native receivers over a private X server and real Xdnd.
Responsibilities: Positive delivery, protocol refusal, timeout and process cleanup.
This helper does not synthesize private queue entries or claim physical dragging.
"""
import argparse
import json
import os
from pathlib import Path
import select
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    build = args.build.resolve(strict=True)
    output = build / 'protocol'
    assert not output.exists() and (build / '.owned').is_file()
    output.mkdir()
    sender = output / 'x11-drop-source'
    flags = ['-O2', '-Wall', '-Wextra', '-Werror']
    if os.uname().sysname == 'FreeBSD':
        flags += ['-I/usr/local/include', '-L/usr/local/lib']
    subprocess.run([args.cc, *flags, str(ROOT / 'tests/services/x11-drop-source.c'),
                    '-lX11', '-o', str(sender)], check=True, timeout=60)
    xvfb_log = (output / 'xvfb.log').open('wb')
    server = subprocess.Popen([shutil.which('Xvfb'), '-displayfd', '1', '-screen', '0',
                               '800x600x24', '+extension', 'GLX', '-nolisten', 'tcp'],
                              stdout=subprocess.PIPE, stderr=xvfb_log)
    records = []
    try:
        assert select.select([server.stdout], [], [], 10)[0], 'Xvfb did not become ready'
        display = server.stdout.readline().decode().strip()
        assert display.isdigit()
        environment = os.environ.copy()
        environment.update(DISPLAY=':' + display, LIBGL_ALWAYS_SOFTWARE='1')
        groups = [group for group in ('GFX2', 'GFX3') if (build / (group + '-receiver')).is_file()]
        assert groups
        for group in groups:
            for scenario in ('files', 'text', 'extended', 'version3', 'version4',
                             'move-copy', 'remote', 'invalid',
                             'oversize', 'incr', 'unsupported-action', 'version',
                             'source-destroy', 'timeout'):
                argument = 'text' if scenario == 'text' else (
                    '' if scenario in ('files', 'extended', 'version3', 'version4', 'move-copy') else 'refuse')
                command = [str(build / (group + '-receiver')), argument]
                if scenario == 'timeout':
                    command += ['timeout']
                receiver = subprocess.Popen(command, stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE, env=environment)
                try:
                    assert select.select([receiver.stdout], [], [], 10)[0], 'receiver did not become ready'
                    first = receiver.stdout.readline()
                    assert first.startswith(b'DROP_WINDOW='), first
                    window = first.decode().strip().split('=')[1]
                    sent = subprocess.run([str(sender), window, scenario], env=environment,
                                          capture_output=True, timeout=10)
                    stdout, stderr = receiver.communicate(timeout=10)
                    log = output / (group + '-' + scenario + '.log')
                    log.write_bytes(first + sent.stdout + sent.stderr + stdout + stderr)
                    record = {'library': group, 'scenario': scenario,
                              'sender_exit': sent.returncode, 'receiver_exit': receiver.returncode,
                              'log': log.name}
                    records.append(record)
                    (output / 'progress.json').write_text(json.dumps(records, indent=2) + '\n')
                    assert sent.returncode == 0 and receiver.returncode == 0 and b'DROP_NATIVE=PASS' in stdout, log.read_text()
                    print('XDND_NATIVE=PASS library=' + group + ' scenario=' + scenario, flush=True)
                finally:
                    if receiver.poll() is None:
                        receiver.kill()
                        receiver.wait(timeout=5)
        (output / 'qualification.json').write_text(json.dumps({
            'format': 'fbc-xdnd-native-1', 'cases': records,
            'physical_drag_tested': False}, indent=2) + '\n')
    finally:
        server.terminate()
        server.wait(timeout=5)
        xvfb_log.close()


if __name__ == '__main__':
    main()
# end of test-x11-drop.py
