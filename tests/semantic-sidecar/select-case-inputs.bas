'' Project: FreeBASIC semantic sidecar tests
'' File: select-case-inputs.bas
'' Purpose: Preserve accepted SELECT ownership and original CASE operands.
'' Responsibilities: Ordinary/constant modes, ranges, nesting and macro clauses.
'' This file intentionally does NOT execute selector or case-side effects.
#lang "fb"

declare function NextValue() as long
#define expanded_clause case 8, 9
sub CheckSelections(byval value as long)
	select case value
	case 1, 2 to 4, is >= 7
		select case as const value
		case 1, 2 to 4
			print value
		case else
			print 0
		end select
	expanded_clause
		print value
	case NextValue()
		print value
	case else
		print -1
	end select
end sub

'' end of select-case-inputs.bas
