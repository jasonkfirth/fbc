"""FreeBASIC runtime tests; File: test-notify-session.py.
Purpose: Qualify real libnotify delivery on an isolated D-Bus session.
Responsibilities: Private server lifetime, observed payloads and native API tests.
This helper does not display notifications or connect to a desktop session bus.
"""
import argparse
import json
import os
from pathlib import Path
import select
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--prefix')
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--inside', action='store_true')
    args = parser.parse_args()
    if not args.inside:
        return subprocess.run(['dbus-run-session', '--', sys.executable, '-B', __file__,
                               *sys.argv[1:], '--inside'], timeout=240).returncode
    assert os.environ.get('DBUS_SESSION_BUS_ADDRESS')
    output = args.output.resolve()
    assert not output.exists() and output.parent.is_dir()
    log = output.parent / (output.name + '-notifications.jsonl')
    server_log = output.parent / (output.name + '-server.log')
    assert not log.exists() and not server_log.exists()
    with server_log.open('xb') as errors:
        server = subprocess.Popen([sys.executable, '-B',
                                   str(ROOT / 'tests/services/notification-server.py'), str(log)],
                                  stdout=subprocess.PIPE, stderr=errors)
        try:
            assert select.select([server.stdout], [], [], 10)[0], 'notification server did not start'
            assert server.stdout.readline().strip() == b'NOTIFY_SERVER_READY', server_log.read_text()
            command = [sys.executable, '-B', str(ROOT / 'build_scripts/test-platform-services.py'),
                       '--compiler', args.compiler, '--cc', args.cc,
                       '--output', str(output), '--live-notifications']
            if args.prefix:
                command += ['--prefix', args.prefix]
            result = subprocess.run(command, timeout=180)
            assert result.returncode == 0
            rows = [json.loads(line) for line in log.read_text().splitlines()]
            notifications = [row for row in rows if row['method'] == 'Notify']
            assert len(notifications) == 2, notifications
            assert notifications[0]['title'] == 'New \u2603'
            assert notifications[0]['body'] == '&lt;b&gt;Post &amp; body&lt;/b&gt;'
            assert notifications[1]['title'] == 'Update'
            assert notifications[1]['body'] == 'Post \U0001f30d'
            assert notifications[0]['replaces_id'] == 0 and notifications[1]['replaces_id'] == 1
            assert any(row['method'] == 'CloseNotification' for row in rows)
            (output / 'notification-delivery.json').write_text(json.dumps({
                'format': 'fbc-native-notification-delivery-1', 'notifications': notifications,
                'input_refusal_verified': True, 'replacement_verified': True,
                'close_verified': True, 'desktop_presentation_tested': False}, indent=2) + '\n')
            print('NOTIFY_DBUS_DELIVERY=PASS', flush=True)
        finally:
            server.terminate()
            server.wait(timeout=5)
    # Reuse the already qualified native image against a plain-text server.
    # This catches accidental entity encoding when body-markup is absent.
    plain_log = output.parent / (output.name + '-plain-notifications.jsonl')
    plain_errors = output.parent / (output.name + '-plain-server.log')
    assert not plain_log.exists() and not plain_errors.exists()
    with plain_errors.open('xb') as errors:
        server = subprocess.Popen([sys.executable, '-B',
                                   str(ROOT / 'tests/services/notification-server.py'),
                                   str(plain_log), 'plain'],
                                  stdout=subprocess.PIPE, stderr=errors)
        try:
            assert select.select([server.stdout], [], [], 10)[0]
            assert server.stdout.readline().strip() == b'NOTIFY_SERVER_READY', plain_errors.read_text()
            result = subprocess.run([str(output / 'notify-native')], capture_output=True, timeout=30)
            (output / 'run-notify-plain.log').write_bytes(result.stdout + result.stderr)
            assert result.returncode == 0 and b'NOTIFY_NATIVE=PASS' in result.stdout
            notifications = [json.loads(line) for line in plain_log.read_text().splitlines()
                             if json.loads(line)['method'] == 'Notify']
            assert len(notifications) == 2
            assert notifications[0]['body'] == '<b>Post & body</b>'
            assert notifications[1]['body'] == 'Post \U0001f30d'
            (output / 'notification-delivery-plain.json').write_text(json.dumps({
                'format': 'fbc-native-notification-plain-1',
                'notifications': notifications, 'native_exit': result.returncode}, indent=2) + '\n')
            print('NOTIFY_DBUS_PLAIN_TEXT=PASS', flush=True)
        finally:
            server.terminate()
            server.wait(timeout=5)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
# end of test-notify-session.py
