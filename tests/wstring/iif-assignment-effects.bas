'' Project: FreeBASIC WSTRING regression tests
'' File: iif-assignment-effects.bas
'' Purpose: Preserve lazy wide IIF evaluation and assignment side effects.
'' Responsibilities: Check both branches, conversions and temporary cleanup.
'' This file intentionally does NOT use an assignment as an expression value.
' TEST_MODE : COMPILE_AND_RUN_OK

dim shared as long true_calls, false_calls

private function true_arm() as wstring ptr
	static as wstring * 16 text
	true_calls += 1
	text = wstr("true")
	return @text
end function

private function false_arm() as wstring ptr
	static as wstring * 16 text
	false_calls += 1
	text = wstr("false")
	return @text
end function

dim as long gate = 1
dim as wstring * 32 result
result = iif(gate, *true_arm(), *false_arm())
if result <> wstr("true") then end 1
if true_calls <> 1 or false_calls <> 0 then end 2

gate = 0
result = iif(gate, *true_arm(), *false_arm())
if result <> wstr("false") then end 3
if true_calls <> 1 or false_calls <> 1 then end 4

dim as string narrow = "converted"
result = narrow
if result <> wstr("converted") then end 5
narrow = result
if narrow <> "converted" then end 6

'' end of iif-assignment-effects.bas
