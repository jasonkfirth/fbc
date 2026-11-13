'' Project: FreeBASIC external-library regressions
'' File: fbext-sharedptr-null-state.bas
'' Purpose: Check empty SharedPtr state and reference-counted scalar lifetimes.
'' Responsibilities: Exercise both constructors, copies, reset, swap and deletion.
'' This file intentionally does NOT initialize graphics or load image data.
#lang "fb"
#include once "ext/memory/sharedptr.bi"

dim shared as long created, destroyed, custom_destroyed, checks

type CountedValue
	value as long
	declare constructor(byval initial_value as long)
	declare destructor()
end type

constructor CountedValue(byval initial_value as long)
	value = initial_value
	created += 1
end constructor

destructor CountedValue()
	destroyed += 1
end destructor

namespace ext
	fbext_Instanciate(fbext_SharedPtr, ((CountedValue)))
end namespace
type CountedPtr as ext.fbext_SharedPtr((CountedValue))

sub DestroyCounted(byval value as CountedValue ptr)
	custom_destroyed += 1
	delete value
end sub

#macro VERIFY(condition)
	checks += 1
	if (condition) = 0 then
		print "SHAREDPTR_NULL_STATE_FAILED "; __LINE__
		end 1
	end if
#endmacro

'' The implementation declares three native pointer fields in this order:
'' object, destroy procedure, reference count. Verify the layout assumption
'' before examining the final slot's object representation as unsigned bytes.
'' Inspecting bytes lets the old constructor fail without reading or following
'' its uninitialized pointer. Do not invoke that old object's destructor.
VERIFY(sizeof(CountedPtr) = 3 * sizeof(any ptr))
#ifdef SHAREDPTR_POISON_CASE
for custom as long = SHAREDPTR_POISON_CASE to SHAREDPTR_POISON_CASE
#else
for custom as long = 0 to 1
#endif
	dim storage as any ptr = allocate(sizeof(CountedPtr))
	if storage = 0 then end 2
	for i as integer = 0 to sizeof(CountedPtr) - 1
		cptr(ubyte ptr, storage)[i] = &hA5
	next
	dim owner as CountedPtr ptr
	if custom then
		owner = new(storage) CountedPtr(0, @DestroyCounted)
	else
		owner = new(storage) CountedPtr()
	end if
	'' The legacy library headers undefine the TRUE/FALSE convenience names.
	dim initialized as boolean = cbool(1)
	for i as integer = 2 * sizeof(any ptr) to sizeof(CountedPtr) - 1
		if cptr(ubyte ptr, storage)[i] <> 0 then initialized = cbool(0)
	next
	if initialized = cbool(0) then
		deallocate(storage)
		print "SHAREDPTR_EMPTY_COUNT_UNINITIALIZED"
		end 3
	end if
	VERIFY(owner->Get() = 0)
	owner->destructor()
	deallocate(storage)
next
VERIFY(custom_destroyed = 0 and created = 0 and destroyed = 0)

scope
	dim empty as CountedPtr
	dim copied as CountedPtr = empty
	VERIFY(empty.Get() = 0 and copied.Get() = 0)
	empty.Reset(0, @DestroyCounted)
	VERIFY(custom_destroyed = 0)
	dim first_owner as CountedPtr = CountedPtr(new CountedValue(41))
	VERIFY(created = 1 and destroyed = 0)
	VERIFY(first_owner.Get()->value = 41)
	scope
		dim second_owner as CountedPtr = first_owner
		VERIFY(second_owner.Get() = first_owner.Get())
		copied = second_owner
		VERIFY(copied.Get() = first_owner.Get())
		first_owner.Reset()
		VERIFY(first_owner.Get() = 0 and destroyed = 0)
		copied.Swap_(first_owner)
		VERIFY(copied.Get() = 0 and first_owner.Get() = second_owner.Get())
	end scope
	VERIFY(destroyed = 0)
	first_owner.Reset()
	VERIFY(destroyed = 1)
	first_owner.Reset(new CountedValue(67), @DestroyCounted)
	VERIFY(created = 2 and first_owner.Get()->value = 67)
	copied = first_owner
	first_owner.Reset()
	VERIFY(destroyed = 1 and custom_destroyed = 0)
	copied.Reset()
	VERIFY(destroyed = 2 and custom_destroyed = 1)
end scope
VERIFY(created = destroyed and custom_destroyed = 1)
print "SHAREDPTR_NULL_STATE_PASS"; checks

'' end of fbext-sharedptr-null-state.bas
