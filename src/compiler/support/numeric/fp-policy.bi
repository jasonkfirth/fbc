'' Project: FreeBASIC compiler - compiler host numeric policy
'' -----------------------------------------
''
'' File: support/numeric/fp-policy.bi
''
'' Purpose:
''
''     Define host floating point folding and bit serialization interfaces.
''
'' Responsibilities:
''
''     - describe or implement host arithmetic and numeric serialization
''     - isolate platform constraints from shared AST and parser code
''
'' This file intentionally does NOT contain:
''
''     - target ABI layout or source grammar
''

#ifndef __FP_POLICY_BI__
#define __FP_POLICY_BI__

#include once "support/common.bi"

'' These operations describe the compiler host, not the user's target ABI.
'' The make source graph selects one implementation for each host policy.
declare function fbHostFloatIsZero( byval f as double ) as integer
declare function fbHostFloatGeZero( byval f as double ) as integer
declare function fbHostFloatCompare( byval op as integer, byval lf as double, byval rf as double ) as longint
declare function fbHostFloatSgn( byval f as double ) as double
declare function fbHostFloatFix( byval f as double, byref hadfrac as integer ) as double
declare function fbHostFloatFloor( byval f as double ) as double
declare function fbHostFloatToULongint( byval f as double ) as ulongint
declare function fbHostFloatPow( byval lf as double, byval rf as double ) as double
declare sub fbHostFloatReset( )
declare function fbHostFloatBits( byval value as double ) as ulongint

'' Older bootstrap compilers need the signed conversion split above 2^63.
'' Keep this macro available through hlp.bi for existing callers.
#define hCastFloatToULongint(f) cunsg( iif( (f) >= 1.e+16, clngint( (f) * 0.5 ) shl 1, clngint( f ) ) )

#endif

'' end of support/numeric/fp-policy.bi
