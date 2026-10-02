'' Project: FreeBASIC compiler tests
'' File: constructs.bas
'' Purpose: Exercise source statement routes and each compound family.
'' Responsibilities: Provide nested, inline, member, label, and assembly cases.
'' This file intentionally does NOT contain: runtime effect or reachability tests.

namespace SourceConstructs

type Pair
	value as long
	union
		low as long
		high as long
	end union
end type

union Choice
	number as long
	bits as ulong
end union

enum Kind
	First = 1, Second = 2
end enum

extern "C"
	declare sub ExternalCall()
end extern

sub Observe()
	dim value as long = 1: value += 2
	dim address_of_value as long ptr = @value
	*address_of_value = 4
	dim item as Pair

labelled:
	if value then value += 1 else value -= 1
	if value > 0 then
		value -= 1
	elseif value < 0 then
		value += 1
	else
		value = 0
	end if

	for outer as long = 1 to 2
		for inner as long = 1 to 2
			value += inner
	next inner, outer

	do
		value += 1
	loop while value < 3
	while value < 4
		value += 1
	wend

	select case value
	case 4
		value = 5
	case else
		value = 6
	end select

	with item
		.value = value
	end with
	scope
		dim nested as long = value + _
			1
		? nested
	end scope
	asm
		nop
	end asm
end sub

end namespace

SourceConstructs.Observe()

'' end of constructs.bas
