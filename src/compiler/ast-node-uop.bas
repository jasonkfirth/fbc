'' AST unary operation nodes
'' l = operand expression; r = NULL
''
'' chng: sep/2004 written [v1ctor]

#include once "fb.bi"
#include once "fbint.bi"
#include once "ir.bi"
#include once "rtl.bi"
#include once "ast.bi"

'' Need to use replacement function for sgn(longint) constant evaluation,
'' because the built-in sgn(longint) was bugged in older fbc versions.
'' This way we can bootstrap safely even using those older versions.
private function hSgnLongInt( byval x as longint ) as longint
	if( x = 0 ) then
		function = 0
	elseif( x > 0 ) then
		function = 1
	else
		function = -1
	end if
end function

private sub hResetDosFpuStack( )
	#if defined( __FB_DOS__ ) and defined( __FB_X86__ )
		'' DJGPP/DPMI can leave stale x87 stack entries between helper calls.
		asm
			fninit
		end asm
	#endif
end sub

private function hFloatConstSgn( byval f as double ) as double
	#if defined( __FB_DOS__ ) and defined( __FB_X86__ )
		dim as ulongint bits = *cptr( ulongint ptr, @f )

		if( (bits and &h7FFFFFFFFFFFFFFFull) = 0 ) then
			function = 0.0
		elseif( (bits and &h8000000000000000ull) <> 0 ) then
			function = -1.0
		else
			function = 1.0
		end if
	#else
		function = sgn( f )
	#endif
end function

private function hFloatConstFix _
	( _
		byval f as double, _
		byref hadfrac as integer _
	) as double

	#if defined( __FB_DOS__ ) and defined( __FB_X86__ )
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
	#else
		hadfrac = (frac( f ) <> 0.0)
		function = fix( f )
	#endif
end function

private function hFloatConstFloor( byval f as double ) as double
	#if defined( __FB_DOS__ ) and defined( __FB_X86__ )
		dim as integer hadfrac = any
		dim as double d = hFloatConstFix( f, hadfrac )
		dim as ulongint bits = *cptr( ulongint ptr, @f )

		if( hadfrac andalso ((bits and &h8000000000000000ull) <> 0) ) then
			d -= 1.0
		end if

		function = d
	#else
		function = int( f )
	#endif
end function

private sub hApplyFloatConstResultType( byval dtype as integer, byref f as double )
	if( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_SINGLE ) then
		'' Float constants are stored as DOUBLE, but a folded SINGLE
		'' operation must still match the runtime SINGLE helper.
		dim as single s = f
		f = s
	end if
end sub

private function hFloatConstUopSingleMath _
	( _
		byval op as integer, _
		byval f as single _
	) as double

	dim as single s = 0.0f

	select case as const( op )
	case AST_OP_SIN   : s =  sin( f )
	case AST_OP_ASIN  : s = asin( f )
	case AST_OP_COS   : s =  cos( f )
	case AST_OP_ACOS  : s = acos( f )
	case AST_OP_TAN   : s =  tan( f )
	case AST_OP_ATAN  : s =  atn( f )
	case AST_OP_SQRT  : s =  sqr( f )
	case AST_OP_LOG   : s =  log( f )
	case AST_OP_EXP   : s =  exp( f )
	case else         : assert( FALSE )
	end select

	function = s
end function

