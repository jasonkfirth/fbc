'' Project: FreeBASIC graphics text argument tests
'' File: text-types-gfx3-api.bas
'' Purpose: Check Unicode asset filename overloads without requiring a GPU.
'' Responsibilities: Compile byte, wide, and UTF-8 surface loading calls.
'' This file intentionally does NOT initialize a display or load assets.

' TEST_MODE : COMPILE_ONLY_OK
#include once "fbgfx3.bi"

private sub check( byref bytes as const string, byref wide as const wstring, byref utf8 as const ustring )
	dim as any ptr surface
	surface = fb.Gfx3SurfaceLoad(bytes)
	surface = fb.Gfx3SurfaceLoad(wide)
	surface = fb.Gfx3SurfaceLoad(utf8)
end sub

'' end of text-types-gfx3-api.bas
