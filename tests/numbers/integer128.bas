'' Project: FreeBASIC test suite
'' -----------------------------------------
''
'' File: numbers/integer128.bas
''
'' Purpose:
''
''     Check the 128-bit integer type and its runtime operations through
''     the regular fbctests suite.
''
'' Responsibilities:
''
''     - verify storage layout, arithmetic, comparisons, shifts, and casts
''     - exercise procedure arguments, results, and function pointers
''
'' This file intentionally does NOT contain:
''
''     - backend-specific assembly or C ABI fixtures
''

#include "fbcunit.bi"

type Integer128Adder as function _
	(byval leftValue as uinteger<128>, byval rightValue as uinteger<128>) as uinteger<128>

type Integer128Layout
	prefix as ubyte
	signedValue as integer<128>
	unsignedValue as uinteger<128>
	suffix as ubyte
end type

#ifdef __FB_WINCE__
	'' CE qualification can compile these tests without a licensed ROM.
	'' Keep target layout checked even when the suite cannot execute there.
	#assert offsetof( Integer128Layout, signedValue ) = 8
	#assert offsetof( Integer128Layout, unsignedValue ) = 24
	#assert offsetof( Integer128Layout, suffix ) = 40
	#assert sizeof( Integer128Layout ) = 48
#endif

private function addUnsigned128 _
	(byval leftValue as uinteger<128>, byval rightValue as uinteger<128>) as uinteger<128>
	return leftValue + rightValue
end function

private function adjustSigned128(byval value as integer<128>) as integer<128>
	value += 1
	return value
end function

private sub incrementSigned128(byref value as integer<128>)
	value += 1
end sub

dim shared as integer<128> sharedNegativeOne = -1
dim shared as uinteger<128> sharedLowWordMaximum = &hFFFFFFFFFFFFFFFFull

