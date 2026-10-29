'' Project: FreeBASIC C emitter regression tests
'' File: c-error-terminal.bas
'' Purpose: Exercise a checked ERROR with no local label addresses.
'' Responsibilities: Expose a CDECL worker and detect an unwanted continuation.
'' This file intentionally does NOT install a legacy label handler.

sub terminal_worker cdecl alias "fb_test_terminal_worker"( byval visits as long ptr )
	*visits += 1
	error 24
	*visits += 1000
end sub

'' end of c-error-terminal.bas
