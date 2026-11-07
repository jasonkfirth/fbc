'' Project: FreeBASIC variant helper library
'' File: variant-add-inv.bas
'' Purpose: Implement the variant add-inv overload family.
'' Responsibilities: Preserve OLE value and text ownership.
'' This file intentionally does NOT declare the public wrapper ABI.


#define VARIANT_NOASSIGNMENT
#include once "variant.bi"
#include once "intern.bi"

VAR_GEN_BOP_INV( +, VarAdd, integer, I4 )
VAR_GEN_BOP_INV( +, VarAdd, uinteger, UI4 )
VAR_GEN_BOP_INV( +, VarAdd, longint, I8 )
VAR_GEN_BOP_INV( +, VarAdd, ulongint, UI8 )
VAR_GEN_BOP_INV( +, VarAdd, single, R4 )
VAR_GEN_BOP_INV( +, VarAdd, double, R8 )

'':::::
operator + _
	( _
		byref lhs as VARIANT_, _
		byref rhs as VARIANT _
	) as VARIANT

	dim as VARIANT_ res = any

	VariantInit( @res )
	VarAdd( @lhs, @rhs.var_, @res )

	return VARIANT( res, FALSE )

end operator

'':::::
operator + _
	( _
		byval lhs as zstring ptr, _
		byref rhs as VARIANT _
	) as VARIANT

	dim as VARIANT_ tmp = any, res = any

	VariantInit( @tmp )
	VariantInit( @res )
	V_VT(@tmp) = VT_BSTR
	V_BSTR(@tmp) = hVarAllocBstrFromZstring( lhs )

	VarAdd( @tmp, @rhs.var_, @res )

	VariantClear( @tmp )

	return VARIANT( res, FALSE )

end operator

'':::::
operator + _
	( _
		byval lhs as wstring ptr, _
		byref rhs as VARIANT _
	) as VARIANT

	dim as VARIANT_ tmp = any, res = any

	VariantInit( @tmp )
	VariantInit( @res )
	V_VT(@tmp) = VT_BSTR
	V_BSTR(@tmp) = SysAllocStringLen( lhs, len( *lhs ) )

	VarAdd( @tmp, @rhs.var_, @res )

	VariantClear( @tmp )

	return VARIANT( res, FALSE )

end operator

'' end of variant-add-inv.bas
