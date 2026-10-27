#pragma once

'' FB does not have a built-in data type corresponding to C's long double,
'' so this provides the extra declarations for use by bindings.
#ifdef __FB_64BIT__
	'' GCC x86_64 & aarch64: sizeof(long double) = 16
	type clongdouble
		__(0 to 16-1) as ubyte
		declare operator cast( ) as byte
		declare operator cast( ) as ubyte
		declare operator cast( ) as short
		declare operator cast( ) as ushort
		declare operator cast( ) as integer
		declare operator cast( ) as uinteger
		declare operator cast( ) as longint
		declare operator cast( ) as ulongint
		declare operator cast( ) as single
		declare operator cast( ) as double
	end type
	#assert sizeof( clongdouble ) = 16
#elseif defined( __FB_ARM__ )
	'' GCC arm: sizeof(long double) = 8
	type clongdouble
		__(0 to 8-1) as ubyte
		declare operator cast( ) as byte
		declare operator cast( ) as ubyte
		declare operator cast( ) as short
		declare operator cast( ) as ushort
		declare operator cast( ) as integer
		declare operator cast( ) as uinteger
		declare operator cast( ) as longint
		declare operator cast( ) as ulongint
		declare operator cast( ) as single
		declare operator cast( ) as double
	end type
	#assert sizeof( clongdouble ) = 8
#else
	'' GCC x86: sizeof(long double) = 12
	type clongdouble
		__(0 to 12-1) as ubyte
		declare operator cast( ) as byte
		declare operator cast( ) as ubyte
		declare operator cast( ) as short
		declare operator cast( ) as ushort
		declare operator cast( ) as integer
		declare operator cast( ) as uinteger
		declare operator cast( ) as longint
		declare operator cast( ) as ulongint
		declare operator cast( ) as single
		declare operator cast( ) as double
	end type
	#assert sizeof( clongdouble ) = 12
#endif

'' Arithmetic uses native C long double in the runtime, because the BASIC
'' compiler has no scalar floating type with this representation.
private enum FB_CLONGDOUBLE_BOP
	FB_CLONGDOUBLE_BOP_ADD = 1
	FB_CLONGDOUBLE_BOP_SUB
	FB_CLONGDOUBLE_BOP_MUL
	FB_CLONGDOUBLE_BOP_DIV
end enum

private enum FB_CLONGDOUBLE_UOP
	FB_CLONGDOUBLE_UOP_NEG = 1
end enum

private enum FB_CLONGDOUBLE_CMP
	FB_CLONGDOUBLE_CMP_EQ = 1
	FB_CLONGDOUBLE_CMP_NE
	FB_CLONGDOUBLE_CMP_GT
	FB_CLONGDOUBLE_CMP_LT
	FB_CLONGDOUBLE_CMP_LE
	FB_CLONGDOUBLE_CMP_GE
end enum

declare sub fb_LongDoubleBop cdecl alias "fb_LongDoubleBop" _
	(byval destination as any ptr, byval lhs as const any ptr, _
	 byval rhs as const any ptr, byval operation as integer, _
	 byval storage_size as uinteger)
declare sub fb_LongDoubleUop cdecl alias "fb_LongDoubleUop" _
	(byval destination as any ptr, byval source as const any ptr, _
	 byval operation as integer, byval storage_size as uinteger)
declare function fb_LongDoubleCompare cdecl alias "fb_LongDoubleCompare" _
	(byval lhs as const any ptr, byval rhs as const any ptr, _
	 byval operation as integer, byval storage_size as uinteger) as integer
declare sub fb_LongDoubleFromSigned cdecl alias "fb_LongDoubleFromSigned" _
	(byval destination as any ptr, byval value as longint, _
	 byval storage_size as uinteger)
declare sub fb_LongDoubleFromUnsigned cdecl alias "fb_LongDoubleFromUnsigned" _
	(byval destination as any ptr, byval value as ulongint, _
	 byval storage_size as uinteger)
declare sub fb_LongDoubleFromDouble cdecl alias "fb_LongDoubleFromDouble" _
	(byval destination as any ptr, byval value as double, _
	 byval storage_size as uinteger)
declare function fb_LongDoubleToSigned cdecl alias "fb_LongDoubleToSigned" _
	(byval source as const any ptr, byval storage_size as uinteger) as longint
declare function fb_LongDoubleToUnsigned cdecl alias "fb_LongDoubleToUnsigned" _
	(byval source as const any ptr, byval storage_size as uinteger) as ulongint
declare function fb_LongDoubleToDouble cdecl alias "fb_LongDoubleToDouble" _
	(byval source as const any ptr, byval storage_size as uinteger) as double

