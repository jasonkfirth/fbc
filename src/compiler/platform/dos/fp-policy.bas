'' Project: FreeBASIC compiler - compiler host numeric policy
'' -----------------------------------------
''
'' File: platform/dos/fp-policy.bas
''
'' Purpose:
''
''     Preserve DJGPP floating point behavior for shared constant folding.
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
		dim as ulongint bits = *cptr( ulongint ptr, @f )
		function = ((bits and &h7FFFFFFFFFFFFFFFull) = 0)
end function

function fbHostFloatGeZero( byval f as double ) as integer
		dim as ulongint bits = *cptr( ulongint ptr, @f )
		dim as integer isnan = any

		isnan = ((bits and &h7FF0000000000000ull) = &h7FF0000000000000ull) andalso _
		        ((bits and &h000FFFFFFFFFFFFFull) <> 0)

		if( isnan ) then
			function = FALSE
		elseif( (bits and &h7FFFFFFFFFFFFFFFull) = 0 ) then
			function = TRUE
		else
			function = ((bits and &h8000000000000000ull) = 0)
		end if
end function

private function hFloatConstOrderKey( byval bits as ulongint ) as ulongint
	if( (bits and &h8000000000000000ull) <> 0 ) then
		function = not bits
	else
		function = bits xor &h8000000000000000ull
	end if
end function

function fbHostFloatCompare _
	( _
		byval op as integer, _
		byval lf as double, _
		byval rf as double _
	) as longint

		function = FALSE

		dim as ulongint lbits = *cptr( ulongint ptr, @lf )
		dim as ulongint rbits = *cptr( ulongint ptr, @rf )
		dim as integer lnan = any, rnan = any, eq = any

		lnan = ((lbits and &h7FF0000000000000ull) = &h7FF0000000000000ull) andalso _
		       ((lbits and &h000FFFFFFFFFFFFFull) <> 0)
		rnan = ((rbits and &h7FF0000000000000ull) = &h7FF0000000000000ull) andalso _
		       ((rbits and &h000FFFFFFFFFFFFFull) <> 0)

		if( lnan or rnan ) then
			function = iif( op = AST_OP_NE, -1, 0 )
			exit function
		end if

		if( (((lbits or rbits) and &h7FFFFFFFFFFFFFFFull) = 0) ) then
			eq = TRUE
		else
			eq = (lbits = rbits)
		end if

		select case as const op
		case AST_OP_NE
			function = iif( eq, 0, -1 )
		case AST_OP_EQ
			function = iif( eq, -1, 0 )
		case AST_OP_GT
			if( eq = FALSE ) then
				function = iif( hFloatConstOrderKey( lbits ) > hFloatConstOrderKey( rbits ), -1, 0 )
			end if
		case AST_OP_LT
			if( eq = FALSE ) then
				function = iif( hFloatConstOrderKey( lbits ) < hFloatConstOrderKey( rbits ), -1, 0 )
			end if
		case AST_OP_LE
			if( eq ) then
				function = -1
			else
				function = iif( hFloatConstOrderKey( lbits ) < hFloatConstOrderKey( rbits ), -1, 0 )
			end if
		case AST_OP_GE
			if( eq ) then
				function = -1
			else
				function = iif( hFloatConstOrderKey( lbits ) > hFloatConstOrderKey( rbits ), -1, 0 )
			end if
		end select
end function

function fbHostFloatSgn( byval f as double ) as double
		dim as ulongint bits = *cptr( ulongint ptr, @f )

		if( (bits and &h7FFFFFFFFFFFFFFFull) = 0 ) then
			function = 0.0
		elseif( (bits and &h8000000000000000ull) <> 0 ) then
			function = -1.0
		else
			function = 1.0
		end if
end function

