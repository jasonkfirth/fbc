"""FreeBASIC private PulseAudio test context
File: sfx_pulse.py
Purpose: Provide a genuine clocked null sink without altering desktop audio.
Responsibilities: Private D-Bus, cookie/socket, readiness and process cleanup.
This file contains no audio emulation or host sound-device configuration.
"""
from contextlib import contextmanager
import os
from pathlib import Path
import secrets
import signal
import subprocess
import tempfile
import time


@contextmanager
def private_pulse(directory, environment):
    server = None
    record = {"stopped": False, "server_log": str(directory / "pulse-server.log")}
    with tempfile.TemporaryDirectory(prefix="fbc-pulse-") as temporary:
        # Keep the Unix socket below its platform path-length limit even when
        # the checkout path is long. The temporary directory is mode 0700.
        work = Path(temporary)
        cookie = work / "cookie"
        cookie.write_bytes(secrets.token_bytes(256))
        cookie.chmod(0o600)
        socket = work / "native.sock"
        session = environment.copy()
        session["PULSE_SERVER"] = "unix:" + str(socket)
        session["PULSE_COOKIE"] = str(cookie)
        with (directory / "pulse-server.log").open("w") as log:
            try:
                # A separate bus permits coexistence with a running desktop
                # PulseAudio service. No existing service is stopped.
                server = subprocess.Popen([
                    "dbus-run-session", "--", "pulseaudio", "--daemonize=no",
                    "--use-pid-file=no", "--exit-idle-time=-1", "--disable-shm=yes", "-n",
                    "-L", "module-native-protocol-unix socket=" + str(socket)
                    + " auth-anonymous=0 auth-cookie=" + str(cookie),
                    "-L", "module-null-sink sink_name=codex_sfx rate=44100 channels=2"],
                    env=session, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
                record["process_group"] = server.pid
                deadline = time.monotonic() + 8
                while time.monotonic() < deadline and server.poll() is None:
                    if socket.exists():
                        probe = subprocess.run(["pactl", "info"], env=session,
                                               capture_output=True, text=True, timeout=2)
                        if probe.returncode == 0:
                            record["server_info"] = probe.stdout
                            break
                    time.sleep(.05)
                else:
                    raise RuntimeError("Private PulseAudio did not become ready")
                subprocess.run(["pactl", "set-default-sink", "codex_sfx"], env=session,
                               capture_output=True, check=True, timeout=3)
                yield session, record
            finally:
                if server is not None:
                    if server.poll() is None:
                        # Ask our authenticated server to exit before using
                        # the process-group fallback. This also reaches the
                        # daemon if it changed its own process group.
                        if socket.exists():
                            try:
                                subprocess.run(["pactl", "exit"], env=session,
                                               capture_output=True, timeout=2)
                            except subprocess.TimeoutExpired:
                                pass
                        try:
                            server.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            try:
                                os.killpg(server.pid, signal.SIGTERM)
                            except ProcessLookupError:
                                pass
                            try:
                                server.wait(timeout=3)
                            except subprocess.TimeoutExpired:
                                try:
                                    os.killpg(server.pid, signal.SIGKILL)
                                except ProcessLookupError:
                                    pass
                                server.wait(timeout=3)
                    record["stopped"] = server.poll() is not None and not socket.exists()
                    if not record["stopped"]:
                        raise RuntimeError("Private PulseAudio socket survived shutdown")

# end of sfx_pulse.py
