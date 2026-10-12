#!/usr/bin/env python3
"""FreeBASIC native libcurl qualification
File: test-curl.py
Purpose: Compare the real C ABI and exercise curl examples over loopback HTTP.
Responsibilities: Isolated SDKs, two backends, exact binary transfers, native
PUT redirects, multipart POST and failure status with owned server cleanup.
This file contains no public endpoint, substitute libcurl or TLS bypass.
"""
from pathlib import Path
import argparse
import email.parser
import email.policy
import hashlib
import http.server
import json
import os
import shlex
import shutil
import struct
import subprocess
import threading
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--bits', type=int, choices=(32, 64), default=64)
    parser.add_argument('--tool-dir', type=Path)
    parser.add_argument('--runtime-prefix', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    windows = os.name == 'nt'
    if windows and args.tool_dir is None:
        parser.error('--tool-dir is required for Windows SDK selection')
    if not windows and (os.name != 'posix' or args.bits != 64):
        parser.error('The Unix lane currently qualifies native Linux x86-64')
    root = Path(__file__).resolve().parents[1]
    compiler = args.fbc.resolve()
    tools = args.tool_dir.resolve() if args.tool_dir else Path(shutil.which('gcc') or '/usr/bin/gcc').parent
    sdk = tools.parent
    suffix = '.exe' if windows else ''
    work = args.output.resolve()
    work.mkdir(parents=True, exist_ok=True)
    folder = root / 'examples/network/curl/CHttp'
    names = ['build_scripts/test-curl.py', 'inc/curl.bi', 'tests/bindings/curl-native.c',
             'tests/bindings/curl-native.bas', 'examples/manual/libraries/curl.bas',
             'doc/manual/cache/ExtLibcurl.wakka']
    names += ['examples/network/curl/CHttp/' + path.name for path in folder.glob('*') if path.suffix in ('.bas', '.bi')]
    hashes = {name: hashlib.sha256((root/name).read_bytes()).hexdigest() for name in names}
    if windows:
        sdk_paths = [sdk/'include/curl/curl.h', sdk/'include/curl/curlver.h', sdk/'include/curl/system.h',
                     sdk/'bin/libcurl-4.dll', sdk/'lib/libcurl.dll.a']
        c_sdk_flags = ['-I'+str(sdk/'include'), '-L'+str(sdk/'lib'), '-lcurl']
    else:
        c_sdk_flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'libcurl'], text=True))
        multiarch = subprocess.check_output([tools/'gcc', '-print-multiarch'], text=True).strip()
        header_roots = [Path(flag[2:]) for flag in c_sdk_flags if flag.startswith('-I')]
        header_roots += [sdk/'include'/multiarch, sdk/'include']
        include = next((path for path in header_roots if (path/'curl/curl.h').is_file()), None)
        if include is None:
            raise RuntimeError('The selected libcurl C headers were not found')
        library = Path(subprocess.check_output([tools/'gcc', '-print-file-name=libcurl.so'], text=True).strip()).resolve()
        sdk_paths = [include/'curl'/name for name in ('curl.h', 'curlver.h', 'system.h')] + [library]
    sdk_hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in sdk_paths}
    receipt = {'status': 'pending', 'platform': 'windows' if windows else 'linux', 'bits': args.bits, 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
               'source_sha256': hashes, 'sdk_sha256': sdk_hashes, 'records': [], 'requests': []}
    target = work/'results.json'

    def save():
        target.write_text(json.dumps(receipt, indent=2)+'\n')

    save()
    env = os.environ.copy()
    env['PATH'] = str(tools) + os.pathsep + env['PATH']
    env['NO_PROXY'] = env['no_proxy'] = '*'
    for label in ('gcc', 'as', 'ld', 'ar'):
        env[label.upper()] = str(tools/(label+suffix))

    # -------------------------------------------------------------------------
    # Private HTTP fixture and independently recorded wire requests
    # -------------------------------------------------------------------------
    body = bytes(range(256))*1025+b'tail'
    body_hash = hashlib.sha256(body).hexdigest()
    server_errors = []
    request_lock = threading.Lock()

    class Handler(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.1'

        def log_message(self, *unused):
            pass

        def handle(self):
            try:
                super().handle()
            except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
                # Clients deliberately close timed-out and completed keepalive
                # connections. Byte comparisons still gate every valid transfer.
                pass

        def record(self, payload=b''):
            item = {'method': self.command, 'path': self.path, 'bytes': len(payload),
                    'body_sha256': hashlib.sha256(payload).hexdigest()}
            with request_lock:
                receipt['requests'].append(item)
            return item

        def reply(self, payload, status=200):
            self.send_response(status)
            self.send_header('Content-Length', str(len(payload)))
            self.end_headers()
            if payload:
                self.wfile.write(payload)
                self.wfile.flush()

        def do_GET(self):
            self.record()
            try:
                if self.path == '/body':
                    self.reply(body)
                elif self.path == '/empty':
                    self.reply(b'')
                elif self.path == '/chunked':
                    self.send_response(200)
                    self.send_header('Transfer-Encoding', 'chunked')
                    self.end_headers()
                    offset = 0
                    for size in (1, 17, 4096, 31, len(body)):
                        block = body[offset:offset+size]
                        if not block:
                            break
                        self.wfile.write(('%x\r\n' % len(block)).encode()+block+b'\r\n')
                        self.wfile.flush()
                        offset += len(block)
                    assert offset == len(body)
                    self.wfile.write(b'0\r\n\r\n')
                elif self.path == '/truncated':
                    self.send_response(200)
                    self.send_header('Content-Length', str(len(body)+1))
                    self.send_header('Connection', 'close')
                    self.end_headers()
                    self.wfile.write(body)
                    self.close_connection = True
                elif self.path == '/slow':
                    time.sleep(.4)
                    self.reply(b'late')
                else:
                    self.reply(b'missing', 404)
            except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
                # A real client deadline deliberately closes this owned connection.
                pass

        def upload_body(self):
            length = int(self.headers.get('Content-Length', '-1'))
            assert 0 <= length <= 2*1024*1024, ('Unexpected upload length', length)
            payload = self.rfile.read(length)
            assert len(payload) == length
            return payload

        def do_PUT(self):
            try:
                payload = self.upload_body()
                self.record(payload)
                assert payload in (body, b'')
                if self.path == '/redirect-upload':
                    self.send_response(307)
                    self.send_header('Location', '/upload')
                    self.send_header('Content-Length', '0')
                    self.end_headers()
                elif self.path == '/upload-text':
                    self.reply(b'UPLOAD_OK')
                else:
                    assert self.path == '/upload'
                    self.reply(payload)
            except Exception as error:
                server_errors.append(repr(error))
                self.close_connection = True

        def do_POST(self):
            try:
                payload = self.upload_body()
                self.record(payload)
                assert self.path == '/form'
                message = email.parser.BytesParser(policy=email.policy.default).parsebytes(
                    ('Content-Type: '+self.headers['Content-Type']+'\r\nMIME-Version: 1.0\r\n\r\n').encode()+payload)
                assert message.is_multipart()
                fields = {part.get_param('name', header='content-disposition'): part.get_payload(decode=True)
                          for part in message.iter_parts()}
                assert fields['field'] == b'alpha' and int(fields['number']) == 42 and fields['typed'] == b'beta'
                self.reply(b'FORM_OK')
            except Exception as error:
                server_errors.append(repr(error))
                self.close_connection = True

    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base_url = 'http://127.0.0.1:' + str(server.server_port)

    # -------------------------------------------------------------------------
    # Native compiler calls and transfer checks
    # -------------------------------------------------------------------------
    def run(command, label, expected=0):
        result = subprocess.run(list(map(str, command)), cwd=work, env=env, stdin=subprocess.DEVNULL,
                                capture_output=True, timeout=30)
        (work/(label+'.log')).write_bytes(repr(list(map(str, command))).encode()+b'\n'+result.stdout+result.stderr)
        assert result.returncode == expected, (label, result.returncode, result.stdout[:1000], result.stderr[:1000])
        return result.stdout

    def machine(binary):
        data = binary.read_bytes()
        if windows:
            offset = struct.unpack_from('<I', data, 0x3c)[0]
            assert struct.unpack_from('<H', data, offset+4)[0] == (0x14c if args.bits == 32 else 0x8664)
        else:
            assert data[:6] == b'\x7fELF\x02\x01' and struct.unpack_from('<H', data, 18)[0] == 62

    def fb_flags(backend):
        flags = [compiler, '-i', root/'inc', '-i', folder, '-gen', backend, '-w', 'all', '-p', sdk/'lib']
        flags += ['-prefix', args.runtime_prefix.resolve()]
        if windows and args.bits == 32:
            flags += ['-target', 'win32', '-prefix', args.runtime_prefix.resolve(), '-buildprefix', str(tools)+'/']
        return flags

    def abi(data):
        return list(map(int, data.decode().splitlines()[0].removeprefix('ABI').split()))

    try:
        native = work/('native-c'+suffix)
        run([tools/('gcc'+suffix), '-Wall', '-Wextra', '-Werror', root/'tests/bindings/curl-native.c',
             *c_sdk_flags, '-o', native], 'c-compile')
        machine(native)
        reference = run([native], 'c-abi')
        receipt['records'].append({'backend': 'C', 'case': 'abi', 'passed': True, 'output': reference.decode().strip()})
        for backend in ('gcc', 'gas' if args.bits == 32 else 'gas64'):
            directory = work/backend
            directory.mkdir(exist_ok=True)
            flags = fb_flags(backend)
            run(flags+['-lib', folder/'CHttp.bas', folder/'CHttpStream.bas', folder/'CHttpForm.bas',
                       '-x', directory/'libCHttp.a'], backend+'-library-compile')
            for name, source in (('native', root/'tests/bindings/curl-native.bas'),
                                 ('manual', root/'examples/manual/libraries/curl.bas'),
                                 ('example', folder/'test.bas')):
                binary = directory/(name+suffix)
                run(flags+['-p', directory, source, '-x', binary], backend+'-'+name+'-compile')
                machine(binary)
                if name == 'native':
                    actual = run([binary, 'abi'], backend+'-abi')
                    assert abi(actual) == abi(reference), (actual, reference)
                    receipt['records'].append({'backend': backend, 'case': 'abi', 'passed': True, 'actual': abi(actual)})
                    first_request = len(receipt['requests'])
                    output = run([binary, base_url], backend+'-runtime')
                    assert b'CURL_NATIVE=PASS' in output, output
                    observed = receipt['requests'][first_request:]
                    puts = [item for item in observed if item['method'] == 'PUT']
                    posts = [item for item in observed if item['method'] == 'POST']
                    assert len(puts) == 15 and len(posts) == 3
                    assert sum(item['body_sha256'] == body_hash for item in puts) == 12
                    assert sum(item['bytes'] == 0 for item in puts) == 3
                    receipt['records'].append({'backend': backend, 'case': 'wrapper', 'passed': True,
                                               'output': output.decode().strip(), 'put_requests': len(puts), 'post_requests': len(posts)})
                else:
                    for path, expected in (('/body', body), ('/chunked', body), ('/empty', b'')):
                        output = run([binary, base_url+path], backend+'-'+name+path.replace('/', '-'))
                        assert output == expected, (name, path, len(output), len(expected))
                        receipt['records'].append({'backend': backend, 'case': name+path, 'passed': True,
                                                   'bytes': len(output), 'sha256': hashlib.sha256(output).hexdigest()})
                    if name == 'manual':
                        for path in ('/missing', '/truncated'):
                            output = run([binary, base_url+path], backend+'-'+name+path.replace('/', '-'), expected=1)
                            assert b'curl transfer failed:' in output
                            receipt['records'].append({'backend': backend, 'case': name+path, 'passed': True, 'exit': 1})
        assert not server_errors, server_errors
        assert hashes == {name: hashlib.sha256((root/name).read_bytes()).hexdigest() for name in names}
        assert sdk_hashes == {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in sdk_paths}
        assert receipt['compiler_sha256'] == hashlib.sha256(compiler.read_bytes()).hexdigest()
        receipt['status'] = 'passed'
        save()
        print('CURL_EXAMPLES=PASS bits=' + str(args.bits) + ' records=' + str(len(receipt['records'])), flush=True)
    except Exception as error:
        receipt['status'] = 'failed'
        receipt['error'] = repr(error)
        save()
        raise
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=2)


if __name__ == '__main__':
    main()

# end of test-curl.py
