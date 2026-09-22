

#include once "variant.bi"
#include once "intern.bi"

VAR_GEN_SELFOP( imp=, VarImp, integer, I4 )
VAR_GEN_SELFOP( imp=, VarImp, uinteger, UI4 )
VAR_GEN_SELFOP( imp=, VarImp, longint, I8 )
VAR_GEN_SELFOP( imp=, VarImp, ulongint, UI8 )

'':::::
operator VARIANT.imp= _
	( _
		byref rhs as VARIANT _
	)

	dim as VARIANT_ res = any

	VariantInit( @res )
	if VarImp( @this.var_, @rhs.var_, @res ) = 0 then
		VariantClear( @this.var_ )
		this.var_ = res
	end if

end operator

'':::::
operator VARIANT.imp= _
	( _
		byref rhs as VARIANT_ _
	)

	dim as VARIANT_ res = any

	VariantInit( @res )
	if VarImp( @this.var_, @rhs, @res ) = 0 then
		VariantClear( @this.var_ )
		this.var_ = res
	end if

end operator