private function hConstUop _
	( _
		byval op as integer, _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval l as ASTNODE ptr _
	) as ASTNODE ptr

	dim as double d = any
	dim as longint i = any

	if( typeGetClass( l->dtype ) = FB_DATACLASS_FPOINT ) then
		hResetDosFpuStack( )

		d = l->val.f
		dim as integer hadfrac = any
		if( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_SINGLE ) then
			select case as const( op )
			case AST_OP_SIN, AST_OP_ASIN, AST_OP_COS, AST_OP_ACOS, _
			     AST_OP_TAN, AST_OP_ATAN, AST_OP_SQRT, AST_OP_LOG, _
			     AST_OP_EXP
				d = hFloatConstUopSingleMath( op, d )
			case else
				select case as const( op )
				case AST_OP_NEG   : d =      -d
				case AST_OP_ABS   : d =  abs( d )
				case AST_OP_SGN   : d =  hFloatConstSgn( d )
				case AST_OP_FLOOR : d =  hFloatConstFloor( d )
				case AST_OP_FIX
					d = hFloatConstFix( d, hadfrac )
				case AST_OP_FRAC  : d = frac( d )
				case else         : assert( FALSE )
				end select
			end select
		else
			select case as const( op )
			case AST_OP_NEG   : d =      -d
			case AST_OP_ABS   : d =  abs( d )
			case AST_OP_SGN   : d =  hFloatConstSgn( d )
			case AST_OP_SIN   : d =  sin( d )
			case AST_OP_ASIN  : d = asin( d )
			case AST_OP_COS   : d =  cos( d )
			case AST_OP_ACOS  : d = acos( d )
			case AST_OP_TAN   : d =  tan( d )
			case AST_OP_ATAN  : d =  atn( d )
			case AST_OP_SQRT  : d =  sqr( d )
			case AST_OP_LOG   : d =  log( d )
			case AST_OP_EXP   : d =  exp( d )
			case AST_OP_FLOOR : d =  hFloatConstFloor( d )
			case AST_OP_FIX
				d = hFloatConstFix( d, hadfrac )
			case AST_OP_FRAC  : d = frac( d )
			case else         : assert( FALSE )
			end select
		end if
		hApplyFloatConstResultType( dtype, d )
		l->val.f = d
	else
		i = l->val.i

		if( typeGetSize( l->dtype ) = 8 ) then
			select case as const( op )
			case AST_OP_NOT : i = not i
			case AST_OP_NEG : i = -i
			case AST_OP_ABS : i = abs( i )
			case AST_OP_SGN : i = hSgnLongInt( i )
			case else       : assert( FALSE )
			end select
		else
			select case as const( op )
			case AST_OP_NOT : i = not  clng( i )
			case AST_OP_NEG : i = -    clng( i )
			case AST_OP_ABS : i = abs( clng( i ) )
			case AST_OP_SGN : i = sgn( clng( i ) )
			case else       : assert( FALSE )
			end select
		end if

		l->val.i = i
		l = astConvertRawCONSTi( dtype, subtype, l )
	end if

	function = l
end function

private function hNewUopNode _
	( _
		byval op as integer, _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval o as ASTNODE ptr _
	) as ASTNODE ptr

	dim as ASTNODE ptr n = astNewNode( AST_NODECLASS_UOP, dtype, subtype )

	n->l = o
	n->r = NULL
	n->op.op = op
	n->op.ex = NULL
	n->op.options = AST_OPOPT_ALLOCRES

	return n
end function

