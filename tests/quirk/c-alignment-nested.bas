'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC C alignment qualification
'' File: c-alignment-nested.bas
'' Purpose: Exercise unaligned nested aggregates as values and BYREF storage.
'' Responsibilities: Cover clearing, member copies, parameter identity and stores.
'' This file intentionally does NOT allocate outside its fixed byte buffer.
#lang "fb"
type nested_value
	marker as ubyte
	value as longint
end type
type nested_container
	marker as ubyte
	child as nested_value
end type
private function copy_value( byval value as nested_value ) as nested_value
	return value
end function
private sub increment_value( byref value as nested_value )
	value.value += 1
end sub
private sub clear_container( byval container as nested_container ptr )
	*container = type<nested_container>(0)
end sub
dim bytes(0 to 255) as ubyte
dim as nested_container ptr container = cptr(nested_container ptr, @bytes(1))
clear_container(container)
if( container->marker <> 0 or container->child.value <> 0 ) then end 1
container->marker = 77
container->child.marker = 88
container->child.value = 123456789012345ll
dim as nested_value aligned_copy = copy_value(container->child)
if( aligned_copy.marker <> 88 or aligned_copy.value <> 123456789012345ll ) then end 2
increment_value(container->child)
if( container->child.value <> 123456789012346ll ) then end 3
container->child = aligned_copy
if( container->marker <> 77 or container->child.value <> 123456789012345ll ) then end 4
print "aggregate-nested-ok"
'' end of c-alignment-nested.bas
