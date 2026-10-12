"""Read and acknowledge only a freshly created hidden test console.
File: windows-test-console.py
Responsibilities: Isolated PID checks, bounded text and normal keypress exit.
This file contains no global keyboard input or inspection of user consoles.
"""
import ctypes,os,subprocess,time
from ctypes import wintypes

kernel=ctypes.WinDLL('kernel32',use_last_error=True)
kernel.GetConsoleWindow.restype=wintypes.HWND
kernel.GetStdHandle.argtypes=[wintypes.DWORD]; kernel.GetStdHandle.restype=wintypes.HANDLE
kernel.SetStdHandle.argtypes=[wintypes.DWORD,wintypes.HANDLE]
user=ctypes.WinDLL('user32')
user.MapVirtualKeyW.argtypes=[wintypes.UINT,wintypes.UINT]; user.MapVirtualKeyW.restype=wintypes.UINT
class Coord(ctypes.Structure): _fields_=[('x',ctypes.c_short),('y',ctypes.c_short)]
class Rect(ctypes.Structure): _fields_=[('left',ctypes.c_short),('top',ctypes.c_short),('right',ctypes.c_short),('bottom',ctypes.c_short)]
class BufferInfo(ctypes.Structure): _fields_=[('size',Coord),('cursor',Coord),('attributes',wintypes.WORD),('window',Rect),('maximum',Coord)]
class KeyChar(ctypes.Union): _fields_=[('unicode',wintypes.WCHAR),('ascii',ctypes.c_char)]
class KeyEvent(ctypes.Structure): _fields_=[('down',wintypes.BOOL),('repeat',wintypes.WORD),('key',wintypes.WORD),('scan',wintypes.WORD),('character',KeyChar),('control',wintypes.DWORD)]
class Event(ctypes.Union): _fields_=[('key',KeyEvent),('storage',ctypes.c_byte*16)]
class Input(ctypes.Structure): _fields_=[('kind',wintypes.WORD),('event',Event)]
assert ctypes.sizeof(Input)==20
kernel.AttachConsole.argtypes=[wintypes.DWORD]; kernel.AttachConsole.restype=wintypes.BOOL
kernel.GetConsoleProcessList.argtypes=[ctypes.POINTER(wintypes.DWORD),wintypes.DWORD]
kernel.CreateFileW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,ctypes.c_void_p,wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]
kernel.CreateFileW.restype=wintypes.HANDLE
kernel.CloseHandle.argtypes=[wintypes.HANDLE]
kernel.GetConsoleScreenBufferInfo.argtypes=[wintypes.HANDLE,ctypes.POINTER(BufferInfo)]
kernel.ReadConsoleOutputCharacterW.argtypes=[wintypes.HANDLE,wintypes.LPWSTR,wintypes.DWORD,Coord,ctypes.POINTER(wintypes.DWORD)]
kernel.WriteConsoleInputW.argtypes=[wintypes.HANDLE,ctypes.POINTER(Input),wintypes.DWORD,ctypes.POINTER(wintypes.DWORD)]

