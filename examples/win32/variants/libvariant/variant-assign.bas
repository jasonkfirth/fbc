'' Project: FreeBASIC variant helper library
'' File: variant-assign.bas
'' Purpose: Implement the variant assign overload family.
'' Responsibilities: Preserve OLE value and text ownership.
'' This file intentionally does NOT declare the public wrapper ABI.


#include once "variant.bi"
#include once "intern.bi"

VAR_GEN_ASSIGN( integer, I4 )
VAR_GEN_ASSIGN( uinteger, UI4 )
VAR_GEN_ASSIGN( longint, I8 )
VAR_GEN_ASSIGN( ulongint, UI8 )
VAR_GEN_ASSIGN( single, R4 )
VAR_GEN_ASSIGN( double, R8 )

'':::::
operator VARIANT.let _
	( _
		byval rhs as VARIANT_NOTHING _
	)

	VariantClear( @this.var_ )

end operator

'':::::
operator VARIANT.let _
	( _
		byref rhs as VARIANT _
	)

	if( @this.var_ = @rhs.var_ ) then exit operator

	'' VariantCopy releases the initialized destination itself.
	VariantCopy( @this.var_, @rhs.var_ )

end operator

'':::::
operator VARIANT.let _
	( _
		byref rhs as VARIANT_ _
	)

	if( @this.var_ = @rhs ) then exit operator

	'' VariantCopy releases the initialized destination itself.
	VariantCopy( @this.var_, @rhs )

end operator

'':::::
operator VARIANT.let _
	( _
		byval rhs as zstring ptr _
	)

	VariantClear( @this.var_ )

	V_VT(@this.var_) = VT_BSTR
	V_BSTR(@this.var_) = hVarAllocBstrFromZstring( rhs )

end operator

'':::::
operator VARIANT.let _
	( _
		byval rhs as wstring ptr _
	)

	VariantClear( @this.var_ )

	V_VT(@this.var_) = VT_BSTR
	V_BSTR(@this.var_) = SysAllocStringLen( rhs, len( *rhs ) )

end operator

'' end of variant-assign.bas
