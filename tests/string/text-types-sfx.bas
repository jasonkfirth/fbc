'' Project: FreeBASIC sound text argument tests
'' File: text-types-sfx.bas
'' Purpose: Check all three string types at sound command argument boundaries.
'' Responsibilities: Exercise UTF-8 conversion through an observing C stub.
'' This file intentionally does NOT open audio devices or process sound files.

'' The Unicode runner links text-types-sfx-stub.c and executes this fixture.
' TEST_MODE : COMPILE_ONLY_OK
#include once "sfxlib_raw.bi"

declare function capturedCalls cdecl alias "fb_text_test_calls" () as long

#macro check(value)
	play value
	play 1, value
	play value, value
	play value, value, value
	note value, 4, 0.01
	note 1, value, 4, 0.01
	music load value
	music play value
	music loop value
	sfx load 1, value
	midi play value
	assert(capture save(value) = 0)
	assert(sfxlib.OutputCaptureSave(value) = 0)
#endmacro

dim as ustring utf8 = uchr(&hE9, &h4E2D, &h1F600)
dim as wstring * 32 wide = wstr(utf8)
dim as string bytes = cast(string, utf8)
check(bytes)
check(wide)
check(utf8)
assert(capturedCalls() = 48)
print "text-types sound: all text commands preserve UTF-8 arguments"

'' end of text-types-sfx.bas