def run_console(command,cwd,environment,timeout=10,hold_console=False):
    # Serial use from a headless helper keeps any user's existing console
    # connection outside this function. Restore its redirected handles later.
    if kernel.GetConsoleWindow(): raise RuntimeError('Run private console checks from a headless helper process')
    standard_handles={label:kernel.GetStdHandle(label) for label in (-10,-11,-12)}
    if hold_console:
        # A fast example can exit before AttachConsole. A private cmd wrapper
        # retains its buffer, preserves its exit status, and emits no text.
        # Only a single trusted executable path is accepted for this wrapper.
        if len(command)!=1 or any(ch in str(command[0]) for ch in '&|<>^()%!"\r\n'):
            raise ValueError('Console wrapper requires a single literal executable path')
        from pathlib import Path
        launcher=Path(cwd)/'private-console-launch.cmd'
        launcher.write_text('@echo off\n"'+str(command[0])+'"\nset "fbc_test_exit=%errorlevel%"\npause >nul\nexit /b %fbc_test_exit%\n')
        command=[environment.get('COMSPEC','C:/Windows/System32/cmd.exe'),'/d','/c',str(launcher)]
    startup=subprocess.STARTUPINFO(); startup.dwFlags=subprocess.STARTF_USESHOWWINDOW; startup.wShowWindow=0
    # Omit redirected handles so the child gets its own real console input.
    child=subprocess.Popen([str(part) for part in command],cwd=cwd,env=environment,
                           startupinfo=startup,creationflags=subprocess.CREATE_NEW_CONSOLE)
    attached=False; handles=[]
    try:
        kernel.FreeConsole() # Detach this helper only, never close another process's console.
        deadline=time.monotonic()+timeout
        while not kernel.AttachConsole(child.pid):
            if child.poll() is not None or time.monotonic()>deadline: raise RuntimeError('Private console attach failed')
            time.sleep(0.01)
        attached=True
        pids=(wintypes.DWORD*8)(); count=kernel.GetConsoleProcessList(pids,8)
        if hold_console:
            # Wait until the native example has exited and only its wrapper
            # and this observer remain. Never send a key to an unknown peer.
            while count!=2 and time.monotonic()<deadline and child.poll() is None:
                time.sleep(0.01)
                count=kernel.GetConsoleProcessList(pids,8)
        if count!=2 or set(pids[:count])!={os.getpid(),child.pid}: raise RuntimeError('Console is not isolated to this test')
        # GENERIC_READ|WRITE, FILE_SHARE_READ|WRITE, OPEN_EXISTING.
        for name in ('CONIN$','CONOUT$'):
            handle=kernel.CreateFileW(name,0xC0000000,3,None,3,0,None)
            if handle in (None,ctypes.c_void_p(-1).value): raise RuntimeError('Console handle acquisition failed')
            handles.append(handle)
        # KEY_EVENT is 1, VK_RETURN is 13. Queue a normal press/release in
        # this child's input buffer; nothing is sent to the user desktop.
        events=(Input*2)()
        for index in range(2):
            events[index].kind=1; events[index].event.key.down=index==0
            events[index].event.key.repeat=1; events[index].event.key.key=13
            events[index].event.key.scan=user.MapVirtualKeyW(13,0)
            events[index].event.key.character.unicode='\r'
        count=kernel.GetConsoleProcessList(pids,8)
        if count!=2 or set(pids[:count])!={os.getpid(),child.pid} or child.poll() is not None: raise RuntimeError('Console owner changed')
        written=wintypes.DWORD()
        attempts=5 if hold_console else 1
        for attempt in range(attempts):
            count=kernel.GetConsoleProcessList(pids,8)
            if count!=2 or set(pids[:count])!={os.getpid(),child.pid}: raise RuntimeError('Console owner changed')
            if not kernel.WriteConsoleInputW(handles[0],events,2,ctypes.byref(written)) or written.value!=2: raise RuntimeError('Private keypress failed')
            if not hold_console: break
            # cmd's PAUSE flushes input on entry. A key queued just before
            # that point can be discarded; retry only in the same owned buffer.
            try:
                child.wait(timeout=0.2)
                break
            except subprocess.TimeoutExpired:
                pass
        child.wait(timeout=timeout)
        info=BufferInfo()
        if not kernel.GetConsoleScreenBufferInfo(handles[1],ctypes.byref(info)): raise RuntimeError('Console bounds unavailable')
        length=(info.cursor.y+1)*info.size.x
        if not 1<=length<=40000 or not 1<=info.size.x<=320: raise RuntimeError('Console text exceeds bounded capture')
        text=ctypes.create_unicode_buffer(length+1); read=wintypes.DWORD()
        if not kernel.ReadConsoleOutputCharacterW(handles[1],text,length,Coord(0,0),ctypes.byref(read)) or read.value!=length: raise RuntimeError('Console text read failed')
        output='\n'.join(text.value[i:i+info.size.x].rstrip() for i in range(0,length,info.size.x)).rstrip()
        return {'exit':child.returncode,'output':output,'normal_keypress':True,'private_console_pids':2,
                'keypress_target':'wrapper after native example exit' if hold_console else 'original example final wait'}
    finally:
        for handle in handles: kernel.CloseHandle(handle)
        if attached: kernel.FreeConsole()
        for label,handle in standard_handles.items(): kernel.SetStdHandle(label,handle)
        if child.poll() is None: child.terminate(); child.wait(timeout=5)
# end of windows-test-console.py
