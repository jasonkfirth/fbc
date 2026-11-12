'' Project: FreeBASIC C backend
'' File: backend/c/c-thread-callbacks.bi
'' Purpose: Call static typed thread workers through the runtime callback ABI.
'' Responsibilities: Own per-unit adapter names and retain typed worker calls.
'' This file intentionally does NOT allocate thread contexts or change user pointers.

private function hIsThreadCallbackParam _
	( byval proc as FBSYMBOL ptr, byval param_index as integer ) as integer
	if( param_index <> 0 ) then return FALSE
	if( proc = NULL orelse symbGetIsRTL(proc) = FALSE ) then return FALSE
	return hGetCId(proc) = "fb_ThreadCreate"
end function

private function hThreadCallbackName( byval entry as CTHREADCALLBACK ptr ) as string
	return "fb_c_thread_callback_" + str(entry->identifier)
end function

private function hThreadCallbackArgument _
	( byval expr as EXPRNODE ptr, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr ) as EXPRNODE ptr
	var leaf = expr
	do while( leaf <> NULL andalso leaf->class = EXPRCLASS_CAST )
		select case typeGetDtAndPtrOnly(leaf->dtype)
		case typeAddrOf(FB_DATATYPE_FUNCTION), typeAddrOf(FB_DATATYPE_VOID)
		case else
			'' An intervening integer or unrelated pointer cast belongs to
			'' the source expression. In particular, narrowing a code address
			'' must not be replaced by the original full procedure address.
			return hCMutablePointerArgument(dtype, subtype, expr)
		end select
		leaf = leaf->l
	loop
	if( leaf <> NULL andalso leaf->class = EXPRCLASS_SYM ) then
		var target = leaf->sym
		if( symbIsProc(target) andalso symbGetProcParams(target) = 1 andalso _
		    symbIsImport(target) = FALSE andalso symbGetIsRTL(target) = FALSE andalso _
		    symbIsAbstract(target) = FALSE andalso _
		    symbGetProcRealType(target) = FB_DATATYPE_VOID ) then
			var parameter = symbGetProcHeadParam(target)
			dim as integer target_dtype = FB_DATATYPE_INVALID
			dim as FBSYMBOL ptr target_subtype = NULL
			if( parameter <> NULL ) then symbGetRealParamDtype(parameter, target_dtype, target_subtype)
			'' On x86 a convention cast can change stack cleanup. Do not
			'' reinterpret such source casts as ordinary pointer variance.
			var compatible_mode = (fbGetCpuFamily() <> FB_CPUFAMILY_X86) orelse _
			                      (symbGetProcMode(target) = env.target.fbcall)
			if( typeIsPtr(target_dtype) andalso compatible_mode andalso _
			    typeGetDtAndPtrOnly(target_dtype) <> typeAddrOf(FB_DATATYPE_FUNCTION) andalso _
			    target_dtype <> typeAddrOf(FB_DATATYPE_VOID) ) then
				var target_name = hGetCId(target)
				var entry = cptr(CTHREADCALLBACK ptr, listGetHead(@ctx.threadcallbacks))
				do while( entry <> NULL )
					if( *entry->target_name.data = target_name ) then exit do
					entry = listGetNext(entry)
				loop
				if( entry = NULL ) then
					entry = listNewNode(@ctx.threadcallbacks)
					ctx.threadcallbackid += 1
					entry->identifier = ctx.threadcallbackid
					DZstrAssign(entry->target_name, strptr(target_name))
					var target_type = hEmitType(target_dtype, target_subtype, TRUE)
					DZstrAssign(entry->target_type, strptr(target_type))
				end if
				var callback_name = hThreadCallbackName(entry)
				hCLeafCallback(target, callback_name)
				exprFreeTree(expr)
				return exprNewTEXT(dtype, subtype, callback_name)
			end if
		end if
	end if
	'' Dynamic callback expressions keep their evaluation and conversion path.
	'' A stack context would be invalid once ThreadCreate returns; only direct
	'' procedure addresses can use these stateless adapters without new lifetime.
	return hCMutablePointerArgument(dtype, subtype, expr)
end function

private sub hEmitThreadCallbackDefinitions( )
	dim as string convention
	if( fbGetCpuFamily() = FB_CPUFAMILY_X86 ) then
		select case env.target.fbcall
		case FB_FUNCMODE_STDCALL, FB_FUNCMODE_STDCALL_MS
			convention = "__attribute__((stdcall)) "
		end select
	end if
	var entry = cptr(CTHREADCALLBACK ptr, listGetHead(@ctx.threadcallbacks))
	do while( entry <> NULL )
		var first = ctx.sections(0).bufferlen
		var callback_name = hThreadCallbackName(entry)
		hWriteLine("static void " + convention + callback_name + "( void* );", TRUE)
		hCLeafNativeSpan(callback_name, TRUE, first, ctx.sections(0).bufferlen)
		entry = listGetNext(entry)
	loop
	var section = sectionGosub(1)
	entry = listGetHead(@ctx.threadcallbacks)
	do while( entry <> NULL )
		var first = ctx.sections(1).bufferlen
		var callback_name = hThreadCallbackName(entry)
		hWriteLine("static void " + convention + callback_name + "( void* parameter )", TRUE)
		hWriteLine("{", TRUE)
		sectionIndent()
		'' The runtime retains this code address and the original user data.
		'' No adapter state is shared between threads or borrowed from a caller.
		hWriteLine(*entry->target_name.data + "( (" + *entry->target_type.data + ")parameter );", TRUE)
		sectionUnindent()
		hWriteLine("}", TRUE)
		hCLeafNativeSpan(callback_name, FALSE, first, ctx.sections(1).bufferlen)
		entry = listGetNext(entry)
	loop
	sectionReturn(section)
end sub

private sub hClearThreadCallbacks( )
	var entry = cptr(CTHREADCALLBACK ptr, listGetHead(@ctx.threadcallbacks))
	do while( entry <> NULL )
		DZstrAllocate(entry->target_name, 0)
		DZstrAllocate(entry->target_type, 0)
		listDelNode(@ctx.threadcallbacks, entry)
		entry = listGetHead(@ctx.threadcallbacks)
	loop
end sub

'' end of backend/c/c-thread-callbacks.bi
