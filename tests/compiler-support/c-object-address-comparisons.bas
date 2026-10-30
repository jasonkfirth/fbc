/'
	Project: FreeBASIC C backend regression controls
	File: c-object-address-comparisons.bas
	Purpose: Fold known object addresses without losing storage dependencies.
	Responsibilities: Expression and branch truth values, static initializer
	targets, object lifetime, reference arguments and evaluated reference calls.
	This file intentionally does NOT contain: Platform APIs or threading.
'/
#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_OBJECT_ADDRESS_COMPARISONS_FAILED", __LINE__
		end 1
	end if
#endmacro

type PlainValue
	value as integer
end type

dim shared as integer global_value, reference_calls, constructed, destroyed
static shared as PlainValue global_record

type TrackedValue
	value as integer
	declare constructor()
	declare destructor()
end type

constructor TrackedValue()
	value = 23
	constructed += 1
end constructor

destructor TrackedValue()
	VERIFY(value = 23)
	destroyed += 1
end destructor

function ReferenceValue() byref as integer
	reference_calls += 1
	return global_value
end function

function ReferencePresent(byref value as integer) as integer
	'' A reference argument can carry a null address without being read.
	'' Its pointer value must not be confused with owned parameter storage.
	return @value <> 0
end function

private sub KnownAddresses()
	dim as integer local_value
	static as integer static_value
	dim as PlainValue local_record
	static as PlainValue static_record
	VERIFY(@global_value <> 0)
	VERIFY(0 <> @global_record)
	VERIFY(@local_value <> 0)
	VERIFY(0 <> @local_record)
	VERIFY(@static_value <> 0)
	VERIFY(0 <> @static_record)
	if @local_value = 0 then end 2
	if 0 = @static_record then end 3
	dim as integer integer_truth = (@static_value <> 0)
	dim as boolean boolean_truth = (@local_record <> 0)
	VERIFY(integer_truth = -1)
	VERIFY(boolean_truth)
#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif
end sub

private sub StaticDependencies(byval expected as integer)
	static as integer retained_value
	static as integer ptr retained_pointer = @retained_value
	static as PlainValue retained_record
	static as PlainValue ptr retained_record_pointer = @retained_record
	'' These comparisons are the only body references to the backing names.
	'' Their static pointer initializers still require both storage objects.
	VERIFY(@retained_value <> 0)
	VERIFY(@retained_record <> 0)
	*retained_pointer += 1
	retained_record_pointer->value += 1
	VERIFY(*retained_pointer = expected)
	VERIFY(retained_record_pointer->value = expected)
#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif
end sub

private sub ObjectLifetime()
	dim as TrackedValue owned_value
	VERIFY(@owned_value <> 0)
	VERIFY(constructed = 1)
	VERIFY(destroyed = 0)
#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif
end sub

private sub UnknownReferences()
	VERIFY(ReferencePresent(global_value) = -1)
	VERIFY(ReferencePresent(byval 0) = 0)
	VERIFY(@(ReferenceValue()) <> 0)
	VERIFY(reference_calls = 1)
	dim as integer ptr unknown_value
	VERIFY(unknown_value = 0)
	unknown_value = @global_value
	VERIFY(unknown_value <> 0)
end sub

KnownAddresses()
StaticDependencies(1)
StaticDependencies(2)
ObjectLifetime()
VERIFY(constructed = 1)
VERIFY(destroyed = 1)
UnknownReferences()
print "C_OBJECT_ADDRESS_COMPARISONS_PASS"

/' end of c-object-address-comparisons.bas '/
