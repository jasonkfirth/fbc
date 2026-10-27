'' Project: FreeBASIC compiler tests
'' File: c-type-names.bas
'' Purpose: Check C type names without changing BASIC type identity or layout.
'' Responsibilities: Exercise enum/union/recursive types, virtual calls and RTTI.
'' This file intentionally does NOT use external test libraries or platform I/O.
'' TEST_MODE : COMPILE_AND_RUN_OK
#lang "fb"

namespace CodegenNames

	enum ValueKind explicit
		ready = 17
	end enum

	union Storage
		value as long
		bytes(0 to 3) as ubyte
	end union

	type Link as Link_
	type Link_
		next_link as Link ptr
		kind as ValueKind
		payload as Storage
	end type

	type ParentValue extends object
		stored as long
		declare virtual function get_value( ) as long
	end type

	type ChildValue extends ParentValue
		declare function get_value( ) as long override
	end type

	function ParentValue.get_value( ) as long
		return stored
	end function

	function ChildValue.get_value( ) as long
		return stored + 1
	end function

end namespace

using CodegenNames

dim as Link first_link, last_link
first_link.next_link = @last_link
last_link.kind = ValueKind.ready
last_link.payload.value = 23
if( first_link.next_link->kind <> ValueKind.ready ) then end 1
if( first_link.next_link->payload.value <> 23 ) then end 2
if( sizeof( Storage ) <> sizeof( long ) ) then end 3

dim as ChildValue instance
instance.stored = 17
dim as ParentValue ptr base_pointer = @instance
if( base_pointer->get_value( ) <> 18 ) then end 4
if( (*base_pointer is ChildValue) = 0 ) then end 5
print "c-type-names-ok"
end 0

'' end of c-type-names.bas
