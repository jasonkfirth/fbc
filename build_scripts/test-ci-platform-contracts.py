#!/usr/bin/env python3
"""Project: FreeBASIC CI platform regression checks
File: test-ci-platform-contracts.py
Purpose: Check analyzer models and small native platform contracts.
Responsibilities: Retain data-flow diagnostics, reject CE precision loss,
exercise the JavaScript path provider, verify dependency mirror fallback,
and check guest package setup against stale shared-library dependencies.
This file intentionally does NOT install tools or emulate a target system.
"""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, HTTPServer
import os
from pathlib import Path
import re
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


def check_dragonfly_tools(root, work):
    """Exercise both guest setup transactions against a stale library image.

    Package records alone do not prove that the live image's shared libraries
    can load. The mock keeps libssh2 broken until it is explicitly refreshed,
    and rejects the dynamic package client during the library transition.
    """
    source = (root / 'build_scripts/dragonfly-vm-build-freebasic.sh').read_text()
    tools = work / 'dragonfly-tools'
    tools.mkdir()
    package_client = tools / 'pkg-static'
    package_client.write_text(
        '#!/bin/sh\nset -eu\n'
        'echo "static:$*" >> "$MOCK_LOG"\n'
        '[ "$1" != "$MOCK_FAIL" ] || exit 17\n'
        'for argument do\n'
        ' case "$argument" in\n'
        '  openssl|libssh2|curl|rsync) touch "$MOCK_STATE/$argument" ;;\n'
        ' esac\n'
        'done\n')
    dynamic_client = tools / 'pkg'
    dynamic_client.write_text('#!/bin/sh\necho "stale dynamic client" >&2\nexit 18\n')
    for tool in ('curl', 'rsync'):
        probe = tools / tool
        probe.write_text(
            '#!/bin/sh\nset -eu\n'
            f'echo "probe:{tool}" >> "$MOCK_LOG"\n'
            f'test -f "$MOCK_STATE/{tool}"\n'
            'test -f "$MOCK_STATE/openssl"\n'
            'test -f "$MOCK_STATE/libssh2"\n')
    for tool in tools.iterdir():
        tool.chmod(0o755)
    for phase in ('build', 'test'):
        guest = source.split(f'write_guest_{phase}_script() {{', 1)[1]
        guest = guest.split('\nEOF\n', 1)[0]
        # Execute the actual prerequisite commands, without booting a VM or
        # running the surrounding disk-formatting and package-build commands.
        block = re.search(r'^pkg-static update -f\n.*?^curl --version >/dev/null'
                          r'(?:\nrsync --version >/dev/null)?', guest,
                          re.MULTILINE | re.DOTALL)
        if block is None:
            raise RuntimeError(f'DragonFly {phase} prerequisite block is absent')
        script = work / 'dragonfly-prerequisites.sh'
        script.write_text('set -eu\n' + block.group() + '\n')
        for failure in ('none', 'update', 'install'):
            state = work / f'dragonfly-{phase}-{failure}'
            state.mkdir()
            log = state / 'commands.log'
            environment = os.environ.copy()
            environment.update(PATH=str(tools) + os.pathsep + environment['PATH'],
                               MOCK_STATE=str(state), MOCK_LOG=str(log),
                               MOCK_FAIL=failure)
            result = subprocess.run(['sh', str(script)], env=environment,
                                    text=True, capture_output=True, timeout=15)
            commands = log.read_text()
            if failure == 'none':
                if result.returncode or 'probe:curl' not in commands:
                    raise RuntimeError(f'DragonFly {phase}: {result.stderr}')
                if phase == 'test' and 'probe:rsync' not in commands:
                    raise RuntimeError('DragonFly test setup did not check rsync')
            elif result.returncode != 17 or 'probe:' in commands:
                raise RuntimeError('DragonFly package failure did not stop setup')
    print('CI_DRAGONFLY_TOOLS_PASS build,test,stale-library,update-failure,install-failure')


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
        check_dragonfly_tools(root, work)
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
