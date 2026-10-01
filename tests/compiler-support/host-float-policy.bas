'' Project: FreeBASIC compiler host policy tests
'' ------------------------------------------
''
'' File: host-float-policy.bas
''
'' Purpose:
''
''     Exercise the host arithmetic interface independently of AST folding.
''
'' Responsibilities:
''
''     - check signed zero, unordered comparisons, and finite arithmetic
''     - check the DOUBLE word normalization policy with explicit bit fixtures
''
'' This file intentionally does NOT contain:
''
''     - target runtime arithmetic tests or a second AST implementation
''

#include once "support/numeric/fp-policy.bi"
#include once "ast/ast-op.bi"

dim as ulongint negativezerobits = &h8000000000000000ull
dim as double negativezero = *cptr( double ptr, @negativezerobits )
dim as ulongint nanbits = &h7FF8000000000001ull
dim as double nanvalue = *cptr( double ptr, @nanbits )

assert( fbHostFloatIsZero( 0.0 ) )
assert( fbHostFloatIsZero( negativezero ) )
assert( fbHostFloatIsZero( nanvalue ) = FALSE )
assert( fbHostFloatGeZero( negativezero ) )
assert( fbHostFloatGeZero( nanvalue ) = FALSE )
assert( fbHostFloatCompare( AST_OP_EQ, 0.0, negativezero ) )
assert( fbHostFloatCompare( AST_OP_LT, 0.0, negativezero ) = FALSE )
assert( fbHostFloatCompare( AST_OP_NE, nanvalue, 1.0 ) )
assert( fbHostFloatCompare( AST_OP_EQ, nanvalue, nanvalue ) = FALSE )
assert( fbHostFloatCompare( AST_OP_GT, nanvalue, 1.0 ) = FALSE )
assert( fbHostFloatCompare( AST_OP_LE, 1.0, nanvalue ) = FALSE )

for leftvalue as integer = -4 to 4
	for rightvalue as integer = -4 to 4
		assert( fbHostFloatCompare( AST_OP_EQ, leftvalue, rightvalue ) = (leftvalue = rightvalue) )
		assert( fbHostFloatCompare( AST_OP_NE, leftvalue, rightvalue ) = (leftvalue <> rightvalue) )
		assert( fbHostFloatCompare( AST_OP_LT, leftvalue, rightvalue ) = (leftvalue < rightvalue) )
		assert( fbHostFloatCompare( AST_OP_GT, leftvalue, rightvalue ) = (leftvalue > rightvalue) )
		assert( fbHostFloatCompare( AST_OP_LE, leftvalue, rightvalue ) = (leftvalue <= rightvalue) )
		assert( fbHostFloatCompare( AST_OP_GE, leftvalue, rightvalue ) = (leftvalue >= rightvalue) )
	next
next

dim as integer hadfraction
assert( fbHostFloatFix( 1.75, hadfraction ) = 1.0 )
assert( hadfraction )
assert( fbHostFloatFix( -1.75, hadfraction ) = -1.0 )
assert( hadfraction )
assert( fbHostFloatFix( 2.0, hadfraction ) = 2.0 )
assert( hadfraction = FALSE )
assert( fbHostFloatFloor( -1.75 ) = -2.0 )
assert( fbHostFloatFloor( 1.75 ) = 1.0 )
assert( fbHostFloatSgn( -1.75 ) = -1.0 )
assert( fbHostFloatSgn( 1.75 ) = 1.0 )
assert( fbHostFloatSgn( negativezero ) = 0.0 )
assert( fbHostFloatToULongint( 0.5 ) = 0 )
assert( fbHostFloatToULongint( 0.75 ) = 1 )
assert( fbHostFloatToULongint( 1.5 ) = 2 )
assert( fbHostFloatToULongint( 2.5 ) = 2 )
assert( fbHostFloatToULongint( 3.5 ) = 4 )
assert( fbHostFloatToULongint( 4294967296.0 ) = 4294967296ull )
assert( fbHostFloatPow( 2.0, 3.0 ) = 8.0 )

#ifdef FB_TEST_APCS_BITS
'' The test process uses native little-endian DOUBLE storage. Feed the raw
'' APCS word order explicitly so this test validates the normalization step.
dim as ulongint apcsbits = &h000000003FF00000ull
dim as double apcsvalue = *cptr( double ptr, @apcsbits )
assert( fbHostFloatBits( apcsvalue ) = &h3FF0000000000000ull )
#else
assert( fbHostFloatBits( 1.0 ) = &h3FF0000000000000ull )
assert( fbHostFloatBits( negativezero ) = negativezerobits )
#endif

print "host float policy passed"

'' end of host-float-policy.bas
