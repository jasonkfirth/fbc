'' Project: FreeBASIC compiler - compiler runtime call lowering
'' -----------------------------------------
''
'' File: runtime/rtl-system-thread.bas
''
'' Purpose:
''
''     Declare and lower threadcall AST transform.
''
'' Responsibilities:
''
''     - declare and select the runtime intrinsic family described below
''     - build typed AST calls and describe required argument conventions
''
'' This file intentionally does NOT contain:
''
''     - the runtime functions implemented under src/rtlib or graphics libraries
''

'' threadcall AST transform
''
'' chng: oct/2011 written [jofers]

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "ast/ast.bi"
#include once "runtime/rtl.bi"

declare function hThreadCallPushType _
	( _
		byval funcexpr as ASTNODE ptr, _
		byval tctype as integer, _
		byval stype as FBSYMBOL ptr _
	) as integer

private function hThreadCallMapType _
	( _
		byval sym as FBSYMBOL ptr, _
		byval udt as integer = FALSE _
	) as integer

	function = -1

	dim as FB_DATATYPE dtype = symbGetType( sym )
	dim as FBSYMBOL ptr subtype = symbGetSubType( sym )

	'' arrays not supported inside udts
	if( symbIsArray( sym ) ) then
		return iif( udt, -1, FB_THREADCALL_PTR )
	end if

	'' Bitfields not supported
	if( symbIsBitfield( sym ) ) then
		exit function
	end if

	if( typeIsPtr( dtype ) ) then
		return FB_THREADCALL_PTR
	end if

	select case( dtype )
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
		function = iif( udt, -1, FB_THREADCALL_PTR )
	case FB_DATATYPE_STRUCT
		'' restrictions to simplify life
		if( symbGetUDTIsUnion( subtype ) or symbGetUDTHasAnonUnion( subtype ) ) then
			exit function
		end if
		if symbGetUDTAlign( subtype ) <> 0 then
			exit function
		end if

		'' FB transforms type with 1 element to that element's type
		dim as FBSYMBOL ptr first = symbUdtGetFirstField( subtype )
		if( first = NULL ) then exit function
		'' no second field?
		if( symbUdtGetNextField( first ) = NULL ) then
			function = hThreadCallMapType( first, TRUE )
		else
			function = FB_THREADCALL_STRUCT
		end if
	case FB_DATATYPE_BYTE, FB_DATATYPE_CHAR, FB_DATATYPE_UBYTE, _
	     FB_DATATYPE_SHORT, FB_DATATYPE_WCHAR, FB_DATATYPE_USHORT, _
	     FB_DATATYPE_INTEGER, FB_DATATYPE_ENUM, FB_DATATYPE_UINT, _
	     FB_DATATYPE_LONG, FB_DATATYPE_ULONG, _
	     FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT, _
	     FB_DATATYPE_SINGLE, FB_DATATYPE_DOUBLE
		select case as const( typeGetSizeType( dtype ) )
		case FB_SIZETYPE_INT8    : function = FB_THREADCALL_INT8
		case FB_SIZETYPE_UINT8   : function = FB_THREADCALL_UINT8
		case FB_SIZETYPE_INT16   : function = FB_THREADCALL_INT16
		case FB_SIZETYPE_UINT16  : function = FB_THREADCALL_UINT16
		case FB_SIZETYPE_INT32   : function = FB_THREADCALL_INT32
		case FB_SIZETYPE_UINT32  : function = FB_THREADCALL_UINT32
		case FB_SIZETYPE_INT64   : function = FB_THREADCALL_INT64
		case FB_SIZETYPE_UINT64  : function = FB_THREADCALL_UINT64
		case FB_SIZETYPE_FLOAT32 : function = FB_THREADCALL_FLOAT32
		case FB_SIZETYPE_FLOAT64 : function = FB_THREADCALL_FLOAT64
		case else                : errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		end select
	case else
		exit function
	end select

end function

private function hThreadCallPushStruct _
	( _
		byval funcexpr as ASTNODE ptr, _
		byval struct as FBSYMBOL ptr _
	) as integer

	dim as FBSYMBOL ptr fld = any
	dim as integer count = any

	'' count number of elements
	count = 0
	fld = symbUdtGetFirstField( struct )
	do
		count += 1
		fld = symbUdtGetNextField( fld )
	loop while( fld )

	'' push number of elements
	if( astNewArg( funcexpr, astNewCONSTi( count ) ) = NULL ) then
		exit function
	end if

	'' push each element
	fld = symbUdtGetFirstField( struct )
	do
		if( hThreadCallPushType( funcexpr, _
		                         hThreadCallMapType( fld, TRUE ), _
		                         symbGetSubType( fld ) ) = FALSE ) then
			exit  function
		end if
		fld = symbUdtGetNextField( fld )
	loop while( fld )

	function = TRUE
