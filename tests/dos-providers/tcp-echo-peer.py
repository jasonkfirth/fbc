#!/usr/bin/env python3
"""FreeBASIC DOS tests: tcp-echo-peer.py.

Check the fixed payload from freebasic-tcp.bas, return it, and require the
guest to close its connection. This independent host peer does not emulate
TCP or alter the guest's data. Bind to loopback unless explicitly configured.
"""

import argparse
import socket
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bind', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=7779)
    parser.add_argument('--connections', type=int, default=1)
    parser.add_argument('--timeout', type=float, default=30)
    parser.add_argument('--bytes', type=int, default=512, dest='payload_bytes')
    parser.add_argument('--fragment', type=int, default=0,
                        help='send the echo in this many bytes per chunk, 20 ms apart')
    parser.add_argument('--close-after-echo', action='store_true',
                        help='Close immediately after sending, to test draining final buffered bytes')
    args = parser.parse_args()
    if args.connections < 1 or args.timeout <= 0 or not 1 <= args.port <= 65535:
        parser.error('connections, timeout and port must be positive and valid')
    if not 1 <= args.payload_bytes <= 32768:
        parser.error('--bytes must be between 1 and 32768')
    if not 0 <= args.fragment <= 32768:
        parser.error('--fragment must be between 0 and 32768')
    expected = bytes((17 * i + 3) & 255 for i in range(args.payload_bytes))
    with socket.socket() as listener:
        listener.bind((args.bind, args.port))
        listener.listen()
        listener.settimeout(args.timeout)
        print(f'Listening on {args.bind}:{args.port}', flush=True)
        for index in range(args.connections):
            conn, address = listener.accept()
            with conn:
                deadline = time.monotonic() + args.timeout
                received = bytearray()
                while len(received) < len(expected):
                    conn.settimeout(max(0.001, deadline - time.monotonic()))
                    chunk = conn.recv(len(expected) - len(received))
                    if not chunk:
                        raise RuntimeError(f'Peer closed after {len(received)} bytes')
                    received.extend(chunk)
                if received != expected:
                    raise RuntimeError('Guest payload differs from the expected bytes')
                conn.settimeout(max(0.001, deadline - time.monotonic()))
                if args.fragment:
                    for offset in range(0, len(received), args.fragment):
                        conn.settimeout(max(0.001, deadline - time.monotonic()))
                        conn.sendall(received[offset:offset + args.fragment])
                        time.sleep(0.02)
                else:
                    conn.sendall(received)
                if not args.close_after_echo and conn.recv(1):
                    raise RuntimeError('Unexpected bytes after the test payload')
            print(f'PASS connection {index + 1}: {len(expected)} exact bytes and echo', flush=True)


if __name__ == '__main__':
    main()

# end of tcp-echo-peer.py
