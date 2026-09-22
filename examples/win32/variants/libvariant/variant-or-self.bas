

#include once "variant.bi"
#include once "intern.bi"

VAR_GEN_SELFOP( or=, VarOr, integer, I4 )
VAR_GEN_SELFOP( or=, VarOr, uinteger, UI4 )
VAR_GEN_SELFOP( or=, VarOr, longint, I8 )
VAR_GEN_SELFOP( or=, VarOr, ulongint, UI8 )

'':::::
operator VARIANT.or= _
	( _
		byref rhs as VARIANT _
	)

	dim as VARIANT_ res = any

	VariantInit( @res )
	if VarOr( @this.var_, @rhs.var_, @res ) = 0 then
		VariantClear( @this.var_ )
		this.var_ = res
	end if

end operator

'':::::
operator VARIANT.or= _
	( _
		byref rhs as VARIANT_ _
	)

	dim as VARIANT_ res = any

	VariantInit( @res )
	if VarOr( @this.var_, @rhs, @res ) = 0 then
		VariantClear( @this.var_ )
		this.var_ = res
	end if

end operator
