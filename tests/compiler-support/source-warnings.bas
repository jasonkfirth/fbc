/'
 Project: FreeBASIC source diagnostic controls
 File: source-warnings.bas
 Purpose: Exercise warnings retained independently of emitted C diagnostics.
 Responsibilities: Select positive examples and conservative negative controls.
 This file intentionally does NOT run its null-access examples.
'/
#lang "fb"

#if SOURCE_WARNING_CASE = 1
sub warning_values(byval ignored as integer, byval observed as integer, byref output_value as integer)
	dim abandoned as integer
	dim assigned_only as integer
	assigned_only = 4
	output_value = observed
end sub
dim result as integer
warning_values(1, 2, result)
print result
#elseif SOURCE_WARNING_CASE = 2
dim signed_value as long
if signed_value = &h80000000 then print "unreachable on a 64-bit target"
#elseif SOURCE_WARNING_CASE = 3
dim p as integer ptr = 0
*p = 2
#elseif SOURCE_WARNING_CASE = 4
dim p as integer ptr = 0
dim q as integer ptr = @*p
print q, sizeof(*p)
dim value as integer
p = @value
*p = 2
print value
#elseif SOURCE_WARNING_CASE = 5
sub replace_pointer(byref p as integer ptr)
	p = cptr(integer ptr, 1)
end sub
dim p as integer ptr = 0
replace_pointer(p)
*p = 2
#elseif SOURCE_WARNING_CASE = 6
dim p as integer ptr = 0
dim value as integer
if command(1) <> "" then p = @value
*p = 2
#elseif SOURCE_WARNING_CASE = 7
sub opaque_body(byval ignored as integer)
	dim local_value as integer
	asm
		nop
	end asm
end sub
opaque_body(1)
#elseif SOURCE_WARNING_CASE = 8
type sample
	value as integer
	declare constructor()
	declare destructor()
end type
constructor sample()
	value = 1
end constructor
destructor sample()
end destructor
dim instance as sample
dim text as string = str(instance.value)
print text
#elseif SOURCE_WARNING_CASE = 9
dim value as ubyte
if value < 0 then print "below range"
if value >= 0 then print "within lower bound"
if value > 255 then print "above range"
dim signed_value as long
if signed_value = culng(&h80000000) then print "target dependent"
#elseif SOURCE_WARNING_CASE = 10
sub unused_values(byval first_value as integer, byval second_value as integer)
end sub
unused_values(1, 2)
#elseif SOURCE_WARNING_CASE = 11
sub include_locations()
	#include "source-warning-location.bi"
end sub
include_locations()
#elseif SOURCE_WARNING_CASE = 12
dim value as integer
dim p as integer ptr = @value
scope
	dim p as integer ptr = 0
	*p = 2
end scope
*p = 3
print value
#elseif SOURCE_WARNING_CASE = 13
dim p as integer ptr = any
*p = 2
#elseif SOURCE_WARNING_CASE = 14
sub output_only(byref value as integer)
	value = 7
end sub
dim value as integer
output_only(value)
print value
#elseif SOURCE_WARNING_CASE = 15
dim shared shared_value as integer
shared_value = 1
#elseif SOURCE_WARNING_CASE = 16
dim p as integer ptr = 0
*cptr(integer ptr, 16) = 2
print p
#elseif SOURCE_WARNING_CASE = 17
dim module_unused as integer
dim module_assigned as integer
module_assigned = 4
#elseif SOURCE_WARNING_CASE = 18
sub static_value()
	static assigned_static as integer
	assigned_static = 2
end sub
static_value()
#elseif SOURCE_WARNING_CASE = 19
sub overwritten_parameter(byval value as integer, byref output_value as integer)
	value = 2
	output_value = 3
end sub
dim value as integer
overwritten_parameter(1, value)
print value
#elseif SOURCE_WARNING_CASE = 20
function change_pointer(byref p as integer ptr) as integer
	p = cptr(integer ptr, 1)
	return 3
end function
dim p as integer ptr = 0
*p = change_pointer(p)
#elseif SOURCE_WARNING_CASE = 21
dim pp as integer ptr ptr = 0
dim p as integer ptr = @**pp
print p
#elseif SOURCE_WARNING_CASE = 22
type layout_sample
	first_value as integer
	second_value as integer
end type
print offsetof(layout_sample, second_value)
#elseif SOURCE_WARNING_CASE = 23
dim value as ubyte = 1
dim p as ubyte ptr = @value
print *p >= 0
#endif

'' end of source-warnings.bas
