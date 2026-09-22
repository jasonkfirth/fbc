

#include once "variant.bi"
#include once "intern.bi"

'':::::
operator VARIANT.shl= _
	( _
		byref rhs as VARIANT _
	)

	dim as VARIANT_ res = any, tmp = any

	VariantInit( @res )
	VariantInit( @tmp )
	V_VT(@tmp) = VT_I4
	V_I4(@tmp) = 1 shl cint( rhs )

	if VarMul( @this.var_, @tmp, @res ) = 0 then
		VariantClear( @this.var_ )
		this.var_ = res
	end if

	VariantClear( @tmp )

end operator

'':::::
operator VARIANT.shl= _
	( _
		byval rhs as integer _
	)

	dim as VARIANT_ res = any, tmp = any

	VariantInit( @res )
	VariantInit( @tmp )
	V_VT(@tmp) = VT_I4
	V_I4(@tmp) = 1 shl rhs

	if VarMul( @this.var_, @tmp, @res ) = 0 then
		VariantClear( @this.var_ )
		this.var_ = res
	end if

	VariantClear( @tmp )

end operator
