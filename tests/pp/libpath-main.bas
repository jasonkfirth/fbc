'' Project: FreeBASIC test suite
'' File: libpath-main.bas
'' Purpose: Verify #LIBPATH contributes a directory to library lookup.
'' Responsibilities: Link and call the routine supplied by the test archive.
'' This file intentionally does NOT contain: the library implementation.

#libpath "pp"

Declare Function fbcLibpathProbe Cdecl Lib "fbc-libpath-test" _
	Alias "fbc_libpath_probe" () As Integer

If fbcLibpathProbe() <> 42 Then End 1
End 0

'' end of libpath-main.bas
