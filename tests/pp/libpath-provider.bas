'' Project: FreeBASIC test suite
'' File: libpath-provider.bas
'' Purpose: Provide a stable function for the #LIBPATH archive test.
'' Responsibilities: Return a known value through a C-linkage symbol.
'' This file intentionally does NOT contain: library search configuration.

Function fbcLibpathProbe Cdecl Alias "fbc_libpath_probe" () As Integer
	Return 42
End Function

'' end of libpath-provider.bas
