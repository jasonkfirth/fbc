'' Project: FreeBASIC compiler tests
'' File: functions/byval-arguments.bas
'' Purpose: Check explicit value arguments and copies of scalar objects.
'' Array parameters are covered in dim/byval-array.bas.

#include "fbcunit.bi"

SUITE( fbc_tests.functions.byval_arguments )

	sub addTo( byref total as integer, amount as integer )
		total += amount
		amount = 0
	end sub

	function twice overload( amount as integer ) as integer
		return amount * 2
	end function

	function twice overload( byref text as string ) as string
		return text & text
	end function

	type Details
		text as string
		values(any) as integer
	end type

	type Record
		detail as Details
	end type

	function changeRecord( byval value as Record ) as string
		value.detail.text = "local"
		value.detail.values(-1) += 1
		return value.detail.text & str( value.detail.values(-1) )
	end function

	type ConstCopy
		number as integer
		declare constructor( )
		declare constructor( byref rhs as const ConstCopy )
	end type

	constructor ConstCopy( )
	end constructor

	constructor ConstCopy( byref rhs as const ConstCopy )
		number = rhs.number + 1
	end constructor

	function copyResult( byval value as ConstCopy ) as integer
		return value.number
	end function

	TEST( explicitScalar )
		dim as integer total = 1, amount = 3
		addTo total, byval amount
		addTo total, (byval amount)
		addTo( byval @total, (byval amount) )
		CU_ASSERT( total = 10 )
		CU_ASSERT( amount = 3 )
		CU_ASSERT( twice( byval amount ) = 6 )
		CU_ASSERT( twice( ((byval amount)) ) = 6 )

		dim as sub( byref as integer, as integer ) apply = @addTo
		apply( total, (byval amount) )
		CU_ASSERT( total = 13 )
	END_TEST

	TEST( scalarUdt )
		dim as Record value
		value.detail.text = "caller"
		redim value.detail.values(-1 to 0)
		value.detail.values(-1) = 4

		CU_ASSERT( changeRecord( value ) = "local5" )
		CU_ASSERT( changeRecord( (byval value) ) = "local5" )
		CU_ASSERT( value.detail.text = "caller" )
		CU_ASSERT( value.detail.values(-1) = 4 )
	END_TEST

	TEST( constCopyConstructor )
		dim as ConstCopy value
		value.number = 7
		CU_ASSERT( copyResult( value ) = 8 )
		CU_ASSERT( copyResult( (byval value) ) = 8 )
		CU_ASSERT( value.number = 7 )
	END_TEST

END_SUITE

'' end of functions/byval-arguments.bas
