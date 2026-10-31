/'
	Project: FreeBASIC C backend regression tests
	File: c-va-start-parameters.bas
	Purpose: Bind va_start to the final incoming C parameter.
	Responsibilities: Check direct, reference, string, array, non-trivial record
	and native long-double carriers without changing BASIC argument passing.
	This file intentionally does NOT contain: Diagnostic suppression or foreign ABI changes.
'/
#lang "fb"
#include once "crt/longdouble.bi"

#macro VERIFY(condition)
	if (condition) = 0 then end 1
#endmacro

#macro READ_TWO(last_parameter)
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
	dim as cva_list arguments = any
	cva_start(arguments, last_parameter)
	dim as long first_value = cva_arg(arguments, long)
	dim as long second_value = cva_arg(arguments, long)
	cva_end(arguments)
	return first_value + second_value
#endmacro

dim shared as long copy_calls, destructor_calls

type NonTrivialWord
	values(0 to 7) as long
	declare constructor()
	declare constructor(byref original as NonTrivialWord)
	declare destructor()
end type

constructor NonTrivialWord()
	for i as long = 0 to 7
		values(i) = 19 + i
	next
end constructor

constructor NonTrivialWord(byref original as NonTrivialWord)
	copy_calls += 1
	for i as long = 0 to 7
		values(i) = original.values(i)
	next
end constructor

destructor NonTrivialWord()
	VERIFY(values(0) = 19 and values(7) = 26)
	destructor_calls += 1
end destructor

function LastPlain cdecl alias "fb_test_va_plain"(byval marker as long, ...) as long
	VERIFY(marker = 11)
	READ_TWO(marker)
end function

function LastReference cdecl alias "fb_test_va_reference"(byref marker as long, ...) as long
	VERIFY(marker = 23)
	READ_TWO(marker)
end function

function LastText cdecl alias "fb_test_va_text"(byref marker as string, ...) as long
	VERIFY(marker = "named")
	READ_TWO(marker)
end function

#ifndef OMIT_ARRAY
function LastArray cdecl alias "fb_test_va_array"(values() as long, ...) as long
	VERIFY(lbound(values) = 2 and ubound(values) = 4)
	VERIFY(values(2) = 29 and values(4) = 31)
	READ_TWO(values)
end function
#endif

function LastRecord cdecl alias "fb_test_va_record"(byval marker as NonTrivialWord, ...) as long
	VERIFY(marker.values(0) = 19 and marker.values(7) = 26)
	READ_TWO(marker)
end function

function LastWide cdecl alias "fb_test_va_wide"(byval marker as clongdouble, ...) as long
	'' Use the recovered wide value directly so this also checks its native
	'' carrier through arithmetic and comparisons in both operand positions.
	VERIFY(marker = 3.25#)
	dim as clongdouble advanced = marker + 0.5#
	VERIFY(advanced = 3.75#)
	VERIFY(marker < cint(4))
	VERIFY(clngint(3) < marker)
	READ_TWO(marker)
end function

dim as long marker = 23
dim as string text_value = "named"
#ifndef OMIT_ARRAY
dim as long array_values(2 to 4) = {29, 30, 31}
#endif
dim as NonTrivialWord record_value
dim as clongdouble wide_value = CLongDoubleFromDoubleValue(3.25#)
VERIFY(LastPlain(11, clng(101), clng(202)) = 303)
VERIFY(LastReference(marker, clng(101), clng(202)) = 303)
VERIFY(LastText(text_value, clng(101), clng(202)) = 303)
#ifndef OMIT_ARRAY
VERIFY(LastArray(array_values(), clng(101), clng(202)) = 303)
#endif
VERIFY(LastRecord(record_value, clng(101), clng(202)) = 303)
VERIFY(copy_calls = 1 and destructor_calls = 1)
VERIFY(record_value.values(0) = 19 and record_value.values(7) = 26)
VERIFY(LastWide(wide_value, clng(101), clng(202)) = 303)
print "C_VA_START_PARAMETERS_PASS"

'' end of c-va-start-parameters.bas
