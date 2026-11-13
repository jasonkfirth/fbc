#!/usr/bin/env python3
"""Project: FreeBASIC CI platform regression checks
File: test-ci-platform-contracts.py
Purpose: Check analyzer models and small native platform contracts.
Responsibilities: Retain data-flow diagnostics, reject CE precision loss,
exercise the JavaScript path provider, verify dependency mirror fallback,
check guest package setup against stale shared-library dependencies, and
retain independent Haiku/FreeBSD test phases and bounded timeout diagnostics.
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


def check_haiku_phases(root, work):
    """Execute the guest's phase dispatch and collector without a VM.

    A separate shell is essential here: an if-tested shell function loses
    errexit inside its body. Each mock deliberately has work after a failing
    command so the check detects that failure being swallowed.
    """
    source = (root / 'build_scripts/haiku-vm-build-freebasic.sh').read_text()
    guest = source.split("cat > \"$path\" <<'EOF'\n", 1)[1].split('\nEOF\n', 1)[0]
    dispatch = guest.split('case "${1:-}" in\n', 1)[1]
    dispatch = 'case "${1:-}" in\n' + dispatch.split('\nesac\n', 1)[0] + '\nesac\n'
    collector = guest.split('\ntest_status=0\n', 1)[1]
    collector = 'test_status=0\n' + collector
    script = work / 'haiku-phases.sh'
    script.write_text(
        '#!/bin/sh\nset -e\n'
        'fail() { echo "ERROR: $*" >&2; exit 1; }\n'
        'run() { "$@"; }\n'
        'touch() { echo fallback >> "$MOCK_LOG"; }\n'
        'ensure_python3() { return "$MOCK_PYTHON_STATUS"; }\n'
        'phase_body() { echo "$1" >> "$MOCK_LOG"; return "$2"; }\n'
        'run_package_smokes() { phase_body smoke "$MOCK_SMOKE_STATUS"; '
        'echo smoke-finished >> "$MOCK_LOG"; }\n'
        'run_fbctests() { phase_body fbctests "$MOCK_FBCTESTS_STATUS"; '
        'echo fbctests-finished >> "$MOCK_LOG"; }\n'
        'run_exampleageddon() { phase_body exampleageddon "$MOCK_EXAMPLES_STATUS"; '
        'echo exampleageddon-finished >> "$MOCK_LOG"; }\n' + dispatch + collector)
    for mask in range(8):
        log = work / 'haiku-phases.log'
        log.unlink(missing_ok=True)
        phases = ('smoke', 'fbctests', 'exampleageddon')
        statuses = [17 if mask & (1 << index) else 0 for index in range(3)]
        environment = os.environ.copy()
        environment.update(MOCK_LOG=str(log), MOCK_PYTHON_STATUS='0',
                           MOCK_SMOKE_STATUS=str(statuses[0]),
                           MOCK_FBCTESTS_STATUS=str(statuses[1]),
                           MOCK_EXAMPLES_STATUS=str(statuses[2]))
        result = subprocess.run(['sh', str(script)], env=environment,
                                text=True, capture_output=True, timeout=15)
        expected = []
        for phase, status in zip(phases, statuses):
            expected.append(phase)
            if not status:
                expected.append(phase + '-finished')
            elif f'{phase} failed with status 17' not in result.stderr:
                raise RuntimeError('Haiku collector lost a phase exit status')
        if log.read_text().splitlines() != expected or result.returncode != bool(mask):
            raise RuntimeError(f'Haiku phases stopped early or hid failure: {mask}\n'
                               f'{result.stdout}{result.stderr}')
        if ('TEST PASSED' in result.stdout) != (mask == 0):
            raise RuntimeError('Haiku collector reported false success')
    # Python fallback must remain available after a failed smoke test.
    log.unlink()
    environment.update(MOCK_SMOKE_STATUS='17', MOCK_FBCTESTS_STATUS='0',
                       MOCK_EXAMPLES_STATUS='0', MOCK_PYTHON_STATUS='1')
    result = subprocess.run(['sh', str(script)], env=environment,
                            text=True, capture_output=True, timeout=15)
    if result.returncode != 1 or log.read_text().splitlines() != [
            'smoke', 'fbctests', 'fbctests-finished', 'fallback']:
        raise RuntimeError('Haiku smoke failure suppressed Python fallback')

    tcp = guest.split('echo "==> running TCP loopback smoke"\n', 1)[1]
    tcp = tcp.split('echo "==> compiling thread-local initialization smoke"', 1)[0]
    script.write_text('#!/bin/sh\nset -e\n'
                      'fail() { echo "ERROR: $*" >&2; exit 1; }\n'
                      'run_limited() { echo "limit=$1 command=$2"; '
                      'return "$MOCK_TCP_STATUS"; }\n' + tcp)
    cases = ((None, 0, 600), ('900', 0, 900), ('600', 124, 600),
             ('600', 23, 600), ('0', 0, None), ('invalid', 0, None))
    for value, status, expected_limit in cases:
        environment = os.environ.copy()
        environment.pop('HAIKU_TCP_SMOKE_TIMEOUT', None)
        environment['MOCK_TCP_STATUS'] = str(status)
        if value is not None:
            environment['HAIKU_TCP_SMOKE_TIMEOUT'] = value
        result = subprocess.run(['sh', str(script)], env=environment,
                                text=True, capture_output=True, timeout=15)
        if (result.returncode == 0) != (expected_limit is not None and status == 0):
            raise RuntimeError('Haiku TCP timeout hid a failure or accepted an invalid limit')
        if expected_limit is not None:
            if f'limit={expected_limit} command=/Work/smoke/tcp' not in result.stdout:
                raise RuntimeError('Haiku TCP timeout did not use the configured bound')
            if status and f'status {status} (limit {expected_limit}s)' not in result.stderr:
                raise RuntimeError('Haiku TCP failure lost its status or timeout bound')
        elif 'limit=' in result.stdout:
            raise RuntimeError('Haiku TCP command ran with an invalid limit')
    # Exercise the real host failure handler too. A failed guest phase must
    # still run requested host-driven examples, without changing the failure
    # into success even if that independent example run passes.
    host = source.split('test_package_in_vm() {\n', 1)[1]
    host = 'test_package_in_vm() {\n' + host.split('\nverify_audio_capture() {', 1)[0]
    script.write_text(
        '#!/bin/bash\nset -e\n'
        'msg() { :; }\nwarn() { echo "$*" >&2; }\n'
        'die() { echo "$*" >&2; exit 1; }\n'
        'format_and_mount_work() { :; }\n'
        'install_test_staging_dependencies() { :; }\n'
        'scp_to_guest() { :; }\nscp_from_guest() { :; }\n'
        'send_tests_tree() { :; }\nsend_exampleageddon_tree() { :; }\n'
        'write_test_runner() { :; }\n'
        'ssh_guest() { case "$3" in\n'
        ' FBCTESTS_JOBS=*) cat >/dev/null; return 17 ;;\n'
        ' "test -f /Work/exampleageddon-needs-host") return "$MOCK_FALLBACK" ;;\n'
        ' *) return 0 ;; esac; }\n'
        'run_host_exampleageddon() { echo host-examples >> "$MOCK_LOG"; '
        'return "$MOCK_HOST_STATUS"; }\n'
        'LOG_DIR="$MOCK_WORK"\n' + host +
        '\ntest_package_in_vm "$MOCK_WORK" 10022 fixture.hpkg\n')
    for fallback, status in ((0, 0), (0, 19), (1, 0)):
        log.unlink(missing_ok=True)
        environment = os.environ.copy()
        environment.update(MOCK_WORK=str(work), MOCK_LOG=str(log),
                           MOCK_FALLBACK=str(fallback), MOCK_HOST_STATUS=str(status))
        result = subprocess.run(['bash', str(script)], env=environment,
                                text=True, capture_output=True, timeout=15)
        if result.returncode != 1 or log.exists() != (fallback == 0):
            raise RuntimeError('Haiku failure handler suppressed fallback or hid failure')
        if log.exists() and log.read_text().splitlines() != ['host-examples']:
            raise RuntimeError('Haiku failure handler repeated host-driven examples')
    print('CI_HAIKU_PHASES_PASS combinations=8 python-fallback timeout-cases=6 host-cases=3')


def check_freebsd_phases(root, work):
    """Keep each real guest phase fatal while collecting independent suites."""
    source = (root / 'build_scripts/freebsd-vm-build-freebasic.sh').read_text()
    guest = source.split("cat > \"$path\" <<'EOF'\n", 1)[1].split('\nEOF\n', 1)[0]
    dispatch = 'case "${1:-}" in\n' + guest.split('case "${1:-}" in\n', 1)[1].split('\nesac\n', 1)[0] + '\nesac\n'
    setup = guest.split('# Package setup and compiler verification', 1)[1]
    setup = '# Package setup and compiler verification' + setup.split('\ntest_status=0\n', 1)[0]
    collector = 'test_status=0\n' + guest.split('\ntest_status=0\n', 1)[1]
    script = work / 'freebsd-phases.sh'
    script.write_text(
        '#!/usr/bin/env bash\nset -euo pipefail\n'
        'fail() { echo "ERROR: $*" >&2; exit 1; }\n'
        'run() { "$@"; }\nstart_xvfb() { :; }\nstop_xvfb() { :; }\n'
        'su() { return "$MOCK_SETUP_STATUS"; }\nfbc() { :; }\n'
        'phase_body() { echo "$1" >> "$MOCK_LOG"; return "$2"; }\n'
        'run_package_smokes() { phase_body smoke "$MOCK_SMOKE_STATUS"; '
        'echo smoke-finished >> "$MOCK_LOG"; }\n'
        'run_fbctests() { phase_body fbctests "$MOCK_FBCTESTS_STATUS"; '
        'echo fbctests-finished >> "$MOCK_LOG"; }\n'
        'run_exampleageddon() { phase_body exampleageddon "$MOCK_EXAMPLES_STATUS"; '
        'echo exampleageddon-finished >> "$MOCK_LOG"; }\n' + dispatch + setup + collector)
    phases = ('smoke', 'fbctests', 'exampleageddon')
    log = work / 'freebsd-phases.log'
    for mask in range(8):
        log.unlink(missing_ok=True)
        statuses = [17 if mask & (1 << index) else 0 for index in range(3)]
        environment = os.environ.copy()
        environment.update(MOCK_LOG=str(log), MOCK_SETUP_STATUS='0', CXX='mock-cxx',
                           MOCK_SMOKE_STATUS=str(statuses[0]),
                           MOCK_FBCTESTS_STATUS=str(statuses[1]),
                           MOCK_EXAMPLES_STATUS=str(statuses[2]))
        result = subprocess.run(['bash', str(script)], env=environment,
                                text=True, capture_output=True, timeout=15)
        expected = []
        for phase, status in zip(phases, statuses):
            expected.append(phase)
            if not status:
                expected.append(phase + '-finished')
            elif f'{phase} failed with status 17' not in result.stderr:
                raise RuntimeError('FreeBSD collector lost a phase exit status')
        if log.read_text().splitlines() != expected or result.returncode != bool(mask):
            raise RuntimeError(f'FreeBSD phases stopped early or hid failure: {mask}\n'
                               f'{result.stdout}{result.stderr}')
        if ('TEST PASSED' in result.stdout) != (mask == 0):
            raise RuntimeError('FreeBSD collector reported false success')
    log.unlink()
    environment['MOCK_SETUP_STATUS'] = '19'
    result = subprocess.run(['bash', str(script)], env=environment,
                            text=True, capture_output=True, timeout=15)
    if result.returncode != 19 or log.exists():
        raise RuntimeError('FreeBSD package setup failure did not stop qualification')

    tcp = guest.split('echo "==> running TCP loopback smoke"\n', 1)[1].split('\nstart_xvfb\n', 1)[0]
    script.write_text('#!/usr/bin/env bash\nset -euo pipefail\n'
                      'fail() { echo "ERROR: $*" >&2; exit 1; }\n'
                      'timeout() { echo "timeout=$*"; return "$MOCK_TCP_STATUS"; }\n' + tcp)
    cases = ((None, 0, 600), ('900', 0, 900), ('600', 124, 600),
             ('600', 23, 600), ('0', 0, None), ('invalid', 0, None))
    for value, status, expected_limit in cases:
        environment = os.environ.copy()
        environment.pop('FREEBSD_TCP_SMOKE_TIMEOUT', None)
        environment['MOCK_TCP_STATUS'] = str(status)
        if value is not None:
            environment['FREEBSD_TCP_SMOKE_TIMEOUT'] = value
        result = subprocess.run(['bash', str(script)], env=environment,
                                text=True, capture_output=True, timeout=15)
        if (result.returncode == 0) != (expected_limit is not None and status == 0):
            raise RuntimeError('FreeBSD TCP timeout hid a failure or accepted an invalid limit')
        if expected_limit is not None:
            if f'timeout=-k 5 {expected_limit} /work/smoke/tcp' not in result.stdout:
                raise RuntimeError('FreeBSD TCP timeout lost its termination bound')
            if status and f'status {status} (limit {expected_limit}s)' not in result.stderr:
                raise RuntimeError('FreeBSD TCP failure lost its status or limit')
        elif 'timeout=' in result.stdout:
            raise RuntimeError('FreeBSD TCP command ran with an invalid limit')
    print('CI_FREEBSD_PHASES_PASS combinations=8 setup-failure timeout-cases=6')


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
        check_haiku_phases(root, work)
        check_freebsd_phases(root, work)
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
