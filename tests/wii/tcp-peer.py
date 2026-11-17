#!/usr/bin/env python3
"""Project: FreeBASIC Wii runtime tests.
File: tcp-peer.py
Purpose: Supply the request/reply peer for tests/wii/tcp.bas.
Responsibilities: Own one loopback socket and bound the acknowledgement wait.
This file does not connect to external services or launch an emulator.
"""
import json
import socket
import time


def main():
    with socket.socket() as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(('127.0.0.1', 18078))
        listener.listen(1)
        listener.settimeout(30)
        print('WII_TCP_PEER_READY', flush=True)
        connection, _ = listener.accept()
        with connection:
            connection.settimeout(5)
            connection.sendall(b'PING')
            started = time.monotonic()
            reply = bytearray()
            while len(reply) < 2:
                part = connection.recv(2 - len(reply))
                if not part:
                    break
                reply.extend(part)
            elapsed = time.monotonic() - started
    passed = bytes(reply) == b'OK'
    print(json.dumps(dict(passed=passed, elapsed_seconds=elapsed)), flush=True)
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
# end of tcp-peer.py
