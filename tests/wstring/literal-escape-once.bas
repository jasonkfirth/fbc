/'
    Project: FreeBASIC C backend regression tests
    File: literal-escape-once.bas
    Purpose: Preserve literal ESC data when emitting wide strings.
    Responsibilities: References, static initializers and fixed buffer limits.
    This file intentionally does NOT call an internal escape decoder.
'/
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"

private function matches( byref actual as const wstring, byref expected as const wstring ) as boolean
	if( len( actual ) <> len( expected ) ) then return false
	for index as integer = 0 to len( expected ) - 1
		if( actual[index] <> expected[index] ) then return false
	next
	return true
end function

'' ASCII ESC (27) is also the compiler's internal escape marker.
'' Its literal value must survive beside text that resembles an escape.
dim shared as wstring * 32 saved_hex = wchr(27) + wstr("u0041")
dim shared as wstring * 32 saved_upper = wchr(27) + wstr("U0001F600")
dim shared as wstring * 32 saved_control = wchr(27) + wstr("n")
dim shared as wstring * 32 saved_octal = wchr(27, 2) + wstr("101")
dim shared as wstring * 1 saved_empty = wstr("")

'' Each buffer reserves its last code unit for NUL. A UTF-16 surrogate
'' pair can therefore be cut between its halves in the smallest buffer.
#if sizeof(wstring) >= 2
dim shared as wstring * 2 saved_short = wstr(!"\U0001F600X")
dim shared as wstring * 3 saved_pair = wstr(!"\U0001F600X")
dim shared as wstring * 4 saved_full = wstr(!"\U0001F600X")
#endif

dim as wstring * 32 folded_hex = wchr(27) + wstr("u0041")
dim as wstring * 32 folded_upper = wchr(27) + wstr("U0001F600")
dim as wstring * 32 folded_control = wchr(27) + wstr("n")
dim as wstring * 32 folded_octal = wchr(27, 2) + wstr("101")
#if sizeof(wstring) >= 2
dim as wstring * 8 folded_unicode = wstr(!"\u20AC\U0001F600X")
#endif

dim as integer runtime_escape = 27, runtime_octal_length = 2
dim as wstring * 32 expected_hex = wchr(runtime_escape) + wstr("u0041")
dim as wstring * 32 expected_upper = wchr(runtime_escape) + wstr("U0001F600")
dim as wstring * 32 expected_control = wchr(runtime_escape) + wstr("n")
dim as wstring * 32 expected_octal = wchr(runtime_escape, runtime_octal_length) + wstr("101")
#if sizeof(wstring) >= 2
'' Build the expected target code units explicitly so this checks storage.
dim as uinteger runtime_bmp = &h20AC
#if sizeof(wstring) = 2
dim as uinteger runtime_high = &hD83D, runtime_low = &hDE00
dim as wstring * 4 runtime_scalar = wchr(runtime_high, runtime_low)
#else
dim as uinteger runtime_supplementary = &h1F600
dim as wstring * 4 runtime_scalar = wchr(runtime_supplementary)
#endif
dim as wstring * 2 expected_short = runtime_scalar + wstr("X")
dim as wstring * 3 expected_pair = runtime_scalar + wstr("X")
dim as wstring * 4 expected_full = runtime_scalar + wstr("X")
dim as wstring * 8 expected_unicode = wchr(runtime_bmp) + runtime_scalar + wstr("X")
#endif
dim as integer failures = 0

if( not matches( folded_hex, expected_hex ) or not matches( folded_upper, expected_upper ) or _
    not matches( folded_control, expected_control ) or not matches( folded_octal, expected_octal ) ) then
	failures or= 1
end if
if( not matches( saved_hex, expected_hex ) or not matches( saved_upper, expected_upper ) or _
    not matches( saved_control, expected_control ) or not matches( saved_octal, expected_octal ) ) then
	failures or= 2
end if
#if sizeof(wstring) >= 2
if( not matches( saved_short, expected_short ) or not matches( saved_pair, expected_pair ) or _
    not matches( saved_full, expected_full ) ) then
	failures or= 4
end if
#endif
if( saved_empty[0] <> 0 ) then failures or= 8
#if sizeof(wstring) >= 2
if( not matches( folded_unicode, expected_unicode ) ) then failures or= 16
#endif
end failures

'' end of literal-escape-once.bas
