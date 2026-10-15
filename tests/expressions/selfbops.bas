'' FreeBASIC expression regression tests
'' File: selfbops.bas
'' Purpose: Check compound assignment, floating grouping and retained side effects.
'' Responsibilities: Bitfield updates, precise grouping and single evaluation.
'' This file contains no optimizer or floating-point backend implementation.

# include once "fbcunit.bi"

SUITE( fbc_tests.expressions.selfbops )

	TEST_GROUP( bitfields )
		type UDT
			a : 1 as integer
			b : 4 as integer
		end type

		TEST( default )
			scope
				dim x as UDT
				CU_ASSERT( x.a = 0 )
				CU_ASSERT( x.b = 0 )

				x.a += 1
				CU_ASSERT( x.a = 1 )
				CU_ASSERT( x.b = 0 )

				x.b += 3
				CU_ASSERT( x.a = 1 )
				CU_ASSERT( x.b = 3 )

				x.a and= -1
				CU_ASSERT( x.a = 1 )
				CU_ASSERT( x.b = 3 )

				x.a and= 0
				CU_ASSERT( x.a = 0 )
				CU_ASSERT( x.b = 3 )

				x.b and= 1
				CU_ASSERT( x.a = 0 )
				CU_ASSERT( x.b = 1 )
			end scope

			scope
				dim x as UDT
				var px = @x
				CU_ASSERT( px->a = 0 )
				CU_ASSERT( px->b = 0 )

				px->a += 1
				CU_ASSERT( px->a = 1 )
				CU_ASSERT( px->b = 0 )

				px->b += 3
				CU_ASSERT( px->a = 1 )
				CU_ASSERT( px->b = 3 )

				px->a and= -1
				CU_ASSERT( px->a = 1 )
				CU_ASSERT( px->b = 3 )

				px->a and= 0
				CU_ASSERT( px->a = 0 )
				CU_ASSERT( px->b = 3 )

				px->b and= 1
				CU_ASSERT( px->a = 0 )
				CU_ASSERT( px->b = 1 )
			end scope
		END_TEST
	END_TEST_GROUP

#if __FB_FPMODE__ <> "fast"
	TEST_GROUP( floatingPointAssociation )
		TEST( preciseModePreservesRightHandGrouping )
			dim as double largeA = 10000000000000000.0
			dim as double largeB = 10000000000000000.0
			dim as double small = 3.0
			dim as double hugeA = 1.0e308
			dim as double hugeB = 1.0e308
			dim as double tiny = 1.0e-308
			dim as double constantBase = 10000000000000000.0
			dim as double constantScale = 9007199254740992.0
			dim as double extendedScale = 18446744073709551616.0
			dim as double constantHuge = 1.0e308
			dim as double one = 1.0, three = 3.0, ten = 10.0, tenth = 0.1
			dim as double scaledBase = constantScale * three
			dim as double extendedBase = extendedScale * three
			dim as double compound = -1.0
			dim as double assignment = -1.0
			dim as double constantAdd = any
			dim as double constantDistribution = any
			dim as double referenceDistribution = any
			dim as double extendedDistribution = any
			dim as double referenceExtendedDistribution = any
			dim as double constantMultiply = any
			dim as double referenceMultiply = any

			compound += (largeA - largeB) + small
			assignment = assignment + ((largeA - largeB) + small)

			CU_ASSERT( compound = 2.0 )
			CU_ASSERT( assignment = 2.0 )

			compound = -1.0
			assignment = -1.0
			compound += hugeA * (hugeB * tiny)
			assignment = assignment + (hugeA * (hugeB * tiny))
			CU_ASSERT( compound > 5.0e307 )
			CU_ASSERT( compound < 1.5e308 )
			CU_ASSERT( assignment > 5.0e307 )
			CU_ASSERT( assignment < 1.5e308 )

			constantAdd = (constantBase - 10000000000000000.0) + 1.0
			'' Precise mode preserves grouping without requiring x87 to narrow
			'' every intermediate to Double. Variable operands provide a reference
			'' that cannot use the constant-only reassociation optimizations.
			'' Test the 53-bit and 64-bit significand boundaries, subtracting the
			'' exact scaled base before storage so either FPU exposes regrouping.
			constantDistribution = ((constantScale + 1.0) * 3.0) - scaledBase
			referenceDistribution = ((constantScale + one) * three) - scaledBase
			extendedDistribution = ((extendedScale + 1.0) * 3.0) - extendedBase
			referenceExtendedDistribution = ((extendedScale + one) * three) - extendedBase
			constantMultiply = ((constantHuge * 10.0) * 0.1) - constantHuge
			referenceMultiply = ((constantHuge * ten) * tenth) - constantHuge
			CU_ASSERT( constantAdd = 1.0 )
			CU_ASSERT( constantDistribution = referenceDistribution )
			CU_ASSERT( extendedDistribution = referenceExtendedDistribution )
			CU_ASSERT( ((constantScale + 1.0) * 3.0) = ((constantScale + one) * three) )
			CU_ASSERT( ((extendedScale + 1.0) * 3.0) = ((extendedScale + one) * three) )
			CU_ASSERT( constantMultiply = referenceMultiply )
		END_TEST
	END_TEST_GROUP
