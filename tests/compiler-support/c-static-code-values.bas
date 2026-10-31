/'
	Project: FreeBASIC C backend regression controls
	File: c-static-code-values.bas
	Purpose: Preserve private constant procedure values without pointer casts in static data.
	Responsibilities: Native address identity, typed calls, repeated reads, and
	boundaries for writes, address escapes, aliases and public storage.
	This file intentionally does NOT contain: Runtime initialization substitutes,
	allocation stress or native warning suppressions.
'/
#lang "fb"

type Callback as function cdecl(byval as long) as long

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_STATIC_CODE_VALUES_FAILED", __LINE__
		end 1
	end if
#endmacro

#macro OPAQUE_BOUNDARY()
#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif
#endmacro

function TargetOne cdecl(byval value as long) as long
	return value + 3
end function
function TargetTwo cdecl(byval value as long) as long
	return value + 7
end function

function ReadOne() as any ptr
	OPAQUE_BOUNDARY()
	static binding as any ptr = @TargetOne
	return binding
end function
function ReadTwo() as any ptr
	OPAQUE_BOUNDARY()
	static binding as any ptr = procptr(TargetTwo)
	return binding
end function
function ReadCast() as any ptr
	OPAQUE_BOUNDARY()
	static binding as any ptr = cptr(any ptr, @TargetOne)
	return binding
end function
function ReadTypedCast() as any ptr
	OPAQUE_BOUNDARY()
	static binding as any ptr = cptr(Callback, @TargetOne)
	return binding
end function
function ReadQualified() as const ubyte ptr
	OPAQUE_BOUNDARY()
	static binding as const ubyte ptr = cptr(const ubyte ptr, @TargetOne)
	return binding
end function
function PassValue(byval value as any ptr) as any ptr
	return value
end function
function ReadArgument() as any ptr
	OPAQUE_BOUNDARY()
	static binding as any ptr = @TargetOne
	return PassValue(binding)
end function
function ReadTyped() as Callback
	static binding as Callback = @TargetOne
	return binding
end function

#ifdef CHECK_BOUNDARIES
extern PublishedCode alias "fb_static_public_code" as any ptr
dim shared PublishedCode as any ptr = @TargetOne

function ReadWritable(byval change_value as long) as any ptr
	static binding as any ptr = @TargetOne
	if change_value then binding = ReadTwo()
	return binding
end function
function ReadAddress() as any ptr ptr
	static binding as any ptr = @TargetOne
	return @binding
end function
sub ReplaceSlot(byref value as any ptr)
	value = ReadTwo()
end sub
function ReadByref() as any ptr
	static binding as any ptr = @TargetOne
	ReplaceSlot(binding)
	return binding
end function
function ReadAlias(byval change_value as long) as any ptr
	static binding as any ptr = @TargetOne
	static binding_address as any ptr ptr = @binding
	if change_value then *binding_address = ReadTwo()
	return binding
end function
function ReadInplace() as any ptr
	static binding as any ptr = @TargetOne
	'' Only the native address word is observed. The toggled value is never
	'' called or dereferenced. Assignment optimization may update it in place.
	binding = cptr(any ptr, cuint(binding) xor cuint(1))
	return binding
end function
#endif

dim as any ptr expected_one = @TargetOne
dim as any ptr expected_two = @TargetTwo
for iteration as long = 1 to 3
	VERIFY(ReadOne() = expected_one)
	VERIFY(ReadTwo() = expected_two)
	VERIFY(ReadCast() = expected_one)
	VERIFY(ReadTypedCast() = expected_one)
	VERIFY(ReadQualified() = cptr(const ubyte ptr, expected_one))
	VERIFY(ReadArgument() = expected_one)
	dim as Callback direct_call = cptr(Callback, ReadOne())
	VERIFY(direct_call(11) = 14)
	dim as Callback second_call = cptr(Callback, ReadTwo())
	VERIFY(second_call(11) = 18)
	dim as Callback typed_call = ReadTyped()
	VERIFY(typed_call(11) = 14)
next

#ifdef CHECK_BOUNDARIES
VERIFY(PublishedCode = expected_one)
PublishedCode = ReadTwo()
VERIFY(PublishedCode = expected_two)
VERIFY(ReadWritable(0) = expected_one)
VERIFY(ReadWritable(1) = expected_two)
VERIFY(ReadWritable(0) = expected_two)
dim as any ptr ptr escaped_slot = ReadAddress()
VERIFY(*escaped_slot = expected_one)
*escaped_slot = expected_two
VERIFY(ReadAddress() = escaped_slot)
VERIFY(*ReadAddress() = expected_two)
VERIFY(ReadByref() = expected_two)
VERIFY(ReadAlias(0) = expected_one)
VERIFY(ReadAlias(1) = expected_two)
VERIFY(ReadAlias(0) = expected_two)
VERIFY(cuint(ReadInplace()) = (cuint(expected_one) xor cuint(1)))
VERIFY(cuint(ReadInplace()) = cuint(expected_one))
#endif
print "C_STATIC_CODE_VALUES_PASS"

/' end of c-static-code-values.bas '/
