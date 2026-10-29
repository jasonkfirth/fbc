'' Project: FreeBASIC C backend regression tests
'' File: c-vtable-abi.bas
'' Purpose: Exercise virtual table storage against the established native ABI.
'' Responsibilities: Check dispatch, abstract inheritance, references, RTTI and deletion.
'' This file intentionally does NOT call mismatched procedure signatures.
#include once "c-vtable-abi.bi"

constructor TableLeaf()
	marker = 17
end constructor

function TableLeaf.number(byval value as long) as long
	return marker + value + 100
end function

function TableLeaf.fraction(byval value as double) as double
	return marker + value * 2
end function

destructor TableLeaf()
	vtable_destroyed += 10
end destructor

dim as TableBase ptr instance = new TableLeaf
if instance = 0 then end 1
if instance->number(5) <> 122 then end 2
if instance->fraction(2.5) <> 22 then end 3
instance->reference() = 29
if instance->number(5) <> 134 then end 4
if instance->fraction(2.5) <> 34 then end 5
if not (*instance is TableLeaf) then end 6
if not (*instance is TableAbstract) then end 7

'' The object points at the first method. The native ABI keeps its null
'' header and RTTI pointer immediately before that address.
dim as any ptr ptr slots = *cptr(any ptr ptr ptr, instance)
if slots[-2] <> 0 then end 8
if slots[-1] = 0 then end 9
delete instance
if vtable_destroyed <> 11 then
	print "Unexpected destruction count:"; vtable_destroyed
	end 10
end if
print "VTABLE_ABI_PASS inherited=1 abstract=1 byref=1 rtti=1 destruction=11"

'' end of c-vtable-abi.bas
