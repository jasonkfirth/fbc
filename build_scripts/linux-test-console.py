#!/usr/bin/env python3
"""FreeBASIC private Linux console checks
File: linux-test-console.py
Purpose: Read and acknowledge the final wait of an owned native console example.
Responsibilities: A private PTY, bounded output, normal input and process cleanup.
This file contains no desktop keyboard input or attachment to another console.
"""
import errno
import fcntl
import os
import pty
import re
import selectors
import signal
import subprocess
import struct
import sys
import termios
import time


def run_console(command, cwd, environment, timeout=10):
    reader, writer = pty.openpty()
    child = None
    output = bytearray()
    sent = False
    last_output = time.monotonic()
    deadline = last_output+timeout
    try:
        session = environment.copy()
        session['TERM'] = 'xterm'
        # A new PTY has zero dimensions until its owner supplies them. Both
        # terminfo and the viewport are required by the native console runtime.
        fcntl.ioctl(writer,termios.TIOCSWINSZ,struct.pack('HHHH',25,80,0,0))
        # Run a small child entry point before exec. It gives the new session
        # a controlling terminal and foreground process group, as required by
        # FreeBASIC's /dev/tty input. This avoids preexec_fn in a Python parent.
        arguments = [sys.executable, os.path.abspath(__file__), '--child', *map(str,command)]
        child = subprocess.Popen(arguments, cwd=cwd, env=session,
                                 stdin=writer, stdout=writer, stderr=writer, start_new_session=True)
        os.close(writer)
        writer = None
        with selectors.DefaultSelector() as selector:
            selector.register(reader, selectors.EVENT_READ)
            while True:
                if time.monotonic() >= deadline:
                    raise TimeoutError('Owned console example did not exit')
                if selector.select(timeout=.05):
                    try:
                        block = os.read(reader,4096)
                    except OSError as error:
                        # Linux signals the final slave close with EIO.
                        if error.errno != errno.EIO:
                            raise
                        block = b''
                    if not block:
                        break
                    output.extend(block)
                    if len(output) > 1024*1024:
                        raise RuntimeError('Owned console output exceeds one MiB')
                    last_output = time.monotonic()
                if output and not sent and child.poll() is None and time.monotonic()-last_output >= .3:
                    os.write(reader,b'\r')
                    sent = True
            result = child.wait(timeout=max(.1,deadline-time.monotonic()))
        text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',output.decode(errors='replace'))
        return {'exit':result, 'output':text, 'normal_keypress':sent, 'keypress_target':'owned example PTY'}
    finally:
        os.close(reader)
        if writer is not None:
            os.close(writer)
        if child is not None and child.poll() is None:
            os.killpg(child.pid,signal.SIGTERM)
            try:
                child.wait(timeout=2)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid,signal.SIGKILL)
                child.wait(timeout=2)


if __name__ == '__main__':
    if len(sys.argv) < 3 or sys.argv[1] != '--child':
        raise SystemExit('This helper is invoked through run_console')
    fcntl.ioctl(0,termios.TIOCSCTTY,0)
    os.tcsetpgrp(0,os.getpgrp())
    os.execvpe(sys.argv[2],sys.argv[2:],os.environ)
# end of linux-test-console.py
