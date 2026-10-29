'' Project: FreeBASIC C backend
'' File: backend/c/c-gfx-callbacks.bi
'' Purpose: Preserve the native graphics blender ABI and BASIC callback types.
'' Responsibilities: Select BLENDER parameters and adapt typed user data or conventions.
'' This file intentionally does NOT contain: pixel operations or graphics state.

private function hGfxBlenderType _
	( byval proc as FBSYMBOL ptr, byval param_index as integer ) as FBSYMBOL ptr

	if( param_index <> 9 andalso param_index <> 12 ) then return NULL
	if( proc = NULL orelse symbGetIsRTL(proc) = FALSE ) then return NULL
	var identifier = hGetCId( proc )
	dim as integer putter_ordinal
	select case( identifier )
	case "fb_GfxPut"
		if( param_index <> 12 ) then return NULL
		putter_ordinal = 11
	case "fb_GfxDrawString", "fb_GfxDrawStringUstr"
		if( param_index <> 9 ) then return NULL
		putter_ordinal = 9
	case else
		return NULL
	end select
	var param = symbGetProcHeadParam( proc )
	for i as integer = 1 to putter_ordinal - 1
		if( param = NULL ) then return NULL
		param = param->next
	next
	if( param = NULL orelse param->subtype = NULL ) then return NULL
	param = symbGetProcHeadParam( param->subtype )
	for i as integer = 1 to 7
		if( param = NULL ) then return NULL
		param = param->next
	next
	if( param = NULL ) then return NULL
	return param->subtype
end function

private function hGfxContextType( byval entry as CGFXBLENDER ptr ) as string
	return "fb_c_gfx_context_type_" + str(entry->identifier)
end function

private function hGfxAdapterName( byval entry as CGFXBLENDER ptr ) as string
	return "fb_c_gfx_blender_" + str(entry->identifier)
end function

private function hGfxBlenderArgument _
	( byval expr as EXPRNODE ptr, byval native_type as FBSYMBOL ptr, _
	  byref adapter as CGFXBLENDER ptr, byref callback_value as string, _
	  byref context_name as string ) as EXPRNODE ptr

	var leaf = expr
	do while( leaf <> NULL andalso leaf->class = EXPRCLASS_CAST )
		leaf = leaf->l
	loop
	if( leaf = NULL ) then return expr
	if( leaf->class = EXPRCLASS_IMM andalso leaf->val.i = 0 ) then
		exprFreeTree( expr )
		return exprNewTEXT( typeAddrOf(FB_DATATYPE_FUNCTION), native_type, "0" )
	end if
	var source_type = leaf->subtype
	if( leaf->class = EXPRCLASS_SYM andalso symbIsProc(leaf->sym) ) then
		source_type = symbAddProcPtrFromFunction( leaf->sym )
	end if
	if( source_type = NULL orelse symbIsProc(source_type) = FALSE ) then return expr
	if( symbGetProcParams(source_type) <> 3 ) then return expr
	var parameter = symbGetProcHeadParam( source_type )->next->next
	dim as integer parameter_dtype
	dim as FBSYMBOL ptr parameter_subtype
	symbGetRealParamDtype( parameter, parameter_dtype, parameter_subtype )
	if( typeIsPtr(parameter_dtype) = FALSE ) then return expr

	'' Remove only the address-erasing casts added for the intrinsic's ANY
	'' input. The source function pointer keeps its complete declaration.
	do while( expr <> leaf )
		var child = expr->l
		expr->l = NULL
		exprFreeTree( expr )
		expr = child
	loop
	if( parameter_dtype = typeAddrOf(FB_DATATYPE_VOID) andalso _
	    symbGetProcMode(source_type) = symbGetProcMode(native_type) ) then
		return expr
	end if

	'' BLENDER receives void*, but BASIC also permits a typed data pointer
	'' and other procedure conventions. A function-pointer cast cannot adapt
	'' either contract. Keep the original function type in a stack context;
	'' the native callback converts the data pointer and makes a typed call.
	var function_type = hEmitType( typeAddrOf(FB_DATATYPE_FUNCTION), source_type, TRUE )
	var parameter_type = hEmitType( parameter_dtype, parameter_subtype, TRUE )
	var reverse_args = (symbGetProcMode(source_type) = FB_FUNCMODE_PASCAL)
	adapter = listGetHead( @ctx.gfxblenders )
	do while( adapter <> NULL )
		if( *adapter->function_type.data = function_type andalso _
		    *adapter->parameter_type.data = parameter_type andalso _
		    adapter->reverse_args = reverse_args ) then exit do
		adapter = listGetNext( adapter )
	loop
	if( adapter = NULL ) then
		adapter = listNewNode( @ctx.gfxblenders )
		ctx.gfxblenderid += 1
		adapter->identifier = ctx.gfxblenderid
		adapter->reverse_args = reverse_args
		DZstrAssign( adapter->function_type, strptr(function_type) )
		DZstrAssign( adapter->parameter_type, strptr(parameter_type) )
	end if
	ctx.gfxcontextid += 1
	context_name = "fb_c_gfx_context_" + str(ctx.gfxcontextid)
	callback_value = exprFlush( expr )
	return exprNewTEXT( typeAddrOf(FB_DATATYPE_FUNCTION), native_type, hGfxAdapterName(adapter) )