private operator clongdouble.cast( ) as byte
	return cast( byte, fb_LongDoubleToSigned( @this, sizeof( clongdouble ) ) )
end operator

private operator clongdouble.cast( ) as ubyte
	return cast( ubyte, fb_LongDoubleToUnsigned( @this, sizeof( clongdouble ) ) )
end operator

private operator clongdouble.cast( ) as short
	return cast( short, fb_LongDoubleToSigned( @this, sizeof( clongdouble ) ) )
end operator

private operator clongdouble.cast( ) as ushort
	return cast( ushort, fb_LongDoubleToUnsigned( @this, sizeof( clongdouble ) ) )
end operator

private operator clongdouble.cast( ) as integer
	return cast( integer, fb_LongDoubleToSigned( @this, sizeof( clongdouble ) ) )
end operator

private operator clongdouble.cast( ) as uinteger
	return cast( uinteger, fb_LongDoubleToUnsigned( @this, sizeof( clongdouble ) ) )
end operator

private operator clongdouble.cast( ) as longint
	return fb_LongDoubleToSigned( @this, sizeof( clongdouble ) )
end operator

private operator clongdouble.cast( ) as ulongint
	return fb_LongDoubleToUnsigned( @this, sizeof( clongdouble ) )
end operator

private operator clongdouble.cast( ) as single
	return cast( single, fb_LongDoubleToDouble( @this, sizeof( clongdouble ) ) )
end operator

private operator clongdouble.cast( ) as double
	return fb_LongDoubleToDouble( @this, sizeof( clongdouble ) )
end operator

private function CLongDoubleFromSignedValue( byval rhs as longint ) as clongdouble
	dim result as clongdouble
	fb_LongDoubleFromSigned( @result, rhs, sizeof( clongdouble ) )
	return result
end function

private function CLongDoubleFromUnsignedValue( byval rhs as ulongint ) as clongdouble
	dim result as clongdouble
	fb_LongDoubleFromUnsigned( @result, rhs, sizeof( clongdouble ) )
	return result
end function

private function CLongDoubleFromDoubleValue( byval rhs as double ) as clongdouble
	dim result as clongdouble
	fb_LongDoubleFromDouble( @result, rhs, sizeof( clongdouble ) )
	return result
end function

private function CLongDoubleBopWithDouble _
	(byref lhs as clongdouble, byval rhs as double, byval operation as integer) as clongdouble
	dim converted as clongdouble
	dim result as clongdouble
	fb_LongDoubleFromDouble( @converted, rhs, sizeof( clongdouble ) )
	fb_LongDoubleBop( @result, @lhs, @converted, operation, sizeof( clongdouble ) )
	return result
end function

private function CLongDoubleCompareWithDouble _
	(byref lhs as clongdouble, byval rhs as double, byval operation as integer) as integer
	dim converted as clongdouble
	fb_LongDoubleFromDouble( @converted, rhs, sizeof( clongdouble ) )
	return fb_LongDoubleCompare( @lhs, @converted, operation, sizeof( clongdouble ) )
end function

private function CLongDoubleCompareWithInteger _
	(byref lhs as clongdouble, byval rhs as integer, byval operation as integer) as integer
	dim converted as clongdouble
	fb_LongDoubleFromSigned( @converted, cast( longint, rhs ), sizeof( clongdouble ) )
	return fb_LongDoubleCompare( @lhs, @converted, operation, sizeof( clongdouble ) )
end function

private function CLongLongCompareWithLongDouble _
	(byval lhs as longint, byref rhs as clongdouble, byval operation as integer) as integer
	dim converted as clongdouble
	fb_LongDoubleFromSigned( @converted, lhs, sizeof( clongdouble ) )
	return fb_LongDoubleCompare( @converted, @rhs, operation, sizeof( clongdouble ) )
end function

private operator +( byref lhs as clongdouble, byref rhs as clongdouble ) as clongdouble
	dim result as clongdouble
	fb_LongDoubleBop( @result, @lhs, @rhs, FB_CLONGDOUBLE_BOP_ADD, sizeof( clongdouble ) )
	return result
end operator

private operator +( byref lhs as clongdouble, byval rhs as double ) as clongdouble
	return CLongDoubleBopWithDouble( lhs, rhs, FB_CLONGDOUBLE_BOP_ADD )
end operator

private operator -( byref lhs as clongdouble, byref rhs as clongdouble ) as clongdouble
	dim result as clongdouble
	fb_LongDoubleBop( @result, @lhs, @rhs, FB_CLONGDOUBLE_BOP_SUB, sizeof( clongdouble ) )
	return result
end operator

