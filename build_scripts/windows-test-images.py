"""FreeBASIC private native image checks
File: windows-test-images.py
Purpose: Decode test files with Windows GDI+ and inspect owned example clients.
Responsibilities: Independent decoding, bounded client pixels, normal key exits.
This file contains no desktop capture, global keyboard input or image editing.
"""
import ctypes as c
from ctypes import wintypes as w
import subprocess
import time


class Startup(c.Structure):
    _fields_ = [('version', w.UINT), ('callback', c.c_void_p), ('background', w.BOOL), ('external', w.BOOL)]


class BitmapData(c.Structure):
    _fields_ = [('width', w.UINT), ('height', w.UINT), ('stride', c.c_int), ('format', c.c_int),
                ('pixels', c.c_void_p), ('reserved', c.c_size_t)]


class Images:
    def __init__(self):
        self.plus = c.WinDLL('gdiplus')
        self.user = c.WinDLL('user32', use_last_error=True)
        self.gdi = c.WinDLL('gdi32', use_last_error=True)
        self.callback = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
        signatures = {
            self.plus: {
                'GdiplusStartup': ([c.POINTER(c.c_size_t), c.POINTER(Startup), c.c_void_p], c.c_int),
                'GdiplusShutdown': ([c.c_size_t], None),
                'GdipCreateBitmapFromFile': ([w.LPCWSTR, c.POINTER(c.c_void_p)], c.c_int),
                'GdipGetImageWidth': ([c.c_void_p, c.POINTER(w.UINT)], c.c_int),
                'GdipGetImageHeight': ([c.c_void_p, c.POINTER(w.UINT)], c.c_int),
                'GdipBitmapLockBits': ([c.c_void_p, c.c_void_p, w.UINT, c.c_int, c.POINTER(BitmapData)], c.c_int),
                'GdipBitmapUnlockBits': ([c.c_void_p, c.POINTER(BitmapData)], c.c_int),
                'GdipDisposeImage': ([c.c_void_p], c.c_int),
            },
            self.user: {
                'EnumWindows': ([self.callback, w.LPARAM], w.BOOL),
                'GetWindowThreadProcessId': ([w.HWND, c.POINTER(w.DWORD)], w.DWORD),
                'IsWindowVisible': ([w.HWND], w.BOOL),
                'GetClientRect': ([w.HWND, c.POINTER(w.RECT)], w.BOOL),
                'GetDC': ([w.HWND], w.HDC), 'ReleaseDC': ([w.HWND, w.HDC], c.c_int),
                'PrintWindow': ([w.HWND, w.HDC, w.UINT], w.BOOL),
                'PostMessageW': ([w.HWND, w.UINT, w.WPARAM, w.LPARAM], w.BOOL),
            },
            self.gdi: {
                'CreateCompatibleDC': ([w.HDC], w.HDC), 'DeleteDC': ([w.HDC], w.BOOL),
                'CreateCompatibleBitmap': ([w.HDC, c.c_int, c.c_int], w.HBITMAP),
                'SelectObject': ([w.HDC, w.HGDIOBJ], w.HGDIOBJ), 'DeleteObject': ([w.HGDIOBJ], w.BOOL),
                'GetDIBits': ([w.HDC, w.HBITMAP, w.UINT, w.UINT, c.c_void_p, c.c_void_p, w.UINT], c.c_int),
            },
        }
        for library, declarations in signatures.items():
            for name, (parameters, result) in declarations.items():
                function = getattr(library, name)
                function.argtypes, function.restype = parameters, result
        self.token = c.c_size_t()
        assert self.plus.GdiplusStartup(c.byref(self.token), c.byref(Startup(1, None, False, False)), None) == 0

    def close(self):
        self.plus.GdiplusShutdown(self.token)

    def decode(self, path):
        bitmap = c.c_void_p()
        data = BitmapData()
        locked = False
        assert self.plus.GdipCreateBitmapFromFile(str(path), c.byref(bitmap)) == 0, path
        try:
            width, height = w.UINT(), w.UINT()
            assert self.plus.GdipGetImageWidth(bitmap, c.byref(width)) == 0
            assert self.plus.GdipGetImageHeight(bitmap, c.byref(height)) == 0
            assert 0 < width.value <= 4096 and 0 < height.value <= 4096
            # PixelFormat32bppARGB is 0x26200a; ImageLockModeRead is 1.
            assert self.plus.GdipBitmapLockBits(bitmap, None, 1, 0x26200a, c.byref(data)) == 0
            locked = True
            assert data.pixels and abs(data.stride) >= width.value*4
            pixels = b''.join(c.string_at(data.pixels+y*data.stride, width.value*4) for y in range(height.value))
            return width.value, height.value, pixels
        finally:
            if locked:
                assert self.plus.GdipBitmapUnlockBits(bitmap, c.byref(data)) == 0
            assert self.plus.GdipDisposeImage(bitmap) == 0

    def owned(self, process, hwnd):
        pid = w.DWORD()
        assert process.poll() is None
        assert self.user.GetWindowThreadProcessId(hwnd, c.byref(pid)) and pid.value == process.pid

    def window(self, process):
        found = []

        @self.callback
        def visit(hwnd, unused):
            pid = w.DWORD()
            self.user.GetWindowThreadProcessId(hwnd, c.byref(pid))
            if pid.value == process.pid and self.user.IsWindowVisible(hwnd):
                found.append(hwnd)
            return True

        assert self.user.EnumWindows(visit, 0)
        assert len(found) <= 1, ('Unexpected extra owned window', found)
        return found[0] if found else None

    def frame(self, process, hwnd):
        import struct
        self.owned(process, hwnd)
        rect = w.RECT()
        assert self.user.GetClientRect(hwnd, c.byref(rect))
        width, height = rect.right, rect.bottom
        assert 0 < width <= 1280 and 0 < height <= 1024
        dc = memory = bitmap = old = None
        try:
            dc = self.user.GetDC(hwnd)
            assert dc
            memory = self.gdi.CreateCompatibleDC(dc)
            assert memory
            bitmap = self.gdi.CreateCompatibleBitmap(dc, width, height)
            assert bitmap
            old = self.gdi.SelectObject(memory, bitmap)
            assert old and old != c.c_void_p(-1).value
            assert self.user.PrintWindow(hwnd, memory, 3)
            assert self.gdi.SelectObject(memory, old)
            header = c.create_string_buffer(struct.pack('<IiiHHIIiiII', 40, width, -height, 1, 32, 0,
                                                       width*height*4, 0, 0, 0, 0))
            pixels = c.create_string_buffer(width*height*4)
            assert self.gdi.GetDIBits(dc, bitmap, 0, height, pixels, header, 0) == height
            return width, height, pixels.raw
        finally:
            if old and old != c.c_void_p(-1).value:
                self.gdi.SelectObject(memory, old)
            if bitmap:
                self.gdi.DeleteObject(bitmap)
            if memory:
                self.gdi.DeleteDC(memory)
            if dc:
                self.user.ReleaseDC(hwnd, dc)

    def run_graphics(self, command, directory, env, references, log_file, sdl=False):
        """References are (client x, y, width, height, top-down BGRA, channel tolerance)."""
        with log_file.open('wb') as log:
            process = subprocess.Popen(list(map(str, command)), cwd=directory, env=env,
                                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic()+10
                matched = False
                differences = []
                while process.poll() is None and time.monotonic() < deadline:
                    hwnd = self.window(process)
                    if hwnd:
                        width, height, actual = self.frame(process, hwnd)
                        differences = []
                        for x, y, rw, rh, pixels, tolerance in references:
                            assert 0 <= x and 0 <= y and x+rw <= width and y+rh <= height
                            region = b''.join(actual[((y+row)*width+x)*4:((y+row)*width+x+rw)*4] for row in range(rh))
                            delta = max(abs(region[index]-pixels[index]) for index in range(len(region)) if index % 4 != 3)
                            differences.append(delta)
                        matched = all(delta <= ref[-1] for delta, ref in zip(differences, references))
                        if matched:
                            break
                    time.sleep(.05)
                assert matched, ('Original client pixels differ', command, process.poll(), differences)
                self.owned(process, hwnd)
                # Post normal keys to this child's window only. FB SLEEP consumes
                # WM_CHAR; SDL's event loop consumes Escape's key down/up messages.
                key = 27 if sdl else 13
                assert self.user.PostMessageW(hwnd, 0x100, key, 1)
                if not sdl:
                    assert self.user.PostMessageW(hwnd, 0x102, key, 1)
                assert self.user.PostMessageW(hwnd, 0x101, key, 0xc0000001)
                status = process.wait(timeout=10)
                assert status == 0, (command, status)
                return {'exit': status, 'client_size': [width, height],
                        'pixels_compared': sum(ref[2]*ref[3] for ref in references),
                        'maximum_channel_differences': differences, 'normal_owned_keypress': True}
            finally:
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=5)
# end of windows-test-images.py
