' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC curses binding test
'' -----------------------------
''
'' File: tests/crt/curses.bas
''
'' Purpose:
''
''     Verify that the generic curses wrapper selects and uses the terminal
''     library supplied by Unix-like operating systems.
''
'' Responsibilities:
''
''     - include curses.bi exactly as user programs do
''     - initialize curses without requiring an interactive terminal
''     - create, write to, and destroy a window through the binding
''
'' This file intentionally does NOT contain:
''
''     - interactive terminal input
''     - terminal control output to the user's console
''     - assumptions about optional terminal descriptions
''

#if defined(__FB_LINUX__) or defined(__FB_CYGWIN__) or defined(__FB_FREEBSD__) or _
    defined(__FB_DRAGONFLY__) or defined(__FB_OPENBSD__) or defined(__FB_NETBSD__) or _
    defined(__FB_DARWIN__) or defined(__FB_HAIKU__) or defined(__FB_SOLARIS__)

#include once "curses.bi"

const SMOKE_VERSION_FAILED = 1
const SMOKE_TEMPFILE_FAILED = 2
const SMOKE_NEWTERM_FAILED = 3
const SMOKE_NEWWIN_FAILED = 4
const SMOKE_WRITE_FAILED = 5
const SMOKE_CLEANUP_FAILED = 6
const SMOKE_MOVE_WRITE_FAILED = 7

dim result as long
dim version as const zstring ptr = curses_version()
dim input_file as FILE ptr
dim output_file as FILE ptr
dim current_screen as SCREEN_ ptr
dim current_window as WINDOW_ ptr

if version = 0 orelse len(*version) = 0 then
	end SMOKE_VERSION_FAILED
end if

input_file = tmpfile()
output_file = tmpfile()

if input_file = 0 orelse output_file = 0 then
	result = SMOKE_TEMPFILE_FAILED
	goto cleanup
end if

current_screen = newterm(@"dumb", output_file, input_file)
if current_screen = 0 then
	result = SMOKE_NEWTERM_FAILED
	goto cleanup
end if

set_term current_screen
current_window = newwin(2, 8, 0, 0)
if current_window = 0 then
	result = SMOKE_NEWWIN_FAILED
	goto cleanup
end if

if waddch(current_window, cast(chtype, asc("F"))) = ERR_ then
	result = SMOKE_WRITE_FAILED
end if

if mvwaddstr(current_window, 0, 1, @"B") = ERR_ andalso result = 0 then
	result = SMOKE_MOVE_WRITE_FAILED
end if

cleanup:

if current_window <> 0 then
	if delwin(current_window) = ERR_ andalso result = 0 then
		result = SMOKE_CLEANUP_FAILED
	end if
end if

if current_screen <> 0 then
	endwin()
	delscreen current_screen
end if

if input_file <> 0 then fclose input_file
if output_file <> 0 then fclose output_file

if result <> 0 then
	print #2, "curses binding smoke failed at stage "; result
end if

end result

#endif

'' end of tests/crt/curses.bas
