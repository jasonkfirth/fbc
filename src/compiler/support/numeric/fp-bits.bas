'' Project: FreeBASIC compiler - compiler host numeric policy
'' -----------------------------------------
''
'' File: support/numeric/fp-bits.bas
''
'' Purpose:
''
''     Read DOUBLE storage as a normalized IEEE-754 bit pattern.
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

#include once "support/numeric/fp-policy.bi"

function fbHostFloatBits( byval value as double ) as ulongint
	dim as ulongint bits = *cptr( ulongint ptr, @value )
	function = bits
end function

'' end of support/numeric/fp-bits.bas
