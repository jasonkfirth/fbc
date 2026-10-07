'' Project: FreeBASIC semantic sidecar tests
'' File: select-case-lowering.bas
'' Purpose: Exercise selected CASE comparisons and AS CONST conversion results.
'' Responsibilities: Numeric coercion, text routes and unsigned table boundaries.
'' This file intentionally does NOT execute selection bodies or compare host text.
#lang "fb"

declare function NextBound() as long

sub CheckNumeric(byval small as byte, byval real_value as single, byval wide_value as ulongint)
	select case small
	case 128, NextBound()
		print small
	end select
	select case real_value
	case 1.25, -2.0 to 2.0
		print real_value
	end select
	select case wide_value
	'' Deliberate mixed signedness exercises the selected unsigned comparison.
	case -1LL, 18446744073709551615ULL
		print wide_value
	end select
end sub

sub CheckText(byref narrow_text as string, byref wide_text as wstring, byref unicode_text as ustring)
	select case narrow_text
	case "a", "b" to "z"
		print narrow_text
	end select
	select case wide_text
	case "a", wstr("b") to wstr("z")
		print wide_text
	end select
	select case unicode_text
	case "a", uchr(98) to uchr(122)
		print unicode_text
	end select
end sub

sub CheckTables(byval signed_value as longint, byval unsigned_value as ulongint)
	select case as const signed_value
	case -2 to 2
		print signed_value
	end select
	select case as const unsigned_value
	case 18446744073709551614ULL to 18446744073709551615ULL
		print unsigned_value
	end select
	select case as const signed_value
	case else
		print signed_value
	end select
end sub

'' end of select-case-lowering.bas