private operator -( byref lhs as clongdouble, byval rhs as double ) as clongdouble
	return CLongDoubleBopWithDouble( lhs, rhs, FB_CLONGDOUBLE_BOP_SUB )
end operator

private operator *( byref lhs as clongdouble, byref rhs as clongdouble ) as clongdouble
	dim result as clongdouble
	fb_LongDoubleBop( @result, @lhs, @rhs, FB_CLONGDOUBLE_BOP_MUL, sizeof( clongdouble ) )
	return result
end operator

private operator *( byref lhs as clongdouble, byval rhs as double ) as clongdouble
	return CLongDoubleBopWithDouble( lhs, rhs, FB_CLONGDOUBLE_BOP_MUL )
end operator

private operator /( byref lhs as clongdouble, byref rhs as clongdouble ) as clongdouble
	dim result as clongdouble
	fb_LongDoubleBop( @result, @lhs, @rhs, FB_CLONGDOUBLE_BOP_DIV, sizeof( clongdouble ) )
	return result
end operator

private operator /( byref lhs as clongdouble, byval rhs as double ) as clongdouble
	return CLongDoubleBopWithDouble( lhs, rhs, FB_CLONGDOUBLE_BOP_DIV )
end operator

private operator -( byref rhs as clongdouble ) as clongdouble
	dim result as clongdouble
	fb_LongDoubleUop( @result, @rhs, FB_CLONGDOUBLE_UOP_NEG, sizeof( clongdouble ) )
	return result
end operator

private operator =( byref lhs as clongdouble, byref rhs as clongdouble ) as integer
	return fb_LongDoubleCompare( @lhs, @rhs, FB_CLONGDOUBLE_CMP_EQ, sizeof( clongdouble ) )
end operator

private operator =( byref lhs as clongdouble, byval rhs as double ) as integer
	return CLongDoubleCompareWithDouble( lhs, rhs, FB_CLONGDOUBLE_CMP_EQ )
end operator

private operator <>( byref lhs as clongdouble, byref rhs as clongdouble ) as integer
	return fb_LongDoubleCompare( @lhs, @rhs, FB_CLONGDOUBLE_CMP_NE, sizeof( clongdouble ) )
end operator

private operator <>( byref lhs as clongdouble, byval rhs as double ) as integer
	return CLongDoubleCompareWithDouble( lhs, rhs, FB_CLONGDOUBLE_CMP_NE )
end operator

private operator >( byref lhs as clongdouble, byref rhs as clongdouble ) as integer
	return fb_LongDoubleCompare( @lhs, @rhs, FB_CLONGDOUBLE_CMP_GT, sizeof( clongdouble ) )
end operator

private operator >( byref lhs as clongdouble, byval rhs as double ) as integer
	return CLongDoubleCompareWithDouble( lhs, rhs, FB_CLONGDOUBLE_CMP_GT )
end operator

private operator <( byref lhs as clongdouble, byref rhs as clongdouble ) as integer
	return fb_LongDoubleCompare( @lhs, @rhs, FB_CLONGDOUBLE_CMP_LT, sizeof( clongdouble ) )
end operator

private operator <( byref lhs as clongdouble, byval rhs as double ) as integer
	return CLongDoubleCompareWithDouble( lhs, rhs, FB_CLONGDOUBLE_CMP_LT )
end operator

private operator <( byref lhs as clongdouble, byval rhs as integer ) as integer
	return CLongDoubleCompareWithInteger( lhs, rhs, FB_CLONGDOUBLE_CMP_LT )
end operator

private operator <( byval lhs as longint, byref rhs as clongdouble ) as integer
	return CLongLongCompareWithLongDouble( lhs, rhs, FB_CLONGDOUBLE_CMP_LT )
end operator

private operator <=( byref lhs as clongdouble, byref rhs as clongdouble ) as integer
	return fb_LongDoubleCompare( @lhs, @rhs, FB_CLONGDOUBLE_CMP_LE, sizeof( clongdouble ) )
end operator

private operator <=( byref lhs as clongdouble, byval rhs as double ) as integer
	return CLongDoubleCompareWithDouble( lhs, rhs, FB_CLONGDOUBLE_CMP_LE )
end operator

private operator >=( byref lhs as clongdouble, byref rhs as clongdouble ) as integer
	return fb_LongDoubleCompare( @lhs, @rhs, FB_CLONGDOUBLE_CMP_GE, sizeof( clongdouble ) )
end operator

private operator >=( byref lhs as clongdouble, byval rhs as double ) as integer
	return CLongDoubleCompareWithDouble( lhs, rhs, FB_CLONGDOUBLE_CMP_GE )
end operator

'' end of crt/longdouble.bi
