'' Project: FreeBASIC CRT identifier regression tests
'' File: crt-thread-names.bas
'' Purpose: Exercise encoded CRT thread names against the real Windows runtime.
'' Responsibilities: Check the worker argument, exit status and handle lifetime.
'' This file intentionally does NOT replace the CRT with test stubs.
#lang "fb"
#include once "crt/process.bi"

'' Only handle-based kernel calls are needed. DWORD and BOOL retain their
'' 32-bit widths; the Windows convention supplies stdcall on 32-bit targets.
extern "Windows"
	declare function WaitForSingleObject(byval handleValue as any ptr, byval milliseconds as ulong) as ulong
	declare function GetExitCodeThread(byval handleValue as any ptr, byval exitCode as ulong ptr) as long
	declare function CloseHandle(byval handleValue as any ptr) as long
end extern

private const WORKER_VALUE as long = 4711
private const WORKER_EXIT as ulong = 37
'' A failed worker must not leave the backend test waiting indefinitely.
private const THREAD_WAIT_MS as ulong = 5000
private const WAIT_OBJECT_0 as ulong = 0

private function worker stdcall(byval context as any ptr) as ulong
	if context = 0 then return 1
	*cptr(long ptr, context) = WORKER_VALUE
	_endthreadex(WORKER_EXIT)
	return 0
end function

dim resultValue as long
dim threadId as ulong
dim threadValue as uintptr_t = _beginthreadex(0, 0, @worker, @resultValue, 0, @threadId)
if threadValue = 0 then end 1
dim threadHandle as any ptr = cptr(any ptr, threadValue)
dim waitResult as ulong = WaitForSingleObject(threadHandle, THREAD_WAIT_MS)
dim exitStatus as ulong
dim queryResult as long = GetExitCodeThread(threadHandle, @exitStatus)
dim closeResult as long = CloseHandle(threadHandle)
if waitResult <> WAIT_OBJECT_0 then end 2
if queryResult = 0 orelse closeResult = 0 then end 3
if exitStatus <> WORKER_EXIT then end 4
'' A successful wait joins the worker before the main thread reads its write.
if resultValue <> WORKER_VALUE then end 5
print "CRT_THREAD_NAMES_PASS"

'' end of crt-thread-names.bas
