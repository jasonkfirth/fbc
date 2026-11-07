/'
	Project: FreeBASIC compiler tests
	File: callback-alignment.bas
	Purpose: Check indirect calls whose record pointers may be unaligned.
	Responsibilities: Exercise callback fields with qualified and recursive
		pointer arguments and verify that expressions are evaluated once.
	This file intentionally does NOT contain: external C callback tests.
'/

#include "fbcunit.bi"

SUITE( fbc_tests.structs.callback_alignment )

	type Payload
		value as longint
	end type

	type Callbacks
		update as function( byval as Payload ptr, byval as longint ) as longint
		readptr as function( byval as Payload ptr ptr ) as longint
		readconst as function( byval as const Payload ptr ) as longint
	end type

	type RecursiveNode
		value as longint
		identity as function( byval as RecursiveNode ptr ) as RecursiveNode ptr
	end type

	private dim shared as Payload ptr current_payload
	private dim shared as integer pointer_evaluations

	private function next_payload() as Payload ptr
		pointer_evaluations += 1
		return current_payload
	end function

	private function update_payload( byval value as Payload ptr, byval amount as longint ) as longint
		value->value += amount
		return value->value
	end function

	private function read_payload_ptr( byval value as Payload ptr ptr ) as longint
		return (*value)->value
	end function

	private function read_const_payload( byval value as const Payload ptr ) as longint
		return value->value
	end function

	private function identity_node( byval node as RecursiveNode ptr ) as RecursiveNode ptr
		return node
	end function

	TEST( unaligned_arguments )
		dim as ubyte ptr storage = callocate( sizeof(Payload) + 1 )
		dim as ubyte ptr pointer_storage = callocate( sizeof(Payload ptr) + 1 )
		CU_ASSERT( storage <> 0 )
		CU_ASSERT( pointer_storage <> 0 )
		if( storage <> 0 andalso pointer_storage <> 0 ) then
			current_payload = cast( Payload ptr, storage + 1 )
			dim as Payload ptr ptr pointer_slot = cast( Payload ptr ptr, pointer_storage + 1 )
			*pointer_slot = current_payload
			current_payload->value = 40
			pointer_evaluations = 0
			dim as Callbacks callback_table
			callback_table.update = @update_payload
			callback_table.readptr = @read_payload_ptr
			callback_table.readconst = @read_const_payload
			CU_ASSERT_EQUAL( callback_table.update(next_payload(), 2), 42 )
			CU_ASSERT_EQUAL( pointer_evaluations, 1 )
			CU_ASSERT_EQUAL( callback_table.readptr(pointer_slot), 42 )
			CU_ASSERT_EQUAL( callback_table.readconst(current_payload), 42 )
			current_payload = 0
		end if
		if( pointer_storage <> 0 ) then deallocate( pointer_storage )
		if( storage <> 0 ) then deallocate( storage )
	END_TEST

	TEST( recursive_record_callback )
		dim as ubyte ptr storage = callocate( sizeof(RecursiveNode) + 1 )
		CU_ASSERT( storage <> 0 )
		if( storage <> 0 ) then
			dim as RecursiveNode ptr node = cast( RecursiveNode ptr, storage + 1 )
			node->value = 73
			node->identity = @identity_node
			dim as RecursiveNode ptr returned = node->identity(node)
			CU_ASSERT( returned = node )
			if( returned <> 0 ) then CU_ASSERT_EQUAL( returned->value, 73 )
			deallocate( storage )
		end if
	END_TEST

END_SUITE

/' end of callback-alignment.bas '/
