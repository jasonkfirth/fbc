/'
	Project: FreeBASIC C backend regression tests
	File: c-static-array-storage.bas
	Purpose: Check unused static arrays without losing initializer dependencies.
	Responsibilities: Exercise data addresses, descriptor uses, initialized arrays
	and static object lifetimes across repeated procedure calls.
	This file intentionally does NOT contain: Compiler diagnostic suppression or
	platform-specific storage assumptions.
'/
#lang "fb"

dim shared as long constructed

type Owned
	value as long
	declare constructor()
	declare destructor()
end type

constructor Owned()
	value = 17
	constructed += 1
end constructor

destructor Owned()
	constructed -= 1
end destructor

private function ArrayBounds(values() as long) as long
	return lbound(values) * 10 + ubound(values)
end function

private sub CheckArrays(byval iteration as long)
	static as long unused(0 to 3)
	static as long retained_by_initializer(0 to 3)
	static as long ptr address_from_initializer = @retained_by_initializer(1)
	static as long indexed(2 to 4)
	static as long initialized(0 to 2) = {2, 3, 5}
	static as Owned owned_values(0 to 1)

	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif

	'' The only reference to this array lives in another static initializer.
	*address_from_initializer += 7
	if( *address_from_initializer <> iteration * 7 ) then end 1
	indexed(2) += 3
	if( indexed(2) <> iteration * 3 ) then end 2
	if( ArrayBounds(indexed()) <> 24 ) then end 3
	if( initialized(0) + initialized(1) + initialized(2) <> 10 ) then end 4
	if( constructed <> 2 ) then end 5
	if( owned_values(0).value + owned_values(1).value <> 34 ) then end 6
end sub

CheckArrays(1)
CheckArrays(2)
print "C_STATIC_ARRAY_STORAGE_PASS"

/' end of c-static-array-storage.bas '/
