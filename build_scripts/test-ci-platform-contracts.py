#!/usr/bin/env python3
"""Project: FreeBASIC CI platform regression checks
File: test-ci-platform-contracts.py
Purpose: Check analyzer models and small native platform contracts.
Responsibilities: Retain data-flow diagnostics, reject CE precision loss,
exercise the JavaScript path provider and verify dependency mirror fallback.
This file intentionally does NOT install tools or emulate a target system.
"""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, HTTPServer
import os
from pathlib import Path
import subprocess
import tempfile
import threading


def run(command, *, success=True):
    result = subprocess.run([str(item) for item in command], capture_output=True,
                            text=True, timeout=120, check=False)
    if (result.returncode == 0) != success:
        raise RuntimeError(f"{command}\n{result.stdout}{result.stderr}")
    return result.stdout + result.stderr


def check_source_fetch(root, work):
    """Mirror failures must neither bypass the digest nor erase a good file."""
    payload = b"pinned dependency fixture\n"
    digest = hashlib.sha256(payload).hexdigest()
    requests = []

    class Mirror(BaseHTTPRequestHandler):
        def do_GET(self):
            requests.append(self.path)
            self.send_response(404 if self.path == '/missing' else 200)
            self.end_headers()
            self.wfile.write(payload if self.path == '/good' else b"wrong archive")

        def log_message(self, *arguments):
            pass

    server = HTTPServer(('127.0.0.1', 0), Mirror)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    output = work / 'source.tar.gz'
    base = f'http://127.0.0.1:{server.server_port}'
    try:
        for paths, success in ((('/missing', '/good'), True),
                               (('/corrupt', '/good'), True),
                               (('/missing', '/corrupt'), False),
                               (('/good', '/missing'), True)):
            requests.clear()
            output.write_bytes(payload)
            run(['sh', root / 'build_scripts/mips/fetch-source.sh', output,
                 digest, *(base + path for path in paths)], success=success)
            expected_requests = list(paths[:1] if paths[0] == '/good' else paths)
            if requests != expected_requests or output.read_bytes() != payload:
                raise RuntimeError('Dependency fallback changed mirror order or pinned content')
            if output.with_suffix(output.suffix + '.part').exists():
                raise RuntimeError('Dependency fallback retained a partial transfer')
    finally:
        server.shutdown()
        thread.join(timeout=5)
        server.server_close()
    print('CI_SOURCE_FETCH_PASS transport,checksum,exhaustion,primary')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gcc', default=os.environ.get('GCC') or 'gcc')
    parser.add_argument('--cppcheck', default='cppcheck')
    options = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    model = root / 'build_scripts/cppcheck-platform-model.h'
    with tempfile.TemporaryDirectory(prefix='fbc-ci-contracts-') as directory:
        work = Path(directory)
        check_source_fetch(root, work)
        source = work / 'model.c'
        command = [options.cppcheck, '--quiet', '--platform=unix64',
                   '--enable=warning,performance,portability', '--error-exitcode=1',
                   '-DFB_CPPCHECK_ANALYSIS=1', '--include=' + str(model), source]
        source.write_text('EM_JS(int, browser_value, (void), {\n'
                          ' return typeof window === "undefined" ? 0 : 1;\n'
                          '});\nint value(void) { return browser_value(); }\n')
        run(command)
        source.write_text('int value(void) { int uninitialized; return uninitialized; }\n')
        diagnostic = run(command, success=False)
        if '[uninitvar]' not in diagnostic:
            raise RuntimeError('The analysis model lost uninitialized-value diagnostics')
        executable = work / ('path.exe' if os.name == 'nt' else 'path')
        run([options.gcc, '-std=c11', '-Wall', '-Wextra', '-Werror',
             '-I' + str(root / 'src/rtlib'),
             root / 'tests/rtlib/js-exepath-bounds.c',
             root / 'src/rtlib/drivers/js/sys_getexepath.c', '-o', executable])
        print(run([executable]).strip())
        run([options.gcc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
             root / 'tests/rtlib/longdouble-parse.c', '-lm', '-o', executable])
        print(run([executable]).strip())
        # Reject accidental dependencies on CE's absent locale header, without
        # substituting a fake runtime implementation or linking foreign code.
        (work / 'locale.h').write_text('#error CE must not include locale.h\n')
        ce_command = [options.gcc, '-std=c99', '-Wall', '-Wextra', '-Werror',
                      '-D_WIN32_WCE=0x0500', '-I' + str(work), '-c',
                      root / 'src/rtlib/math/longdouble_ops.c', '-o', work / 'ce.o']
        # These probes use the GNU x86 long-double switches, on the x86 CI host.
        run([*ce_command, '-mlong-double-64'])
        diagnostic = run([*ce_command, '-mlong-double-80'], success=False)
        if 'requires the native double format' not in diagnostic:
            raise RuntimeError('The CE precision guard did not reject narrowing')
    print('CI_PLATFORM_CONTRACTS_PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-ci-platform-contracts.py
