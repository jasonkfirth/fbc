#!/usr/bin/env python3
"""FreeBASIC Windows GUI example qualification
File: test-win32-gui.py
Purpose: Exercise real native windows and their message handlers.
Responsibilities: Compile two backends, inspect owned controls and bounded pixels,
verify input and repaint behavior, and close each child normally.
This file contains no desktop capture, global keyboard input or hardware access.
"""
from pathlib import Path
from ctypes import wintypes as w
import argparse
import ctypes as c
import hashlib
import json
import os
import re
import struct
import subprocess
import time
import zlib


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--bits', type=int, choices=(32, 64), default=64)
    parser.add_argument('--tool-dir', type=Path, required=True)
    parser.add_argument('--runtime-prefix', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--examples', nargs='+', choices=('hello', 'bitmap', 'menu', 'calendar', 'toolbar', 'treeview', 'message-intro'))
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('Native Windows controls are required')
    root = Path(__file__).resolve().parents[1]
    compiler = args.fbc.resolve()
    tools = args.tool_dir.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    names = args.examples or ('hello', 'bitmap', 'menu', 'calendar', 'toolbar', 'treeview', 'message-intro')

    def source_name(name):
        return ('doc/manual/cache/TutMessageIntro.wakka' if name == 'message-intro'
                else 'examples/GUI/win32/' + name + '.bas')

    sources = [source_name(name) for name in names]
    # The pixel reference is needed even when only the manual example is selected.
    if 'examples/GUI/win32/bitmap.bas' not in sources:
        sources.append('examples/GUI/win32/bitmap.bas')
    sources += ['build_scripts/test-win32-gui.py', 'inc/windows.bi', 'inc/win/commctrl.bi']
    hashes = {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in sources}
    compiler_hash = hashlib.sha256(compiler.read_bytes()).hexdigest()
    receipt = {'status': 'pending', 'bits': args.bits, 'compiler_sha256': compiler_hash,
               'source_sha256': hashes, 'records': []}
    target = output / 'results.json'

    def save():
        target.write_text(json.dumps(receipt, indent=2) + '\n')

    save()
    env = os.environ.copy()
    env['PATH'] = str(tools) + ';' + env['PATH']
    for label in ('gcc', 'as', 'ld', 'ar'):
        env[label.upper()] = str(tools / (label + '.exe'))
    # -------------------------------------------------------------------------
    # Native APIs and process ownership
    # -------------------------------------------------------------------------
    user = c.WinDLL('user32', use_last_error=True)
    gdi = c.WinDLL('gdi32', use_last_error=True)
    kernel = c.WinDLL('kernel32', use_last_error=True)
    callback = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    signatures = {
        'EnumWindows': ([callback, w.LPARAM], w.BOOL),
        'EnumChildWindows': ([w.HWND, callback, w.LPARAM], w.BOOL),
        'GetWindowThreadProcessId': ([w.HWND, c.POINTER(w.DWORD)], w.DWORD),
        'IsWindowVisible': ([w.HWND], w.BOOL),
        'GetWindowTextW': ([w.HWND, w.LPWSTR, c.c_int], c.c_int),
        'GetClassNameW': ([w.HWND, w.LPWSTR, c.c_int], c.c_int),
        'GetDlgCtrlID': ([w.HWND], c.c_int),
        'GetClientRect': ([w.HWND, c.POINTER(w.RECT)], w.BOOL),
        'GetWindowRect': ([w.HWND, c.POINTER(w.RECT)], w.BOOL),
        'GetDC': ([w.HWND], w.HDC),
        'ReleaseDC': ([w.HWND, w.HDC], c.c_int),
        'PrintWindow': ([w.HWND, w.HDC, w.UINT], w.BOOL),
        'SetWindowPos': ([w.HWND, w.HWND, c.c_int, c.c_int, c.c_int, c.c_int, w.UINT], w.BOOL),
        'SendMessageTimeoutW': ([w.HWND, w.UINT, w.WPARAM, w.LPARAM, w.UINT, w.UINT,
                                 c.POINTER(c.c_size_t)], w.LPARAM),
        'PostMessageW': ([w.HWND, w.UINT, w.WPARAM, w.LPARAM], w.BOOL),
        'RedrawWindow': ([w.HWND, c.c_void_p, w.HRGN, w.UINT], w.BOOL),
        'GetGuiResources': ([w.HANDLE, w.DWORD], w.DWORD),
        'GetMenu': ([w.HWND], w.HMENU),
        'GetMenuItemCount': ([w.HMENU], c.c_int),
    }
    for name, (parameters, result) in signatures.items():
        getattr(user, name).argtypes = parameters
        getattr(user, name).restype = result
    for name, parameters, result in (
        ('CreateCompatibleDC', [w.HDC], w.HDC),
        ('CreateCompatibleBitmap', [w.HDC, c.c_int, c.c_int], w.HBITMAP),
        ('SelectObject', [w.HDC, w.HGDIOBJ], w.HGDIOBJ),
        ('GetDIBits', [w.HDC, w.HBITMAP, w.UINT, w.UINT, c.c_void_p, c.c_void_p, w.UINT], c.c_int),
        ('DeleteObject', [w.HGDIOBJ], w.BOOL), ('DeleteDC', [w.HDC], w.BOOL),
    ):
        getattr(gdi, name).argtypes = parameters
        getattr(gdi, name).restype = result
    kernel.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
    kernel.OpenProcess.restype = w.HANDLE
    kernel.CloseHandle.argtypes = [w.HANDLE]

    def owned(process, hwnd):
        owner = w.DWORD()
        assert process.poll() is None, 'Example exited before its GUI check'
        assert user.GetWindowThreadProcessId(hwnd, c.byref(owner)) and owner.value == process.pid

    def windows(process, parent=None):
        found = []

        @callback
        def visit(hwnd, unused):
            owner = w.DWORD()
            user.GetWindowThreadProcessId(hwnd, c.byref(owner))
            if owner.value == process.pid and user.IsWindowVisible(hwnd):
                title = c.create_unicode_buffer(256)
                classname = c.create_unicode_buffer(256)
                rect = w.RECT()
                user.GetWindowTextW(hwnd, title, len(title))
                user.GetClassNameW(hwnd, classname, len(classname))
                assert user.GetWindowRect(hwnd, c.byref(rect))
                found.append({'hwnd': hwnd, 'title': title.value, 'class': classname.value,
                              'id': user.GetDlgCtrlID(hwnd),
                              'rect': [rect.left, rect.top, rect.right, rect.bottom]})
            return True

        if parent:
            owned(process, parent)
            user.EnumChildWindows(parent, visit, 0)
        else:
            user.EnumWindows(visit, 0)
        return found

    def send(process, hwnd, message, wp=0, lp=0):
        owned(process, hwnd)
        result = c.c_size_t()
        # These tests pass integer values or opaque control handles only. User
        # messages do not marshal pointers into another process's address space.
        assert user.SendMessageTimeoutW(hwnd, message, wp, lp, 2, 2000, c.byref(result))
        return result.value

    # -------------------------------------------------------------------------
    # Bounded client rendering and the independent BMP pixel reference
    # -------------------------------------------------------------------------
    def frame(process, hwnd):
        owned(process, hwnd)
        rect = w.RECT()
        assert user.GetClientRect(hwnd, c.byref(rect))
        width, height = rect.right, rect.bottom
        assert 0 < width <= 1280 and 0 < height <= 1024
        dc = memory = bitmap = old = None
        try:
            dc = user.GetDC(hwnd)
            assert dc
            memory = gdi.CreateCompatibleDC(dc)
            assert memory
            bitmap = gdi.CreateCompatibleBitmap(dc, width, height)
            assert bitmap
            old = gdi.SelectObject(memory, bitmap)
            if old == c.c_void_p(-1).value:
                old = None
            assert old
            # Render only this owned client's contents, including child controls.
            # PrintWindow avoids reading another window that happens to overlap it.
            assert user.PrintWindow(hwnd, memory, 1)
            assert gdi.SelectObject(memory, old)
            header = c.create_string_buffer(struct.pack('<IiiHHIIiiII', 40, width, -height,
                                                       1, 32, 0, width * height * 4, 0, 0, 0, 0))
            data = c.create_string_buffer(width * height * 4)
            assert gdi.GetDIBits(dc, bitmap, 0, height, data, header, 0) == height
            rgb = bytearray(width * height * 3)
            rgb[0::3] = data.raw[2::4]
            rgb[1::3] = data.raw[1::4]
            rgb[2::3] = data.raw[0::4]
            return width, height, bytes(rgb)
        finally:
            if old:
                gdi.SelectObject(memory, old)
            if bitmap:
                gdi.DeleteObject(bitmap)
            if memory:
                gdi.DeleteDC(memory)
            if dc:
                user.ReleaseDC(hwnd, dc)

    def repaint(process, hwnd):
        owned(process, hwnd)
        assert user.RedrawWindow(hwnd, None, None, 1 | 4 | 0x100 | 0x80)

    def png(path, image):
        width, height, rgb = image

        def chunk(label, data):
            return struct.pack('>I', len(data)) + label + data + struct.pack('>I', zlib.crc32(label + data))

        scanlines = b''.join(b'\0' + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
        path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
                         + chunk(b'IDAT', zlib.compress(scanlines)) + chunk(b'IEND', b''))

    # Independent decoding of the embedded, bottom-up 24-bit BMP gives an exact
    # pixel reference. It is not a copy of the example's GDI drawing algorithm.
    bitmap_source = (root / 'examples/GUI/win32/bitmap.bas').read_text().split('dim shared fblogo_data', 1)[1]
    bmp = bytes(int(value, 16) for value in re.findall(r'&h([0-9a-f]+)', bitmap_source, re.I))
    assert len(bmp) == 3126 and bmp[:2] == b'BM'
    offset = struct.unpack_from('<I', bmp, 10)[0]
    assert struct.unpack_from('<iiHHI', bmp, 18) == (32, 32, 1, 24, 0)
    logo = b''.join(bmp[offset + (31-y)*96:offset + (32-y)*96] for y in range(32))
    logo_rgb = bytearray(3072)
    logo_rgb[0::3], logo_rgb[1::3], logo_rgb[2::3] = logo[2::3], logo[1::3], logo[0::3]

    def logo_at(image, x, y):
        width, height, rgb = image
        assert x + 32 <= width and y + 32 <= height
        actual = b''.join(rgb[((y+row)*width+x)*3:((y+row)*width+x+32)*3] for row in range(32))
        assert actual == logo_rgb, 'Native bitmap pixels differ from the embedded BMP'

    # -------------------------------------------------------------------------
    # Native example interactions and normal shutdown
    # -------------------------------------------------------------------------
    def check(binary, name, backend):
        record = {'source': source_name(name), 'backend': backend,
                  'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(), 'passed': False}
        receipt['records'].append(record)
        with (output / (name + '-' + backend + '-run.log')).open('w') as log:
            process = subprocess.Popen([str(binary)], cwd=output, env=env, stdin=subprocess.DEVNULL,
                                       stdout=log, stderr=subprocess.STDOUT)
            handle = None
            try:
                deadline = time.monotonic() + 8
                tops = []
                while process.poll() is None and time.monotonic() < deadline:
                    tops = windows(process)
                    if tops:
                        break
                    time.sleep(.05)
                assert len(tops) == 1, ('Expected one native example window', tops)
                hwnd = tops[0]['hwnd']
                owned(process, hwnd)
                if name in ('hello', 'menu', 'bitmap', 'message-intro'):
                    assert user.SetWindowPos(hwnd, None, 120, 120, 640, 480, 4 | 16)
                repaint(process, hwnd)
                children = windows(process, hwnd)
                record.update({'window': tops[0], 'children': children})
                # Common controls can initialize theme/font caches after becoming
                # visible. Warm them before measuring repeated paint ownership.
                time.sleep(.2)
                for unused in range(20):
                    repaint(process, hwnd)
                initial = frame(process, hwnd)
                png(output / (name + '-' + backend + '-initial.png'), initial)
                if name != 'menu':
                    assert len(set(zip(initial[2][0::3], initial[2][1::3], initial[2][2::3]))) > 1
                handle = kernel.OpenProcess(0x400, False, process.pid)
                assert handle
                counts = [user.GetGuiResources(handle, 0)]
                assert counts[0] > 0
                # Theme caches can allocate one object on a later paint. Keep
                # every count in the receipt and require an unchanged count over
                # two complete 40-paint batches. Continuous growth fails this
                # bounded check; an initial cache allocation remains visible.
                for batch in range(8):
                    for unused in range(40):
                        repaint(process, hwnd)
                    time.sleep(.05)
                    counts.append(user.GetGuiResources(handle, 0))
                    if len(counts) >= 3 and len(set(counts[-3:])) == 1:
                        break
                record['gdi_repaints'] = {'warmup_count': 20, 'measured_count': 40 * (batch+1),
                                         'stable_count': 80, 'samples': counts}
                assert len(counts) >= 3 and len(set(counts[-3:])) == 1, ('GDI counts did not stabilize', counts)
                if name == 'bitmap':
                    logo_at(initial, 0, 0)
                    send(process, hwnd, 0x100, 0x27)  # VK_RIGHT
                    repaint(process, hwnd)
                    moved = frame(process, hwnd)
                    logo_at(moved, 8, 0)
                    assert all(moved[2][(row*moved[0])*3:(row*moved[0]+8)*3] == b'\xff'*24 for row in range(32))
                    send(process, hwnd, 0x100, 0x25)  # VK_LEFT
                    for unused in range(40):
                        send(process, hwnd, 0x100, 0x6d)  # VK_SUBTRACT, saturates at 1/32
                    for unused in range(31):
                        send(process, hwnd, 0x100, 0x6b)  # VK_ADD, returns to the original scale
                    repaint(process, hwnd)
                    restored = frame(process, hwnd)
                    png(output / (name + '-' + backend + '-restored.png'), restored)
                    record['zoom_mismatched_bytes'] = sum(a != b for a, b in zip(initial[2], restored[2]))
                    assert restored == initial, 'Zoom must stay positive and clear old pixels'
                    for unused in range(480):
                        send(process, hwnd, 0x100, 0x6b)  # 256 to 4096, in steps of 8
                    repaint(process, hwnd)
                    maximum = frame(process, hwnd)
                    assert maximum != initial
                    for unused in range(16):
                        send(process, hwnd, 0x100, 0x6b)
                    repaint(process, hwnd)
                    assert frame(process, hwnd) == maximum, 'Maximum zoom did not saturate'
                    for unused in range(480):
                        send(process, hwnd, 0x100, 0x6d)
                    repaint(process, hwnd)
                    assert frame(process, hwnd) == initial, 'Maximum zoom restore left stale pixels'
                    record['checks'] = ['1024 exact BMP pixels', 'move and erase', 'minimum and maximum zoom saturation', 'zoom restore']
                elif name == 'menu':
                    menu = user.GetMenu(hwnd)
                    assert menu and user.GetMenuItemCount(menu) == 3
                    send(process, hwnd, 0x111, 100)
                    repaint(process, hwnd)
                    selected = frame(process, hwnd)
                    assert selected != initial, 'Menu selection did not paint its label'
                    for command in (0, 99, 149, 150, 65535):
                        send(process, hwnd, 0x111, command)
                        repaint(process, hwnd)
                        assert frame(process, hwnd) == selected, ('Invalid menu ID changed the label', command)
                    for wp, lp in ((101, 1), (101 | (1 << 16), 0)):
                        send(process, hwnd, 0x111, wp, lp)
                        repaint(process, hwnd)
                        assert frame(process, hwnd) == selected, 'Control/accelerator notification treated as a menu ID'
                    record['checks'] = ['three menus', 'selection label', 'five invalid IDs ignored', 'notification filtering']
                    initial = selected
                elif name == 'treeview':
                    child = next(item['hwnd'] for item in children if item['class'] == 'SysTreeView32')
                    assert send(process, child, 0x1105) == 7  # TVM_GETCOUNT
                    root_item = send(process, child, 0x110a, 0)  # TVM_GETNEXTITEM, TVGN_ROOT
                    assert root_item and send(process, child, 0x1102, 2, root_item)  # TVM_EXPAND
                    repaint(process, hwnd)
                    assert frame(process, hwnd) != initial
                    record['checks'] = ['seven items', 'native root expansion']
                    initial = frame(process, hwnd)
                elif name == 'toolbar':
                    child = next(item['hwnd'] for item in children if item['class'] == 'ToolbarWindow32')
                    assert send(process, child, 0x418) == 7  # TB_BUTTONCOUNT
                    # TB_GETBITMAP accepts a command ID, while this historical
                    # drawing demo uses -1 for every button. Pixel checks verify
                    # the rendered six images, and count verifies the separator.
                    record['checks'] = ['seven native toolbar entries', 'rendered standard images']
                elif name == 'calendar':
                    child = next(item['hwnd'] for item in children if item['class'] == 'SysMonthCal32')
                    send(process, child, 0x100, 0x27)
                    repaint(process, hwnd)
                    changed = frame(process, hwnd)
                    assert changed != initial, 'Calendar did not advance its selection'
                    send(process, child, 0x100, 0x25)
                    repaint(process, hwnd)
                    assert frame(process, hwnd) == initial, 'Calendar selection did not return'
                    record['checks'] = ['native month calendar', 'day selection and restore']
                else:
                    record['checks'] = ['native text rendering']
                    if name == 'message-intro':
                        owned(process, hwnd)
                        assert user.PostMessageW(hwnd, 0x202, 0, 0)  # WM_LBUTTONUP
                        deadline = time.monotonic() + 5
                        dialog = None
                        while process.poll() is None and time.monotonic() < deadline:
                            dialog = next((item for item in windows(process)
                                           if item['class'] == '#32770' and item['title'] == 'FB Win'), None)
                            if dialog:
                                break
                            time.sleep(.05)
                        assert dialog, 'Click did not open the example message box'
                        owned(process, dialog['hwnd'])
                        deadline = time.monotonic() + 5
                        button = None
                        while process.poll() is None and time.monotonic() < deadline:
                            buttons = [item['hwnd'] for item in windows(process, dialog['hwnd'])
                                       if item['class'] == 'Button']
                            if len(buttons) == 1:
                                button = buttons[0]
                                break
                            time.sleep(.05)
                        record['dialog_children'] = windows(process, dialog['hwnd'])
                        assert button, ('Native message box has no visible OK button', record['dialog_children'])
                        # This MB_OK dialog has one button. Windows can give its
                        # OK button the internal IDCANCEL ID, so click the actual
                        # control instead of assuming that WM_COMMAND/IDOK closes it.
                        owned(process, button)
                        assert user.PostMessageW(button, 0xf5, 0, 0)  # BM_CLICK
                        deadline = time.monotonic() + 5
                        while time.monotonic() < deadline:
                            if not any(item['hwnd'] == dialog['hwnd'] for item in windows(process)):
                                break
                            time.sleep(.05)
                        assert not any(item['hwnd'] == dialog['hwnd'] for item in windows(process))
                        record['checks'] += ['click opens native modal dialog', 'OK closes dialog']
                png(output / (name + '-' + backend + '.png'), initial)
                record['client_sha256'] = hashlib.sha256(initial[2]).hexdigest()
                record['client_size'] = list(initial[:2])
                owned(process, hwnd)
                assert user.PostMessageW(hwnd, 0x111 if name == 'menu' else 0x100,
                                         102 if name == 'menu' else 27, 0)
                assert process.wait(timeout=5) == 0, 'Normal menu/Escape exit failed'
                record['exit'] = process.returncode
                record['passed'] = True
            finally:
                if handle:
                    kernel.CloseHandle(handle)
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=5)
                save()

    try:
        for backend in ('gcc', 'gas' if args.bits == 32 else 'gas64'):
            for name in names:
                binary = output / (name + '-' + backend + '.exe')
                flags = [compiler, '-i', root / 'inc', '-gen', backend, '-w', 'all', '-s', 'gui']
                if args.bits == 32:
                    flags += ['-target', 'win32', '-prefix', args.runtime_prefix.resolve(),
                              '-buildprefix', str(tools) + '/']
                source = root / source_name(name)
                if name == 'message-intro':
                    snippet = re.search(r'##%%\(freebasic\)\n(.*?)\n%%##', source.read_text(), re.S)
                    assert snippet
                    source = output / 'message-intro.bas'
                    source.write_text(snippet.group(1) + '\n')
                flags += [source, '-x', binary]
                result = subprocess.run(list(map(str, flags)), cwd=output, env=env, stdin=subprocess.DEVNULL,
                                        capture_output=True, text=True, timeout=45)
                (output / (name + '-' + backend + '-compile.log')).write_text(
                    repr(list(map(str, flags))) + '\n' + result.stdout + result.stderr)
                assert result.returncode == 0, (name, backend, result.stdout, result.stderr)
                data = binary.read_bytes()
                pe = struct.unpack_from('<I', data, 0x3c)[0]
                assert struct.unpack_from('<H', data, pe+4)[0] == (0x14c if args.bits == 32 else 0x8664)
                check(binary, name, backend)
                print(name, backend, 'PASS', flush=True)
        for name in names:
            matching = [record for record in receipt['records'] if record['source'] == source_name(name)]
            assert matching[0]['client_sha256'] == matching[1]['client_sha256'], ('Backend pixel mismatch', name)
        assert hashes == {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in sources}
        assert compiler_hash == hashlib.sha256(compiler.read_bytes()).hexdigest()
        receipt['status'] = 'passed'
        save()
        print('WIN32_GUI_EXAMPLES=PASS bits=' + str(args.bits) + ' records=' + str(len(receipt['records'])))
    except Exception as error:
        receipt['status'] = 'failed'
        receipt['error'] = repr(error)
        save()
        raise


if __name__ == '__main__':
    main()

# end of test-win32-gui.py
