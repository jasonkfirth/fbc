#!/usr/bin/env python3
"""FreeBASIC wx-c native qualification
File: test-wxc.py
Purpose: Check the authentic i386 Windows SDK and its real GUI examples.
Responsibilities: C/FB ABI parity, native pixels, modal events and normal exit.
This file contains no widget substitute, global input injection or installation.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'sdk-prefix', 'runtime-prefix', 'tool-dir', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('this runner qualifies the publisher Windows i386 DLL')
    root = Path(__file__).resolve().parents[1]
    compiler, sdk, tools = args.fbc.resolve(), args.sdk_prefix.resolve(), args.tool_dir.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    source_names = ['build_scripts/test-wxc.py', 'tests/bindings/wxc-native.c', 'tests/bindings/wxc-native.bas',
                    'inc/wx-c/common.bi']
    source_names += [path.relative_to(root).as_posix() for path in sorted((root / 'examples/GUI/wx-c').glob('*.bas'))]
    source_hashes = {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in source_names}
    receipt_path = output / 'results.json'
    # Invalidate an earlier pass before any new load/build attempt. A running
    # receipt is not evidence that its process is alive, or that a gate passed.
    receipt_path.write_text(json.dumps({'status': 'pending', 'source_sha256': source_hashes}, indent=2) + '\n')
    library = sdk / 'bin/wx-c-0-9-0-2.dll'
    expected = '1549ef397b395b8d00bd2ad2057030fa6fce5febd3f1e8d175f56ee3c5ad303c'
    assert hashlib.sha256(library.read_bytes()).hexdigest() == expected, 'publisher DLL identity changed'
    environment = os.environ.copy()
    environment['PATH'] = str(sdk / 'bin') + os.pathsep + str(tools) + os.pathsep + environment['PATH']
    environment['WXC_NATIVE_DLL'] = str(library)
    for name in ('gcc', 'as', 'ld', 'ar'):
        environment[name.upper()] = str(tools / (name + '.exe'))

    def run(arguments, tag):
        result = subprocess.run([str(value) for value in arguments], cwd=output, env=environment,
                                capture_output=True, text=True, timeout=60)
        (output / (tag + '.log')).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(tag + '\n' + result.stdout + result.stderr)
        return result.stdout

    def require_i386(binary):
        data = binary.read_bytes()
        header = struct.unpack_from('<I', data, 0x3c)[0]
        assert data[header:header + 4] == b'PE\0\0'
        assert struct.unpack_from('<H', data, header + 4)[0] == 0x14c, binary

    native = output / 'native-c.exe'
    run([tools / 'gcc.exe', '-Wall', '-Wextra', '-Werror', root / 'tests/bindings/wxc-native.c',
         '-lgdi32', '-o', native], 'compile-c')
    require_i386(native)
    reference = run([native, library], 'native-c')
    assert 'WXC_NATIVE=PASS' in reference
    flags = [compiler, '-target', 'win32', '-prefix', args.runtime_prefix.resolve(),
             '-buildprefix', str(tools) + '/', '-i', root / 'inc', '-p', sdk / 'lib', '-w', 'all']

    # Every HWND action below first checks the exact launched process owner.
    # Posted button messages use the framework's ordinary event path, including
    # nested modal loops. They do not move the mouse or type into the desktop.
    user = c.WinDLL('user32', use_last_error=True)
    callback = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    signatures = {
        'EnumWindows': ([callback, w.LPARAM], w.BOOL),
        'EnumChildWindows': ([w.HWND, callback, w.LPARAM], w.BOOL),
        'GetWindowThreadProcessId': ([w.HWND, c.POINTER(w.DWORD)], w.DWORD),
        'GetWindowTextW': ([w.HWND, w.LPWSTR, c.c_int], c.c_int),
        'GetClassNameW': ([w.HWND, w.LPWSTR, c.c_int], c.c_int),
        'GetWindowRect': ([w.HWND, c.POINTER(w.RECT)], w.BOOL),
        'IsWindowVisible': ([w.HWND], w.BOOL),
        'IsWindowEnabled': ([w.HWND], w.BOOL),
        'GetDlgCtrlID': ([w.HWND], c.c_int),
        'PostMessageW': ([w.HWND, w.UINT, w.WPARAM, w.LPARAM], w.BOOL),
        'SendMessageTimeoutW': ([w.HWND, w.UINT, w.WPARAM, w.LPARAM, w.UINT, w.UINT, c.POINTER(c.c_size_t)], w.LPARAM),
        'SetWindowPos': ([w.HWND, w.HWND, c.c_int, c.c_int, c.c_int, c.c_int, w.UINT], w.BOOL),
    }
    for name, (parameters, result) in signatures.items():
        getattr(user, name).argtypes = parameters
        getattr(user, name).restype = result

    def owned(pid, hwnd):
        owner = w.DWORD()
        user.GetWindowThreadProcessId(hwnd, c.byref(owner))
        assert owner.value == pid, 'window ownership changed'

    def windows(pid, parent=None):
        found = []
        @callback
        def collect(hwnd, unused):
            owner = w.DWORD()
            user.GetWindowThreadProcessId(hwnd, c.byref(owner))
            if owner.value == pid:
                title, klass, rect = c.create_unicode_buffer(512), c.create_unicode_buffer(256), w.RECT()
                user.GetWindowTextW(hwnd, title, len(title))
                user.GetClassNameW(hwnd, klass, len(klass))
                assert user.GetWindowRect(hwnd, c.byref(rect))
                found.append({'hwnd': int(hwnd), 'title': title.value, 'class': klass.value,
                              'visible': bool(user.IsWindowVisible(hwnd)), 'id': user.GetDlgCtrlID(hwnd),
                              'rect': [rect.left, rect.top, rect.right, rect.bottom]})
            return True
        if parent is None:
            user.EnumWindows(collect, 0)
        else:
            user.EnumChildWindows(parent, collect, 0)
        return found

    def post(pid, hwnd, message):
        owned(pid, hwnd)
        assert user.PostMessageW(hwnd, message, 0, 0)

    def send(pid, hwnd, message, value=0):
        owned(pid, hwnd)
        result = c.c_size_t()
        # SMTO_ABORTIFHUNG bounds requests to a stuck test process.
        assert user.SendMessageTimeoutW(hwnd, message, value, 0, 2, 2000, c.byref(result))
        return result.value

    def await_condition(process, predicate, seconds=10):
        deadline = time.monotonic() + seconds
        while process.poll() is None and time.monotonic() < deadline:
            value = predicate()
            if value:
                return value
            time.sleep(.05)
        raise RuntimeError('GUI condition failed for ' + str(process.args))

    def test_gui(binary, name, tag):
        env = environment.copy()
        if name == 'native':
            env['WXC_GUI_TEST'] = '1'
        log_path = output / (tag + '-gui.log')
        evidence = {'test': 'GUI', 'case': name, 'binary': binary.name}
        with log_path.open('w') as log:
            process = subprocess.Popen([str(binary)], cwd=output, env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                title = {'native': 'wx-c native qualification', 'fonts_and_colours': 'Fonts and Colours',
                         'mdi': 'parent', 'sizers': 'Welcome to WX-C', 'wx-c_demo': 'Welcome to WX-C'}[name]
                top = await_condition(process, lambda: next((item for item in windows(process.pid)
                                     if item['visible'] and item['title'] == title), None))
                hwnd = top['hwnd']
                time.sleep(.5)
                children = windows(process.pid, hwnd)
                evidence.update({'window': top, 'children': children})
                if name == 'fonts_and_colours':
                    label = next(item for item in children if item['title'] == 'Label:' and item['visible'])
                    assert label['rect'][3] - label['rect'][1] >= 16, '12-point label was clipped'
                    assert any(item['class'] == 'Edit' and item['title'] == 'Text' for item in children)
                elif name == 'mdi':
                    mdi = [item for item in children if item['class'] == 'wxMDIChildFrameClassNR']
                    assert {item['title'] for item in mdi} == {'child_1', 'child_2', 'child_3', 'child_4'}
                    assert all(item['visible'] for item in mdi)
                elif name == 'sizers':
                    buttons = sorted((item for item in children if item['class'] == 'Button'), key=lambda item: item['id'])
                    assert [item['title'] for item in buttons] == ['Button1', 'Button2']
                    original = [item['rect'] for item in buttons]
                    owned(process.pid, hwnd)
                    assert user.SetWindowPos(hwnd, None, 0, 0, 360, 260, 2 | 4 | 16) # preserve position, order and activation
                    time.sleep(.3)
                    resized = sorted((item for item in windows(process.pid, hwnd) if item['class'] == 'Button'), key=lambda item: item['id'])
                    rectangles = [item['rect'] for item in resized]
                    assert rectangles != original
                    heights = [rect[3] - rect[1] for rect in rectangles]
                    assert min(heights) > 0 and abs(heights[0] - heights[1]) <= 1
                    assert rectangles[0][3] <= rectangles[1][1]
                    evidence['resized_buttons'] = rectangles
                elif name == 'wx-c_demo':
                    click = next(item for item in children if item['class'] == 'Button' and item['title'] == 'Click me!')
                    for cycle in range(2):
                        post(process.pid, click['hwnd'], 0xF5) # BM_CLICK
                        dialog = await_condition(process, lambda: next((item for item in windows(process.pid)
                                                 if item['visible'] and item['title'] == 'Press Exit'), None))
                        assert not user.IsWindowEnabled(hwnd), 'modal dialog did not disable its parent'
                        controls = windows(process.pid, dialog['hwnd'])
                        listing = next(item for item in controls if item['class'] == 'ListBox')
                        assert send(process.pid, listing['hwnd'], 0x18B) == 16 # LB_GETCOUNT
                        exit_button = next(item for item in controls if item['class'] == 'Button' and item['title'].replace('&', '') == 'Exit')
                        post(process.pid, exit_button['hwnd'], 0xF5)
                        await_condition(process, lambda: user.IsWindowEnabled(hwnd) and not any(item['visible']
                                        and item['title'] == 'Press Exit' for item in windows(process.pid)))
                    evidence['modal_cycles'] = 2
                    evidence['list_items'] = 16
                    exit_button = next(item for item in children if item['class'] == 'Button' and item['title'] == 'Exit')
                    post(process.pid, exit_button['hwnd'], 0xF5)
                    goodbye = await_condition(process, lambda: next((item for item in windows(process.pid)
                                               if item['visible'] and item['title'] == 'Window Closed!'), None))
                    okay = next(item for item in windows(process.pid, goodbye['hwnd'])
                                if item['class'] == 'Button' and item['title'].replace('&', '') == 'OK')
                    post(process.pid, okay['hwnd'], 0xF5)
                if name != 'wx-c_demo':
                    post(process.pid, hwnd, 0x10) # WM_CLOSE, not forced widget destruction
                assert process.wait(timeout=10) == 0, log_path.read_text()
                assert not windows(process.pid), 'test process left a window'
            finally:
                if process.poll() is None:
                    # A failed test may be hung. Terminate only this Popen child.
                    process.kill()
                    process.wait(timeout=5)
        if name == 'native':
            assert 'WXC_GUI=PASS init=1 exit=1 pixels=49 frame_destroyed=1' in log_path.read_text()
        evidence['status'] = 'pass'
        print('WXC_GUI=PASS ' + tag, flush=True)
        return evidence

    records = [test_gui(native, 'native', 'native-c')]
    for backend in ('gcc', 'gas'):
        binary = output / ('native-' + backend + '.exe')
        run([*flags, '-gen', backend, root / 'tests/bindings/wxc-native.bas', '-x', binary], 'compile-' + backend)
        require_i386(binary)
        assert run([binary], 'native-' + backend) == reference, backend
        records.append({'test': 'ABI', 'backend': backend, 'status': 'pass'})
        records.append(test_gui(binary, 'native', 'native-' + backend))
        for source in sorted((root / 'examples/GUI/wx-c').glob('*.bas')):
            binary = output / (source.stem + '-' + backend + '.exe')
            run([*flags, '-gen', backend, source, '-x', binary], 'compile-' + binary.stem)
            require_i386(binary)
            records.append(test_gui(binary, source.stem, binary.stem))
    receipt = {'status': 'pass', 'platform': 'Windows i386', 'native_C_ABI': reference.splitlines()[0],
               'native_window_pixel_comparisons': 147, 'real_example_runs': 8, 'records': records,
               'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(), 'library_sha256': expected,
               'source_sha256': source_hashes}
    assert all(hashlib.sha256((root / name).read_bytes()).hexdigest() == digest for name, digest in source_hashes.items()), 'source changed during qualification'
    receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    main()
# end of test-wxc.py