end function

private sub hEmitGfxBlenderDefinitions( )
	'' PUT and DRAW STRING call their blenders synchronously and do not retain
	'' user data. Each call owns its stack context through the last pixel.
	'' Adapters are stateless, so nested calls and threads cannot share data.
	dim as string convention
	if( fbGetCpuFamily( ) = FB_CPUFAMILY_X86 ) then
		select case( env.target.fbcall )
		case FB_FUNCMODE_STDCALL, FB_FUNCMODE_STDCALL_MS
			convention = "__attribute__((stdcall)) "
		end select
	end if
	var entry = cptr(CGFXBLENDER ptr, listGetHead(@ctx.gfxblenders))
	do while( entry <> NULL )
		hWriteLine( "struct " + hGfxContextType(entry) + " { " + _
			*entry->function_type.data + " callback; void* parameter; };", TRUE )
		hWriteLine( "static uint32 " + convention + hGfxAdapterName(entry) + _
			"( uint32, uint32, void* );", TRUE )
		entry = listGetNext( entry )
	loop
	var section = sectionGosub( 1 )
	entry = listGetHead( @ctx.gfxblenders )
	do while( entry <> NULL )
		hWriteLine( "static uint32 " + convention + hGfxAdapterName(entry) + _
			"( uint32 source, uint32 destination, void* parameter )", TRUE )
		hWriteLine( "{", TRUE )
		sectionIndent( )
		hWriteLine( "struct " + hGfxContextType(entry) + "* context = (struct " + _
			hGfxContextType(entry) + "*)parameter;", TRUE )
		var argument = "(" + *entry->parameter_type.data + ")context->parameter"
		if( entry->reverse_args ) then
			hWriteLine( "return context->callback( " + argument + ", destination, source );", TRUE )
		else
			hWriteLine( "return context->callback( source, destination, " + argument + " );", TRUE )
		end if
		sectionUnindent( )
		hWriteLine( "}", TRUE )
		entry = listGetNext( entry )
	loop
	sectionReturn( section )
end sub

private sub hClearGfxBlenders( )
	var entry = cptr(CGFXBLENDER ptr, listGetHead(@ctx.gfxblenders))
	do while( entry <> NULL )
		DZstrAllocate( entry->function_type, 0 )
		DZstrAllocate( entry->parameter_type, 0 )
		listDelNode( @ctx.gfxblenders, entry )
		entry = listGetHead( @ctx.gfxblenders )
	loop
end sub

'' end of backend/c/c-gfx-callbacks.bi
