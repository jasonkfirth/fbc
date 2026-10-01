'' Project: FreeBASIC compiler - compiler host numeric policy
'' -----------------------------------------
''
'' File: support/numeric/fp-policy.bas
''
'' Purpose:
''
''     Provide host arithmetic for shared constant folding operations.
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
#include once "ast/ast-op.bi"

function fbHostFloatIsZero( byval f as double ) as integer
	function = (f = 0.0)
end function

function fbHostFloatGeZero( byval f as double ) as integer
	function = (f >= 0.0)
end function

function fbHostFloatCompare _
	( _
		byval op as integer, _
		byval lf as double, _
		byval rf as double _
	) as longint

	function = FALSE

	select case as const op
	case AST_OP_NE  : function = (lf <> rf)
	case AST_OP_EQ  : function = (lf =  rf)
	case AST_OP_GT  : function = (lf >  rf)
	case AST_OP_LT  : function = (lf <  rf)
	case AST_OP_LE  : function = (lf <= rf)
	case AST_OP_GE  : function = (lf >= rf)
	end select
end function

function fbHostFloatSgn( byval f as double ) as double
	function = sgn( f )
end function

function fbHostFloatFix _
	( _
		byval f as double, _
		byref hadfrac as integer _
	) as double

	hadfrac = (frac( f ) <> 0.0)
	function = fix( f )
end function

function fbHostFloatFloor( byval f as double ) as double
	function = int( f )
end function

function fbHostFloatToULongint( byval f as double ) as ulongint
	function = hCastFloatToULongint( f )
end function

sub fbHostFloatReset( )
end sub

function fbHostFloatPow( byval lf as double, byval rf as double ) as double
	function = lf ^ rf
end function

'' end of support/numeric/fp-policy.bas
