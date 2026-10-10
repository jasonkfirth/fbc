"""Project: FreeBASIC SDL3 example checks
File: sdl3_xvfb.py
Purpose: Give one example a fresh, authenticated Xvfb server.
Responsibilities: Wait for readiness, select X11, record the server, and reap it.
This file intentionally does NOT contain: example execution or desktop control.
"""

from __future__ import annotations

from contextlib import contextmanager
import os
from pathlib import Path
import secrets
import selectors
import subprocess
import tempfile
import time


@contextmanager
def private_display(directory: Path, environment: dict[str, str]):
    server = None
    record = {"server_log": str(directory / "xvfb.log"), "stopped": False}
    with tempfile.TemporaryDirectory(prefix="xvfb-", dir=directory) as temporary:
        authority = Path(temporary) / "Xauthority"
        authority.touch(mode=0o600)
        cookie = secrets.token_hex(16)
        # The server consumes the cookie before its automatic display number
        # is known. Add the matching client entry once -displayfd reports it.
        subprocess.run(["xauth", "-f", str(authority), "add", ":0", ".", cookie],
                       check=True, capture_output=True, timeout=5)
        with (directory / "xvfb.log").open("w") as log:
            reader, writer = os.pipe()
            try:
                # Xvfb chooses an unused display atomically. Readiness is
                # signalled through the pipe instead of guessing a delay.
                command = ["Xvfb", "-displayfd", str(writer), "-screen", "0",
                           "1280x1024x24", "-nolisten", "tcp", "-auth", str(authority)]
                server = subprocess.Popen(command, pass_fds=(writer,), stdout=log,
                                          stderr=subprocess.STDOUT, start_new_session=True)
                record["server_pid"] = server.pid
                os.close(writer)
                writer = None
                with selectors.DefaultSelector() as selector:
                    selector.register(reader, selectors.EVENT_READ)
                    message = b""
                    deadline = time.monotonic() + 10
                    # Xvfb may write the number and newline separately.
                    # Closing the reader after the first write breaks startup.
                    while b"\n" not in message:
                        remaining = deadline - time.monotonic()
                        if remaining <= 0 or not selector.select(timeout=remaining):
                            raise RuntimeError("Xvfb did not become ready; see " + record["server_log"])
                        block = os.read(reader, 32)
                        if not block or len(message) + len(block) > 32:
                            raise RuntimeError("Xvfb startup pipe failed; see " + record["server_log"])
                        message += block
                number = message.strip()
                if not message.endswith(b"\n") or not number.isdigit() or len(number) > 6:
                    raise RuntimeError("Xvfb returned an invalid display; see " + record["server_log"])
                display = ":" + number.decode("ascii")
                subprocess.run(["xauth", "-f", str(authority), "add", display, ".", cookie],
                               check=True, capture_output=True, timeout=5)
                session = environment.copy()
                session["DISPLAY"] = display
                session["XAUTHORITY"] = str(authority)
                session["SDL_VIDEODRIVER"] = "x11"
                session.pop("WAYLAND_DISPLAY", None)
                record["display"] = display
                # This also verifies that authentication works before the
                # example starts; SDL cannot fall back to the user's display.
                probe = subprocess.run(["xdpyinfo"], env=session, capture_output=True,
                                       text=True, timeout=5)
                (directory / "x-display.log").write_text(probe.stdout + probe.stderr)
                if probe.returncode:
                    raise RuntimeError("Cannot connect to the private X display")
                yield session, record
            finally:
                os.close(reader)
                if writer is not None:
                    os.close(writer)
                if server is not None:
                    if server.poll() is None:
                        server.terminate()
                        try:
                            server.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            server.kill()
                            server.wait(timeout=5)
                    record["server_exit_code"] = server.returncode
                    record["stopped"] = server.poll() is not None

# end of sdl3_xvfb.py
