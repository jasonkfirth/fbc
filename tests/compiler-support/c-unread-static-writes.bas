/'
	Project: FreeBASIC C backend regression tests
	File: c-unread-static-writes.bas
	Purpose: Verify unread static writes can disappear without losing evaluation.
	Responsibilities: Check repeated calls, pointer arrays, initializer-held
	addresses, escaped storage and effectful array designators.
	This file intentionally does NOT contain: Native warning suppression or
	assumptions about physical pointer size.
'/
#lang "fb"

dim shared as long calls, pointed_value = 31
dim shared as long evaluation_order

private function NextValue() as long
	calls += 1
	return calls
end function

private function NextPointer() as any ptr
	calls += 1
	return @pointed_value
end function

private sub WriteOnlyScalar()
	static ignored as long
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
	ignored = NextValue()
end sub

private sub WriteOnlyArray()
	static ignored(3 to 4) as any ptr
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
	for i as long = 3 to 4
		ignored(i) = NextPointer()
	next
end sub

private function PersistentValue() as long
	static value as long
	value += 1
	return value
end function

private function ReadAddress(byval value as long ptr) as long
	return *value
end function

private sub CheckInitializerAddresses(byval iteration as long)
	static scalar as long
	static scalar_address as long ptr = @scalar
	static values(2 to 3) as long
	static array_address as long ptr = @values(2)
	'' These native addresses live in initializer trees, before the body has
	'' any ordinary reference to the corresponding static object.
	scalar = iteration * 7
	values(2) = iteration * 11
	if( *scalar_address <> iteration * 7 ) then end 1
	if( *array_address <> iteration * 11 ) then end 2
end sub

private sub CheckEscapes(byval iteration as long)
	static scalar as long
	static values(0 to 1) as long
	scalar = iteration * 13
	values(1) = iteration * 17
	if( ReadAddress(@scalar) <> iteration * 13 ) then end 3
	if( ReadAddress(@values(1)) <> iteration * 17 ) then end 4
end sub

private function OrderedValue() as long
	evaluation_order = evaluation_order * 10 + 1
	return 7
end function

private function OrderedIndex() as long
	evaluation_order = evaluation_order * 10 + 2
	return 0
end function

private sub CheckEffectfulDesignator()
	static ignored(0 to 1) as long
	'' Preserve the complete designator when its index has effects. The
	'' ordinary assignment loader evaluates the value before the destination.
	ignored(OrderedIndex()) = OrderedValue()
end sub

private sub CheckExplicitInitializer(byval iteration as long)
	static value as long = 19
	value += 1
	if( value <> 19 + iteration ) then end 5
end sub

for iteration as long = 1 to 3
	WriteOnlyScalar()
	WriteOnlyArray()
	if( calls <> iteration * 3 ) then end 6
	if( PersistentValue() <> iteration ) then end 7
	CheckInitializerAddresses(iteration)
	CheckEscapes(iteration)
	CheckExplicitInitializer(iteration)
next
CheckEffectfulDesignator()
if( evaluation_order <> 12 ) then end 8
print "C_UNREAD_STATIC_WRITES_PASS"

/' end of c-unread-static-writes.bas '/
