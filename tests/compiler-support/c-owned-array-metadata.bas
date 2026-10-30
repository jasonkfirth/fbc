/'
	Project: FreeBASIC C backend regression tests
	File: c-owned-array-metadata.bas
	Purpose: Omit unused metadata while retaining fixed-array element lifetimes.
	Responsibilities: Check constructor/destructor counts, descriptor consumers,
	dynamic arrays and managed string cleanup.
	This file intentionally does NOT contain: Runtime diagnostic suppression or
	changes to array storage ownership.
'/
#lang "fb"

dim shared as long constructed, destroyed
dim shared as long initialized_calls

type InitializedOnly
	value as long
	declare constructor(byval initial_value as long)
end type

constructor InitializedOnly(byval initial_value as long)
	initialized_calls += 1
	value = initial_value
end constructor

private sub ExplicitInitialization()
	dim as InitializedOnly values(0 to 2) = {11, 23, 31}
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
	if( initialized_calls <> 3 ) then end 8
	if( values(0).value + values(1).value + values(2).value <> 65 ) then end 9
end sub

type Tracked
	value as long
	declare constructor()
	declare destructor()
end type

constructor Tracked()
	constructed += 1
	value = 17
end constructor

destructor Tracked()
	if( value <> 17 ) then end 10
	destroyed += 1
end destructor

private sub UnusedMetadata()
	dim as Tracked values(3 to 5)
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
	if( constructed <> 3 or destroyed <> 0 ) then end 1
end sub

private function Inspect(values() as Tracked) as long
	if( lbound(values) <> 7 or ubound(values) <> 9 ) then return -1
	return lbound(values) * 10 + ubound(values) + values(7).value
end function

private sub RequiredMetadata()
	dim as Tracked values(7 to 9)
	if( Inspect(values()) <> 96 ) then end 2
end sub

private sub DynamicLifetime()
	dim as Tracked dynamic_values()
	redim dynamic_values(1 to 2)
	if( lbound(dynamic_values) <> 1 or ubound(dynamic_values) <> 2 ) then end 3
	if( dynamic_values(1).value <> 17 or dynamic_values(2).value <> 17 ) then end 3
	erase dynamic_values
	dim as string managed_strings(0 to 2)
	managed_strings(0) = "retained"
	if( managed_strings(0) <> "retained" ) then end 4
end sub

ExplicitInitialization()
UnusedMetadata()
if( constructed <> 3 or destroyed <> 3 ) then end 5
RequiredMetadata()
if( constructed <> 6 or destroyed <> 6 ) then end 6
DynamicLifetime()
if( constructed <> 8 or destroyed <> 8 ) then end 7
print "C_OWNED_ARRAY_METADATA_PASS"

/' end of c-owned-array-metadata.bas '/