function fbHostFloatFix _
	( _
		byval f as double, _
		byref hadfrac as integer _
	) as double

		dim as ulongint bits = *cptr( ulongint ptr, @f )
		dim as ulongint signbit = bits and &h8000000000000000ull
		'' IEEE-754 DOUBLE has an 11-bit exponent at bit 52, biased by 1023.
		const DOUBLE_EXPONENT_SHIFT = 52
		const DOUBLE_EXPONENT_MASK = &h7FF
		const DOUBLE_EXPONENT_BIAS = 1023
		const DOUBLE_MANTISSA_BITS = 52
		dim as integer expraw = (bits shr DOUBLE_EXPONENT_SHIFT) and DOUBLE_EXPONENT_MASK
		dim as integer expnt = expraw - DOUBLE_EXPONENT_BIAS

		hadfrac = FALSE

		if( expraw = &h7FF ) then
			return f
		end if

		if( expnt < 0 ) then
			hadfrac = ((bits and &h7FFFFFFFFFFFFFFFull) <> 0)
			bits = signbit
			return *cptr( double ptr, @bits )
		end if

		if( expnt >= DOUBLE_MANTISSA_BITS ) then
			return f
		end if

		dim as ulongint mant = (bits and &h000FFFFFFFFFFFFFull) or &h0010000000000000ull
		dim as ulongint fracmask = (1ull shl (DOUBLE_MANTISSA_BITS - expnt)) - 1

		hadfrac = ((mant and fracmask) <> 0)
		if( hadfrac ) then
			mant and= not fracmask
			bits = signbit or (culngint( expraw ) shl DOUBLE_EXPONENT_SHIFT) or (mant and &h000FFFFFFFFFFFFFull)
			function = *cptr( double ptr, @bits )
		else
			function = f
		end if
end function

function fbHostFloatFloor( byval f as double ) as double
		dim as integer hadfrac = any
		dim as double d = fbHostFloatFix( f, hadfrac )
		dim as ulongint bits = *cptr( ulongint ptr, @f )

		if( hadfrac andalso ((bits and &h8000000000000000ull) <> 0) ) then
			d -= 1.0
		end if

		function = d
end function

function fbHostFloatToULongint( byval f as double ) as ulongint
		dim as ulongint bits = *cptr( ulongint ptr, @f )
		'' IEEE-754 DOUBLE has an 11-bit exponent at bit 52, biased by 1023.
		const DOUBLE_EXPONENT_SHIFT = 52
		const DOUBLE_EXPONENT_MASK = &h7FF
		const DOUBLE_EXPONENT_BIAS = 1023
		const DOUBLE_MANTISSA_BITS = 52
		const ULONGINT_MAX_EXPONENT = 63
		dim as integer rawexp = (bits shr DOUBLE_EXPONENT_SHIFT) and DOUBLE_EXPONENT_MASK
		dim as integer expnt = rawexp - DOUBLE_EXPONENT_BIAS

		'' DJGPP's direct DOUBLE -> ULONGINT conversion does not match
		'' FreeBASIC's round-to-nearest rules on all DOS hosts.  For
		'' positive finite constants, round the IEEE mantissa directly.
		if( ((bits and &h8000000000000000ull) = 0) andalso _
		    (rawexp <> 0) andalso (rawexp <> &h7FF) ) then
			dim as ulongint mant = (bits and &h000FFFFFFFFFFFFFull) or &h0010000000000000ull

			if( expnt < -1 ) then
				function = 0
			elseif( expnt = -1 ) then
				if( mant > &h0010000000000000ull ) then
					function = 1
				else
					function = 0
				end if
			elseif( expnt < DOUBLE_MANTISSA_BITS ) then
				dim as integer shift = DOUBLE_MANTISSA_BITS - expnt
				dim as ulongint result = mant shr shift
				dim as ulongint remainder = mant and ((1ull shl shift) - 1ull)
				dim as ulongint half = 1ull shl (shift - 1)

				if( (remainder > half) or _
				    ((remainder = half) andalso ((result and 1ull) <> 0)) ) then
					result += 1
				end if

				function = result
			elseif( expnt <= ULONGINT_MAX_EXPONENT ) then
				function = mant shl (expnt - DOUBLE_MANTISSA_BITS)
			else
				function = 0
			end if
		else
			function = hCastFloatToULongint( f )
		end if
end function


private function hFloatConstPositiveInf( ) as double
		dim as ulongint bits = &h7FF0000000000000ull
		function = *cptr( double ptr, @bits )
end function

sub fbHostFloatReset( )
		'' DJGPP/DPMI can leave stale x87 stack entries between helper calls.
		'' This file is compiled only for the x86 DOS compiler target.
		#ifdef __FB_DOS__
		asm
			fninit
		end asm
		#endif
end sub

function fbHostFloatPow( byval lf as double, byval rf as double ) as double
	if( fbHostFloatIsZero( lf ) andalso _
	    (fbHostFloatCompare( AST_OP_LT, rf, 0.0 ) <> 0) ) then
		function = hFloatConstPositiveInf( )
	else
		function = lf ^ rf
	end if
end function

'' end of platform/dos/fp-policy.bas
