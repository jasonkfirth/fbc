'' Project: FreeBASIC semantic sidecar tests
'' File: select-case-coercions.bas
'' Purpose: Exercise operational CASE types beyond ordinary scalar operands.
'' Responsibilities: Character loads, bit fields, enum types and UDT conversions.
'' This file intentionally does NOT execute selection bodies or cast side effects.
#lang "fb"

type TextAlternative
	value as string
	declare operator cast() as string
end type

operator TextAlternative.cast() as string
	return value
end operator

type PackedSelector
	value : 3 as ubyte
end type

enum Choice
	FirstChoice = 1
	LastChoice = 2
end enum

sub CheckCoercions(byval character as zstring ptr, byref packed as PackedSelector, _
                  byval selected_choice as Choice, byref text as string, byref alternative as TextAlternative)
	if( character <> 0 ) then
		select case as const *character
		case 65
			print 1
		end select
	end if
	select case packed.value
	case 1, 7
		print 2
	end select
	select case selected_choice
	case FirstChoice
		print 3
	end select
	select case as const selected_choice
	case FirstChoice to LastChoice
		print 4
	end select
	select case text
	case alternative
		print 5
	end select
end sub

'' end of select-case-coercions.bas
