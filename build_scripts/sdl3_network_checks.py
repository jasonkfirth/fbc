"""Project: FreeBASIC SDL3 examples
File: sdl3_network_checks.py
Purpose: Exercise the translated networking programs against local peers.
Responsibilities: Bound services, exchange real payloads, and preserve diagnostics.
This file intentionally does NOT contain: public network requests or library mocks.
"""

from __future__ import annotations

import os
from pathlib import Path
import signal
import socket
import subprocess
import threading
import time


def stop(child: subprocess.Popen) -> None:
    if child.poll() is not None:
        return
    os.killpg(child.pid, signal.SIGTERM)
    try:
        child.wait(timeout=3)
    except subprocess.TimeoutExpired:
        os.killpg(child.pid, signal.SIGKILL)
        child.wait()


def available_port(kind: int) -> int:
    with socket.socket(socket.AF_INET, kind) as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def wait_for_log(child: subprocess.Popen, log: Path, text: str, seconds: int = 5) -> None:
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if text in log.read_text():
            return
        if child.poll() is not None:
            break
        time.sleep(0.02)
    raise RuntimeError("Service did not become ready; see " + str(log))


def echo(executable: Path, directory: Path, environment: dict[str, str]) -> dict:
    port = available_port(socket.SOCK_STREAM)
    log = directory / "loopback-server.log"
    payload = b"FreeBASIC SDL3 stream roundtrip\n"
    with log.open("w") as output:
        server = subprocess.Popen([str(executable), "127.0.0.1", "--port", str(port)],
                                  cwd=directory, env=environment, stdout=output,
                                  stderr=subprocess.STDOUT, start_new_session=True)
        try:
            wait_for_log(server, log, "Server is ready!")
            with socket.create_connection(("127.0.0.1", port), timeout=3) as client:
                client.sendall(payload)
                received = b""
                while len(received) < len(payload):
                    block = client.recv(len(payload) - len(received))
                    if not block:
                        break
                    received += block
            return dict(passed=received == payload, bytes_received=len(received), log=str(log))
        finally:
            stop(server)


def datagram(executable: Path, directory: Path, environment: dict[str, str]) -> dict:
    port = available_port(socket.SOCK_DGRAM)
    log = directory / "loopback-server.log"
    payload = b"FreeBASIC SDL3 datagram check"
    with log.open("w") as output:
        server = subprocess.Popen([str(executable), "127.0.0.1", "--server", "--port", str(port)],
                                  cwd=directory, env=environment, stdout=output,
                                  stderr=subprocess.STDOUT, start_new_session=True)
        try:
            wait_for_log(server, log, "SERVER: Listening")
            deadline = time.monotonic() + 3
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
                while time.monotonic() < deadline:
                    client.sendto(payload, ("127.0.0.1", port))
                    time.sleep(0.02)
                    if f"got {len(payload)}-byte datagram" in log.read_text():
                        break
            received = f"got {len(payload)}-byte datagram" in log.read_text()
        finally:
            stop(server)
    client_log = directory / "loopback-client.log"
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as peer:
        peer.bind(("127.0.0.1", 0))
        peer.settimeout(3)
        with client_log.open("w") as output:
            command = [str(executable), "127.0.0.1", "--port", str(peer.getsockname()[1])]
            client = subprocess.Popen(command, cwd=directory, env=environment, stdout=output,
                                      stderr=subprocess.STDOUT, start_new_session=True)
            try:
                data, _ = peer.recvfrom(4096)
                code = client.wait(timeout=3)
            finally:
                stop(client)
    return dict(passed=received and len(data) == 128 and code == 0,
                bytes_received=len(data), server_log=str(log), client_log=str(client_log))


def http(executable: Path, directory: Path, environment: dict[str, str]) -> dict:
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.bind(("127.0.0.1", 0))
    port = listener.getsockname()[1]
    listener.listen(1)
    listener.settimeout(5)
    requests = []
    errors = []
    body = b"FreeBASIC local HTTP peer\n"

    def serve() -> None:
        try:
            connection, _ = listener.accept()
            with connection:
                connection.settimeout(3)
                request = b""
                while b"\r\n\r\n" not in request and len(request) < 4096:
                    block = connection.recv(4096 - len(request))
                    if not block:
                        break
                    request += block
                requests.append(request)
                response = b"HTTP/1.0 200 OK\r\nContent-Length: " + str(len(body)).encode()
                connection.sendall(response + b"\r\n\r\n" + body)
        except OSError as error:
            errors.append(str(error))
        finally:
            listener.close()

    thread = threading.Thread(target=serve)
    thread.start()
    try:
        result = subprocess.run([str(executable), "--port", str(port), "127.0.0.1"], cwd=directory,
                                env=environment, capture_output=True, text=True, timeout=8)
    finally:
        thread.join(timeout=8)
    log = directory / "loopback-http.log"
    log.write_text(result.stdout + result.stderr)
    return dict(passed=result.returncode == 0 and body.decode().strip() in result.stdout
                and bool(requests) and requests[0].startswith(b"GET / HTTP/1.0\r\n") and not errors,
                exit_code=result.returncode, log=str(log), errors=errors)


def voip(executable: Path, directory: Path, environment: dict[str, str]) -> dict:
    port = available_port(socket.SOCK_DGRAM)
    server_log = directory / "loopback-server.log"
    client_log = directory / "loopback-client.log"
    session_environment = environment.copy()
    # Keep the server running until the client has sent its initial keepalive.
    session_environment["FB_SDL3_SMOKE_FRAMES"] = "0"
    with server_log.open("w") as output:
        server = subprocess.Popen([str(executable), "127.0.0.1", "--server", "--port", str(port)],
                                  cwd=directory, env=session_environment, stdout=output,
                                  stderr=subprocess.STDOUT, start_new_session=True)
        try:
            wait_for_log(server, server_log, "SERVER: Listening")
            result = subprocess.run([str(executable), "127.0.0.1", "--port", str(port)],
                                    cwd=directory, env=environment, capture_output=True, text=True, timeout=10)
            client_log.write_text(result.stdout + result.stderr)
            wait_for_log(server, server_log, "Creating voice idnum=")
            return dict(passed=result.returncode == 0 and "keepalive" in client_log.read_text(),
                        exit_code=result.returncode, server_log=str(server_log), client_log=str(client_log))
        finally:
            stop(server)


CHECKS = {"net/echo-server.bas": echo, "net/datagram.bas": datagram,
          "net/simple-http-get.bas": http, "net/voipchat.bas": voip}

# end of sdl3_network_checks.py
