

#include once "variant.bi"
#include once "intern.bi"

VAR_GEN_SELFOP( xor=, VarXor, integer, I4 )
VAR_GEN_SELFOP( xor=, VarXor, uinteger, UI4 )
VAR_GEN_SELFOP( xor=, VarXor, longint, I8 )
VAR_GEN_SELFOP( xor=, VarXor, ulongint, UI8 )

'':::::
operator VARIANT.xor= _
	( _
		byref rhs as VARIANT _
	)

	dim as VARIANT_ res = any

	VariantInit( @res )
	if VarXor( @this.var_, @rhs.var_, @res ) = 0 then
		VariantClear( @this.var_ )
		this.var_ = res
	end if

end operator

'':::::
operator VARIANT.xor= _
	( _
		byref rhs as VARIANT_ _
	)

	dim as VARIANT_ res = any

	VariantInit( @res )
	if VarXor( @this.var_, @rhs, @res ) = 0 then
		VariantClear( @this.var_ )
		this.var_ = res
	end if

end operator
