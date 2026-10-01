'' Project: FreeBASIC semantic sidecar tests
'' File: lifetimes.bas
'' Purpose: Exercise compiler-selected construction and destruction relationships.
'' Responsibilities: Cover local, field, argument, return, temporary, and heap use.
'' This file intentionally does NOT contain: explicit destructor calls.

type Tracked
	value as long
	declare constructor()
	declare constructor(byref source as Tracked)
	declare constructor(byval value as long)
	declare destructor()
end type

constructor Tracked()
	this.value = 0
end constructor

constructor Tracked(byref source as Tracked)
	this.value = source.value
end constructor

constructor Tracked(byval value as long)
	this.value = value
end constructor

destructor Tracked()
	this.value = -1
end destructor

type Holder
	default_field as Tracked
	converted_field as Tracked = 7
end type

sub TakeValue(byval item as Tracked)
	print item.value
end sub

function MakeValue(byval value as long) as Tracked
	return value
end function

sub OptionalValue(byval item as Tracked = 8)
	print item.value
end sub

type Plain
	value as long
end type

sub ProbeLifetimes()
	dim default_value as Tracked
	dim copied_value as Tracked = default_value
	dim converted_value as Tracked = 7
	dim heap_value as Tracked ptr = new Tracked(3)
	dim heap_array as Tracked ptr = new Tracked[2]
	delete heap_value
	delete[] heap_array
	TakeValue(4)
	TakeValue(MakeValue(5))
	print iif(default_value.value, MakeValue(1), MakeValue(2)).value
	with MakeValue(6)
		print .value
	end with
	scope
		dim leaving_value as Tracked
		goto done
	end scope
done:
	for index as long = 0 to 1
		dim loop_value as Tracked
		if index = 0 then continue for
		exit for
	next
	dim plain_value as Plain
	dim plain_heap as Plain ptr = new Plain
	delete plain_heap
end sub

#define MACRO_TEMP MakeValue(10)
#define MACRO_EXIT goto macro_done
sub ProbeNonphysicalCleanup()
	TakeValue(MACRO_TEMP)
	scope
		dim macro_value as Tracked
		MACRO_EXIT
	end scope
macro_done:
end sub

ProbeLifetimes()
ProbeNonphysicalCleanup()

'' end of lifetimes.bas
