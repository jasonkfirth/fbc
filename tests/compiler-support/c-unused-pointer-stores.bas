/'
	Project: FreeBASIC C backend controls
	File: c-unused-pointer-stores.bas
	Purpose: Observe pointer store evaluation and allocation ownership.
	Responsibilities: Retain strings and exactly one tiny tracked allocation.
	This file intentionally does NOT allocate unbounded memory.
'/
#lang "fb"
type Metadata
	data as string ptr
	count as integer
end type
private sub metadataValues()
	dim values(0 to 1) as string => {"alpha", "beta"}
	dim unobserved as Metadata = any
	unobserved.data = @values(0)
	unobserved.count = 2
	if values(0) <> "alpha" or values(1) <> "beta" then end 1
end sub
type TrackedValue
	number as integer
	declare operator new(byval bytes as uinteger) as any ptr
	declare operator delete(byval address as any ptr)
	declare constructor()
	declare destructor()
end type
dim shared as TrackedValue ptr allocation
dim shared as integer allocations, constructors, destructors, releases
operator TrackedValue.new(byval bytes as uinteger) as any ptr
	if bytes <> sizeof(TrackedValue) then end 2
	allocation = allocate(bytes)
	if allocation = 0 then end 3
	allocations += 1
	return allocation
end operator
operator TrackedValue.delete(byval address as any ptr)
	if address <> allocation then end 4
	releases += 1
	deallocate(address)
end operator
constructor TrackedValue()
	constructors += 1
	number = 61
end constructor
destructor TrackedValue()
	if number <> 61 then end 5
	destructors += 1
end destructor
private sub discardedAllocation()
	dim ignored as TrackedValue ptr = new TrackedValue
end sub
metadataValues()
discardedAllocation()
if allocations <> 1 or constructors <> 1 then end 6
if allocation = 0 then end 7
delete allocation
if destructors <> 1 or releases <> 1 then end 8
print "POINTER_STORE_VALUES_PASS"
'' end of c-unused-pointer-stores.bas
