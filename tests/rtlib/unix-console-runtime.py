"""FreeBASIC native console runtime control.

File: unix-console-runtime.py
Run the BASIC regression on a real pseudo-terminal and answer its terminal queries.
Own one child and its descriptors. Compilation belongs to the calling build.
"""
import argparse
import errno
import fcntl
import json
import os
from pathlib import Path
import pty
import select
import struct
import subprocess
import termios
import time


def attach_terminal():
    os.setsid()
    fcntl.ioctl(0, termios.TIOCSCTTY, 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    master, slave = pty.openpty()
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack('HHHH', 12, 40, 0, 0))
    environment = dict(os.environ, TERM='xterm', LC_ALL='C.UTF-8')
    process = subprocess.Popen([str(args.exe.resolve())], stdin=slave, stdout=slave,
        stderr=subprocess.PIPE, env=environment, close_fds=True,
        preexec_fn=attach_terminal)
    os.close(slave)
    output = bytearray()
    cursor_replies = window_replies = 0
    narrow_sent = wide_sent = False
    idle_since = time.monotonic()
    failure = None
    try:
        while True:
            readable, _, _ = select.select([master], [], [], 0.1)
            if readable:
                try:
                    data = os.read(master, 4096)
                except OSError as error:
                    if error.errno != errno.EIO:
                        raise
                    data = b''
                if not data:
                    break
                output.extend(data)
                idle_since = time.monotonic()
                assert len(output) < 128 * 1024
                count = output.count(b'\x1b[6n')
                while cursor_replies < count:
                    os.write(master, b'\x1b[1;1R')
                    cursor_replies += 1
                count = output.count(b'\x1b[18t')
                while window_replies < count:
                    os.write(master, b'\x1b[8;12;40t')
                    window_replies += 1
                if not narrow_sent and b'NARROW>' in output:
                    os.write(master, b'narrow-line\n')
                    narrow_sent = True
                if not wide_sent and b'WIDE>' in output:
                    os.write(master, b'wide-line\n')
                    wide_sent = True
            if process.poll() is not None and not readable:
                break
            # Every input is supplied above. Silence here diagnoses a blocked
            # interactive operation and bounds a regression's child lifetime.
            if time.monotonic() - idle_since > 30:
                failure = 'No terminal activity after all observed queries were answered'
                process.kill()
                break
        try:
            _, errors = process.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            failure = 'The child closed its terminal before completing runtime shutdown'
            process.kill()
            _, errors = process.communicate()
    finally:
        os.close(master)
        if process.poll() is None:
            process.kill()
            process.wait()
    complete = (process.returncode == 0 and narrow_sent and wide_sent and
        b'CONSOLE_RUNTIME_PASS' in output and b'A\xc3\xa9\xe2\x82\xac' in output and
        b'\x1b%G' in output)
    result = dict(checks_complete=complete, exit=process.returncode,
        cursor_replies=cursor_replies, window_replies=window_replies,
        narrow_input_sent=narrow_sent, wide_input_sent=wide_sent,
        output_hex=output.hex(), stderr=errors.decode(errors='replace'), failure=failure)
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    if not complete:
        print(json.dumps(result), flush=True)
        return 1
    print('CONSOLE_RUNTIME_PTY_PASS', len(output), cursor_replies, window_replies, flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of unix-console-runtime.py
