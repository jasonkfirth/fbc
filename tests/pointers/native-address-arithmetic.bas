/'
	Project: FreeBASIC compiler tests
	File: native-address-arithmetic.bas
	Purpose: Check integer addresses formed from null and wrapping origins.
	Responsibilities: Exercise signed indices, field layouts, qualifiers and
		single evaluation without dereferencing the calculated addresses.
	This file intentionally does NOT access storage through an invalid pointer.
'/

#include "fbcunit.bi"

SUITE( fbc_tests.pointers.native_address_arithmetic )

	type InnerRecord
		first_value as integer
		second_value as integer
	end type

	type OuterRecord field = 1
		tag as ubyte
		nested as InnerRecord
	end type

	private dim shared as integer ptr current_origin
	private dim shared as integer origin_calls, index_calls
	private dim shared as OuterRecord ptr current_record
	private dim shared as integer record_calls

	private function next_origin() as integer ptr
		origin_calls += 1
		return current_origin
	end function

	private function next_index( byval value as integer ) as integer
		index_calls += 1
		return value
	end function

	private function next_record() as OuterRecord ptr
		record_calls += 1
		return current_record
	end function

	TEST( null_and_wrapping_addresses )
		current_origin = 0
		origin_calls = 0
		index_calls = 0
		dim as uinteger positive = cuint(@(next_origin())[next_index(3)])
		CU_ASSERT_EQUAL( positive, 3 * sizeof(integer) )
		CU_ASSERT_EQUAL( origin_calls, 1 )
		CU_ASSERT_EQUAL( index_calls, 1 )

		dim as uinteger negative = cuint(@current_origin[next_index(-1)])
		CU_ASSERT_EQUAL( negative, not cuint(sizeof(integer) - 1) )
		dim as uinteger top = not cuint(0)
		dim as integer ptr wrap_origin = cast( integer ptr, top - (sizeof(integer) - 1) )
		dim as uinteger wrapped = cuint(@wrap_origin[next_index(1)])
		CU_ASSERT_EQUAL( wrapped, 0 )
	END_TEST

	TEST( element_types_and_owned_storage )
		dim as ubyte ptr bytes = 0
		CU_ASSERT_EQUAL( cuint(@bytes[next_index(3)]), 3 )
		dim as integer ptr ptr pointer_origin = 0
		CU_ASSERT_EQUAL( cuint(@pointer_origin[next_index(2)]), 2 * sizeof(integer ptr) )
		dim as const integer ptr readonly_origin = 0
		CU_ASSERT_EQUAL( cuint(@readonly_origin[next_index(2)]), 2 * sizeof(integer) )

		dim as integer values(0 to 3)
		dim as integer ptr origin = @values(0)
		CU_ASSERT( @origin[next_index(2)] = @values(2) )
	END_TEST

	TEST( indexed_load_and_store )
		dim as integer values(0 to 3)
		dim as integer ptr origin = @values(0)
		origin[next_index(1)] = 42
		CU_ASSERT_EQUAL( origin[next_index(1)], 42 )
	END_TEST

	TEST( field_integer_addresses )
		dim as OuterRecord ptr empty_record = 0
		dim as const OuterRecord ptr const_record = 0
		CU_ASSERT_EQUAL( cuint(@empty_record->tag), 0 )
		CU_ASSERT_EQUAL( cuint(@empty_record->nested.second_value), 1 + sizeof(integer) )
		CU_ASSERT_EQUAL( cuint(@const_record->nested.second_value), 1 + sizeof(integer) )

		current_record = 0
		record_calls = 0
		dim as uinteger address = cuint(@(next_record())->nested.second_value)
		CU_ASSERT_EQUAL( address, 1 + sizeof(integer) )
		CU_ASSERT_EQUAL( record_calls, 1 )

		dim as OuterRecord storage
		dim as OuterRecord ptr record_ptr = @storage
		CU_ASSERT_EQUAL( cuint(@record_ptr->nested.second_value), cuint(@storage) + 1 + sizeof(integer) )
	END_TEST

END_SUITE

/' end of native-address-arithmetic.bas '/
