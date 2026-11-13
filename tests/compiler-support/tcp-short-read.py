#!/usr/bin/env python3
"""Project: FreeBASIC TCP tests
File: tcp-short-read.py
Purpose: Exercise the loopback test's payload reader across packet boundaries.
Responsibilities: Force a short array read and check complete and closed streams.
This file does not replace the complete TCP runtime or guest qualification.
"""
import argparse
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--fbc', default='fbc')
    parser.add_argument('--prefix', type=Path)
    parser.add_argument('--work-dir', type=Path)
    parser.add_argument('--clang', action='store_true')
    options = parser.parse_args()
    root = options.root.resolve()
    fbc = Path(shutil.which(options.fbc) or options.fbc).resolve()
    source = (root / 'tests/file/tcp.bas').read_text(encoding='utf-8-sig')
    helpers = source.split('function wait_for_tcp_data(', 1)[1]
    helpers = 'function wait_for_tcp_data(' + helpers.split('\nsub server_thread(', 1)[0]
    # Release the tail only after the actual array GET returns. The peer waits
    # for this byte, so packet fragmentation does not depend on a sleep length.
    marker = '\tif( bytesread < 0 or bytesread > count ) then'
    assert helpers.count(marker) == 1
    helpers = helpers.replace(marker, '\tdim as ubyte acknowledgement = 1\n'
                              '\tassert( put( #handle, , acknowledgement ) = 0 )\n' + marker)
    program = ('const STREAM_WAIT_ITERATIONS = 5000\n' + helpers +
               '\ndim as integer stream = freefile()\n'
               'dim as ubyte payload(0 to 3)\n'
               'assert( open tcp( "host=127.0.0.1,port=" & command(1) as #stream ) = 0 )\n'
               'dim as integer complete = read_tcp_payload( stream, payload() )\n'
               'if( command(2) = "complete" ) then\n'
               ' assert( complete )\n'
               ' for i as integer = 0 to 3\n'
               '  assert( payload(i) = 10 + i )\n'
               ' next\n'
               'else\n'
               ' assert( complete = FALSE )\n'
               'end if\n'
               'close #stream\nprint "TCP_SHORT_READ_PASS"\n')
    cases = (('whole', b'\x0a\x0b\x0c\x0d', b'', True),
             ('fragmented', b'\x0a\x0b', b'\x0c\x0d', True),
             ('truncated', b'\x0a\x0b', b'', False),
             ('empty', b'', b'', False))
    checks = 0
    with tempfile.TemporaryDirectory(prefix='fbc-tcp-short-', dir=options.work_dir) as temporary:
        work = Path(temporary)
        basic = work / 'probe.bas'
        basic.write_text(program, encoding='utf-8')
        for backend in (('gcc', 'clang') if options.clang else ('gcc',)):
            executable = work / ('probe.exe' if os.name == 'nt' else 'probe')
            command = [str(fbc), '-gen', backend, '-mt', '-g', '-i', str(root / 'inc')]
            if options.prefix:
                command += ['-prefix', str(options.prefix.resolve())]
            built = subprocess.run([*command, str(basic), '-x', str(executable)],
                                   text=True, capture_output=True, timeout=120)
            if built.returncode:
                raise RuntimeError(built.stdout + built.stderr)
            for name, head, tail, complete in cases:
                with socket.socket() as listener:
                    listener.bind(('127.0.0.1', 0))
                    listener.listen(1)
                    listener.settimeout(10)
                    process = subprocess.Popen(
                        [str(executable), str(listener.getsockname()[1]),
                         'complete' if complete else 'closed'],
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                    try:
                        connection, _ = listener.accept()
                        with connection:
                            connection.settimeout(10)
                            if head:
                                connection.sendall(head)
                                if connection.recv(1) != b'\x01':
                                    raise RuntimeError('array GET did not acknowledge its first read')
                            if tail:
                                connection.sendall(tail)
                            connection.shutdown(socket.SHUT_WR)
                            output, error = process.communicate(timeout=15)
                        if process.returncode or output.strip() != 'TCP_SHORT_READ_PASS':
                            raise RuntimeError(f'{backend}/{name}: {output}{error}')
                    finally:
                        if process.poll() is None:
                            process.kill()
                        process.communicate()
                checks += 1
                print(f'TCP_SHORT_READ_CASE_PASS {backend}/{name}', flush=True)
    print(f'TCP_SHORT_READ_PASS cases={checks}')


if __name__ == '__main__':
    main()

# end of tcp-short-read.py
