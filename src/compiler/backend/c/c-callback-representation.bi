'' Project: FreeBASIC C backend
'' File: backend/c/c-callback-representation.bi
'' Purpose: Recognize generic callback representation transfers.
'' Responsibilities: Keep erased callback values distinct from typed call adapters.
'' This file intentionally does NOT change call signatures or calling conventions.

private function hCNoReturnCallbackIdentity(byval n as EXPRNODE ptr) as integer
	if( typeGetDtAndPtrOnly(n->dtype) <> typeAddrOf(FB_DATATYPE_FUNCTION) ) then return FALSE
	var value = n->l
	if( value->class <> EXPRCLASS_SYM ) then return FALSE
	if( typeGetDtAndPtrOnly(value->dtype) <> typeAddrOf(FB_DATATYPE_FUNCTION) ) then return FALSE
	if( value->sym = NULL orelse symbIsProc(value->sym) = FALSE ) then return FALSE
	if( hProcDoesNotReturn(value->sym) = FALSE ) then return FALSE
	'' The interned procedure-pointer signature excludes the body's noreturn
	'' property. A matching callback accepts the function address directly;
	'' an explicit cast instead makes Clang diagnose the lost attribute.
	'' Exact identity retains parameter types, result mode and convention.
	return n->subtype = symbAddProcPtrFromFunction(value->sym)
end function

private function hCErasedCallbackType(byval dtype as integer, byval subtype as FBSYMBOL ptr) as integer
	if( typeGetDtAndPtrOnly(dtype) <> typeAddrOf(FB_DATATYPE_FUNCTION) ) then return FALSE
	if( subtype = NULL orelse symbIsProc(subtype) = FALSE ) then return FALSE
	return symbGetProcRealType(subtype) = FB_DATATYPE_VOID andalso symbGetProcParams(subtype) = 0
end function

private function hCCallbackHasCLongDoubleByValue(byval proc as FBSYMBOL ptr) as integer
	if( hIsCLongDoubleType(symbGetProcRealType(proc), symbGetProcRealSubtype(proc)) ) then return TRUE
	dim as FBSYMBOL ptr param = symbGetProcHeadParam(proc)
	do while( param <> NULL )
		if( symbGetParamMode(param) <> FB_PARAMMODE_VARARG ) then
			dim as integer dtype
			dim as FBSYMBOL ptr subtype
			symbGetRealParamDtype(param, dtype, subtype)
			if( hIsCLongDoubleType(dtype, subtype) ) then return TRUE
		end if
		param = param->next
	loop
	return FALSE
end function

private function hCCallbackRepresentationCast _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	  byval input_dtype as integer, byval input_subtype as FBSYMBOL ptr ) as integer
	if( typeGetDtAndPtrOnly(dtype) <> typeAddrOf(FB_DATATYPE_FUNCTION) or _
	    typeGetDtAndPtrOnly(input_dtype) <> typeAddrOf(FB_DATATYPE_FUNCTION) ) then return FALSE
	if( subtype = NULL or input_subtype = NULL ) then return FALSE
	if( symbIsProc(subtype) = FALSE or symbIsProc(input_subtype) = FALSE ) then return FALSE
	'' A generic callback is a stored code address, not a promise that the
	'' function can be invoked without arguments. APIs such as GCallback
	'' restore the actual signature when dispatching the registered callback.
	'' Keep typed-to-typed variance and actual convention changes on their existing
	'' path: a representation transfer alone cannot adapt those calls.
	if( symbGetProcMode(subtype) <> symbGetProcMode(input_subtype) ) then
		'' The C emitter only spells these convention attributes on x86.
		'' Elsewhere they share the target's native calling convention. This
		'' transfers a stored address; the restored typed signature still owns
		'' argument and result lowering when that address is actually called.
		if( fbGetCpuFamily() = FB_CPUFAMILY_X86 ) then return FALSE
		'' CDECL selects native C long-double values instead of BASIC record
		'' arguments/results. Keep that actual ABI difference visible.
		if( (symbGetProcMode(subtype) = FB_FUNCMODE_CDECL) xor _
		    (symbGetProcMode(input_subtype) = FB_FUNCMODE_CDECL) ) then
			if( hCCallbackHasCLongDoubleByValue(subtype) or _
			    hCCallbackHasCLongDoubleByValue(input_subtype) ) then return FALSE
		end if
		select case( symbGetProcMode(subtype) )
		case FB_FUNCMODE_CDECL, FB_FUNCMODE_STDCALL, FB_FUNCMODE_STDCALL_MS
		case else
			return FALSE
		end select
		select case( symbGetProcMode(input_subtype) )
		case FB_FUNCMODE_CDECL, FB_FUNCMODE_STDCALL, FB_FUNCMODE_STDCALL_MS
		case else
			return FALSE
		end select
	end if
	'' Two erased signatures are already compatible; avoid temporary carriers
	'' for an ordinary spelling change between their interned typedefs.
	return hCErasedCallbackType(dtype, subtype) xor hCErasedCallbackType(input_dtype, input_subtype)
end function

'' end of backend/c/c-callback-representation.bi
