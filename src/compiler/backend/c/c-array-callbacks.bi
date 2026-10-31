'' Project: FreeBASIC C backend
'' File: backend/c/c-array-callbacks.bi
'' Purpose: Adapt typed BASIC element methods to the runtime callback ABI.
'' Responsibilities: Callback parameter spelling and per-unit C adapters.
'' This file intentionally does NOT contain: BASIC procedure definitions or runtime layout changes.

private function hIsArrayCallbackParam _
	( _
		byval proc as FBSYMBOL ptr, _
		byval param_index as integer _
	) as integer

	if( proc = NULL orelse symbGetIsRTL( proc ) = FALSE ) then return FALSE
	var identifier = hGetCId( proc )
	select case( identifier )
	case "fb_ArrayRedimObj", "fb_ArrayRedimPresvObj"
		return (param_index = 2) or (param_index = 3)
	case "fb_ArrayRedimTo"
		return (param_index = 3) or (param_index = 4)
	case "fb_ArrayClearObj", "fb_ArrayEraseObj"
		return (param_index = 1) or (param_index = 2)
	case "fb_ArrayDestructObj"
		return (param_index = 1)
	end select
	return FALSE
end function

private function hArrayCallbackName( byval entry as CARRAYCALLBACK ptr ) as string
	function = "fb_c_array_callback_" + str( entry->identifier )
end function

private function hArrayCallbackArgument( byval expr as EXPRNODE ptr ) as EXPRNODE ptr
	var leaf = expr
	do while( leaf <> NULL andalso leaf->class = EXPRCLASS_CAST )
		leaf = leaf->l
	loop
	if( leaf <> NULL andalso leaf->class = EXPRCLASS_IMM ) then
		if( typeGetClass( leaf->dtype ) = FB_DATACLASS_INTEGER andalso leaf->val.i = 0 ) then
			exprFreeTree( expr )
			return exprNewTEXT( typeAddrOf(FB_DATATYPE_VOID), NULL, "0" )
		end if
	end if

	'' rtl-array.bas supplies a null or a single-instance procedure address.
	'' A void(Type*) method cannot be called as void(void*) in C, even when
	'' the platform passes both pointers identically. The adapter has the
	'' erased signature and calls the original through its typed declaration.
	'' Its explicit element conversion keeps strict pointer diagnostics
	'' enabled without introducing an implicit void-pointer conversion.
	if( leaf <> NULL andalso leaf->class = EXPRCLASS_SYM ) then
		var target = leaf->sym
		if( symbIsProc(target) andalso symbGetProcParams(target) = 1 andalso _
		    symbProcReturnsOnStack(target) = FALSE ) then
			var param = symbGetProcHeadParam( target )
			dim as integer dtype = FB_DATATYPE_INVALID
			dim as FBSYMBOL ptr subtype = NULL
			if( param <> NULL ) then symbGetRealParamDtype( param, dtype, subtype )
			if( typeIsPtr(dtype) ) then
				var target_name = hGetCId( target )
				var entry = cptr(CARRAYCALLBACK ptr, listGetHead(@ctx.arraycallbacks))
				do while( entry <> NULL )
					if( entry->target_name.data <> NULL andalso *entry->target_name.data = target_name ) then exit do
					entry = listGetNext( entry )
				loop
				if( entry = NULL ) then
					entry = listNewNode( @ctx.arraycallbacks )
					ctx.arraycallbackid += 1
					entry->identifier = ctx.arraycallbackid
					DZstrAssign( entry->target_name, strptr(target_name) )
					var target_type = hEmitType( dtype, subtype, TRUE )
					DZstrAssign( entry->target_type, strptr(target_type) )
				end if
				'' The adapter may be shared by several procedures. Keep each
				'' caller edge and the adapter's typed target before erasing the
				'' symbol expression; the deferred body contains only owned names.
				var callback_name = hArrayCallbackName(entry)
				hCLeafCallback(target, callback_name)
				exprFreeTree( expr )
				return exprNewTEXT( typeAddrOf(FB_DATATYPE_VOID), NULL, callback_name )
			end if
		end if
	end if

	'' Retain explicitly supplied pointer expressions as an ABI conversion.
	'' Compiler-generated constructor/destructor addresses use adapters above.
	var value = exprFlush( expr )
	return exprNewTEXT( typeAddrOf(FB_DATATYPE_VOID), NULL, "(void (*)(void*))(" + value + ")" )
end function

private sub hEmitArrayCallbackDefinitions( )
	'' Prototype-only declarations go into the header. Bodies follow all
	'' original procedures, so local generated helpers are already defined.
	'' Stored C names remain valid after a local symbol table is released.
	var entry = cptr(CARRAYCALLBACK ptr, listGetHead(@ctx.arraycallbacks))
	do while( entry <> NULL )
		var first = ctx.sections(0).bufferlen
		hWriteLine( "static void " + hArrayCallbackName(entry) + "( void* );", TRUE )
		var callback_name = hArrayCallbackName(entry)
		hCLeafNativeSpan(callback_name, TRUE, first, ctx.sections(0).bufferlen)
		entry = listGetNext( entry )
	loop
	var section = sectionGosub( 1 )
	entry = listGetHead( @ctx.arraycallbacks )
	do while( entry <> NULL )
		var first = ctx.sections(1).bufferlen
		hWriteLine( "static void " + hArrayCallbackName(entry) + "( void* fb_c_array_element )", TRUE )
		hWriteLine( "{", TRUE )
		sectionIndent( )
		hWriteLine( *entry->target_name.data + "( (" + *entry->target_type.data + ")fb_c_array_element );", TRUE )
		sectionUnindent( )
		hWriteLine( "}", TRUE )
		var callback_name = hArrayCallbackName(entry)
		hCLeafNativeSpan(callback_name, FALSE, first, ctx.sections(1).bufferlen)
		entry = listGetNext( entry )
	loop
	sectionReturn( section )
end sub

private sub hClearArrayCallbacks( )
	var entry = cptr(CARRAYCALLBACK ptr, listGetHead(@ctx.arraycallbacks))
	do while( entry <> NULL )
		DZstrAllocate( entry->target_name, 0 )
		DZstrAllocate( entry->target_type, 0 )
		listDelNode( @ctx.arraycallbacks, entry )
		entry = listGetHead( @ctx.arraycallbacks )
	loop
end sub

'' end of backend/c/c-array-callbacks.bi