function astNewUOP _
	( _
		byval op as integer, _
		byval o as ASTNODE ptr _
	) as ASTNODE ptr

	dim as integer dtype = any, rank = any, intrank = any, uintrank = any
	dim as FBSYMBOL ptr subtype = any
	dim as integer do_promote = any

	function = NULL

	if( o = NULL ) then
		exit function
	end if

	'' check op overloading
	if( symb.globOpOvlTb(op).head <> NULL ) then
		dim as FBSYMBOL ptr proc = any
		dim as FB_ERRMSG err_num = FB_ERRMSG_OK
		proc = symbFindUopOvlProc( op, o, @err_num )
		if( proc <> NULL ) then
			'' build a proc call
			return astBuildCall( proc, o )
		else
			if( err_num <> FB_ERRMSG_OK ) then
				exit function
			end if
		end if
	end if

	select case( op )
	case AST_OP_SWZ_REPEAT
		return hNewUopNode( op, o->dtype, o->subtype, o )

	case AST_OP_LEN
		'' The len() UOP is only allowed if overloaded
		exit function
	end select

	'' string? can't operate
	if( typeGetClass( o->dtype ) = FB_DATACLASS_STRING ) then
		exit function
	end if

	select case as const( typeGet( o->dtype ) )
	'' CHAR and WCHAR literals are also from the INTEGER class..
	case FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
		'' only if it's a deref pointer, to allow "NOT *p" etc
		if( astIsDEREF( o ) = FALSE ) then
			exit function
		end if

	'' UDT?
	case FB_DATATYPE_STRUCT ', FB_DATATYPE_CLASS
		'' try to convert to the most precise type
		'' (astNewCONV() will try symbFindCastOvlProc() which gives
		'' special treatment to the FB_DATATYPE_VOID)
		o = astNewCONV( typeJoin( o->dtype, FB_DATATYPE_VOID ), NULL, o )
		if( o = NULL ) then
			exit function
		end if

	'' Enum operand? Convert to integer (but preserve CONSTs)
	case FB_DATATYPE_ENUM
		'' See also astNewBOP() - when doing math on enum constants,
		'' we don't know whether the resulting integer value will be
		'' a part of that enum, so it's better to convert to integer.
		'' For typesafe enums, an error would have to be shown here.
		o = astNewCONV( typeJoin( o->dtype, FB_DATATYPE_INTEGER ), NULL, o )

	'' pointer?
	case FB_DATATYPE_POINTER
		'' no UOP's allowed with pointers
		exit function

	end select

	''
	'' Promote smaller integer types to [U]INTEGER before the operation,
	'' see also astNewBOP()
	''
	'' - do nothing if operand is boolean with NOT operator

	do_promote = (env.clopt.lang <> FB_LANG_QB) and (typeGetClass( o->dtype ) = FB_DATACLASS_INTEGER)

	if( typeGetDtAndPtrOnly( o->dtype ) = FB_DATATYPE_BOOLEAN ) then
		if( op = AST_OP_NOT ) then
			do_promote = FALSE
		else
			'' no other operation allowed with booleans
			exit function
		end if
	end if

	if( do_promote ) then

		rank = typeGetIntRank( typeGetRemapType( o->dtype ) )
		intrank = typeGetIntRank( FB_DATATYPE_INTEGER )
		uintrank = typeGetIntRank( FB_DATATYPE_UINT )

		'' o < INTEGER?
		if( rank < intrank ) then
			o = astNewCONV( typeJoin( o->dtype, FB_DATATYPE_INTEGER ), NULL, o )
		else
			'' INTEGER < o < UINTEGER?
			if( (intrank < rank) and (rank < uintrank) ) then
				'' Convert to UINTEGER for consistency with
				'' the above conversion to INTEGER (this can
				'' happen with ULONG on 32bit, and ULONGINT
				'' on 64bit, due to the ranking order)
				o = astNewCONV( typeJoin( o->dtype, FB_DATATYPE_UINT ), NULL, o )
			end if
		end if
	end if

	'' Result type normally is the same as the operand
	dtype = o->dtype
	subtype = o->subtype

	select case as const op
	'' NOT can only operate on integers
	case AST_OP_NOT
		if( typeGetClass( o->dtype ) <> FB_DATACLASS_INTEGER ) then
			o = astNewCONV( typeJoin( o->dtype, FB_DATATYPE_INTEGER ), NULL, o )
			dtype = o->dtype
			subtype = o->subtype
		end if

	'' with SGN(int) and [u]integer negation the result is always a signed integer
	case AST_OP_SGN
		if( typeGetClass( o->dtype ) = FB_DATACLASS_INTEGER ) then
			dtype = typeToSigned( dtype )
		end if

	case AST_OP_NEG
		if( not typeIsSigned( dtype ) ) then
			'' Check for constant overflows, for example:
			'' NEG( cushort( 32769 ) ) is -32769,
			'' but the lowest short is -32768.
			if( astIsCONST( o ) ) then
				if( astShouldShowWarnings( ) ) then
					'' Highest bit set? (meaning the negation cannot be represented,
					'' since the highest bit will be overwritten with the sign bit)
					if( astConstGetUint( o ) > (1ull shl (typeGetBits( dtype ) - 1)) ) then
						errReportWarn( FB_WARNINGMSG_IMPLICITCONVERSION )
					end if
				end if
			end if

			'' Negation on unsigned gives signed result
			dtype = typeToSigned( dtype )
		end if

	'' transcendental can only operate on floats
	case AST_OP_SIN, AST_OP_ASIN, AST_OP_COS, AST_OP_ACOS, _
		 AST_OP_TAN, AST_OP_ATAN, AST_OP_SQRT, AST_OP_LOG, _
		 AST_OP_EXP

		if( typeGetClass( o->dtype ) <> FB_DATACLASS_FPOINT ) then
			o = astNewCONV( typeJoin( o->dtype, FB_DATATYPE_DOUBLE ), NULL, o )
			dtype = o->dtype
			subtype = o->subtype
		end if

	'' fix and floor only affect floats
	case AST_OP_FIX, AST_OP_FLOOR
		'' integer?
		if( typeGetClass( o->dtype ) = FB_DATACLASS_INTEGER ) then
			'' return value unchanged (hack: add 0 to prevent passing byref)
			return astNewBOP( AST_OP_ADD, o, astNewCONSTi( 0, dtype ) )
		end if

	'' frac returns 0 for non-floats
	case AST_OP_FRAC
		'' integer?
		if( typeGetClass( o->dtype ) = FB_DATACLASS_INTEGER ) then
			'' return zero (opimization should eliminate 'AND 0' tree if no classes on o)
			return astNewBOP( AST_OP_AND, o, astNewCONSTi( 0, dtype ) )
		end if

	'' '+'? do nothing..
	case AST_OP_PLUS
		return o

	end select

	'' constant folding
	if( astIsCONST( o ) ) then
		o = hConstUop( op, dtype, subtype, o )
		o->dtype = dtype
		return o
	end if

	'' Optimize bitwise NOT on boolean expressions (where NOT = logical negation)
	if( op = AST_OP_NOT ) then
		if( astIsRelationalBop( o ) ) then
			'' let the backend deal with the inverse logic, unless we are in '-fpmode fast'
			if( (typeGetClass( astGetDataType( o->l ) ) = FB_DATACLASS_FPOINT) and _
			    (env.clopt.fpmode = FB_FPMODE_PRECISE) ) then
				o->op.options xor= AST_OPOPT_DOINVERSE
			else
				o->op.op = astGetInverseLogOp( o->op.op )
			end if
			return o
		end if
	end if

	if( irGetOption( IR_OPT_MISSINGOPS ) ) then
		'' Call RTL function if backend doesn't support this op directly
		if( irSupportsOp( op, dtype ) = FALSE ) then
			return rtlMathUop( op, o )
		end if
	end if

	return hNewUopNode( op, dtype, subtype, o )
end function

'':::::
function astLoadUOP _
	( _
		byval n as ASTNODE ptr _
	) as IRVREG ptr

	dim as ASTNODE ptr o = any
	dim as integer op = any
	dim as IRVREG ptr v1 = any, vr = NULL

	o = n->l
	op = n->op.op

	if( o = NULL ) then
		return NULL
	end if

	if( o->class = AST_NODECLASS_CONV ) then
		astUpdateCONVFD2FS( o, n->dtype, TRUE )
	end if

	v1 = astLoad( o )

	if( ast.doemit ) then
		if( (n->op.options and AST_OPOPT_ALLOCRES) <> 0 ) then
			vr = irAllocVREG( astGetFullType( o ), o->subtype )
			v1->vector = n->vector
			vr->vector = n->vector
		else
			vr = NULL
			v1->vector = n->vector
		end if

		irEmitUOP( op, v1, vr )

		'' "op var" optimizations
		if( vr = NULL ) then
			vr = v1
		end if
	end if

	astDelNode( o )

	function = vr

end function
