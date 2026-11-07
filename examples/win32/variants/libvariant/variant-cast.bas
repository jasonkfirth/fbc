'' Project: FreeBASIC variant helper library
'' File: variant-cast.bas
'' Purpose: Convert wrapper values while preserving result ownership.
'' Responsibilities: Return scalar, BASIC string and independently owned OLE values.
'' This file intentionally does NOT construct or assign wrapper instances.


#include once "variant.bi"
#include once "intern.bi"

VAR_GEN_CAST( integer, I4 )
VAR_GEN_CAST( uinteger, UI4 )
VAR_GEN_CAST( longint, I8 )
VAR_GEN_CAST( ulongint, UI8 )
VAR_GEN_CAST( single, R4 )
VAR_GEN_CAST( double, R8 )

'':::::
operator VARIANT.cast _
	( _
		_
	) as VARIANT_

	'' The native result is unmanaged BASIC storage. Its caller owns the
	'' VariantCopy result and must eventually release it with VariantClear.
	dim as VARIANT_ result

	VariantInit( @result )
	VariantCopy( @result, @this.var_ )
	operator = result

end operator

'':::::
operator VARIANT.cast _
	( _
		_
	) as string

	dim as VARIANT_ tmp = any

	VariantInit( @tmp )
	VariantChangeTypeEx( @tmp, @this.var_, NULL, VARIANT_NOVALUEPROP, VT_BSTR )

	operator = *cast( wstring ptr, V_BSTR(@tmp) )

	VariantClear( @tmp )

end operator

'':::::
operator VARIANT.cast _
	( _
		_
	) as wstring ptr

	dim as VARIANT_ tmp = any

	VariantInit( @tmp )
	VariantChangeTypeEx( @tmp, @this.var_, NULL, VARIANT_NOVALUEPROP, VT_BSTR )

	'' The returned BSTR has independent ownership; callers release it with SysFreeString.
	operator = SysAllocString(V_BSTR(@tmp))

	VariantClear( @tmp )

end operator

'' end of variant-cast.bas