#endif

	TEST_GROUP( sidefxRemoval )
		dim shared as integer f1calls, f2calls
		dim shared as integer i

		function f1( ) as integer
			f1calls += 1
			function = 0
		end function

		function f2( ) byref as integer
			f2calls += 1
			function = i
		end function

		TEST( default )
			dim array(0 to 0) as integer
			CU_ASSERT( f1calls = 0 )
			CU_ASSERT( array(0) = 0 )
			array(f1( )) += 123
			CU_ASSERT( f1calls = 1 )
			CU_ASSERT( array(0) = 123 )

			CU_ASSERT( f2calls = 0 )
			CU_ASSERT( i = 0 )
			f2( ) += 456
			CU_ASSERT( f2calls = 1 )
			CU_ASSERT( i = 456 )
		END_TEST
	END_TEST_GROUP

	TEST_GROUP( sidefxRemovalWithBitfields )
		type UDT
			a : 1 as integer
			b : 1 as integer
		end type

		dim shared as integer fcalls
		dim shared as UDT x

		function f( ) byref as UDT
			fcalls += 1
			function = x
		end function

		TEST( default )
			CU_ASSERT( fcalls = 0 )
			CU_ASSERT( x.a = 0 )
			CU_ASSERT( x.b = 0 )
			f( ).a += 1
			CU_ASSERT( fcalls = 1 )
			CU_ASSERT( x.a = 1 )
			CU_ASSERT( x.b = 0 )
			f( ).b += 1
			CU_ASSERT( fcalls = 2 )
			CU_ASSERT( x.a = 1 )
			CU_ASSERT( x.b = 1 )
		END_TEST
	END_TEST_GROUP

	TEST_GROUP( sidefxRemovalWithStringConcat )
		dim shared as integer fcalls
		dim shared as integer globali
		dim shared as string a, b

		function f( byval i as integer, byref s as string ) byref as integer
			if( i = 0 ) then
				CU_ASSERT( s = a + b )
			else
				CU_ASSERT( s = b + a )
			end if
			fcalls += 1
			function = globali
		end function

		TEST( default )
			a = "a"
			b = "b"

			CU_ASSERT( fcalls = 0 )
			CU_ASSERT( globali = 0 )
			f( 0, a + b ) += 5
			CU_ASSERT( fcalls = 1 )
			CU_ASSERT( globali = 5 )
			f( 1, b + a ) += 5
			CU_ASSERT( fcalls = 2 )
			CU_ASSERT( globali = 10 )
		END_TEST
	END_TEST_GROUP

END_SUITE

'' end of selfbops.bas
