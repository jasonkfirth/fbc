' TEST_MODE : COMPILE_AND_RUN
''
'' FreeBASIC CRT tests
'' File: unistd-haiku.bas
'' Purpose: Verify native Haiku POSIX declarations and pipe I/O.
'' Responsibilities: Check ABI widths, header coexistence, process ID, cwd,
'' transferred byte counts and descriptor cleanup. This file has no UI.
''

#ifdef __FB_HAIKU__
#include once "crt/sys/socket.bi"
#include once "crt/unistd.bi"

#assert SizeOf(pid_t) = 4
#assert SizeOf(off_t) = 8
#assert SizeOf(ssize_t) = SizeOf(any ptr)
if getpid() <= 0 then end 2

dim as zstring * 4096 current_path
if getcwd(@current_path, SizeOf(current_path)) = 0 then end 3
if Len(current_path) = 0 then end 4

dim as long descriptors(0 to 1) ' FB-LINTER: DISABLE-LINE FBL423 REASON: POSIX pipe descriptors use 32-bit C int values.
if pipe_(@descriptors(0)) <> 0 then end 5 ' FB-LINTER: DISABLE-LINE FBL-SEC-008 REASON: This single-threaded test closes both descriptors and never execs a child.
dim as zstring * 4 message = "fb!"
dim as zstring * 4 received = ""
'' POSIX read transfers raw bytes and does not append a string terminator.
received[3] = 0
dim as integer failed = 0
if write_(descriptors(1), @message, 3) <> 3 then
    failed = 6
elseif read_(descriptors(0), @received, 3) <> 3 then
    failed = 7
elseif received <> message then
    failed = 8
end if
if close_(descriptors(0)) <> 0 andalso failed = 0 then failed = 9
if close_(descriptors(1)) <> 0 andalso failed = 0 then failed = 10
end failed
#endif

'' end of unistd-haiku.bas
