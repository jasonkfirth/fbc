/'
Project: FreeBASIC C backend regression tests
File: c-qualified-float-conversions.bas
Purpose: Preserve scalar precision when rounding read-only floating values.
Responsibilities: Exercise integer widths, indexed reads and Double controls.
This file does not depend on graphics, external assets or platform APIs.
'/
#lang "fb"
#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
	#cmdline "-Wc -Werror=double-promotion"
#endif

type RoundedValues
	narrow as long
	wide as longint
	unsigned_wide as ulongint
#ifdef __FB_64BIT__
	huge as integer<128>
	unsigned_huge as uinteger<128>
#endif
end type

type SampleValue
	value as single
end type

dim shared as integer index_calls

function next_index(byval index as integer) as integer
	index_calls += 1
	return index
end function

sub round_view(byval values as const single ptr, byval index as integer, byref result as RoundedValues)
	result.narrow = clng(values[next_index(index)])
	result.wide = clngint(values[next_index(index)])
	result.unsigned_wide = culngint(values[next_index(index)])
#ifdef __FB_64BIT__
	result.huge = cast(integer<128>, values[next_index(index)])
	result.unsigned_huge = cast(uinteger<128>, values[next_index(index)])
#endif
end sub

function round_reference(byref value as const single) as longint
	return clngint(value)
end function

function round_member(byval sample as const SampleValue ptr) as long
	return clng(sample->value)
end function

function round_double(byval value as const double ptr) as longint
	return clngint(*value)
end function

function square_root_view(byval value as const single ptr) as single
	return sqr(*value)
end function

dim as single samples(0 to 4) = {0.5f, 1.5f, 2.5f, 3.5f, 16777218.0f}
dim as longint expected(0 to 4) = {0, 2, 2, 4, 16777218}
dim as RoundedValues result
dim as SampleValue sample

for index as integer = 0 to ubound(samples)
	index_calls = 0
	round_view(@samples(0), index, result)
	if result.narrow <> expected(index) then end 1
	if result.wide <> expected(index) then end 2
	if result.unsigned_wide <> culngint(expected(index)) then end 3
#ifdef __FB_64BIT__
	if result.huge <> cast(integer<128>, expected(index)) then end 4
	if result.unsigned_huge <> cast(uinteger<128>, expected(index)) then end 5
	if index_calls <> 5 then end 6
#else
	if index_calls <> 3 then end 6
#endif
	if round_reference(samples(index)) <> expected(index) then end 7
	sample.value = samples(index)
	if round_member(@sample) <> expected(index) then end 8
next

sample.value = -1.5f
if round_member(@sample) <> -2 then end 9
if round_reference(sample.value) <> -2 then end 10

' This integer is exact in Double and cannot be represented in Single.
dim as double wide_sample = 16777217.0
if round_double(@wide_sample) <> 16777217 then end 11
sample.value = 4.0f
if square_root_view(@sample.value) <> 2.0f then end 12
print "C_QUALIFIED_FLOAT_CONVERSIONS_PASS"

' end of c-qualified-float-conversions.bas