end function

'':::::
private function hThreadCallPushType _
	( _
		byval funcexpr as ASTNODE ptr, _
		byval tctype as integer, _
		byval stype as FBSYMBOL ptr _
	) as integer

	function = false

	'' Unsupported datatype
	if( tctype = -1 ) then
		errReport( FB_ERRMSG_UNSUPPORTEDFUNCTION )
		exit function
	end if

	'' push argument on stack
	dim as ASTNODE ptr typeexpr
	typeexpr = astNewCONSTi( tctype )
	if( astNewARG( funcexpr, typeexpr ) = NULL ) then
		exit function
	end if

	'' push type info to the stack
	if( tctype = FB_THREADCALL_STRUCT ) then
		if( hThreadCallPushStruct( funcexpr, stype ) = FALSE ) then
			exit function
		end if
	end if

	function = true
end function

private function hGetExprRef( byref expr as ASTNODE ptr ) as ASTNODE ptr
	dim as FBSYMBOL ptr tmpvar = any

	if( astIsVAR( expr ) ) then
		'' already a variable? just get the address
		'' @expr
		expr = astNewADDROF( expr )
	elseif( typeGetClass( astGetDataType( expr ) ) <> FB_DATACLASS_UDT andalso _
	        typeGetClass( astGetDataType( expr ) ) <> FB_DATACLASS_STRING ) then
		'' astRemSideFx() owns the canonical scalar temporary handling.
		'' hGetExprRef() additionally turns the resulting value into the
		'' pointer expected by the thread-call argument packer.
		function = astRemSideFx( expr )
		expr = astNewADDROF( expr )
	else
		'' Complex values must be copied into storage that remains valid while
		'' the thread-call argument packer reads it.
		tmpvar = symbAddTempVar( astGetDataType( expr ), astGetSubType( expr ) )

		'' tmpvar = expr
		function = astNewASSIGN( astNewVAR( tmpvar ), expr, AST_OPOPT_DONTCHKPTR )

		'' @tmpvar
		expr = astNewADDROF( astNewVAR( tmpvar ) )
	end if
end function

'' -------------------------------------------------------------------------
'' Lifetime of the ABI argument after ordinary call lowering
'' -------------------------------------------------------------------------

declare function hThreadCallUsesTempStorage _
	( byval expr as ASTNODE ptr, byval storage as integer = FALSE ) as integer

private function hThreadCallIifUsesTemp _
	( byval branch as ASTNODE ptr, byval result as FBSYMBOL ptr ) as integer

	if( branch = NULL ) then return FALSE
	if( branch->class = AST_NODECLASS_LINK ) then
		return hThreadCallIifUsesTemp( branch->l, result ) orelse _
		       hThreadCallIifUsesTemp( branch->r, result )
	end if
	if( (branch->class = AST_NODECLASS_ASSIGN) andalso branch->l andalso branch->l->sym = result ) then
		return hThreadCallUsesTempStorage( branch->r )
	end if
	return FALSE
end function

