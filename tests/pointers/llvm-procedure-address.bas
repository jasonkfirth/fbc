'' Project: FreeBASIC procedure pointer tests
'' File: llvm-procedure-address.bas
'' Purpose: Exercise procedure addresses without reading variable storage.
'' Responsibilities: Check scalar and aggregate results through procedure pointers.
'' This file intentionally does NOT contain: external functions or runtime dispatch.

#include "fbcunit.bi"

SUITE( fbc_tests.pointers.llvm_procedure_address )

	'' Four DOUBLEs require a memory result on the System V x86-64 ABI.
	type Result
		values(0 to 3) as double
	end type

	private function scalar() as long
		return 42
	end function

	private function floating() as double
		return 3.5
	end function

	private function aggregate() as Result
		dim value as Result
		for i as integer = 0 to 3
			value.values(i) = i + 1
		next
		return value
	end function

	private function apply( byval callback as function() as long ) as long
		return callback()
	end function

	TEST( addresses )
		dim scalar_callback as function() as long = @scalar
		dim floating_callback as function() as double = @floating
		dim aggregate_callback as function() as Result = @aggregate
		CU_ASSERT_EQUAL( scalar_callback(), 42 )
		CU_ASSERT_EQUAL( floating_callback(), 3.5 )
		dim value as Result = aggregate_callback()
		for i as integer = 0 to 3
			CU_ASSERT_EQUAL( value.values(i), i + 1 )
		next
		CU_ASSERT_EQUAL( apply(@scalar), 42 )
	END_TEST

END_SUITE

'' end of llvm-procedure-address.bas
