#!/usr/bin/env python3
"""
Project: FreeBASIC compiler regression tests
File: console-runtime.py
Purpose: Exercise WSTRING input and address-valued DATA with the linked runtime.
Responsibilities: Drive stdin through a pipe and a Linux pseudo-terminal.
This file intentionally does not contain console or Unicode implementations.
"""

import errno
import os
from pathlib import Path
import pty
import selectors
import subprocess
import sys
from tempfile import TemporaryDirectory
import time


RECORD = """
dim word as wstring * 32, limited as wstring * 3, line_text as wstring * 20
dim n as long, text as string
input n, word, text
if n <> 7 or word <> wstr("x, ") + wchr(&h3A9) + wchr(&h1F642) or text <> "tail" then end 1
input limited, n
if limited <> wchr(&h3A9) + wchr(&h3A9) or n <> 41 then end 2
line input line_text
if line_text <> wchr(&h3A9) + wchr(&h1F642) then end 3
line input #0, line_text
if line_text <> wchr(&h4E2D) then end 4
print "PASS"
"""

RAW_INPUT = """
print "RAW_READY"
dim text as wstring * 24 = winput(2)
if text <> wchr(&h3A9) + wchr(&h4E2D) then end 1
print "PASS"
"""

INTERACTIVE = """
dim text as wstring * 20, n as long
input "INPUT_READY"; n, text
if n <> 7 or text <> wchr(&h3A9) then end 1
line input "LINE_READY"; text
if text <> wchr(&h4E2D) then end 2
print "PASS"
"""

MALFORMED = """
dim text as wstring * 24 = winput(2)
if text <> wchr(&hFFFD) + wstr("!") then end 1
print "PASS"
"""

DATA = """
dim shared target_value as long
addresses:
data @target_value, @target_value, @target_value, @target_value, 41
dim s as string, w as wstring * 40, short_w as wstring * 3
dim u as ustring, n as long
restore addresses
read s, w, short_w, u, n
if s <> str(culngint(@target_value)) or w <> wstr(culngint(@target_value)) then end 1
if short_w <> left(w, 2) or u <> s or n <> 41 then end 2
print "PASS"
"""

ENCODED = """
dim text as wstring * 20
if open("wide.txt" for output encoding "utf-8" as #1) <> 0 then end 1
print #1, wchr(&h3A9) + wchr(&h1F642)
print #1, wchr(&h4E2D)
close #1
if open("wide.txt" for input encoding "utf-8" as #1) <> 0 then end 2
line input #1, text
if text <> wchr(&h3A9) + wchr(&h1F642) then end 3
input #1, text
if text <> wchr(&h4E2D) then end 4
close #1
print "PASS"
"""


def check_pipe(executable, folder, data=b""):
    result = subprocess.run(
        [executable], cwd=folder, input=data, capture_output=True, timeout=10
    )
    if result.returncode or b"PASS" not in result.stdout:
        raise RuntimeError(
            f"{executable.name} exited {result.returncode}: "
            f"{result.stdout!r} {result.stderr!r}"
        )


def check_terminal(executable, folder, exchanges):
    master, slave = pty.openpty()
    output = bytearray()
    process = None
    try:
        process = subprocess.Popen(
            [executable], cwd=folder, stdin=slave, stdout=slave, stderr=slave,
            env={**os.environ, "TERM": "xterm"},
        )
        os.close(slave)
        slave = None
        pending = iter(exchanges)
        exchange = next(pending, None)
        deadline = time.monotonic() + 10
        with selectors.DefaultSelector() as selector:
            selector.register(master, selectors.EVENT_READ)
            while time.monotonic() < deadline:
                if not selector.select(max(0, deadline - time.monotonic())):
                    break
                try:
                    chunk = os.read(master, 4096)
                except OSError as error:
                    if error.errno != errno.EIO:
                        raise
                    break
                if not chunk:
                    break
                output.extend(chunk)
                if exchange is not None and exchange[0] in output:
                    os.write(master, exchange[1])
                    exchange = next(pending, None)
        if process.wait(timeout=1) or exchange is not None or b"PASS" not in output:
            raise RuntimeError(f"{executable.name} terminal output: {bytes(output)!r}")
    finally:
        if process is not None and process.poll() is None:
            process.kill()
            process.wait()
        if slave is not None:
            os.close(slave)
        os.close(master)


def main():
    compiler = sys.argv[1:]
    if compiler and compiler[0] == "--":
        compiler.pop(0)
    if not compiler:
        raise SystemExit("pass the FreeBASIC compiler command after --")
    with TemporaryDirectory(prefix="fbc-wstring-runtime-") as temporary:
        folder = Path(temporary)
        cases = {
            "record": RECORD, "raw": RAW_INPUT, "interactive": INTERACTIVE,
            "malformed": MALFORMED, "data": DATA, "encoded": ENCODED,
        }
        for name, source in cases.items():
            (folder / f"{name}.bas").write_text(source, encoding="ascii")
            subprocess.run(
                [*compiler, f"{name}.bas", "-x", name], cwd=folder, check=True, timeout=30
            )
        check_pipe(folder / "record", folder, (
            '7,"x, Ω🙂",tail\r\n' + "Ω" * 5000 + ",41\nΩ🙂\n中\n"
        ).encode())
        check_pipe(folder / "raw", folder, "Ω中".encode())
        check_pipe(folder / "malformed", folder, b"\xe2!")
        check_pipe(folder / "data", folder)
        check_pipe(folder / "encoded", folder)
        check_terminal(folder / "interactive", folder, [
            (b"INPUT_READY", "7,Ω\r".encode()), (b"LINE_READY", "中\r".encode()),
        ])
        check_terminal(folder / "raw", folder, [(b"RAW_READY", "Ω中\n".encode())])
    print("WSTRING stdin, terminal and DATA checks passed")


if __name__ == "__main__":
    main()

# end of console-runtime.py