private function hThreadCallUsesTempStorage _
	( byval expr as ASTNODE ptr, byval storage as integer ) as integer

	if( expr = NULL ) then return FALSE
	select case expr->class
	case AST_NODECLASS_VAR, AST_NODECLASS_IDX, AST_NODECLASS_OFFSET
		if( expr->sym = NULL ) then return FALSE
		if( symbIsVar( expr->sym ) andalso symbIsDescriptor( expr->sym ) ) then
			var array = expr->sym->var_.desc.array
			'' A fixed array field gets a fresh descriptor at each call.
			'' A named array has a descriptor with its own lifetime.
			if( array andalso (symbIsField( array ) orelse symbIsTemp( array )) ) then return TRUE
		end if
		return symbIsTemp( expr->sym ) andalso _
		    (storage orelse typeIsPtr( astGetFullType( expr ) ))
	case AST_NODECLASS_ADDROF, AST_NODECLASS_ASSIGN
		return hThreadCallUsesTempStorage( expr->l, TRUE )
	case AST_NODECLASS_CONV, AST_NODECLASS_FIELD, AST_NODECLASS_PTRCHK
		return hThreadCallUsesTempStorage( expr->l, storage )
	case AST_NODECLASS_DEREF
		return hThreadCallUsesTempStorage( expr->l )
	case AST_NODECLASS_LINK
		'' Initializers and index calculations can use their own temps.
		'' Only the storage selected by the result can escape to the worker.
		return hThreadCallUsesTempStorage( astGetEffectiveNode( expr ), storage )
	case AST_NODECLASS_BOP
		if( expr->op.op = AST_OP_ADD orelse expr->op.op = AST_OP_SUB ) then
			return hThreadCallUsesTempStorage( expr->l ) orelse hThreadCallUsesTempStorage( expr->r )
		end if
	case AST_NODECLASS_TYPEINI, AST_NODECLASS_CALLCTOR
		return TRUE
	case AST_NODECLASS_IIF
		if( storage ) then return TRUE
		'' The two sides assign to the same generated result slot. For
		'' a pointer-valued IIF, inspect the values assigned to that slot.
		if( expr->l andalso expr->l->sym andalso expr->r ) then
			return hThreadCallIifUsesTemp( expr->r->r, expr->l->sym )
		end if
	case AST_NODECLASS_CALL
		if( expr->sym = NULL ) then return FALSE
		var dtype = symbGetType( expr->sym )
		if( symbIsReturnByRef( expr->sym ) = FALSE ) then
			if( typeIsDynamicString( dtype ) orelse dtype = FB_DATATYPE_WCHAR orelse _
			    dtype = FB_DATATYPE_STRUCT ) then return TRUE
			if( typeIsPtr( dtype ) = FALSE ) then return FALSE
		end if

		'' A reference returned by another call can alias one of its
		'' own reference or pointer arguments.
		var arg = expr->r
		while( arg )
			if( arg->sym andalso (symbGetParamMode( arg->sym ) = FB_PARAMMODE_BYREF orelse _
			    symbGetParamMode( arg->sym ) = FB_PARAMMODE_BYDESC orelse _
			    typeIsPtr( symbGetType( arg->sym ) )) ) then
				if( hThreadCallUsesTempStorage( arg->l ) ) then return TRUE
			end if
			arg = arg->r
		wend
	end select
	return FALSE
end function

private function hThreadCallArgType _
	( byval proc as FBSYMBOL ptr, byval arg as ASTNODE ptr, byval ordinal as integer ) as integer

	var param = arg->sym
	var mode = symbGetParamMode( param )
	var dtype = symbGetType( param )
	var can_copy_string = (env.clopt.target <> FB_COMPTARGET_NUTTX) andalso _
	                      (env.clopt.target <> FB_COMPTARGET_XBOX)
	select case mode
	case FB_PARAMMODE_BYREF, FB_PARAMMODE_BYDESC
		if( hThreadCallUsesTempStorage( arg->l ) ) then
			if( mode = FB_PARAMMODE_BYREF andalso typeIsDynamicString( dtype ) andalso can_copy_string ) then
				return FB_THREADCALL_STRING
			end if
			errReportParam( proc, ordinal, NULL, FB_ERRMSG_THREADCALLTEMPREF )
			return -1
		end if
		return FB_THREADCALL_PTR
	case FB_PARAMMODE_BYVAL
		if( typeIsDynamicString( dtype ) ) then
			if( can_copy_string ) then return FB_THREADCALL_STRING
		elseif( dtype <> FB_DATATYPE_STRUCT orelse symbCompIsTrivial( symbGetSubtype( param ) ) ) then
			if( typeIsPtr( dtype ) andalso hThreadCallUsesTempStorage( arg->l ) ) then
				errReportParam( proc, ordinal, NULL, FB_ERRMSG_THREADCALLTEMPREF )
				return -1
			end if
			var tctype = hThreadCallMapType( param )
			if( tctype <> -1 ) then return tctype
		end if
	end select
	errReportParam( proc, ordinal, NULL, FB_ERRMSG_UNSUPPORTEDFUNCTION )
	return -1
end function

