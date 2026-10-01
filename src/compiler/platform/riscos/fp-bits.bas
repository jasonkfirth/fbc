'' Project: FreeBASIC compiler - compiler host numeric policy
'' -----------------------------------------
''
'' File: platform/riscos/fp-bits.bas
''
'' Purpose:
''
''     Normalize the RISC OS APCS DOUBLE word order for numeric serialization.
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
	'' APCS stores the most significant DOUBLE word first.
	bits = (bits shl 32) or (bits shr 32)
	function = bits
end function

'' end of platform/riscos/fp-bits.bas
