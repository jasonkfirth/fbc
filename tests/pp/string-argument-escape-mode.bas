'' Project: FreeBASIC preprocessor regression tests
'' File: string-argument-escape-mode.bas
'' Purpose: Keep raw quotes separate from escaped quotes in macro arguments.
'' Responsibilities: Check extraction, grouped expansion and both quote forms.
'' This file intentionally does NOT change the argument text it receives.
' TEST_MODE : COMPILE_AND_RUN_OK

#assert __FB_ARG_EXTRACT__(1, $"raw\", 321) = 321
#assert __FB_ARG_EXTRACT__(1, !"escaped\", comma", 654) = 654

dim as long groups, total

#macro check_pair(text_value, number_value)
	scope
		const spelling = __FB_QUOTE__(text_value)
		#assert len(spelling) > 0
		groups += 1
		total += number_value
	end scope
#endmacro

__FB_ARG_LISTEXPAND__(check_pair, 2, $"first\", 321, $"second\", 654)
if groups <> 2 or total <> 975 then end 1

groups = 0
total = 0
__FB_ARG_LISTEXPAND__(check_pair, 2, !"first\", comma", 321, !"second\", comma", 654)
if groups <> 2 or total <> 975 then end 2

'' end of string-argument-escape-mode.bas