'':::::
function rtlThreadCall(byval callexpr as ASTNODE ptr) as ASTNODE ptr

	function = NULL

	dim as FBSYMBOL ptr proc, param
	dim as ASTNODE ptr procmodeexpr, t
	dim as ASTNODE ptr stacksizeexpr, argsexpr, ptrexpr

	if( callexpr = NULL ) then exit function
	if( astIsCALL( callexpr ) = FALSE ) then
		astDelTree( callexpr )
		exit function
	end if
	proc = callexpr->sym

	'' A normal SUB call copies converted writable strings back immediately
	'' after return. The launching thread cannot do that for an async call.
	if( callexpr->call.strtail ) then
		errReport( FB_ERRMSG_THREADCALLCOPYBACK )
		astDelTree( callexpr )
		exit function
	end if

	'' Check every argument before transferring ownership of the AST nodes.
	dim args as integer = callexpr->call.args
	dim arg as ASTNODE ptr = callexpr->r
	dim argupper as integer = iif( args=0, 1, args )
	redim argexpr( 1 to argupper ) as ASTNODE ptr
	redim argtype( 1 to argupper ) as integer
	for i as integer = 1 to args
		if( arg = NULL orelse arg->sym = NULL orelse arg->l = NULL ) then
			astDelTree( callexpr )
			exit function
		end if
		argtype( args-i+1 ) = hThreadCallArgType( proc, arg, args-i+1 )
		if( argtype( args-i+1 ) = -1 ) then
			astDelTree( callexpr )
			exit function
		end if
		arg = arg->r
	next i

	arg = callexpr->r
	for i as integer = 1 to args
		'' Take the argument expression out of the ARG node
		argexpr( args-i+1 ) = arg->l
		arg->l = NULL

		arg = arg->r
	next i

	'' Delete the CALL and its now empty ARGs
	astDelTREE( callexpr )

	'' create new call
	dim as ASTNODE ptr expr = astNewCall( PROCLOOKUP( THREADCALL ) )

	'' push function argument
	if( astNewARG( expr, astBuildProcAddrOf( proc ) ) = NULL ) then
		exit function
	end if

	'' get calling convention
	dim as integer procmode, procmode_fb
	procmode_fb = symbGetProcMode( proc )
	if procmode_fb = FB_FUNCMODE_FBCALL then procmode_fb = env.target.fbcall
	if( procmode_fb = FB_FUNCMODE_CDECL ) then
		procmode = FB_THREADCALL_CDECL
	elseif( ((procmode_fb = FB_FUNCMODE_STDCALL) or _
			(procmode_fb = FB_FUNCMODE_STDCALL_MS)) and _
		((env.clopt.target = FB_COMPTARGET_WIN32) or _
		 (env.clopt.target = FB_COMPTARGET_WINCE) or _
		 (env.clopt.target = FB_COMPTARGET_CYGWIN) or _
		 (env.clopt.target = FB_COMPTARGET_XBOX)) ) then
		procmode = FB_THREADCALL_STDCALL
	else
		errReport( FB_ERRMSG_UNSUPPORTEDFUNCTION )
		exit function
	end if

	'' push calling convention
	procmodeexpr = astNewCONSTi( procmode )
	if( astNewARG( expr, procmodeexpr ) = NULL ) then
		exit function
	end if

	'' push stack size (not in syntax)
	stacksizeexpr = astNewCONSTi( 0 )
	if( astNewARG( expr, stacksizeexpr ) = NULL ) then
		exit function
	end if

	'' push number of arguments
	argsexpr = astNewCONSTi( args )
	if( astNewARG( expr, argsexpr ) = NULL ) then
		exit function
	end if

	'' push each argument type
	param = symbGetProcLastParam( proc )
	for i as integer = 1 to args

		'' push parameter type
		dim as FBSYMBOL ptr stype = symbGetSubType( param )
		if hThreadCallPushType( expr, argtype( i ), stype ) = FALSE then
			exit function
		end if

		'' The normal call already lowered BYREF and BYDESC to pointers.
		'' Hand the packer one address of that ABI value, as for BYVAL.
		ptrexpr = argexpr( i )
		t = astNewLINK( t, hGetExprRef( ptrexpr ), AST_LINK_RETURN_RIGHT )

		if( ptrexpr = NULL ) then
			exit function
		end if

		'' push pointer to argument
		if( astNewARG( expr, ptrexpr ) = NULL ) then
			exit function
		end if

		param = symbGetProcPrevParam( proc, param )
	next

	function = astNewLINK( t, expr, AST_LINK_RETURN_RIGHT )
end function

'' end of runtime/rtl-system-thread.bas