SUITE( fbc_tests.numbers.integer128 )

	TEST( layout )
		CU_ASSERT_EQUAL( sizeof( integer<128> ), 16 )
		CU_ASSERT_EQUAL( sizeof( uinteger<128> ), 16 )

		dim as Integer128Layout layoutValue
		dim as ubyte ptr baseAddress = @layoutValue
		#ifdef __FB_M68K__
			CU_ASSERT_EQUAL( cuint( @layoutValue.signedValue ) - cuint( baseAddress ), 2 )
			CU_ASSERT_EQUAL( cuint( @layoutValue.unsignedValue ) - cuint( baseAddress ), 18 )
			CU_ASSERT_EQUAL( cuint( @layoutValue.suffix ) - cuint( baseAddress ), 34 )
			CU_ASSERT_EQUAL( sizeof( layoutValue ), 36 )
		#elseif defined( __FB_WINCE__ )
			CU_ASSERT_EQUAL( cuint( @layoutValue.signedValue ) - cuint( baseAddress ), 8 )
			CU_ASSERT_EQUAL( cuint( @layoutValue.unsignedValue ) - cuint( baseAddress ), 24 )
			CU_ASSERT_EQUAL( cuint( @layoutValue.suffix ) - cuint( baseAddress ), 40 )
			CU_ASSERT_EQUAL( sizeof( layoutValue ), 48 )
			CU_ASSERT_EQUAL( cuint( baseAddress ) and 7, 0 )
		#else
			CU_ASSERT_EQUAL( cuint( @layoutValue.signedValue ) - cuint( baseAddress ), 16 )
			CU_ASSERT_EQUAL( cuint( @layoutValue.unsignedValue ) - cuint( baseAddress ), 32 )
			CU_ASSERT_EQUAL( cuint( @layoutValue.suffix ) - cuint( baseAddress ), 48 )
			CU_ASSERT_EQUAL( sizeof( layoutValue ), 64 )
			#if defined( __FB_WIN32__ ) and (__FB_BACKEND__ = "gas")
				'' GAS32 preserves the four-byte Win32 stack ABI. Field layout
				'' still uses the natural 16-byte alignment within the UDT.
				CU_ASSERT_EQUAL( cuint( baseAddress ) and 3, 0 )
			#else
			CU_ASSERT_EQUAL( cuint( baseAddress ) and 15, 0 )
			#endif
		#endif
	END_TEST

	TEST( add_subtract_multiply )
		dim as uinteger<128> oneValue = 1
		dim as uinteger<128> lowWordMaximum = (oneValue shl 64) - 1
		dim as uinteger<128> highWordOne = oneValue shl 64
		dim as uinteger<128> highValue = oneValue shl 100
		dim as uinteger<128> allBits = -1

		'' Carry and borrow must cross the 64-bit word boundary.
		CU_ASSERT_EQUAL( lowWordMaximum + 1, highWordOne )
		CU_ASSERT_EQUAL( highWordOne - 1, lowWordMaximum )
		CU_ASSERT_EQUAL( (highValue + 7) - highValue, 7 )

		'' Multiplication must retain the upper word and wrap at 128 bits.
		CU_ASSERT_EQUAL( (oneValue shl 80) * 3, (oneValue shl 80) + (oneValue shl 81) )
		CU_ASSERT_EQUAL( allBits + 1, 0 )
		CU_ASSERT_EQUAL( (oneValue shl 127) * 2, 0 )

		'' Compound assignments use the same wide arithmetic as ordinary ops.
		dim as uinteger<128> compoundValue = lowWordMaximum
		compoundValue += 1
		CU_ASSERT_EQUAL( compoundValue, highWordOne )
		compoundValue -= 1
		CU_ASSERT_EQUAL( compoundValue, lowWordMaximum )
		compoundValue *= 3
		CU_ASSERT_EQUAL( compoundValue, lowWordMaximum * 3 )
	END_TEST

	TEST( division_remainder )
		dim as uinteger<128> oneValue = 1
		dim as uinteger<128> highValue = oneValue shl 100
		dim as uinteger<128> quotientValue = highValue \ 5
		dim as uinteger<128> remainderValue = highValue mod 5
		dim as uinteger<128> allBits = -1
		dim as integer<128> signedHighValue = -cast( integer<128>, highValue )
		dim as integer<128> signedHighQuotient = signedHighValue \ 7
		dim as integer<128> signedHighRemainder = signedHighValue mod 7

		CU_ASSERT_EQUAL( quotientValue * 5 + remainderValue, highValue )
		CU_ASSERT_EQUAL( remainderValue, 1 )
		CU_ASSERT_EQUAL( (allBits \ 3) * 3 + (allBits mod 3), allBits )
		CU_ASSERT_EQUAL( allBits mod 3, 0 )

		CU_ASSERT_EQUAL( cint<128>( -100 ) \ 7, -14 )
		CU_ASSERT_EQUAL( cint<128>( -100 ) mod 7, -2 )
		CU_ASSERT_EQUAL( cint<128>( 100 ) \ -7, -14 )
		CU_ASSERT_EQUAL( cint<128>( 100 ) mod -7, 2 )
		CU_ASSERT_EQUAL( signedHighQuotient * 7 + signedHighRemainder, signedHighValue )
		CU_ASSERT( signedHighRemainder < 0 )
		CU_ASSERT( signedHighRemainder > -7 )
	END_TEST

	TEST( bitwise_and_shifts )
		dim as uinteger<128> oneValue = 1
		dim as uinteger<128> highValue = oneValue shl 100
		dim as uinteger<128> highWordOne = oneValue shl 64
		dim as uinteger<128> signBit = oneValue shl 127
		dim as uinteger<128> allBits = -1
		dim as uinteger<128> lowWordMaximum = highWordOne - 1
		dim as uinteger<128> pattern = highValue or 15

		CU_ASSERT_EQUAL( pattern and 15, 15 )
		CU_ASSERT_EQUAL( pattern xor 15, highValue )
		CU_ASSERT_EQUAL( oneValue eqv oneValue, allBits )
		CU_ASSERT_EQUAL( 0 imp oneValue, allBits )

		CU_ASSERT_EQUAL( oneValue shl 0, oneValue )
		CU_ASSERT_EQUAL( oneValue shl 63, &h8000000000000000ull )
		CU_ASSERT_EQUAL( highWordOne shr 64, oneValue )
		CU_ASSERT_EQUAL( oneValue shl 65, highWordOne shl 1 )
		CU_ASSERT_EQUAL( oneValue shl 127, signBit )
		CU_ASSERT_EQUAL( signBit shr 127, 1 )
		CU_ASSERT_EQUAL( signBit shr 64, oneValue shl 63 )
		CU_ASSERT_EQUAL( signBit shr 63, highWordOne )
		CU_ASSERT_EQUAL( highValue shr 64, oneValue shl 36 )
		CU_ASSERT_EQUAL( cint<128>( -8 ) shr 2, -2 )
		CU_ASSERT_EQUAL( cint<128>( -1 ) shr 127, -1 )
	END_TEST

	TEST( comparisons_and_unary_ops )
		dim as uinteger<128> oneValue = 1
		dim as uinteger<128> signBit = oneValue shl 127
		dim as uinteger<128> allBits = -1
		dim as integer<128> signedMinimum = cast( integer<128>, signBit )
		dim as integer<128> signedMaximum = cast( integer<128>, signBit - 1 )
		dim as integer<128> negativeWideValue = -cast( integer<128>, oneValue shl 100 )
		dim as integer<128> negativeValue = -8
		dim as integer<128> zeroValue = 0

		CU_ASSERT( allBits > signBit )
		CU_ASSERT( allBits >= signBit )
		CU_ASSERT( signBit > (signBit - 1) )
		CU_ASSERT( signBit >= signBit )
		CU_ASSERT( signBit <= signBit )
		CU_ASSERT( (signBit - 1) < signBit )
		CU_ASSERT( (signBit - 1) <= signBit )
		CU_ASSERT( (signBit - 1) <> signBit )
		CU_ASSERT( signedMinimum < -1 )
		CU_ASSERT( signedMinimum < signedMaximum )
		CU_ASSERT( signedMaximum > 0 )
		CU_ASSERT( negativeValue < 0 )
		CU_ASSERT( negativeValue <> 0 )
		CU_ASSERT_EQUAL( -negativeWideValue, cast( integer<128>, oneValue shl 100 ) )

		CU_ASSERT_EQUAL( not zeroValue, -1 )
		CU_ASSERT_EQUAL( abs( negativeValue ), 8 )
		CU_ASSERT_EQUAL( abs( negativeWideValue ), cast( integer<128>, oneValue shl 100 ) )
		CU_ASSERT_EQUAL( abs( signBit ), signBit )
		CU_ASSERT_EQUAL( sgn( negativeValue ), -1 )
		CU_ASSERT_EQUAL( sgn( signedMinimum ), -1 )
		CU_ASSERT_EQUAL( sgn( zeroValue ), 0 )
		CU_ASSERT_EQUAL( sgn( signBit ), 1 )
	END_TEST

	TEST( conversions )
		dim as integer<128> negativeValue = -1
		dim as integer<128> positiveTieEven = 2.5
		dim as integer<128> positiveTieOdd = 3.5
		dim as integer<128> negativeTieEven = -2.5
		dim as integer<128> negativeTieOdd = -3.5
		dim as integer<128> roundedPositive = 12.75
		dim as integer<128> roundedNegative = -12.75
		dim as single singleTie = 3.5
		dim as integer<128> roundedSingleTie = singleTie
		dim as uinteger<128> unsignedTieEven = 2.5
		dim as uinteger<128> unsignedTieOdd = 3.5
		dim as uinteger<128> unsignedNegativeValue = -1
		dim as uinteger<128> zeroUnsigned = 0
		dim as uinteger<128> signBit = cuint<128>( 1 ) shl 127
		dim as ulongint lowWord
		dim as double signedAsDouble
		dim as double wideSignedAsDouble
		dim as double unsignedAsDouble

		CU_ASSERT_EQUAL( negativeValue shr 64, -1 )
		CU_ASSERT_EQUAL( unsignedNegativeValue, not zeroUnsigned )
		CU_ASSERT_EQUAL( positiveTieEven, 2 )
		CU_ASSERT_EQUAL( positiveTieOdd, 4 )
		CU_ASSERT_EQUAL( negativeTieEven, -2 )
		CU_ASSERT_EQUAL( negativeTieOdd, -4 )
		CU_ASSERT_EQUAL( roundedPositive, 13 )
		CU_ASSERT_EQUAL( roundedNegative, -13 )
		CU_ASSERT_EQUAL( roundedSingleTie, 4 )
		CU_ASSERT_EQUAL( unsignedTieEven, 2 )
		CU_ASSERT_EQUAL( unsignedTieOdd, 4 )
		CU_ASSERT_EQUAL( sharedNegativeOne shr 64, -1 )
		CU_ASSERT_EQUAL( sharedLowWordMaximum, (cuint<128>( 1 ) shl 64) - 1 )

		lowWord = culngint( signBit )
		CU_ASSERT_EQUAL( lowWord, 0 )
		lowWord = culngint( signBit + 7 )
		CU_ASSERT_EQUAL( lowWord, 7 )
		CU_ASSERT( cbool( signBit ) )
		CU_ASSERT_FALSE( cbool( zeroUnsigned ) )
		signedAsDouble = cdbl( cint<128>( -123 ) )
		wideSignedAsDouble = cdbl( -cast( integer<128>, signBit - 1 ) )
		unsignedAsDouble = cdbl( signBit )
		CU_ASSERT_DOUBLE_EXACT( signedAsDouble, -123 )
		CU_ASSERT( wideSignedAsDouble < 0.0 )
		CU_ASSERT( unsignedAsDouble > 0.0 )
	END_TEST

	TEST( procedure_calls_and_loops )
		dim as uinteger<128> oneValue = 1
		dim as uinteger<128> highValue = oneValue shl 100
		dim as Integer128Adder callback = @addUnsigned128
		dim as integer<128> callerValue = -41
		dim as integer<128> signedSum = 0
		dim as uinteger<128> unsignedSum = 0

		CU_ASSERT_EQUAL( addUnsigned128( highValue, 11 ), highValue + 11 )
		CU_ASSERT_EQUAL( callback( highValue, 11 ), highValue + 11 )
		CU_ASSERT_EQUAL( callback( callback( 1, 2 ), 4 ), 7 )
		CU_ASSERT_EQUAL( adjustSigned128( callerValue ), -40 )
		CU_ASSERT_EQUAL( callerValue, -41 )
		incrementSigned128( callerValue )
		CU_ASSERT_EQUAL( callerValue, -40 )

		for signedIndex as integer<128> = 0 to 3
			signedSum += signedIndex
		next signedIndex
		CU_ASSERT_EQUAL( signedSum, 6 )

		for unsignedIndex as uinteger<128> = 1 to 3 step 1
			unsignedSum += unsignedIndex
		next unsignedIndex
		CU_ASSERT_EQUAL( unsignedSum, 6 )
	END_TEST

END_SUITE

'' end of numbers/integer128.bas
